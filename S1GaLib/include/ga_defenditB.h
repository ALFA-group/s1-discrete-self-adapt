#ifndef GA_DEFENDITB_H
#define GA_DEFENDITB_H

#include <stdbool.h>

/**
 * @brief Configuration settings for the Defend-It game simulation.
 *
 * Contains all parameters needed to configure and run a Defend-It game
 * including resource counts, budgets, mutation parameters, and run settings.
 */
typedef struct {
    unsigned short num_resources;
    unsigned short num_timesteps;
    short res_value;
    short res_cost;
    unsigned short def_budgets[3];
    unsigned short attack_budgets[3];

    int default_mut_threshold;
    unsigned short prob_mut_thresh_inc;
    unsigned short default_prob_large_dist;
    unsigned short prob_large_dist_step;
    unsigned short min_mut_threshold;
    unsigned short max_mut_threshold;
    bool adapt_mut_thresh;
    bool adapt_prob_large_dist;
    unsigned short small_dist_max;
    unsigned short large_dist_max;

    unsigned int fitness_evals;
    unsigned int budget_change_interval;
    unsigned int generations;
    // unsigned short sampling_offset;
    unsigned short checkpoint_interval;
    bool is_local;

    int seed;
} gdib_Settings;

/**
 * @brief Create and return a gdib_Settings struct initialized with default values.
 *
 * @return gdib_Settings struct populated with default configuration values.
 */
gdib_Settings make_default_settings();

/**
 * @brief Print the configuration settings to stdout in a human-readable format.
 *
 * @param config Pointer to the gdib_Settings struct to print.
 */
void gdib_print_settings(gdib_Settings *config);

/**
 * @brief Write the configuration settings to a FILE stream in JSON format.
 *
 * @param f FILE stream to write JSON output to.
 * @param settings Pointer to the gdib_Settings struct to serialize.
 */
void gdib_write_settings_json(FILE* f, const gdib_Settings* settings);

/**
 * @brief Initialize the population for the Defend-It game simulation.
 *
 * Sets up the initial population state based on the provided configuration.
 *
 * @param run_settings Configuration settings to use for initialization.
 */
void gdib_initialize_pop(gdib_Settings run_settings);

/**
 * @brief Compute the final scores for attacker and defender after a game.
 *
 * Calculates final payoffs based on scores, costs, and remaining budgets.
 * Results are written back to the provided score expressions.
 *
 * @param att_score SC expression for attacker score (updated in place).
 * @param def_score SC expression for defender score (updated in place).
 * @param att_cost SC expression containing attacker's total cost.
 * @param def_cost SC expression containing defender's total cost.
 * @param att_move_budget SC expression containing attacker's move budget.
 * @param def_move_budget SC expression containing defender's move budget.
 * @param run_settings Configuration settings for the game.
 */
void gdib_computeFinalScore(scExpr att_score, scExpr def_score, scExpr att_cost, scExpr def_cost, scExpr att_move_budget, scExpr def_move_budget, gdib_Settings run_settings);

/**
 * @brief Execute a single turn of moves for both attacker and defender.
 *
 * Processes the moves, updates costs, budgets, resource ownership, and
 * calculates scores for the current turn.
 *
 * @param att_moves SC expression containing attacker's moves for this turn.
 * @param def_moves SC expression containing defender's moves for this turn.
 * @param att_cost SC expression for attacker's cost (updated in place).
 * @param def_cost SC expression for defender's cost (updated in place).
 * @param att_move_budget SC expression for attacker's remaining budget.
 * @param def_move_budget SC expression for defender's remaining budget.
 * @param curr_res_owners SC expression tracking current resource ownership.
 * @param run_settings Configuration settings for the game.
 * @param ret_att_score SC expression to store attacker's score for this turn.
 * @param ret_def_score SC expression to store defender's score for this turn.
 */
void gdib_playMove(scExpr att_moves, scExpr def_moves, scExpr att_cost, scExpr def_cost, scExpr att_move_budget, scExpr def_move_budget, scExpr curr_res_owners, gdib_Settings run_settings, scExpr ret_att_score, scExpr ret_def_score);

/**
 * @brief Simulate a game where an agent attacker plays against its own defender.
 *
 * Runs a complete game where the same agent controls both attacker and
 * defender roles, useful for self-evaluation.
 *
 * @param run_settings Configuration settings for the game.
 * @param attack_move_budget SC expression for attacker's move budget.
 * @param defend_move_budget SC expression for defender's move budget.
 * @param ret_att_payoff SC expression to store final attacker payoff.
 * @param ret_def_payoff SC expression to store final defender payoff.
 */
void gdib_playAgainstSelf(gdib_Settings run_settings, scExpr attack_move_budget, scExpr defend_move_budget, scExpr ret_att_payoff, scExpr ret_def_payoff);

/**
 * @brief Simulate a game between the current agent and a neighboring agent.
 *
 * Runs a complete game where the current agent plays against a selected
 * neighbor in either the attacker or defender role.
 *
 * @param selected_neighbor SC expression identifying the neighbor to play against.
 * @param play_as_defender If true, current agent plays as defender; if false,
 *                         plays as attacker.
 * @param att_move_budget SC expression for attacker's move budget.
 * @param def_move_budget SC expression for defender's move budget.
 * @param ret_att_payoff SC expression to store final attacker payoff.
 * @param ret_def_payoff SC expression to store final defender payoff.
 * @param run_settings Configuration settings for the game.
 */
void gdib_playAgainstNeighbor(scExpr selected_neighbor, bool play_as_defender, scExpr att_move_budget, scExpr def_move_budget, scExpr ret_att_payoff, scExpr ret_def_payoff, gdib_Settings run_settings);

#endif