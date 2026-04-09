#ifndef GA_CGP_H
#define GA_CGP_H

#include <S1Core/S1Core.h>

void cgp_initGenome(int nb_genomes, int nb_inputs);
void cgp_resetFitness();
void cgp_computeFitnessL1(scExpr outputArr, scExpr idx, int nb_outputs);
void cgp_computeFitnessL2(scExpr outputArr, scExpr idx, int nb_outputs);
void cgp_execAndComputeFitness(scExpr inputArray, scExpr outputArr, scExpr idx, int nb_outputs);
void cgp_copyGenome(scExpr bestNeighbor);
// scExpr cgp_tournament();
void cgp_mutateGenome(float mutationRate);
void cgp_loadData(char* fileName_x, char* fileName_y, int nbLines, uint32_t cuAddr_x, uint32_t cuAddr_y, int x_nb_cols, int y_nb_cols);
void cgp_mainLoop(int nbLines, uint32_t cuAddr_x, uint32_t cuAddr_y, int nb_genomes, int nb_loop, int report_every, int seed);
scExpr cgp_getFitness();
scExpr cgp_getResults();
void cgp_data_callback_writeBestFits(uint16_t* data, int size, void* extraData);
void cgp_data_callback_writeBestGenomes(uint16_t* data, int size, void* extraData);



#endif //GA_CGP_H