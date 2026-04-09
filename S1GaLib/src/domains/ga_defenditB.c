#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <S1Core/S1Core.h>
#include <S1GaLib/S1GaLib.h>

unsigned short const MAX_WORD_USAGE = 450; // words of memory for individual, rest is reserved for system

gdib_Settings make_default_settings() {
    gdib_Settings config;
    config.num_resources = 10;
    config.num_timesteps = 40;
    config.res_value = 14;
    config.res_cost = 14;
    unsigned short defBudgets[3] = {28, 280, 2800};
    memcpy(config.def_budgets, defBudgets, sizeof(defBudgets));
    unsigned short attBudgets[3] = {28, 280, 2800};
    memcpy(config.attack_budgets, attBudgets, sizeof(attBudgets));

    config.default_mut_threshold = 16; // default (6553) / MAX_INT (65535) = mut_rate (0.1), random if < 0
    config.prob_mut_thresh_inc = MAX_INT_VALUE / 2; // 50% of MAX_INT
    config.default_prob_large_dist = 1; // x/MAX_INT probability
    config.prob_large_dist_step = 655; // 1% of MAX_INT
    config.min_mut_threshold = 1;
    config.max_mut_threshold = MAX_INT_VALUE;
    config.adapt_mut_thresh = true;
    config.adapt_prob_large_dist = true;
    config.small_dist_max = 6;
    config.large_dist_max = MAX_INT_VALUE / 2;

    config.generations = 299;
    config.budget_change_interval = config.generations / 3;
    config.is_local = false;
    config.checkpoint_interval = 10;

    config.seed = 123;

    return config;
}

void gdib_write_settings_json(FILE* f, const gdib_Settings* settings) {
    fprintf(f, "{\n");
    fprintf(f, "  \"num_resources\": %u,\n", settings->num_resources);
    fprintf(f, "  \"num_timesteps\": %u,\n", settings->num_timesteps);
    fprintf(f, "  \"res_value\": %d,\n", settings->res_value);
    fprintf(f, "  \"res_cost\": %d,\n", settings->res_cost);
    fprintf(f, "  \"def_budgets\": [%u, %u, %u],\n", 
            settings->def_budgets[0], settings->def_budgets[1], settings->def_budgets[2]);
    fprintf(f, "  \"attack_budgets\": [%u, %u, %u],\n", 
            settings->attack_budgets[0], settings->attack_budgets[1], settings->attack_budgets[2]);
    fprintf(f, "  \"default_mut_threshold\": %d,\n", settings->default_mut_threshold);
    fprintf(f, "  \"prob_mut_thresh_inc\": %u,\n", settings->prob_mut_thresh_inc);
    fprintf(f, "  \"default_prob_large_dist\": %u,\n", settings->default_prob_large_dist);
    fprintf(f, "  \"prob_large_dist_step\": %u,\n", settings->prob_large_dist_step);
    fprintf(f, "  \"min_mut_threshold\": %u,\n", settings->min_mut_threshold);
    fprintf(f, "  \"max_mut_threshold\": %u,\n", settings->max_mut_threshold);
    fprintf(f, "  \"adapt_mut_thresh\": %s,\n", settings->adapt_mut_thresh ? "true" : "false");
    fprintf(f, "  \"adapt_prob_large_dist\": %s,\n", settings->adapt_prob_large_dist ? "true" : "false");
    fprintf(f, "  \"small_dist_max\": %u,\n", settings->small_dist_max);
    fprintf(f, "  \"large_dist_max\": %u,\n", settings->large_dist_max);
    fprintf(f, "  \"fitness_evals\": %u,\n", settings->fitness_evals);
    fprintf(f, "  \"budget_change_interval\": %u,\n", settings->budget_change_interval);
    fprintf(f, "  \"generations\": %u,\n", settings->generations);
    // fprintf(f, "  \"sampling_offset\": %u,\n", settings->sampling_offset);
    fprintf(f, "  \"checkpoint_interval\": %u,\n", settings->checkpoint_interval);
    fprintf(f, "  \"is_local\": %s,\n", settings->is_local ? "true" : "false");
    fprintf(f, "  \"seed\": %d\n", settings->seed);
    fprintf(f, "}\n");
    fflush(f);
}

void gdib_print_settings(gdib_Settings *config) {
    gdib_write_settings_json(stdout, config);
}

void gdib_initialize_pop(gdib_Settings run_settings){
    int mem_usage_nb_words = (run_settings.num_resources * run_settings.num_timesteps * 2 + 15) / 16; // *2 to account for both populations
    if (mem_usage_nb_words  > MAX_WORD_USAGE){
        printf("Error: Memory usage exceeds maximum allowed words. %d > %d\n", mem_usage_nb_words , MAX_WORD_USAGE);
        exit(EXIT_FAILURE);
    }
    printf("APE Memory usage for individual: %d words (Out of %d Allowed)\n\n", mem_usage_nb_words , MAX_WORD_USAGE);

    // Initialize the genome as a binary string
    gb_initBinaryString(mem_usage_nb_words);

    DeclareCUVar(cuI, Int);
    DeclareApeVar(apeI, Int);
    CUFor(cuI, IntConst(0), IntConst(mem_usage_nb_words-1), IntConst(1));
        Set(apeI, cuI);
        gb_setVectorEntryExpr(apeI, IntConst(0));
    CUForEnd();
}

void gdib_computeFinalScore(scExpr att_score, scExpr def_score, scExpr att_cost, scExpr def_cost, scExpr att_move_budget, scExpr def_move_budget, gdib_Settings run_settings){ //TODO clean up args
    DeclareApeVar(res_lo, Int);
    DeclareApeVar(res_hi, Int);
    
    gn_uintMul(def_score, IntConst(run_settings.res_value), res_lo, res_hi); //TODO change to signed mult ideally
    Set(def_score, res_lo);
    gn_uintMul(att_score, IntConst(run_settings.res_value), res_lo, res_hi);
    Set(att_score, res_lo);

    ApeIf(Gt(att_cost, att_move_budget));
        Set(att_score, Add(Not(att_cost), IntConst(1))); // same as multiplying cost by -1
    ApeFi();
    ApeIf(Gt(def_cost, def_move_budget));
        Set(def_score, Add(Not(def_cost), IntConst(1))); // same as multiplying cost by -1
    ApeFi();
}

void gdib_playMove(scExpr att_moves, scExpr def_moves, scExpr att_cost, scExpr def_cost, scExpr att_move_budget, scExpr def_move_budget, scExpr curr_res_owners, gdib_Settings run_settings, scExpr ret_att_score, scExpr ret_def_score){ //TODO clean up args
    DeclareApeVar(num_ones, Int);
    DeclareApeVar(att_captures, Int);
    DeclareApeVar(def_captures, Int);

    // Check if overbudget and zero out moves if so
    Set(num_ones, gu_countOneBitsSWAR(att_moves));
    Set(att_cost, Add(num_ones, att_cost))
    ApeIf(Gt(att_cost, att_move_budget));
        Set(att_moves, IntConst(0));
    ApeFi();

    Set(num_ones, gu_countOneBitsSWAR(def_moves));
    Set(def_cost, Add(num_ones, def_cost))
    ApeIf(Gt(def_cost, def_move_budget));
        Set(def_moves, IntConst(0));
    ApeFi();
    
    // Find resources where only attacker or only defender is attempting to capture
    Set(att_captures, And(att_moves, Not(def_moves))); 
    Set(def_captures, And(def_moves, Not(att_moves)));

    // Set bits where attacker captures to 0 and defender captures to 1
    Set(curr_res_owners, And(curr_res_owners, Not(att_captures))); 
    Set(curr_res_owners, Or(curr_res_owners, def_captures));

    // Add round score to total scores
    Set(num_ones, gu_countOneBitsSWAR(curr_res_owners));

    Set(ret_def_score, Add(ret_def_score, num_ones));
    Set(ret_att_score, Add(ret_att_score, Sub(IntConst(run_settings.num_resources), num_ones)));

}

void gdib_playAgainstSelf(gdib_Settings run_settings, scExpr attack_move_budget, scExpr defend_move_budget, scExpr ret_att_payoff, scExpr ret_def_payoff){ //TODO clean up args
    int genome_length = run_settings.num_resources * run_settings.num_timesteps;
    DeclareApeVar(att_genome_pos, Int);
    Set(att_genome_pos, IntConst(0));
    DeclareApeVar(def_genome_pos, Int);
    Set(def_genome_pos, IntConst(genome_length))
    DeclareApeVar(att_score, Int);
    Set(att_score, IntConst(0));
    DeclareApeVar(def_score, Int);
    Set(def_score, IntConst(0));
    DeclareApeVar(att_cost, Int);
    Set(att_cost, IntConst(0));
    DeclareApeVar(def_cost, Int);
    Set(def_cost, IntConst(0));
    DeclareApeVar(att_moves, Int);
    DeclareApeVar(def_moves, Int);

    //TODO check to see if max possible score will fit in 16bits (signed)

    if(run_settings.num_resources > 16){
        printf("S1 Board currently only supports up to 16 defendit-B resources (you requested %d). Please update the run settings and retry.\n\n", run_settings.num_resources);
        exit(EXIT_FAILURE); //TODO implement a version that works for more than 16 resources
    }
    else{
        unsigned short init_owner_bits = (1 << run_settings.num_resources) - 1;
        DeclareApeVar(curr_res_owners, Int);
        Set(curr_res_owners, IntConst(init_owner_bits));

        DeclareCUVar(cuI_play, Int);
        CUFor(cuI_play, IntConst(0), IntConst(run_settings.num_timesteps - 1), IntConst(1));
            // Get the moves made this timestep for each resource
            Set(att_moves, gb_get(att_genome_pos, run_settings.num_resources));
            Set(def_moves, gb_get(def_genome_pos, run_settings.num_resources));

            gdib_playMove(att_moves, def_moves, att_cost, def_cost, attack_move_budget, defend_move_budget, curr_res_owners, run_settings, att_score, def_score);

            // Increment to starting pos in genome for next timestep
            Set(att_genome_pos, Add(att_genome_pos, IntConst(run_settings.num_resources)));
            Set(def_genome_pos, Add(def_genome_pos, IntConst(run_settings.num_resources)));
        CUForEnd();

        gdib_computeFinalScore(att_score, def_score, att_cost, def_cost, attack_move_budget, defend_move_budget, run_settings);

        Set(ret_att_payoff, att_score);
        Set(ret_def_payoff, def_score);
    }

}

void gdib_playAgainstNeighbor(scExpr selected_neighbor, bool play_as_defender, scExpr att_move_budget, scExpr def_move_budget, scExpr ret_att_payoff, scExpr ret_def_payoff, gdib_Settings run_settings){ //TODO clean up args
    int genome_length = run_settings.num_resources * run_settings.num_timesteps;
    DeclareApeVar(att_genome_pos, Int);
    Set(att_genome_pos, IntConst(0));
    DeclareApeVar(def_genome_pos, Int);
    Set(def_genome_pos, IntConst(genome_length))
    DeclareApeVar(att_score, Int);
    Set(att_score, IntConst(0));
    DeclareApeVar(def_score, Int);
    Set(def_score, IntConst(0));
    DeclareApeVar(att_cost, Int);
    Set(att_cost, IntConst(0));
    DeclareApeVar(def_cost, Int);
    Set(def_cost, IntConst(0));

    DeclareApeVar(att_moves, Int);
    DeclareApeVar(def_moves, Int);
    DeclareApeVar(transfer_moves, Int);

    //TODO check to see if max possible score will fit in 16bits (signed)

    if(run_settings.num_resources > 16){
        printf("S1 Board currently only supports up to 16 defendit-B resources (you requested %d). Please update the run settings and retry.\n\n", run_settings.num_resources);
        exit(EXIT_FAILURE); //TODO implement a version that works for more than 16 resources
    }
    else{
        unsigned short init_owner_bits = (1 << run_settings.num_resources) - 1;
        DeclareApeVar(curr_res_owners, Int);
        Set(curr_res_owners, IntConst(init_owner_bits));

        DeclareCUVar(cuI_play, Int);
        CUFor(cuI_play, IntConst(0), IntConst(run_settings.num_timesteps - 1), IntConst(1));
            // Get the moves made this timestep for each resource
            if(play_as_defender){
                Set(transfer_moves, gb_get(att_genome_pos, run_settings.num_resources));
                if(run_settings.is_local){
                    eApeR(apeGet, att_moves, transfer_moves, selected_neighbor);
                }
                else{
                    gu_ApeGetGlobal(transfer_moves, att_moves, selected_neighbor);
                }
                Set(def_moves, gb_get(def_genome_pos, run_settings.num_resources));
            }
            else{
                Set(att_moves, gb_get(att_genome_pos, run_settings.num_resources));
                Set(transfer_moves, gb_get(def_genome_pos, run_settings.num_resources));
                if(run_settings.is_local){
                    eApeR(apeGet, def_moves, transfer_moves, selected_neighbor);
                }
                else{
                    gu_ApeGetGlobal(transfer_moves, def_moves, selected_neighbor);
                }
            }

            gdib_playMove(att_moves, def_moves, att_cost, def_cost, att_move_budget, def_move_budget, curr_res_owners, run_settings, att_score, def_score);

            // Increment to starting pos in genome for next timestep
            Set(att_genome_pos, Add(att_genome_pos, IntConst(run_settings.num_resources)));
            Set(def_genome_pos, Add(def_genome_pos, IntConst(run_settings.num_resources)));
        CUForEnd();

        gdib_computeFinalScore(att_score, def_score, att_cost, def_cost, att_move_budget, def_move_budget, run_settings);
        
        Set(ret_att_payoff, att_score);
        Set(ret_def_payoff, def_score);
    }
}