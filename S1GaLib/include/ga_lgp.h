#ifndef GA_LGP_H
#define GA_LGP_H

#include <S1Core/S1Core.h>

typedef struct {
    short seed;
    unsigned short input_size;
    unsigned short output_size;
    unsigned short num_operators;
    unsigned short num_registers;
    unsigned short max_genome_len;
    unsigned short default_genome_len;
    float mutation_rate;
    unsigned int num_generations;
    unsigned int train_examples;
    unsigned int test_examples;
    unsigned int checkpoint;
    unsigned short is_min;
    unsigned short sampling_offset;
} glgp_Config;

typedef struct {
    unsigned short total_registers;
    unsigned short opcode_bits;
    unsigned short reg_id_bits;
    unsigned short bits_per_instruction;
    unsigned short genome_bits;
    unsigned short genome_words;
} glgp_GenomeInfo;

typedef void (*operation)(scExpr, scExpr, scExpr);

typedef struct {
    operation func;
    const char* name;
} OpEntry;

extern OpEntry lgp_op_funcs[]; // Array of operation names

/**
 * @brief Returns a glgp_Config struct with default configuration values for LGP.
 * @return glgp_Config struct with default values.
 */
glgp_Config make_default_config();

/**
 * @brief Computes genome information (registers, bits, words) from the given config.
 * @param config LGP configuration struct.
 * @return glgp_GenomeInfo struct with calculated genome parameters.
 */
glgp_GenomeInfo glgp_GetGenomeInfo(glgp_Config config);

/**
 * @brief Prints the configuration settings to stdout.
 * @param config LGP configuration struct.
 */
void glgp_PrintConfig(glgp_Config config);

/**
 * @brief Prints formatted genome information to stdout.
 * @param genome_info LGP genome information struct.
 */
void glgp_PrintGenomeInfo(glgp_GenomeInfo genome_info);


// Register access functions

/**
 * @brief Initializes LGP registers: internal registers to index+1, input registers to 0.
 * @param config LGP configuration struct.
 */
void glgp_resetRegisters(glgp_Config config);

/**
 * @brief Reads the value from the specified LGP register.
 * @param reg_id Index of the register to read.
 * @return Value stored in register.
 */
scExpr glgp_readValueFromLGPRegister(scExpr reg_id);

/**
 * @brief Get ApeMemVector containing all lgp registers.
 * @return ApeMemVector containing register values
 */
scExpr glgp_getRegisters();

/**
 * @brief Loads input values into the input registers from the input array.
 * @param config LGP configuration struct.
 * @param inputDataArray Array of input values.
 * @param input_row_idx Row idx in the input array to load into the registers.
 */
void glgp_LoadInputRegisters(glgp_Config config, scExpr inputDataArray, scExpr input_row_idx);

// Main LGP functions

/**
 * @brief Initializes the LGP population by generating a random genome and initializing registers.
 * @param config LGP configuration struct.
 */
void glgp_initPopulation(glgp_Config config);

/**
 * @brief Mutates the genome of the current individual based on mutation rate in config.
 * @param config LGP configuration struct.
 */
void glgp_MutateGenome(glgp_Config config);

/**
 * @brief Executes the genome for the current individual.
 * @param config LGP configuration struct.
 */
void glgp_RunGenome(glgp_Config config);

#endif