#ifndef GA_BINARYSTRING_H
#define GA_BINARYSTRING_H

#include<S1Core/S1Core.h>

/**
 * @brief Initialize a binary string (bit vector) in APE memory
 *
 * Allocates and initializes a binary string as an ApeMemVector of `nb_words` 16-bit
 * integers, allowing storage of `nb_words * 16` bits. This must be called once before
 * using any other gb_* functions. The binary string is stored globally and accessed
 * by subsequent get/set operations.
 *
 * @param nb_words number of 16-bit words to allocate for the binary string
 */
void gb_initBinaryString(int nb_words);

/**
 * @brief Extract a range of bits from the binary string
 *
 * Retrieves `size` consecutive bits starting at bit position `startPos` from the
 * binary string. Handles bits that span across 16-bit vector boundaries. The
 * retrieved bits are packed into the low bits of the returned scExpr.
 *
 * @param startPos starting bit position (as scExpr Int)
 * @param size number of bits to extract (must be <= 16)
 * @return scExpr containing the extracted bits in the low positions
 */
scExpr gb_get(scExpr startPos, int size);

/**
 * @brief Set a range of bits in the binary string
 *
 * Writes `size` bits from `newValue` into the binary string starting at bit position
 * `startPos`. Correctly handles bits that span two adjacent 16-bit vector elements.
 * Clears the target bit range before writing (read-modify-write) to avoid data corruption.
 *
 * @param startPos starting bit position (as scExpr Int)
 * @param size number of bits to write (must be <= 16)
 * @param newValue scExpr Int containing the bits to write (should have size bits or fewer)
 */
void gb_set(scExpr startPos, int size, scExpr newValue);

/**
 * @brief Set a full 16-bit vector entry using an scExpr Int index
 *
 * Writes `val` to the vector element at index `idx` (scExpr Int), replacing all 16 bits.
 * This is a lower-level function; prefer gb_set() for sub-word bit ranges.
 *
 * @param idx vector index (as scExpr)
 * @param val 16-bit value to write
 */
void gb_setVectorEntryExpr(scExpr idx, scExpr val);

/**
 * @brief Get a full 16-bit vector entry using an scExpr Int index
 *
 * Reads and returns the full 16-bit value at vector index `idx` (scExpr Int).
 *
 * @param idx vector index (as scExpr)
 * @return scExpr containing the 16-bit value at that index
 */
scExpr gb_getVectorEntryExpr(scExpr idx);

/**
 * @brief Set a full 16-bit vector entry using an int index
 *
 * Convenience wrapper around gb_setVectorEntryExpr() for compile-time constant indices.
 * Writes `val` to vector element at position `idx`.
 *
 * @param idx vector index (constant)
 * @param val 16-bit value to write
 */
void gb_setVectorEntry(int idx, scExpr val);

/**
 * @brief Get a full 16-bit vector entry using an integer index
 *
 * Convenience wrapper around gb_getVectorEntryExpr() for compile-time constant indices.
 *
 * @param idx vector index (constant)
 * @return scExpr containing the 16-bit value at that index
 */
scExpr gb_getVectorEntry(int idx);

/**
 * @brief Get the entire binary string (internal vector)
 *
 * Returns the underlying ApeMemVector handle. Useful for direct vector operations
 * or passing to other functions that expect a vector reference.
 *
 * @return scExpr representing the entire binary string vector
 */
scExpr gb_getFullVector();

/**
 * @brief Get the size of the binary string in 16-bit words
 *
 * Returns the number of 16-bit words allocated for the binary string
 * (as passed to gb_initBinaryString).
 *
 * @return number of 16-bit words
 */
int gb_getNbWords();

#endif