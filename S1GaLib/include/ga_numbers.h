#ifndef GA_NUMBERS_H
#define GA_NUMBERS_H

#include<S1Core/S1Core.h>

/**
 * @brief Convert a 16-bit unsigned integer into the approximate (floating) representation used by the APE
 *
 * Performs the conversion used by the APE instruction set: for each set bit in the input this
 * function accumulates the corresponding Approx(2^i) value into an Approx result.
 *
 * @param uintX input integer (as an scExpr) to convert
 * @return scExpr representing the converted Approx value
 */
scExpr gn_intToApprox(scExpr uintX);

/**
 * @brief Compute base^exp for Approx values
 *
 * Repeatedly multiplies `base` by itself `exp` times and returns the result as an Approx scExpr. The
 * exponent cannot be an APEVar since the loop must be executed on the CU.
 * This is a helper for other numerical routines that need power on APE approximated types.
 *
 * @param base base value (Approx) expressed as scExpr
 * @param exp exponent (Int) expressed as scExpr
 * @return scExpr result = base ^ exp
 */
scExpr gn_powApprox(scExpr base, scExpr exp);

/**
 * @brief Integer division: quotient and remainder
 *
 * Computes quotient and remainder of `numer / denomin` and writes results into the provided
 * scExpr variables. Handles sign/absolute conversion internally so the result follows
 * two's-complement semantics for negative values.
 *
 * @param num dividend (scExpr)
 * @param denom divisor (scExpr)
 * @param quotient output scExpr (Int) that will contain the quotient
 * @param remainder output scExpr (Int) will contain the remainder
 */
void gn_intDiv(scExpr num, scExpr denom, scExpr quotient, scExpr remainder);

/**
 * @brief Faster integer division when number of bits is known
 *
 * Variant of gn_intDiv that iterates only over the specified number of bits (`num_bits`) for
 * the dividend. Use when the bit-width of operands is known to improve performance.
 *
 * @param numer dividend scExpr (Int)
 * @param denomin divisor scExpr (Int)
 * @param num_bits number of bits in `num` to process
 * @param quotient output scExpr (Int) for quotient
 * @param remainder output scExpr (Int) for remainder
 */
void gn_intDivFast(scExpr numer, scExpr denomin, int num_bits, scExpr quotient, scExpr remainder);

/**
 * @brief Unsigned 16-bit multiplication producing 32-bit result
 *
 * Multiplies `nbA * nbB` producing a 32-bit result split across `ret_lo` (low 16 bits)
 * and `ret_hi` (high 16 bits). Both inputs are treated as unsigned 16-bit values.
 *
 * @param nbA multiplicand (scExpr Int)
 * @param nbB multiplier (scExpr Int)
 * @param ret_lo output scExpr (Int) for low 16 bits of result
 * @param ret_hi output scExpr (Int) for high 16 bits of result
 */
void gn_uintMul(scExpr nbA, scExpr nbB, scExpr ret_lo, scExpr ret_hi);

/**
 * @brief Integer multiplication implemented on the CU
 *
 * Simple multiply implemented with repeated addition on the CU. Useful when one operand
 * is small or when CU-based multiply is preferred.
 *
 * @param nbA multiplicand (scExpr Int)
 * @param nbB multiplier (scExpr Int)
 * @return scExpr (Int) containing the product
 */
scExpr gn_intMulCU(scExpr nbA, scExpr nbB);

/**
 * @brief Scale a 16-bit scExpr (Int) into range [0, upper_bound)
 *
 * Implements a bitwise range-scaling algorithm that maps a 16-bit integer into the
 * target range with low bias for small upper_bound values (upper_bound <= 255).
 *
 * @param num input 16-bit number (scExpr Int)
 * @param upper_bound exclusive upper bound for the returned value
 * @return scExpr (Int) the scaled value in [0, upper_bound)
 */
scExpr gn_bitwiseRangeScaling(scExpr num, int upper_bound);

/**
 * @brief Coordinate-wise minimization across APEs
 *
 * Collects the minimum fitness value across a board region and records the APE coordinates
 * of the best fitness in `apeRowBestFitness`/`apeColBestFitness`. Typically used to find
 * the location of the best candidate within a neighborhood.
 *
 * @param fitness local fitness value (scExpr)
 * @param bestFitnessTransfer working scExpr used to transfer candidate fitness values
 * @param apeRowBestFitness output scExpr set to row of best fitness
 * @param apeColBestFitness output scExpr set to column of best fitness
 */
void gn_coordMin(scExpr fitness, scExpr bestFitnessTransfer, scExpr apeRowBestFitness, scExpr apeColBestFitness);

/**
 * @brief Coordinate-wise maximization across APEs
 *
 * Same as gn_coordMin but selects the maximum fitness and records the coordinates.
 *
 * @param fitness local fitness value (scExpr)
 * @param bestFitnessTransfer working scExpr used to transfer candidate fitness values
 * @param apeRowBestFitness output scExpr set to row of best fitness
 * @param apeColBestFitness output scExpr set to column of best fitness
 */
void gn_coordMax(scExpr fitness, scExpr bestFitnessTransfer, scExpr apeRowBestFitness, scExpr apeColBestFitness);

#endif