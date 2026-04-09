#ifndef GA_SELECTION_H
#define GA_SELECTION_H

#include <S1Core/S1Core.h>

/**
 * @brief Copy a genome from the selected neighbor into the current genome buffer.
 *
 * @param bestNeighbor SC expression identifying the neighbor genome to copy.
 * @param nbWords Number of 16-bit words in the genome.
 * @param is_local If true, treat each chip as its own local population; otherwise use entire board as one population.
 */
void gs_copyGenome(scExpr bestNeighbor, int nbWords, bool is_local);

/**
 * @brief Compute the list of valid neighboring genomes for selection.
 *
 * @param ret_neighbors SC expression to store the neighbor indices.
 * @param ret_numNeighbors SC expression to store the number of valid neighbors.
 * @param is_local If true, use local chip neighborhood; otherwise use board-wide neighborhood.
 */
void gs_getValidNeighbors(scExpr ret_neighbors, scExpr ret_numNeighbors, bool is_local);

/**
 * @brief Select the elite neighbor based on fitness.
 *
 * @param fitness SC expression containing fitness values.
 * @param is_min If non-zero, lower fitness is better; otherwise higher is better.
 * @param is_local If true, restrict selection to local neighborhood.
 * @return SC expression identifying the selected elite neighbor.
 */
scExpr gs_selectionElite(scExpr fitness, int is_min, bool is_local);

/**
 * @brief Select a neighbor using tournament selection.
 *
 * @param fitness SC expression containing fitness values.
 * @param tourny_size Number of competitors in the tournament.
 * @param is_min If non-zero, lower fitness is better; otherwise higher is better.
 * @param If true, restrict selection to local neighborhood.
 * @return SC expression identifying the selected neighbor.
 */
scExpr gs_selectionTournament(scExpr fitness, int tourny_size, int is_min, bool is_local);

#endif