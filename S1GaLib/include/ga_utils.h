#ifndef GA_UTILS_H
#define GA_UTILS_H

#include<S1Core/S1Core.h>

extern const unsigned short MAX_INT_VALUE;

/**
 * @brief Get the current APE row within its chip
 *
 * Returns an scExpr representing the APE's row index inside the local chip.
 *
 * @return scExpr (Int) APE row index (0..apeRows-1)
 */
scExpr gu_GetApeRow();

/**
 * @brief Get the current APE column within its chip
 *
 * Returns an scExpr representing the APE's column index inside the local chip.
 *
 * @return scExpr (Int) APE column index (0..apeCols-1)
 */
scExpr gu_GetApeCol();

/**
 * @brief Get the global APE row index across the whole board
 *
 * Computes the APE row index across all chips (chipRows * apeRows). Useful for
 * algorithms that need a global coordinate rather than a per-chip coordinate.
 *
 * @return scExpr (Int) global APE row index
 */
scExpr gu_GetApeGlobalRow();

/**
 * @brief Get the global APE column index across the whole board
 *
 * Computes the APE column index across all chips (chipCols * apeCols). Useful for
 * algorithms that need a global coordinate rather than a per-chip coordinate.
 *
 * @return scExpr (Int) global APE column index
 */
scExpr gu_GetApeGlobalCol();

/**
 * @brief Retrieve a variable from a neighbor APE across chips. WARNING: Do not do this within
 * an ApeIf, it causes issues.
 *
 * Copies the value from `srcVar` on a neighbor in the given direction into `dstVar` on the
 * local APE. `direction` should be one of the directional constants (getNorth/getSouth/getEast/getWest).
 *
 * @param srcVar scExpr describing the source variable to read from
 * @param dstVar scExpr to store the retrieved value in
 * @param direction direction identifier (getNorth/getSouth/getEast/getWest)
 */
void gu_ApeGetGlobal(scExpr srcVar, scExpr dstVar, scExpr direction);

/**
 * @brief Check whether a given direction points outside the local chip.
 *
 * Tests whether moving from the current APE in `direction` would cross a chip
 * edge. Useful for neighbor-selection logic where border handling differs.
 *
 * @param direction directional code (getNorth/getSouth/getEast/getWest)
 * @return scExpr (Int) non-zero if the direction points outside the chip, zero otherwise
 */
scExpr gu_isEdge(scExpr direction);

/**
 * @brief Count the number of set bits in a 16-bit integer using SWAR.
 *
 * Uses a SWAR (SIMD Within A Register) bit-counting method to compute the
 * count of the input expression.
 *
 * @param num scExpr (Int) input value to count set bits in.
 * @return scExpr (Int) number of 1 bits in `num`.
 */
scExpr gu_countOneBitsSWAR(scExpr num);

/**
 * @brief Compute the next power of two >= n
 *
 * Returns the smallest power of two greater than or equal to `n`.
 * If `n` is zero returns 1.
 *
 * @param n input value
 * @return next power of two >= n
 */
unsigned short gu_next_power_of_2(unsigned short n);

/**
 * @brief Number of bits required to represent N distinct values
 *
 * Returns the number of bits required to represent values in the range [0, N-1].
 * For N==0 the function returns 0.
 *
 * @param N the exclusive upper bound of the range
 * @return number of bits required
 */
unsigned short gu_bits_needed(unsigned short N);

#endif