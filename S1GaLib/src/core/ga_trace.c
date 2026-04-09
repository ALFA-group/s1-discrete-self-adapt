#include <stdio.h>
#include <stdlib.h>

#include<S1Core/S1Core.h>
#include<S1GaLib/S1GaLib.h>

extern int chipRows, chipCols, apeRows, apeCols;

#define MAX_TRACE_REGISTRY_ENTRIES 100

/**
 * Global registry managing all active traces
 */
typedef struct {
    TraceEntry *entries;
    int capacity;
    int nextId;
    int initialized;
} TraceRegistry;

static TraceRegistry g_traceRegistry = {
    .entries = NULL,
    .capacity = 0,
    .nextId = 0,
    .initialized = 0
};

// =============================================================================
// Type-Specific Print Functions
// =============================================================================

static void _printUint(unsigned short value){
    printf("%d", value);
}

static void _printInt(unsigned short value){
    printf("%d", (int16_t)value);
}

static void _printApprox(unsigned short value){
    printf("%.4f", cvtFloat(value));
}

// Lookup table for type-specific print functions
static const TypePrintFn TYPE_PRINTERS[] = {
    [sc_uint] = _printUint,
    [sc_int] = _printInt,
    [sc_approx] = _printApprox,
};

static TypePrintFn _getTypePrinter(TraceType type) {
    if (type >= 0 && type < sizeof(TYPE_PRINTERS) / sizeof(TYPE_PRINTERS[0])) {
        return TYPE_PRINTERS[type];
    }
    return NULL;
}

// =============================================================================
// Registry Management
// =============================================================================

/**
 * Initialize the trace registry
 * @return 1 on success, 0 on failure
 */
static int _registryInit(void) {
    if (g_traceRegistry.initialized) {
        return 1;
    }

    g_traceRegistry.entries =
        calloc(MAX_TRACE_REGISTRY_ENTRIES, sizeof(TraceEntry));

    if (!g_traceRegistry.entries) {
        fprintf(stderr, "ERROR: Failed to allocate trace registry\n");
        return 0;
    }

    g_traceRegistry.capacity = MAX_TRACE_REGISTRY_ENTRIES;
    g_traceRegistry.nextId = 0;
    g_traceRegistry.initialized = 1;
    return 1;
}

/**
 * Execute a trace by registry ID
 * @param strId String representation of the registry ID
 * @param state Current machine state
 */
static void _traceExecute(char* strId, MachineState* state){
    int regId = atoi(strId);
    if (regId < 0 || regId >= g_traceRegistry.nextId) {
        printf("[ERROR]: invalid regId %d (nextId=%d)\n",
            regId, g_traceRegistry.nextId);
        return;
    }

    TraceEntry *entry = &g_traceRegistry.entries[regId];
    entry->fmt(entry->info, state, entry->print);
}

/**
 * Register a new trace entry
 * @param info Trace information structure
 * @param fmt Format function to use
 * @param output_type Type of output to trace
 * @return Trace ID on success, -1 on failure
 */
static int _traceRegister(TraceInfo info, TraceFmtFn fmt, TraceType output_type) {
    if (!_registryInit()) {
        return -1;
    }

    if (g_traceRegistry.nextId >= g_traceRegistry.capacity) {
        fprintf(stderr, "ERROR: Trace registry full!\n");
        return -1;
    }

    int traceId = g_traceRegistry.nextId++;

    TypePrintFn print_fn = _getTypePrinter(output_type);
    if (!print_fn) {
        fprintf(stderr, "ERROR: Invalid TraceType\n");
        return -1;
    }

    TraceEntry *entry = &g_traceRegistry.entries[traceId];
    entry->fmt   = fmt;
    entry->info  = info;
    entry->print = print_fn;
    sprintf(entry->id, "%d", traceId);

    TraceCallback(_traceExecute, g_traceRegistry.entries[traceId].id);

    return traceId;
}

// =============================================================================
// Helper Functions
// =============================================================================

grid_pos apeCoordinatesToGridPos(unsigned short row, unsigned short col){
    grid_pos position = {
        .apeRow = row % apeRows,
        .apeCol = col % apeCols,
        .chipRow = row / apeRows,
        .chipCol = col / apeCols
    };

    if (position.chipRow >= chipRows || position.chipCol >= chipCols) {
        fprintf(stderr, 
                "[ERROR] Ape index (%u,%u) maps to chip (%d,%d) "
                "which exceeds board dimensions (%dx%d)\n",
                row, col, position.chipRow, position.chipCol, 
                chipRows, chipCols);
        exit(EXIT_FAILURE);
    }
    
    return position;
}

static TraceType _getExprType(scExpr expr){
    if (scType(expr) == scTypeInt) return sc_uint;
    if (scType(expr) == scTypeApprox) return sc_approx;
    return -1;
}

// =============================================================================
// Trace Formatting Functions
// =============================================================================

/**
 * Format and print a single variable trace
 */
static void _traceVarFmt(TraceInfo info, MachineState *state, TypePrintFn printer) {
    grid_pos pos = info.pos;
    unsigned short mem_data =
        state->ApeMemory[pos.chipRow][pos.chipCol][pos.apeRow][pos.apeCol][MemAddress(info.expr)];

    if (info.text[0] != '\0') {
        printf("%s", info.text);
    } else {
        printf("<%s> %s for APE (%d, %d): ",
               trace_type_to_string(info.type),
               info.expr_name,
               pos.apeRow,
               pos.apeCol);
    }
    
    printer(mem_data);
    printf("\n");
}

/**
 * Format and print a vector trace
 */
static void _traceVectorFmt(TraceInfo info, MachineState *state, TypePrintFn printer){
    grid_pos pos = info.pos;

    if (info.text[0] != '\0') {
        printf("%s [", info.text);
    } else {
        printf("<%s vector> %s for APE (%d, %d): [",
               trace_type_to_string(info.type),
               info.expr_name,
               pos.apeRow,
               pos.apeCol);
    }

    for (int i = MemAddress(info.expr); i < MemAfter(info.expr); i++) {
        if (i > MemAddress(info.expr)) {
            printf(", ");
        }
        
        unsigned short value = state->ApeMemory[pos.chipRow][pos.chipCol][pos.apeRow][pos.apeCol][i];
        printer(value);
    }
    
    printf("]\n");
}

/**
 * Format and print an array trace
 */
static void _traceArrayFmt(TraceInfo info, MachineState *state, TypePrintFn printer){
    grid_pos pos = info.pos;

    if (info.text[0] != '\0') {
        printf("%s [", info.text);
    } else {
        printf("<%s array> %s for APE (%d, %d): \n[",
               trace_type_to_string(info.type),
               info.expr_name,
               pos.apeRow,
               pos.apeCol);
    }

    int counter = 0;
    int num_cols = MemColumns(info.expr);
    
    for (int i = MemAddress(info.expr); i < MemAfter(info.expr); i++) {
        if (i > MemAddress(info.expr)) {
            printf(", ");
            if (counter % num_cols == 0) {
                printf("\n");
            }
        }
        
        unsigned short value = state->ApeMemory[pos.chipRow][pos.chipCol][pos.apeRow][pos.apeCol][i];
        printer(value);
        counter++;
    }
    
    printf("]\n");
}

/**
 * Format and print a single variable trace for a range of APEs
 */
static void _traceVarRangeFmt(TraceInfo info, MachineState *state, TypePrintFn printer) {
    unsigned short startApeRow = info.pos.chipRow * apeRows + info.pos.apeRow;
    unsigned short startApeCol = info.pos.chipCol * apeCols + info.pos.apeCol;
    unsigned short endApeRow = info.posEnd.chipRow * apeRows + info.posEnd.apeRow;
    unsigned short endApeCol = info.posEnd.chipCol * apeCols + info.posEnd.apeCol;
    
    // Print header
    if (info.text[0] != '\0') {
        printf("%s\n", info.text);
    } else {
        printf("<%s> %s for APE range [(%d,%d) to (%d,%d)]:\n",
               trace_type_to_string(info.type),
               info.expr_name,
               startApeRow, startApeCol,
               endApeRow, endApeCol);
    }
    
    int colWidth = (info.type == sc_approx) ? 12 : 8;
    int numCols = endApeCol - startApeCol + 1;
    
    // Print column headers
    printf("     ");
    for (unsigned short col = startApeCol; col <= endApeCol; col++) {
        printf("%*d", colWidth, col);
        
        // Extra space at chip boundary
        if (col < endApeCol) {
            unsigned short nextCol = col + 1;
            grid_pos currentPos = apeCoordinatesToGridPos(0, col);
            grid_pos nextPos = apeCoordinatesToGridPos(0, nextCol);
            if (nextPos.chipCol != currentPos.chipCol) {
                printf("  ");  // Double space for chip boundary
            }
        }
    }
    printf("\n");
    
    // Print separator line
    printf("    +");
    for (int i = 0; i < numCols; i++) {
        for (int j = 0; j < colWidth; j++) {
            printf("-");
        }
        if (i < numCols - 1) {
            unsigned short col = startApeCol + i;
            unsigned short nextCol = col + 1;
            grid_pos currentPos = apeCoordinatesToGridPos(0, col);
            grid_pos nextPos = apeCoordinatesToGridPos(0, nextCol);
            if (nextPos.chipCol != currentPos.chipCol) {
                printf("--");
            }
        }
    }
    printf("\n");
    
    // Print grid
    for (unsigned short row = startApeRow; row <= endApeRow; row++) {
        printf("%3d |", row);
        
        for (unsigned short col = startApeCol; col <= endApeCol; col++) {
            grid_pos currentPos = apeCoordinatesToGridPos(row, col);
            
            unsigned short data = 
                state->ApeMemory[currentPos.chipRow][currentPos.chipCol]
                               [currentPos.apeRow][currentPos.apeCol]
                               [MemAddress(info.expr)];
            
            // Print value
            if (info.type == sc_approx) {
                printf("%*.4f", colWidth, cvtFloat(data));
            } else if (info.type == sc_int) {
                printf("%*d", colWidth, (int16_t)data);
            } else {
                printf("%*d", colWidth, data);
            }
            
            // Extra space at chip boundary
            if (col < endApeCol) {
                unsigned short nextCol = col + 1;
                grid_pos nextPos = apeCoordinatesToGridPos(row, nextCol);
                if (nextPos.chipCol != currentPos.chipCol) {
                    printf("  ");  // Double space
                }
            }
        }
        printf("\n");
        
        // Extra blank line at chip row boundary
        if (row < endApeRow) {
            unsigned short nextRow = row + 1;
            grid_pos nextRowPos = apeCoordinatesToGridPos(nextRow, startApeCol);
            grid_pos currentRowPos = apeCoordinatesToGridPos(row, startApeCol);
            
            if (nextRowPos.chipRow != currentRowPos.chipRow) {
                printf("\n");  // Blank line between chip rows
            }
        }
    }
    printf("\n");
}

// =============================================================================
// Public API
// =============================================================================

void traceExprOneApe(scExpr expr, unsigned short apeRow, unsigned short apeCol, char* expr_name){
    TraceType output_type = _getExprType(expr);
    traceExprOneApeTyped(expr, apeRow, apeCol, output_type, expr_name);
}

void traceExprOneApeTyped(scExpr expr, unsigned short apeRow, unsigned short apeCol, TraceType output_type, 
    char* expr_name){
    TraceInfo info = {0};
    info.expr = expr;
    info.type = output_type;
    info.pos = apeCoordinatesToGridPos(apeRow, apeCol);
    strncpy(info.expr_name, expr_name, sizeof(info.expr_name) - 1);

    // Handle APE variable expressions
    if (scKind(expr) == ApeVarKind && scType(expr) == scTypeInt) {
        DeclareApeMem(temp_mem, Int);
        Set(temp_mem, expr);
        info.expr = temp_mem;
    }
    if (scKind(expr) == ApeVarKind && scType(expr) == scTypeApprox) {
        DeclareApeMem(temp_mem, Approx);
        Set(temp_mem, expr);
        info.expr = temp_mem;
    }

    // Determine trace format based on memory size
    int mem_size = MemAfter(info.expr) - MemAddress(info.expr);
    
    if (mem_size == 1) {
        // Single variable
        _traceRegister(info, _traceVarFmt, output_type);
    } else if (MemColumns(info.expr) == mem_size) {
        // Vector (1D array)
        _traceRegister(info, _traceVectorFmt, output_type);
    } else {
        // Multi-dimensional array
        _traceRegister(info, _traceArrayFmt, output_type);
    }
}

void traceExprApeRange(scExpr expr, unsigned short rowStart, unsigned short colStart, unsigned short rowEnd, 
    unsigned short colEnd, char* expr_name){
    TraceType output_type = _getExprType(expr);
    traceExprApeRangeTyped(expr, rowStart, colStart, rowEnd, colEnd, output_type, expr_name);
}

void traceExprApeRangeTyped(scExpr expr, unsigned short rowStart, unsigned short colStart, unsigned short rowEnd, 
    unsigned short colEnd, TraceType output_type, char* expr_name){
    TraceInfo info = {0};
    info.expr = expr;
    info.type = output_type;
    info.pos = apeCoordinatesToGridPos(rowStart, colStart);
    info.posEnd = apeCoordinatesToGridPos(rowEnd, colEnd);
    strncpy(info.expr_name, expr_name, sizeof(info.expr_name) - 1);

    // Handle APE variable expressions
    if (scKind(expr) == ApeVarKind && scType(expr) == scTypeInt) {
        DeclareApeMem(temp_mem, Int);
        Set(temp_mem, expr);
        info.expr = temp_mem;
    }
    if (scKind(expr) == ApeVarKind && scType(expr) == scTypeApprox) {
        DeclareApeMem(temp_mem, Approx);
        Set(temp_mem, expr);
        info.expr = temp_mem;
    }

    // Determine trace format based on memory size
    int mem_size = MemAfter(info.expr) - MemAddress(info.expr);
    
    if (mem_size == 1) {
        // Single variable
        _traceRegister(info, _traceVarRangeFmt, output_type);
    }
    // TODO: should we support arrays and vectors?
    // } else if (MemColumns(info.expr) == mem_size) {
    //     // Vector (1D array)
    //     _traceRegister(info, _traceVectorRangeFmt, output_type);
    // } else {
    //     // Multi-dimensional array
    //     _traceRegister(info, _traceArrayRangeFmt, output_type);
    // }
    
}