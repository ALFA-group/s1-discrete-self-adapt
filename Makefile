# ============================================================================
# User-Configurable Parameters
# ============================================================================
MAIN_SRC  ?= defenditb_exp.c
CC        ?= clang
CFLAGS    ?= -O2
LDFLAGS   ?= -O2
LDLIBS    ?= -lm

CYAN := \033[1;36m
GREEN := \033[32m
YELLOW := \033[33m
DIM := \033[2m
RESET := \033[0m

# ============================================================================
# Project Structure
# ============================================================================
GA_SRC_DIR   := ./S1GaLib
NOVA_SRC_DIR := ./S1Core
TEST_SRC_DIR := ./test
OBJ_DIR      := ./build

# Include paths
INCLUDE_PATHS := -I. -I./S1GaLib
CFLAGS += $(INCLUDE_PATHS)

# Target configuration
TARGET := run
S1USER := sjorgensen

# ============================================================================
# Source and Object Files
# ============================================================================
# Find all source files
GA_C_FILES   := $(shell find $(GA_SRC_DIR) -name '*.c')
NOVA_C_FILES := $(wildcard $(NOVA_SRC_DIR)/*.c)
TEST_C_FILES := $(wildcard $(TEST_SRC_DIR)/*.c)

# Generate object file paths
GA_OBJECTS   := $(patsubst $(GA_SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(GA_C_FILES))
NOVA_OBJECTS := $(patsubst $(NOVA_SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(NOVA_C_FILES))
TEST_OBJECTS := $(patsubst $(TEST_SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(TEST_C_FILES))
MAIN_OBJECT  := $(OBJ_DIR)/$(notdir $(MAIN_SRC:.c=.o))

# Object file names for board compilation (relative paths)
GA_OBJECTS_BOARD   := $(notdir $(GA_C_FILES:.c=.o))
NOVA_OBJECTS_BOARD := $(notdir $(NOVA_C_FILES:.c=.o))

# ============================================================================
# Local Build Targets
# ============================================================================
.PHONY: all clean

all: $(TARGET)

$(TARGET): $(MAIN_OBJECT) $(GA_OBJECTS) $(NOVA_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(MAIN_OBJECT): $(MAIN_SRC)
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(GA_SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(NOVA_SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(TEST_SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/*.o $(TARGET) out_bin

# ============================================================================
# Board Configuration (Remote Build)
# ============================================================================
BOARD_HOST := s1board
BOARD_PASS := root
BOARD_SEED ?= 123
BOARD_OUTDIR ?= run_temp
BOARD_DIR  := /home/$(S1USER)/millionNodes
LOCAL_DIR := ../s1-constrained-coevo

# Board compiler flags
BOARD_CFLAGS     := -Ofast -mcpu=cortex-a9 -mfloat-abi=softfp -mfp=vfpv3-fp16 -lm -lrt
BOARD_CFLAGS_C99 := -std=c99 $(BOARD_CFLAGS)

# SSH/SCP commands
SSH_CMD   = sshpass -p "$(BOARD_PASS)" ssh $(BOARD_HOST)
SCP_CMD   = sshpass -p "$(BOARD_PASS)" scp -r
REMOTE_CD = cd $(BOARD_DIR) &&

# Directories to sync
SYNC_DIRS := S1GaLib S1Core

# ============================================================================
# Board Source Tracking (for stamp-based incremental builds)
# ============================================================================
CORE_SOURCES  := $(wildcard $(LOCAL_DIR)/S1Core/*.c $(LOCAL_DIR)/S1Core/*.h)
GALIB_SOURCES := $(shell find $(LOCAL_DIR)/S1GaLib -name '*.c' -o -name '*.h')
MAIN_SOURCE   := $(LOCAL_DIR)/$(MAIN_SRC)

# ============================================================================
# Board Build Targets with Stamp-Based Tracking
# ============================================================================
.PHONY: init_board fix_date board_run board_fetch board_force board_clean clean_stamps

# Initialize directory structure on board
init_board:
	$(SSH_CMD) "mkdir -p $(BOARD_DIR)/S1GaLib $(BOARD_DIR)/S1Core $(BOARD_DIR)/output"

fix_date:
	$(SSH_CMD) "date `date \"+%m%d%H%M%Y.%S\"`"

# ===== STAMP-BASED INCREMENTAL BUILDS =====

# Core: deploy + compile if sources changed
.board_core_stamp: $(CORE_SOURCES)
	@printf "$(CYAN)==> Deploying S1Core$(RESET) ($(YELLOW)%d$(RESET) files)\n" $(words $(CORE_SOURCES))
	@$(SCP_CMD) $(LOCAL_DIR)/S1Core $(BOARD_HOST):$(BOARD_DIR)/
	@printf "$(CYAN)==> Compiling core$(RESET)\n"
	@$(SSH_CMD) "$(REMOTE_CD) gcc -c $(BOARD_CFLAGS) $(INCLUDE_PATHS) $(NOVA_C_FILES)"
	@printf "$(GREEN)    ✓ Core complete$(RESET)\n\n"
	@touch .board_core_stamp

.board_galib_stamp: $(GALIB_SOURCES)
	@printf "$(CYAN)==> Deploying S1GaLib$(RESET) ($(YELLOW)%d$(RESET) files)\n" $(words $(GALIB_SOURCES))
	@$(SCP_CMD) $(LOCAL_DIR)/S1GaLib $(BOARD_HOST):$(BOARD_DIR)/
	@printf "$(CYAN)==> Compiling GA library$(RESET)\n"
	@$(SSH_CMD) "$(REMOTE_CD) gcc -c $(BOARD_CFLAGS_C99) $(INCLUDE_PATHS) $(GA_C_FILES)"
	@printf "$(GREEN)    ✓ GA library complete$(RESET)\n\n"
	@touch .board_galib_stamp

.board_main_stamp: $(MAIN_SOURCE) .board_core_stamp .board_galib_stamp
	@printf "$(CYAN)==> Deploying main source$(RESET)\n"
	@$(SCP_CMD) $(MAIN_SOURCE) $(BOARD_HOST):$(BOARD_DIR)/
	@printf "$(CYAN)==> Linking executable$(RESET)\n"
	@$(SSH_CMD) "$(REMOTE_CD) gcc $(BOARD_CFLAGS_C99) $(INCLUDE_PATHS) -o $(TARGET) $(GA_OBJECTS_BOARD) $(NOVA_OBJECTS_BOARD) $(MAIN_SRC)"
	@printf "$(GREEN)    ✓ Build complete$(RESET)\n\n"
	@touch .board_main_stamp

# ===== MANUAL CONTROL TARGETS =====
.PHONY: board_core board_galib board_main board_all

board_core: .board_core_stamp

board_galib: .board_galib_stamp

board_main: .board_main_stamp

board_all: .board_main_stamp

# ===== WORKFLOW TARGETS =====
.PHONY: board_init board_quick board_rebuild board_debug board_q

# First-time setup: initialize board + full build
board_setup: init_board .board_main_stamp

# Quick development cycle: build what changed, run, fetch
board_quick: .board_main_stamp board_run board_fetch

# Force rebuild and deploy everything
board_rebuild: board_force

# Quick debugging: ensure built, then run
board_debug: .board_main_stamp board_run

# ===== RUN AND FETCH =====

# Run the executable on board (doesn't trigger builds)
board_run:
	@printf "$(CYAN)==> Running $(TARGET)$(RESET)\n"
	@printf "$(DIM)────────────────────────────────────────────────────────────────$(RESET)\n"
	@START=$$(date +%s); \
	 $(SSH_CMD) "$(REMOTE_CD) mkdir -p $(BOARD_OUTDIR) && ./$(TARGET) real 0 $(BOARD_OUTDIR) $(BOARD_SEED)"; \
	 END=$$(date +%s); \
	 ELAPSED=$$((END - START)); \
	 MINS=$$((ELAPSED / 60)); \
	 SECS=$$((ELAPSED % 60)); \
	 printf "$(DIM)────────────────────────────────────────────────────────────────$(RESET)\n"; \
	 printf "$(GREEN)✓ Runtime: $(YELLOW)$${MINS}m $${SECS}s$(RESET)\n\n"

board_fetch:
	@printf "$(CYAN)==> Fetching output files$(RESET)\n"
	@mkdir -p ./board_output/$(BOARD_OUTDIR)
	@./scp_progress.sh "$(BOARD_PASS)" "$(BOARD_HOST)" "$(BOARD_DIR)/output" "./board_output/$(BOARD_OUTDIR)"
	@printf "$(CYAN)==> Cleaning remote output$(RESET)\n"
	@$(SSH_CMD) "$(REMOTE_CD) rm -r $(BOARD_DIR)/output/*"
	@printf "$(GREEN)    ✓ Fetch complete$(RESET)\n\n"

# ===== CLEANUP =====

# Force rebuild everything (delete stamps)
board_force:
	@echo "Forcing complete rebuild..."
	rm -f .board_core_stamp .board_galib_stamp .board_main_stamp
	$(MAKE) board_quick

# Clean build artifacts on board and remove stamps
board_clean:
	@echo "Cleaning board build artifacts..."
	$(SSH_CMD) "$(REMOTE_CD) rm -f *.o $(TARGET)"
	rm -f .board_core_stamp .board_galib_stamp .board_main_stamp

# Just remove stamps (forces next build to redeploy)
clean_stamps:
	rm -f .board_core_stamp .board_galib_stamp .board_main_stamp

help:
	@echo "$(CYAN)Board Targets:$(RESET)"
	@echo "  $(YELLOW)board_setup$(RESET)    - First-time setup (init + full build)"
	@echo "  $(YELLOW)board_quick$(RESET)    - Build changes + run + fetch (most common)"
	@echo "  $(YELLOW)board_force$(RESET)    - Force complete rebuild"
	@echo "  $(YELLOW)board_run$(RESET)      - Just run (no build)"
	@echo "  $(YELLOW)board_fetch$(RESET)    - Fetch output files"
	@echo "  $(YELLOW)board_clean$(RESET)    - Clean build artifacts"
	@echo ""
	@echo "$(CYAN)Local Targets:$(RESET)"
	@echo "  $(YELLOW)all$(RESET)            - Build locally"
	@echo "  $(YELLOW)clean$(RESET)          - Clean local build"

.PHONY: help