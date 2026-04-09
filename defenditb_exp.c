#include <stdio.h>
#include <sys/stat.h>

#include<S1Core/S1Core.h>
#include<S1GaLib/S1GaLib.h>

#define DEFAULT_SEED 123
#define MAX_PATH_LENGTH 256
#define FILENAME_BUFFER_SIZE 512
#define SELF_INDEX 4
#define MAX_NEIGHBORS 4
#define DEFAULT_OUTPUT_DIR "run_temp"

typedef struct {
    FILE* attacker_fits;
    FILE* defender_fits;
    FILE* attacker_mut_rates;
    FILE* defender_mut_rates;
    FILE* attacker_prob_large_dist;
    FILE* defender_prob_large_dist;
    FILE* params;
} OutputFiles;

typedef struct {
    int emulated;           // 0 for real, 1 for emulated
    int traceFlags;         // Bits controling how much tracing to do, per scAcceleratorAPI.h
    char outputDir[256];
    int seed;
} ProgramArgs;

// Machine configuration
extern int scTotalCyclesTaken;    // if emulated, total cycles taken to run last kernel
extern MachineState *scEmulatedMachineState;   // if emulated, pointer to emulated machine's state
extern LLKernel *llKernel;

// Size info of the real or emulated machine
int chipRows;
int chipCols;
int apeRows;
int apeCols;


/******************************************************************************
 * I/O FUNCS
 ******************************************************************************/

/**
 * Print usage information and exit
 */
static void printUsageAndExit(const char* programName) {
    fprintf(stderr, "Usage: %s <machine> <trace> <output_dir> [seed]\n\n", programName);
    fprintf(stderr, "Arguments:\n");
    fprintf(stderr, "  <machine>      'real' or 'emulated'\n");
    fprintf(stderr, "  <trace>        Trace flags (Translate | Emit | API | States | Instructions)\n");
    fprintf(stderr, "  <output_dir>   Output directory name\n");
    fprintf(stderr, "  [seed]         Optional seed value (default: %d)\n", DEFAULT_SEED);
    exit(EXIT_FAILURE);
}

/**
 * Process command line arguments
 * Returns: Parsed arguments struct
 * Note: Exits on error via printUsageAndExit()
 */
ProgramArgs processArgs(int argc, char *argv[]) {
    ProgramArgs args = {
        .seed = DEFAULT_SEED
    };
    
    const int MIN_ARGS = 4;  // Changed from 3
    const int MAX_ARGS = 5;
    
    if (argc < MIN_ARGS) {
        fprintf(stderr, "Error: Missing required arguments\n\n");
        printUsageAndExit(argv[0]);
    }
    
    if (argc > MAX_ARGS) {
        fprintf(stderr, "Error: Too many arguments\n\n");
        printUsageAndExit(argv[0]);
    }
    
    // Parse machine type (required)
    if (strcmp(argv[1], "real") == 0) {
        args.emulated = 0;
    } else if (strcmp(argv[1], "emulated") == 0) {
        args.emulated = 1;
    } else {
        fprintf(stderr, "Error: Machine argument must be 'real' or 'emulated', got '%s'\n\n", 
                argv[1]);
        printUsageAndExit(argv[0]);
    }
    
    // Parse trace flags (required)
    args.traceFlags = atoi(argv[2]);
    
    // Parse output directory (required)
    strncpy(args.outputDir, argv[3], sizeof(args.outputDir) - 1);
    args.outputDir[sizeof(args.outputDir) - 1] = '\0';
    
    // Parse optional seed
    if (argc >= 5) {
        args.seed = atoi(argv[4]);
    }
    
    return args;
}

OutputFiles open_output_files(const char* outputDir, int seed) {
    char filename[MAX_PATH_LENGTH];
    OutputFiles files;
    
    snprintf(filename, sizeof(filename), "%s/attacker_fits_seed%d.csv", outputDir, seed);
    files.attacker_fits = fopen(filename, "w");
    
    snprintf(filename, sizeof(filename), "%s/defender_fits_seed%d.csv", outputDir, seed);
    files.defender_fits = fopen(filename, "w");
    
    snprintf(filename, sizeof(filename), "%s/attacker_mutRates_seed%d.csv", outputDir, seed);
    files.attacker_mut_rates = fopen(filename, "w");
    
    snprintf(filename, sizeof(filename), "%s/defender_mutRates_seed%d.csv", outputDir, seed);
    files.defender_mut_rates = fopen(filename, "w");

    snprintf(filename, sizeof(filename), "%s/attacker_prob_large_dist_seed%d.csv", outputDir, seed);
    files.attacker_prob_large_dist = fopen(filename, "w");
    
    snprintf(filename, sizeof(filename), "%s/defender_prob_large_dist_seed%d.csv", outputDir, seed);
    files.defender_prob_large_dist = fopen(filename, "w");

    snprintf(filename, sizeof(filename), "%s/run_params.json", outputDir);
    files.params = fopen(filename, "w");
    
    return files;
}

void close_output_files(OutputFiles* files) {
    if (files->attacker_fits) fclose(files->attacker_fits);
    if (files->defender_fits) fclose(files->defender_fits);
    if (files->attacker_mut_rates) fclose(files->attacker_mut_rates);
    if (files->defender_mut_rates) fclose(files->defender_mut_rates);
    if (files->attacker_prob_large_dist) fclose(files->attacker_prob_large_dist);
    if (files->defender_prob_large_dist) fclose(files->defender_prob_large_dist);
    if (files->params) fclose(files->params);
}


/******************************************************************************
 * EVOLUTION FUNCS
 ******************************************************************************/

void spatialParetoDomSelection(gdib_Settings run_settings, scExpr att_budget, scExpr def_budget, scExpr ret_att_payoff, scExpr ret_def_payoff, scExpr ret_best_idx){
    DeclareApeVar(a1d1_a_payoff, Int);
    DeclareApeVar(a1d1_d_payoff, Int);
    DeclareApeVar(a1d2_a_payoff, Int);
    DeclareApeVar(a1d2_d_payoff, Int);  
    DeclareApeVar(a2d1_a_payoff, Int);
    DeclareApeVar(a2d1_d_payoff, Int);
    DeclareApeVar(att_move_budget, Int);
    DeclareApeVar(def_move_budget, Int);
    DeclareApeMemVector(neighbors, Int, MAX_NEIGHBORS);
    DeclareApeVar(numNeighbors, Int);
    DeclareApeVar(selected_neighbor, Int);

    gs_getValidNeighbors(neighbors, numNeighbors, run_settings.is_local);
    Set(selected_neighbor, gr_randIntModExpr(numNeighbors));
    Set(selected_neighbor, IndexVector(neighbors, selected_neighbor));

    int genome_length = run_settings.num_resources * run_settings.num_timesteps;

    DeclareApeVar(remainder, Int);
    gn_intDiv(att_budget, IntConst(run_settings.res_cost), att_move_budget, remainder);
    gn_intDiv(def_budget, IntConst(run_settings.res_cost), def_move_budget, remainder);

    gdib_playAgainstSelf(run_settings, att_move_budget, def_move_budget, a1d1_a_payoff, a1d1_d_payoff);
    gdib_playAgainstNeighbor(selected_neighbor, false, att_move_budget, def_move_budget, a1d2_a_payoff, a1d2_d_payoff, run_settings);
    gdib_playAgainstNeighbor(selected_neighbor, true, att_move_budget, def_move_budget, a2d1_a_payoff, a2d1_d_payoff, run_settings);

    Set(ret_att_payoff, a1d1_a_payoff)
    Set(ret_def_payoff, a1d1_d_payoff)

    DeclareApeVar(test_res, Bool);
    
    Set(test_res, And(Le(a1d1_d_payoff, a1d2_d_payoff), Le(a1d1_a_payoff, a2d1_a_payoff)))
    ApeIf(test_res);
        Set(ret_best_idx, selected_neighbor)
    ApeElse();
        Set(ret_best_idx, IntConst(SELF_INDEX))
    ApeFi();
}

void mutateMutProbLargeDist(gdib_Settings run_settings, scExpr prob_large_dist){
    DeclareApeVar(uint_res, Int);
    DeclareApeVar(test_res, Int);
    DeclareApeVar(test_res2, Int);
    DeclareApeVar(rand_thresh, Int);
    DeclareApeVarInit(prob_stepsize, Int, IntConst(run_settings.prob_large_dist_step));
    DeclareApeVarInit(prob_mut_thresh_inc, Int, IntConst(run_settings.prob_mut_thresh_inc));

    Set(rand_thresh, gr_genRandomNumber());
    eApeR(apeTestULE, test_res, rand_thresh, prob_mut_thresh_inc);
    ApeIf(test_res);
        eApeR(apeAdd, uint_res, prob_large_dist, prob_stepsize);
        eApeR(apeTestC, test_res2, uint_res, prob_large_dist); //check if the unsigned add overflowed

        ApeIf(test_res2);
            Set(prob_large_dist, IntConst(run_settings.max_mut_threshold));
        ApeElse();
            Set(prob_large_dist, uint_res);
        ApeFi();

    ApeElse();
        eApeR(apeSub, uint_res, prob_large_dist, prob_stepsize);
        eApeR(apeTestUGT, test_res2, uint_res, prob_large_dist); //check if the unsigned sub underflowed
        ApeIf(test_res2);
            Set(prob_large_dist, IntConst(run_settings.min_mut_threshold));
        ApeElse();
            Set(prob_large_dist, uint_res);
        ApeFi();
    ApeFi();
}

void mutateMutThreshold(gdib_Settings run_settings, scExpr mut_threshold, scExpr prob_large_dist){
    DeclareApeVar(uint_res, Int);
    DeclareApeVar(test_res, Int);
    DeclareApeVar(test_res2, Int);
    DeclareApeVar(test_res3, Int);
    DeclareApeVar(rand_thresh, Int);
    DeclareApeVar(mut_stepsize, Int);
    DeclareApeVarInit(prob_mut_thresh_inc, Int, IntConst(run_settings.prob_mut_thresh_inc));
    // Check if we use large distribution or small distribution
    Set(rand_thresh, gr_genRandomNumber());
    Set(test_res3, IntConst(0));
    eApeR(apeTestULE, test_res3, rand_thresh, prob_large_dist);
    ApeIf(test_res3);
        Set(mut_stepsize, gr_randIntMod(run_settings.large_dist_max));
    ApeElse();
        Set(mut_stepsize, gr_randInt8bit(run_settings.small_dist_max));
    ApeFi();

    Set(rand_thresh, gr_genRandomNumber());
    eApeR(apeTestULE, test_res, rand_thresh, prob_mut_thresh_inc);
    ApeIf(test_res);
        eApeR(apeAdd, uint_res, mut_threshold, mut_stepsize);
        eApeR(apeTestC, test_res2, uint_res, mut_threshold); //check if the unsigned add overflowed

        ApeIf(test_res2);
            Set(mut_threshold, IntConst(run_settings.max_mut_threshold));
        ApeElse();
            Set(mut_threshold, uint_res);
        ApeFi();

    ApeElse();
        eApeR(apeSub, uint_res, mut_threshold, mut_stepsize);
        eApeR(apeTestUGT, test_res2, uint_res, mut_threshold); //check if the unsigned sub underflowed
        ApeIf(test_res2);
            Set(mut_threshold, IntConst(run_settings.min_mut_threshold));
        ApeElse();
            Set(mut_threshold, uint_res);
        ApeFi();
    ApeFi();
}

void mutateGenome(gdib_Settings run_settings, unsigned short genome_start, scExpr mut_threshold){
    DeclareCUVar(cui_mut, Int);
    DeclareApeVar(rand_thresh, Int);
    DeclareApeVar(curr_bit, Int);
    DeclareApeVar(test_res, Int);

    CUFor(cui_mut, IntConst(genome_start), IntConst(run_settings.num_resources*run_settings.num_timesteps + genome_start -1), IntConst(1));
        Set(rand_thresh, gr_genRandomNumber());
        eApeR(apeTestULE, test_res, rand_thresh, mut_threshold); // if random number is under mut threshold, flip bit
        ApeIf(test_res);
            Set(curr_bit, gb_get(cui_mut, 1));
            gb_set(cui_mut, 1, Not(curr_bit));
        ApeFi();
    CUForEnd();
}

void replaceIndividuals(gdib_Settings run_settings, scExpr best_idx, scExpr att_payoff, scExpr def_payoff, scExpr att_mut_thresh, scExpr def_mut_thresh, scExpr att_prob_large_dist, scExpr def_prob_large_dist){
    int mem_usage_nb_words = (run_settings.num_resources * run_settings.num_timesteps * 2 + 15) / 16;  
    DeclareApeVar(transfer_att_payoff, Int);
    DeclareApeVar(transfer_def_payoff, Int);
    DeclareApeVar(transfer_att_mut, Int);
    DeclareApeVar(transfer_def_mut, Int);
    DeclareApeVar(transfer_att_prob_large_dist, Int);
    DeclareApeVar(transfer_def_prob_large_dist, Int);

    if(run_settings.is_local){
        eApeR(apeGet, transfer_att_payoff, att_payoff, best_idx);
        eApeR(apeGet, transfer_def_payoff, def_payoff, best_idx);
        eApeR(apeGet, transfer_att_mut, att_mut_thresh, best_idx);
        eApeR(apeGet, transfer_def_mut, def_mut_thresh, best_idx);
        eApeR(apeGet, transfer_att_prob_large_dist, att_prob_large_dist, best_idx);
        eApeR(apeGet, transfer_def_prob_large_dist, def_prob_large_dist, best_idx);
    }
    else{
        gu_ApeGetGlobal(att_payoff, transfer_att_payoff, best_idx);
        gu_ApeGetGlobal(def_payoff, transfer_def_payoff, best_idx);
        gu_ApeGetGlobal(att_mut_thresh, transfer_att_mut, best_idx);
        gu_ApeGetGlobal(def_mut_thresh, transfer_def_mut, best_idx);
        gu_ApeGetGlobal(att_prob_large_dist, transfer_att_prob_large_dist, best_idx);
        gu_ApeGetGlobal(def_prob_large_dist, transfer_def_prob_large_dist, best_idx);
    }

  ApeIf(Lt(best_idx, IntConst(SELF_INDEX))); // Replace if idx != self (4)
        Set(att_payoff, transfer_att_payoff);
        Set(def_payoff, transfer_def_payoff);
        Set(att_mut_thresh, transfer_att_mut);
        Set(def_mut_thresh, transfer_def_mut);
        Set(att_prob_large_dist, transfer_att_prob_large_dist);
        Set(def_prob_large_dist, transfer_def_prob_large_dist);
  ApeFi();

  gs_copyGenome(best_idx, mem_usage_nb_words, run_settings.is_local);
}

void getCurrBudget(gdib_Settings run_settings, scExpr curr_att_budget, scExpr curr_def_budget, scExpr budget_idx){
    CUIf(Eq(budget_idx, IntConst(0)));
        Set(curr_att_budget, IntConst(run_settings.attack_budgets[0]));
        Set(curr_def_budget, IntConst(run_settings.def_budgets[0]));
    CUFi();
    CUIf(Eq(budget_idx, IntConst(1)));
        Set(curr_att_budget, IntConst(run_settings.attack_budgets[1]));
        Set(curr_def_budget, IntConst(run_settings.def_budgets[1]));
    CUFi();
    CUIf(Eq(budget_idx, IntConst(2)));
        Set(curr_att_budget, IntConst(run_settings.attack_budgets[2]));
        Set(curr_def_budget, IntConst(run_settings.def_budgets[2]));
    CUFi();
    // CUIf(Eq(budget_idx, IntConst(3)));
    //     Set(curr_att_budget, IntConst(run_settings.attack_budgets[3]));
    //     Set(curr_def_budget, IntConst(run_settings.def_budgets[3]));
    // CUFi();
    // CUIf(Eq(budget_idx, IntConst(4)));
    //     Set(curr_att_budget, IntConst(run_settings.attack_budgets[4]));
    //     Set(curr_def_budget, IntConst(run_settings.def_budgets[4]));
    // CUFi();
    // CUIf(Eq(budget_idx, IntConst(5)));
    //     Set(curr_att_budget, IntConst(run_settings.attack_budgets[5]));
    //     Set(curr_def_budget, IntConst(run_settings.def_budgets[5]));
    // CUFi();
    // CUIf(Eq(budget_idx, IntConst(6)));
    //     Set(curr_att_budget, IntConst(run_settings.attack_budgets[6]));
    //     Set(curr_def_budget, IntConst(run_settings.def_budgets[6]));
    // CUFi();
    // CUIf(Eq(budget_idx, IntConst(7)));
    //     Set(curr_att_budget, IntConst(run_settings.attack_budgets[7]));
    //     Set(curr_def_budget, IntConst(run_settings.def_budgets[7]));
    // CUFi();
    // CUIf(Eq(budget_idx, IntConst(8)));
    //     Set(curr_att_budget, IntConst(run_settings.attack_budgets[8]));
    //     Set(curr_def_budget, IntConst(run_settings.def_budgets[8]));
    // CUFi();
    // CUIf(Eq(budget_idx, IntConst(9)));
    //     Set(curr_att_budget, IntConst(run_settings.attack_budgets[9]));
    //     Set(curr_def_budget, IntConst(run_settings.def_budgets[9]));
    // CUFi();
}

void defenditBKernel(gdib_Settings run_settings, OutputFiles* out_files){
    eCUC(cuSetMaskMode, _, _, 1); // turn mask mode on
    gr_setSeed(run_settings.seed);
    
    gdib_initialize_pop(run_settings);

    int att_genome_start = 0;
    int def_genome_start = run_settings.num_resources * run_settings.num_timesteps;

    DeclareApeVar(att_mut_threshold, Int);
    DeclareApeVar(def_mut_threshold, Int);
    DeclareApeVar(att_prob_large_dist, Int);
    DeclareApeVar(def_prob_large_dist, Int);

    if (run_settings.default_mut_threshold < 0){
        Set(att_mut_threshold, gr_genRandomNumber());
        Set(def_mut_threshold, gr_genRandomNumber());
    }
    else{
        Set(att_mut_threshold, IntConst(run_settings.default_mut_threshold));
        Set(def_mut_threshold, IntConst(run_settings.default_mut_threshold));
    }

    Set(att_prob_large_dist, IntConst(run_settings.default_prob_large_dist));
    Set(def_prob_large_dist, IntConst(run_settings.default_prob_large_dist));

    DeclareApeVar(curr_att_budget, Int);
    DeclareApeVar(curr_def_budget, Int);
    DeclareApeVar(att_payoff, Int);
    DeclareApeVar(def_payoff, Int);
    DeclareApeVar(best_idx, Int);
    DeclareCUMemArray(best_payoffs_cu, Approx, chipRows, chipCols);

    DeclareCUVarInit(gen_counter, Int, IntConst(0));
    DeclareCUVarInit(checkpoint_counter, Int, IntConst(0));
    DeclareCUVarInit(curr_budget_idx, Int, IntConst(0));
    DeclareCUVar(gen, Int);

    // Main evolutionary loop
    CUFor(gen, IntConst(0), IntConst(run_settings.generations), IntConst(1));
        CUIf(Eq(gen_counter, IntConst(0)));
            // gd_printCUVar(gen, "Generation:");
            getCurrBudget(run_settings, curr_att_budget, curr_def_budget, curr_budget_idx);
            // gd_printCUVar(gen, "BUDGET CHANGE GEN: ");
        CUFi();

        spatialParetoDomSelection(run_settings, curr_att_budget, curr_def_budget, att_payoff, def_payoff, best_idx);
        replaceIndividuals(run_settings, best_idx, att_payoff, def_payoff, att_mut_threshold, def_mut_threshold, att_prob_large_dist, def_prob_large_dist);

        CUIf(Eq(checkpoint_counter, IntConst(0)));
            gd_writeApeVarFullBoardToCSV(att_payoff, out_files->attacker_fits);
            gd_writeApeVarFullBoardToCSV(def_payoff, out_files->defender_fits);
            gd_writeApeVarFullBoardToCSV(att_mut_threshold, out_files->attacker_mut_rates);
            gd_writeApeVarFullBoardToCSV(def_mut_threshold, out_files->defender_mut_rates);
            gd_writeApeVarFullBoardToCSV(att_prob_large_dist, out_files->attacker_prob_large_dist);
            gd_writeApeVarFullBoardToCSV(def_prob_large_dist, out_files->defender_prob_large_dist);
        CUFi();

        if (run_settings.adapt_prob_large_dist){
            mutateMutProbLargeDist(run_settings, att_prob_large_dist);
            mutateMutProbLargeDist(run_settings, def_prob_large_dist);
        }
    
        if (run_settings.adapt_mut_thresh){
            mutateMutThreshold(run_settings, att_mut_threshold, att_prob_large_dist);
            mutateMutThreshold(run_settings, def_mut_threshold, def_prob_large_dist);
        }

        mutateGenome(run_settings, att_genome_start, att_mut_threshold);
        mutateGenome(run_settings, def_genome_start, def_mut_threshold);

        Set(gen_counter, Add(gen_counter, IntConst(1)));
        Set(checkpoint_counter, Add(checkpoint_counter, IntConst(1)));
        CUIf(Eq(gen_counter, IntConst(run_settings.budget_change_interval)));
            Set(curr_budget_idx, Add(curr_budget_idx, IntConst(1)));
            Set(gen_counter, IntConst(0));
        CUFi();

        CUIf(Eq(checkpoint_counter, IntConst(run_settings.checkpoint_interval)));
            Set(checkpoint_counter, IntConst(0));
        CUFi();
    CUForEnd();

    spatialParetoDomSelection(run_settings, curr_att_budget, curr_def_budget, att_payoff, def_payoff, best_idx); // do final eval

    gd_writeApeVarFullBoardToCSV(att_payoff, out_files->attacker_fits);
    gd_writeApeVarFullBoardToCSV(def_payoff, out_files->defender_fits);
    gd_writeApeVarFullBoardToCSV(att_mut_threshold, out_files->attacker_mut_rates);
    gd_writeApeVarFullBoardToCSV(def_mut_threshold, out_files->defender_mut_rates);
    gd_writeApeVarFullBoardToCSV(att_prob_large_dist, out_files->attacker_prob_large_dist);
    gd_writeApeVarFullBoardToCSV(def_prob_large_dist, out_files->defender_prob_large_dist);
    gd_printCUVar(gen, "Done at Gen:");

    gd_transferFinish();
}


/******************************************************************************
 * MAIN
 ******************************************************************************/

int main (int argc, char *argv[]) {
    ProgramArgs args = processArgs(argc, argv);

    // Create a machine
    if (args.emulated){
        chipRows = 2; //[1 ... 4]
        chipCols = 2; //[1 ... 4]
        apeRows = 11; //[1 ... 48]
        apeCols = 10; //[1 ... 44]
    }
    else{
        chipRows = 4; //[1 ... 4]
        chipCols = 4; //[1 ... 4]
        apeRows = 48; //[1 ... 48]
        apeCols = 44; //[1 ... 44]
    }
    gset_initS1Machine(args.emulated, chipRows, chipCols, apeRows, apeCols, args.traceFlags);

    //---DEFENDIT-SETTINGS---
    gdib_Settings run_settings = make_default_settings();
    run_settings.seed = args.seed;
    // unsigned short newBudgets[10] = {14*230, 14*80, 14*150, 14*340, 14*90, 14*260, 14*170, 14*40, 14*310, 14*120};
    // memcpy(run_settings.def_budgets, newBudgets, sizeof(newBudgets));
    // memcpy(run_settings.attack_budgets, newBudgets, sizeof(newBudgets));
    run_settings.generations = 299;
    run_settings.budget_change_interval = run_settings.generations / 3;
    run_settings.checkpoint_interval = 1;
    run_settings.adapt_mut_thresh = true;
    run_settings.adapt_prob_large_dist = true;
    run_settings.default_prob_large_dist = 1;
    gdib_print_settings(&run_settings);

    //---OUTPUT-SETUP---
    char runDir[FILENAME_BUFFER_SIZE];
    char filename[FILENAME_BUFFER_SIZE];
    snprintf(runDir, sizeof(runDir), "./output/%s/", args.outputDir);
    mkdir(runDir, 0755);
    OutputFiles out_files = open_output_files(runDir, run_settings.seed);
    gdib_write_settings_json(out_files.params, &run_settings);

    gset_startKernelCreate();
    //---KERNEL-CODE---

    defenditBKernel(run_settings, &out_files);

    //---END-KERNEL---
    gset_compileAndRunKernel();

    gd_waitForTransfers();
    close_output_files(&out_files);

    gset_stopS1Machine();
    return 0;
}