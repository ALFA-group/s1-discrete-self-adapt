#include <stdio.h>

#include <S1Core/S1Core.h>
#include <S1GaLib/S1GaLib.h>

unsigned short const LGP_MAX_WORD_USAGE = 400; // words of memory for individual, rest is reserved for system
Declare(registers);

glgp_Config make_default_config() {
    glgp_Config config;
    config.seed = 123;
    config.input_size = 1;
    config.output_size = 1;
    config.num_operators = 4;
    config.num_registers = 10;
    config.max_genome_len = 100;
    config.default_genome_len = 100;
    config.mutation_rate = 0.01;
    config.num_generations = 100;
    config.train_examples = 100;
    config.test_examples = 10;
    config.checkpoint = 10;
    config.is_min = 0;
    config.sampling_offset = 4;
    return config;
}

glgp_GenomeInfo glgp_GetGenomeInfo(glgp_Config config) {
    glgp_GenomeInfo genome_info;
    genome_info.total_registers = config.input_size + config.num_registers;
    genome_info.opcode_bits = gu_bits_needed(config.num_operators);
    genome_info.reg_id_bits = gu_bits_needed(genome_info.total_registers);
    genome_info.bits_per_instruction = genome_info.opcode_bits + (genome_info.reg_id_bits * 3); // each instruction has 3 args: dst, src1, src2
    genome_info.genome_bits = config.max_genome_len * genome_info.bits_per_instruction;
    genome_info.genome_words = (genome_info.genome_bits + 15) / 16; // round up to next word
    return genome_info;
}

void glgp_PrintConfig(glgp_Config config) {
    printf("CONFIG SETTINGS:\n");
    printf("Seed: %d\n", config.seed);
    printf("Input Size: %d\n", config.input_size);
    printf("Output Size: %d\n", config.output_size);
    printf("Number of Operators: %d\n", config.num_operators);
    printf("Number of Registers: %d\n", config.num_registers);
    printf("Max Genome Length: %d\n", config.max_genome_len);
    printf("Default Genome Length: %d\n", config.default_genome_len);
    printf("Mutation Rate: %.2f\n", config.mutation_rate);
    printf("Number of Generations: %d\n", config.num_generations);
    printf("Training Examples: %d\n", config.train_examples);
    printf("Testing Examples: %d\n", config.test_examples);
    printf("Checkpoint Interval: %d\n", config.checkpoint);
    printf("-------------------------\n\n");
}

void glgp_PrintGenomeInfo(glgp_GenomeInfo genome_info) {
    printf("GENOME INFO:\n");
    printf("Total Registers (input + num_registers): %d\n", genome_info.total_registers);
    printf("Bits needed for opcode: %d\n", genome_info.opcode_bits);
    printf("Bits needed for reg_id: %d\n", genome_info.reg_id_bits);
    printf("Bits per instruction: %d\n", genome_info.bits_per_instruction);
    printf("Genome bits: %d\n", genome_info.genome_bits);
    printf("Genome words: %d\n", genome_info.genome_words);
    printf("Total memory usage: %d words\n", genome_info.total_registers + genome_info.genome_words);
    printf("-------------------------\n\n");
}


// ## LGP operators ##########
void glgp_op_add(scExpr dst_reg_idx, scExpr src_reg1_idx, scExpr src_reg2_idx) {
    // Add the values in src_reg1 and src_reg2 and store the result in dst_reg
    DeclareApeVar(src_reg1_val, Approx);
    DeclareApeVar(src_reg2_val, Approx);
    Set(src_reg1_val, IndexVector(registers, src_reg1_idx));
    Set(src_reg2_val, IndexVector(registers, src_reg2_idx));
    Set(IndexVector(registers, dst_reg_idx), Add(src_reg1_val, src_reg2_val));
}

void glgp_op_sub(scExpr dst_reg_idx, scExpr src_reg1_idx, scExpr src_reg2_idx) {
    // Subtract the values in src_reg1 and src_reg2 and store the result in dst_reg
    DeclareApeVar(src_reg1_val, Approx);
    DeclareApeVar(src_reg2_val, Approx);
    Set(src_reg1_val, IndexVector(registers, src_reg1_idx));
    Set(src_reg2_val, IndexVector(registers, src_reg2_idx));
    Set(IndexVector(registers, dst_reg_idx), Sub(src_reg1_val, src_reg2_val));
}

void glgp_op_mult(scExpr dst_reg_idx, scExpr src_reg1_idx, scExpr src_reg2_idx) {
    // Multiply the values in src_reg1 and src_reg2 and store the result in dst_reg
    //TODO check if input is approx
    DeclareApeVar(src_reg1_val, Approx);
    DeclareApeVar(src_reg2_val, Approx);
    Set(src_reg1_val, IndexVector(registers, src_reg1_idx));
    Set(src_reg2_val, IndexVector(registers, src_reg2_idx));
    Set(IndexVector(registers, dst_reg_idx), Mul(src_reg1_val, src_reg2_val));
}

void glgp_op_div(scExpr dst_reg_idx, scExpr src_reg1_idx, scExpr src_reg2_idx) {
    // Divide the values in src_reg1 and src_reg2 and store the result in dst_reg
    //TODO Test this works
    DeclareApeVar(src_reg1_val, Approx);
    DeclareApeVar(src_reg2_val, Approx);
    Set(src_reg1_val, IndexVector(registers, src_reg1_idx));
    Set(src_reg2_val, IndexVector(registers, src_reg2_idx));
    DeclareApeVarInit(test, Int, IntConst(0));
    DeclareApeVarInit(test2, Int, IntConst(0));

    // If denominator within this range, treat it as 0
    eApeC(apeTestALT, test, src_reg2_val, AConst(0.0001));
    eApeC(apeTestAGT, test2, src_reg2_val, AConst(-0.0001));

    ApeIf(Not(And(test, test2))); 
        Set(IndexVector(registers, dst_reg_idx), Div(src_reg1_val, src_reg2_val));
    ApeElse();
        Set(IndexVector(registers, dst_reg_idx), AConst(1)); // If div by zero, return 1 instead
    ApeFi();
}

void glgp_op_sqrt(scExpr dst_reg_idx, scExpr src_reg1_idx, scExpr src_reg2_idx) {
    // Take the square root of src_reg1_val and store the result in dst_reg
    DeclareApeVar(src_reg1_val, Approx);
    Set(src_reg1_val, IndexVector(registers, src_reg1_idx));

    ApeIf(Ge(src_reg1_val, AConst(0))); // If we try to take the square root of a negative number, instead do nothing
        Set(IndexVector(registers, dst_reg_idx), Sqrt(src_reg1_val));
    ApeFi();
}

void glgp_op_nop(scExpr dst_reg_idx, scExpr src_reg1_idx, scExpr src_reg2_idx) {
    Add(IntConst(0), IntConst(0)); // No operation
}

OpEntry lgp_op_funcs[] = { // Mapping of function to function name
    { glgp_op_add,  "ADD"  },
    { glgp_op_sub,  "SUB"  },
    { glgp_op_mult, "MULT" },
    // { glgp_op_div,  "DIV"  },
    // { glgp_op_sqrt, "SQRT" },
    { glgp_op_nop,  "NOP"  }
};
// ## End LGP operations ##########


// Register access management
void glgp_resetRegisters(glgp_Config config){
    glgp_GenomeInfo genome_info = glgp_GetGenomeInfo(config);
    DeclareApeVar(reg_value, Approx);
    Set(reg_value, AConst(1));
    for (int i = 0; i < config.num_registers; i++){ // initialize internal registers to hold their index value
        Set(IndexVector(registers, IntConst(i)), reg_value);
        Set(reg_value, Add(reg_value, AConst(1)));
    }

    for (int i = config.num_registers; i < genome_info.total_registers; i++){ // initialize input registers to 0
        Set(IndexVector(registers, IntConst(i)), AConst(0));
    }
}

scExpr glgp_readValueFromLGPRegister(scExpr reg_id){
    DeclareApeVar(ret, Approx);
    Set(ret, IndexVector(registers, reg_id));
    return ret;
}

scExpr glgp_getRegisters(){
  return registers;
}

void glgp_LoadInputRegisters(glgp_Config config, scExpr inputDataArray, scExpr input_row_idx){
    DeclareCUVar(I_cu, Int);
    DeclareCUVar(offset, Int);
    CUFor(I_cu, IntConst(0), IntConst(config.input_size-1), IntConst(1));
        Set(offset, Add(I_cu, IntConst(config.num_registers)));
        Set(IndexVector(registers, offset), IndexArray(inputDataArray, input_row_idx, I_cu));
    CUForEnd();
}


// Main LGP Functions
void glgp_initPopulation(glgp_Config config){
    glgp_GenomeInfo genome_info = glgp_GetGenomeInfo(config);
    unsigned short mem_usage = genome_info.total_registers + genome_info.genome_words;

    if (mem_usage > LGP_MAX_WORD_USAGE){
        printf("Error: Memory usage exceeds maximum allowed words. %d > %d\n", mem_usage, LGP_MAX_WORD_USAGE);
        exit(EXIT_FAILURE);
    }
    printf("APE Memory usage for individual: %d words (Out of %d Allowed)\n\n", mem_usage, LGP_MAX_WORD_USAGE);

    // Initialize the genome as a binary string
    gb_initBinaryString(genome_info.genome_words);
    
    DeclareApeVar(rand_num, Int);
    DeclareCUVar(I_cu, Int);
    DeclareApeVar(offset, Int);
    DeclareApeVar(I_ape, Int);
    Set(offset, IntConst(0));

    // Generate the random genome instructions one at a time
    CUFor(I_cu, IntConst(0), IntConst(config.default_genome_len - 1), IntConst(1));
        // Generate random opcode
        Set(rand_num, gr_randIntMod(config.num_operators));
        gb_set(offset, genome_info.opcode_bits, rand_num);
        Set(offset, Add(offset, IntConst(genome_info.opcode_bits)));

        // Generate random destination register (dest cannot be an input reg)
        Set(rand_num, gr_randIntMod(config.num_registers));
        gb_set(offset, genome_info.reg_id_bits, rand_num);
        Set(offset, Add(offset, IntConst(genome_info.reg_id_bits)));

        // Generate random source register 1
        Set(rand_num, gr_randIntMod(genome_info.total_registers));
        gb_set(offset, genome_info.reg_id_bits, rand_num);
        Set(offset, Add(offset, IntConst(genome_info.reg_id_bits)));

        // Generate random source register 2
        Set(rand_num, gr_randIntMod(genome_info.total_registers));
        gb_set(offset, genome_info.reg_id_bits, rand_num);
        Set(offset, Add(offset, IntConst(genome_info.reg_id_bits)));
    CUForEnd();

    // Initialize lgp registers (input + internal registers)
    ApeMemVector(registers, Approx, genome_info.total_registers);
    glgp_resetRegisters(config);

}

void glgp_MutateGenome(glgp_Config config) {
    //TODO split into generalized subfunctions
    DeclareCUVar(I_cu, Int);
    DeclareCUVar(J_cu, Int);
    DeclareApeVar(offset, Int);
    DeclareApeVar(I_ape, Int);
    DeclareApeVar(rand_num, Int);
    DeclareApeVar(test_res, Int);
    DeclareApeVar(temp, Int);
    Set(offset, IntConst(0));
    unsigned short mut_threshold = (unsigned short)(MAX_INT_VALUE * config.mutation_rate);
    glgp_GenomeInfo genome_info = glgp_GetGenomeInfo(config);

    Set(I_ape, IntConst(0));
    // Set(temp, IntConst(0));
    CUFor(I_cu, IntConst(0), IntConst(config.default_genome_len - 1), IntConst(1));
        // Mutate opcode
        Set(rand_num, gr_genRandomNumber());
        eApeR(apeTestULE, test_res, rand_num, IntConst(mut_threshold))
        ApeIf(test_res); // if random number is less than mutation threshold
            // Mutate the genome at this position
            //gr_randIntScale8bit(config.num_operators, temp);
            Set(temp, gr_randIntMod(config.num_operators));
            gb_set(I_ape, genome_info.opcode_bits, temp);
        ApeFi();
        Set(I_ape, Add(I_ape, IntConst(genome_info.opcode_bits)));

        // Mutate destination register
        Set(rand_num, gr_genRandomNumber());
        eApeR(apeTestULE, test_res, rand_num, IntConst(mut_threshold))
        ApeIf(test_res); // if random number is less than mutation threshold
            //gr_randIntScale8bit(config.num_registers, temp); // only write to non-input reg
            Set(temp, gr_randIntMod(config.num_registers));
            gb_set(I_ape, genome_info.reg_id_bits, temp);
        ApeFi();
        Set(I_ape, Add(I_ape, IntConst(genome_info.reg_id_bits)));

        // Mutate source register 1
        Set(rand_num, gr_genRandomNumber());
        eApeR(apeTestULE, test_res, rand_num, IntConst(mut_threshold))
        ApeIf(test_res); // if random number is less than mutation threshold
            //gr_randIntScale8bit(genome_info.total_registers,  temp);
            Set(temp, gr_randIntMod(genome_info.total_registers));
            gb_set(I_ape, genome_info.reg_id_bits, temp);
        ApeFi();
        Set(I_ape, Add(I_ape, IntConst(genome_info.reg_id_bits)));

        // Mutate source register 2
        Set(rand_num, gr_genRandomNumber());
        eApeR(apeTestULE, test_res, rand_num, IntConst(mut_threshold))
        ApeIf(test_res); // if random number is less than mutation threshold
            //gr_randIntScale8bit(genome_info.total_registers, temp);
            Set(temp, gr_randIntMod(genome_info.total_registers));
            gb_set(I_ape, genome_info.reg_id_bits, temp);
        ApeFi();
        Set(I_ape, Add(I_ape, IntConst(genome_info.reg_id_bits)));
    CUForEnd();
}

void glgp_RunGenome(glgp_Config config){
    DeclareCUVar(instruct_idx, Int);
    DeclareCUVar(op_idx, Int);
    DeclareApeVar(inst_idx_ape, Int);
    DeclareApeVar(inst_start_pos, Int);
    DeclareApeVar(inst_curr_pos, Int);
    DeclareApeVar(inst_opcode, Int);
    DeclareApeVar(inst_dst_reg_idx, Int);
    DeclareApeVar(inst_reg1_idx, Int);
    DeclareApeVar(inst_reg2_idx, Int);

    glgp_GenomeInfo genome_info = glgp_GetGenomeInfo(config);

    Set(inst_curr_pos, IntConst(0));
    CUFor(instruct_idx, IntConst(0), IntConst(config.max_genome_len-1), IntConst(1));
        Set(inst_opcode, gb_get(inst_curr_pos, genome_info.opcode_bits)); // get the opcode from the binary string
        Set(inst_curr_pos, Add(inst_curr_pos, IntConst(genome_info.opcode_bits)));

        Set(inst_dst_reg_idx, gb_get(inst_curr_pos, genome_info.reg_id_bits)); // get the destination register index
        Set(inst_curr_pos, Add(inst_curr_pos, IntConst(genome_info.reg_id_bits)));

        Set(inst_reg1_idx, gb_get(inst_curr_pos, genome_info.reg_id_bits)); // get the first source register index
        Set(inst_curr_pos, Add(inst_curr_pos, IntConst(genome_info.reg_id_bits)));

        Set(inst_reg2_idx, gb_get(inst_curr_pos, genome_info.reg_id_bits)); // get the second source register index
        Set(inst_curr_pos, Add(inst_curr_pos, IntConst(genome_info.reg_id_bits)));

        for (int i = 0; i < config.num_operators; i++) {
            // Check if the opcode matches the current operator
            ApeIf(Eq(inst_opcode, IntConst(i)));
                // Call the corresponding operation function
                lgp_op_funcs[i].func(inst_dst_reg_idx, inst_reg1_idx, inst_reg2_idx);
            ApeFi();
        }
    CUForEnd();         
}
