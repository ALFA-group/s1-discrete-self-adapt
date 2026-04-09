#ifndef GA_RANDOM_H
#define GA_RANDOM_H

/**
 * @brief Sets the seed for the random number generator.
 * @param seed Integer seed value.
 */
void gr_setSeed(int seed);

/**
 * @brief Generates a random 16-bit number (0 - 65,535) and stores it in the provided variable.
 * @return Generated 16-bit APE Int.
 */
scExpr gr_genRandomNumber();

/**
 * @brief Generates a random integer in [0, upper_bound) using modulo division. Can be slightly biased
 * @param upper_bound Upper bound (exclusive) as a scExpr Int.
 * @return Generated random integer as an APE Int.
 */
scExpr gr_randIntModExpr(scExpr upper_bound);

/**
 * @brief Generates a random integer in [0, upper_bound) using modulo division. Can be slightly biased
 * @param upper_bound Upper bound (exclusive) as a integer.
 * @return Generated random integer as an APE Int.
 */
scExpr gr_randIntMod(int upper_bound);

/**
 * @brief Fast method that generates a random integer in [0, upper_bound) using 8-bit scaling. Limited to 8-bit numbers only.
 * @param upper_bound Upper bound (exclusive) as an integer.
 * @return Generated random 8-bit integer as an APE Int.
 */
scExpr gr_randInt8bit(int upper_bound);

/**
 * @brief Initialize a 2D array with random values.
 *
 * Fills the destination array with random 16-bit values.
 *
 * @param dstArray SC expression for the destination array to populate.
 * @param rows Number of rows in the destination array.
 * @param cols Number of columns in the destination array.
 */
void gr_initRandomArray(scExpr dstArray, int rows, int cols);

#endif