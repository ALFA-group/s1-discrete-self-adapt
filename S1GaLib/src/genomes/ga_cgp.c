
// #include <S1Core/S1Core.h>
// #include <S1GaLib/S1GaLib.h>


int cgp_bitsPerId = 8;
int cgp_bitsPerOp = 2;
int cgp_nbOfOperation = 3;
extern int chipRows, chipCols, apeRows, apeCols;

// Declare(results);
// Declare(fitness);
// static int nbGenomes;
// static int nbInputs;
// static int nbWords;

// static int isInit = 0;


// void cgp_initGenome(int nb_genomes, int nb_inputs){

//   DeclareCUVar(I_cu, Int);
//   DeclareCUVar(J_cu, Int);
//   DeclareApeVar(transfer, Int);
//   DeclareApeVar(startPos, Int);

//   ApeMemVector(results, Approx, nb_genomes);
//   ApeVar(fitness, Approx);

//   nbGenomes = nb_genomes;
//   nbInputs = nb_inputs;

//   int bitsPerGenome = (2*cgp_bitsPerId) + cgp_bitsPerOp;
//   nbWords = (bitsPerGenome *nb_genomes)/16 + 1;

//   gb_initBinaryString(nbWords);
//   Set(startPos, IntConst(0));
//   CUFor(I_cu, IntConst(0), IntConst(nb_inputs-1), IntConst(1));
//     CUFor(J_cu, IntConst(0), IntConst(1), IntConst(1));
//       gb_set(startPos, cgp_bitsPerId, IntConst(0));

//       Set(startPos, Add(startPos, IntConst(cgp_bitsPerId)));


//     CUForEnd();

//   gb_set(startPos, cgp_bitsPerOp, IntConst(0));
//   Set(startPos, Add(startPos, IntConst(cgp_bitsPerOp)));

//   CUForEnd();


//   //we start after the nb inputs
//   CUFor(I_cu, IntConst(nb_inputs), IntConst(nb_genomes-1), IntConst(1));

//     //gn_intMul(I_cu, IntConst(bitsPerGenome), startPos);

//     //if we want to have more than 2 inputs this should be fairly easy to change it
//     CUFor(J_cu, IntConst(0), IntConst(1), IntConst(1));

//       Set(transfer, gr_randIntModExpr(I_cu)); //gen random int
//       gb_set(startPos, cgp_bitsPerId, transfer); //write random number to the binary string

//       Set(startPos, Add(startPos, IntConst(cgp_bitsPerId))); //increment startPos to write the 2nd id

//     CUForEnd();

//     Set(transfer, gr_randIntMod(cgp_nbOfOperation));
//     gb_set(startPos, cgp_bitsPerOp, transfer);
//     Set(startPos, Add(startPos, IntConst(cgp_bitsPerOp)));

//   CUForEnd();

//   isInit = 1;

// }

// void cgp_executeGenome(scExpr inputArray, scExpr idx){

//   if(isInit != 1){
//     printf("[ERROR] : The cgp init has not been called, it must be called before using cgp_executeGenome");
//     return;
//   }

//   //1. we set the results to the input
//   //2. then we execute each node with a loop
//   //3. we send the results somewhere

//   DeclareCUVar(I_cu, Int);
//   DeclareCUVar(startPos, Int);
//   DeclareApeVar(transfer, Int);
//   DeclareApeVar(nodeId1, Int);
//   DeclareApeVar(nodeId2, Int);

//   //DeclareApeVar(debug, Approx);


//   Set(startPos, IntConst(nbInputs*(2*cgp_bitsPerId + cgp_bitsPerOp)));

//   //00 = +, 01 == -, 10 == *.

//   CUFor(I_cu, IntConst(0), IntConst(nbInputs-1), IntConst(1));
//     Set(IndexVector(results, I_cu), IndexArray(inputArray, idx, I_cu));
//   CUForEnd();


//   CUFor(I_cu, IntConst(nbInputs), IntConst(nbGenomes-1), IntConst(1));

//     //reset the state from previous calculation
//     Set(IndexVector(results, I_cu), AConst(0));

//     //get node ids and opcode

//     Set(nodeId1, gb_get(startPos, cgp_bitsPerId));
//     Set(startPos, Add(startPos, IntConst(cgp_bitsPerId)));
//     Set(nodeId2, gb_get(startPos, cgp_bitsPerId));
//     Set(startPos, Add(startPos, IntConst(cgp_bitsPerId)));
//     Set(transfer, gb_get(startPos, cgp_bitsPerOp));
//     Set(startPos, Add(startPos, IntConst(cgp_bitsPerOp)));

//     //debug genome binary reading
//     /*TraceMessage("I_cu : ");
//     TraceRegisterCU(I_cu);
//     TraceMessage("---node id1 : ");
//     TraceOneRegisterOneApe(nodeId1, 17, 26);
//     TraceMessage("---node id2 : ");
//     TraceOneRegisterOneApe(nodeId2, 17, 26);
//     TraceMessage("---operator : ");
//     TraceOneRegisterOneApe(transfer, 17, 26);*/

//     /*TraceMessage("I_cu : ");
//     TraceRegisterCU(I_cu);
//     TraceMessage("---val node1 : ");
//     Set(debug, IndexVector(results, nodeId1));
//     TraceOneRegisterOneApe(debug, 0, 0);
//     TraceMessage("---val node2 : ");
//     Set(debug, IndexVector(results, nodeId2));
//     TraceOneRegisterOneApe(debug, 0, 0);
//     TraceMessage("---operator : ");
//     TraceOneRegisterOneApe(transfer, 0, 0);*/


//     ApeIf(Eq(transfer, IntConst(0)));
//       //00 == +
//       Set(IndexVector(results, I_cu), Add(IndexVector(results, nodeId1), IndexVector(results, nodeId2)));
//    	  //DEBUG
//       //Set(IndexVector(results, I_cu), AConst(1.f));


//     ApeFi();

//     ApeIf(Eq(transfer, IntConst(1)));
//       //01 == -
//       Set(IndexVector(results, I_cu), Sub(IndexVector(results, nodeId1), IndexVector(results, nodeId2)));

//       //DEBUG
//       //Set(IndexVector(results, I_cu), AConst(1.f));

//     ApeFi();

//     ApeIf(Eq(transfer, IntConst(2)));
//       //10 == *
//       Set(IndexVector(results, I_cu), Mul(IndexVector(results, nodeId1), IndexVector(results, nodeId2)));

//       //DEBUG
//       //Set(IndexVector(results, I_cu), AConst(1.f));

//     ApeFi();

//     //TraceMessage("---------\n");

//   CUForEnd();


//   //debugFitness computation
//   /*TraceMessage("Fitness for (39, 7) of line idx=");
//   TraceRegisterCU(idx);
//   TraceMemVectorOneApe(results, 17, 26, approxFormat);*/

// }

// void cgp_resetFitness(){
//   Set(fitness, AConst(0.0));
// }

// void cgp_computeFitnessL1(scExpr outputArr, scExpr idx, int nb_outputs){
//   DeclareCUVar(I_cu, Int);

//   CUFor(I_cu, IntConst(1), IntConst(nb_outputs), IntConst(1)); //for i in [1 --> nbOutputs]

//     ApeIf(Gt( //if results[nbGenomes - i] > outputArr[idx, i-1]
//       IndexVector(results, Sub(IntConst(nbGenomes), I_cu)),
//       IndexArray(outputArr, idx, Sub(I_cu, IntConst(1)))
//     ));

//       Set(fitness, Add(
//         fitness,
//         Sub(
//           IndexVector(results, Sub(IntConst(nbGenomes), I_cu)),
//           IndexArray(outputArr, idx, Sub(I_cu, IntConst(1)))
//         )
//       ));

//     ApeElse(); //if output is bigger than result

//       Set(fitness, Add(
//         fitness,
//         Sub(
//           IndexArray(outputArr, idx, Sub(I_cu, IntConst(1))),
//           IndexVector(results, Sub(IntConst(nbGenomes), I_cu))
//         )
//       ));

//     ApeFi();

//   CUForEnd();


//   ApeIf(Eq(
//     And(As(Int, fitness), IntConst(NAN_MASK)),
//     IntConst(NAN_MASK)
//   ));
//   	Set(fitness, As(Approx, IntConst(approxMax)));
//   ApeFi();
// }

// void cgp_computeFitnessL2(scExpr outputArr, scExpr idx, int nb_outputs){
//   DeclareCUVar(I_cu, Int);

//   CUFor(I_cu, IntConst(1), IntConst(nb_outputs), IntConst(1)); //for i in [1 --> nbOutputs]

//     Set(fitness, Add(
//       fitness,
//         Mul(
//           Sub(
//             IndexArray(outputArr, idx, Sub(I_cu, IntConst(1))),
//             IndexVector(results, Sub(IntConst(nbGenomes), I_cu))
//       	  ),
//           Sub(
//             IndexArray(outputArr, idx, Sub(I_cu, IntConst(1))),
//             IndexVector(results, Sub(IntConst(nbGenomes), I_cu))
//           )
//         )
//     ));

//   CUForEnd();

//   ApeIf(Eq(
//     And(As(Int, fitness), IntConst(NAN_MASK)),
//     IntConst(NAN_MASK)
//   ));
//   	Set(fitness, As(Approx, IntConst(approxMax)));
//   ApeFi();

// }


// void cgp_execAndComputeFitness(scExpr inputArray, scExpr outputArr, scExpr idx, int nb_outputs){
//   cgp_executeGenome(inputArray, idx);
//   cgp_computeFitnessL2(outputArr, idx, nb_outputs);
// }


// void cgp_copyGenome(scExpr bestNeighbor){

//   DeclareApeVar(data, Int);
//   DeclareCUVar(I_cu, Int);
//   Set(data, IntConst(0));

//   CUFor(I_cu, IntConst(0), IntConst(nbWords-1), IntConst(1));
//     Set(data, IndexVector(gb_getFullVector(), I_cu));

//     ApeIf(Ne(bestNeighbor, IntConst(4)));
//       eApeC(apeGet, data, data, bestNeighbor);
//       Set(IndexVector(gb_getFullVector(), I_cu), data);
//     ApeFi();

//   CUForEnd();


// }


// void cgp_mutateGenome(float mutationRate){
//   float ones15bits = 32767.0f;
//   int upperBound = (int)floor(ones15bits * mutationRate);

//   DeclareApeVar(randTransfer, Int);
//   DeclareCUVar(I_cu, Int);
//   DeclareCUVar(J_cu, Int);
//   DeclareCUVar(writePos, Int);
//   Set(writePos, IntConst(0));


//   CUFor(I_cu, IntConst(0), IntConst(nbGenomes - 1), IntConst(1));

//     CUFor(J_cu, IntConst(0), IntConst(1), IntConst(1));
//       Set(randTransfer, gr_genRandomNumber());
//       Set(randTransfer, And(randTransfer, IntConst(0b0111111111111111))); //make it unsigned

//       ApeIf(Lt(randTransfer, IntConst(upperBound)));
//         //we do a change
//         Set(randTransfer, gr_randIntModExpr(I_cu));
//         gb_set(writePos, cgp_bitsPerId, randTransfer);

//       ApeFi();

//       Set(writePos, Add(writePos, IntConst(cgp_bitsPerId)));

//     CUForEnd();

//     Set(randTransfer, gr_genRandomNumber());
//     Set(randTransfer, And(randTransfer, IntConst(0b0111111111111111))); //make it unsigned

//     ApeIf(Lt(randTransfer, IntConst(upperBound)));

//       Set(randTransfer, gr_randIntModExpr(IntConst(cgp_nbOfOperation)));
//       gb_set(writePos, cgp_bitsPerOp, randTransfer);

//     ApeFi();

//     Set(writePos, Add(writePos, IntConst(cgp_bitsPerOp)));


//   CUForEnd();


// }


// void cgp_loadData(char* fileName_x, char* fileName_y, int nbLines, uint32_t cuAddr_x, uint32_t cuAddr_y, int x_nb_cols, int y_nb_cols){
//   gd_loadDataset(fileName_x, nbLines, x_nb_cols, cuAddr_x, ',');
//   gd_loadDataset(fileName_y, nbLines, y_nb_cols, cuAddr_y, ',');

// }

// void cgp_printDatasetHead(scExpr x_data, scExpr y_data, int nb_inputs, int nb_outputs){
//   int headSize = 5;
//   DeclareCUVar(I_cu, Int);
//   DeclareCUVar(J_cu, Int);
//   DeclareCUVar(debug, Approx);

//   TraceMessage("--------\n");
//   CUFor(I_cu, IntConst(0), IntConst(headSize - 1), IntConst(1));
//     CUFor(J_cu, IntConst(0), IntConst(nb_inputs - 1), IntConst(1));
//       TraceMessage("--x : ");
//       Set(debug, IndexArray(x_data, I_cu, J_cu));
//       TraceRegisterCU(debug);

//     CUForEnd();

//     CUFor(J_cu, IntConst(0), IntConst(nb_outputs - 1), IntConst(1));
//       TraceMessage("--y : ");
//       Set(debug, IndexArray(y_data, I_cu, J_cu));
//       TraceRegisterCU(debug);

//     CUForEnd();

//   CUForEnd();


// }


// void cgp_printBestFitnessEachChip(scExpr bestFitnesses, scExpr bestFitApeRows, scExpr bestFitApeCols){
//   //bestFitness bestFitnessApeRow bestFitnessApeCol

//   DeclareCUVar(I_cu, Int);
//   DeclareCUVar(J_cu, Int);
//   DeclareCUVar(debugA, Approx);
//   DeclareCUVar(debugI, Int);


//   CUFor(I_cu, IntConst(0), IntConst(chipRows - 1), IntConst(1));
//     CUFor(J_cu, IntConst(0), IntConst(chipCols - 1), IntConst(1));
//       TraceMessage("--------\n");
//       TraceMessage("-- apeRow : ");
//       Set(debugI, IndexArray(bestFitApeRows, I_cu, J_cu));
//       TraceRegisterCU(debugI);
//       TraceMessage("-- apeCol : ");
//       Set(debugI, IndexArray(bestFitApeCols, I_cu, J_cu));
//       TraceRegisterCU(debugI);
//       TraceMessage("-- fitness : ");
//       Set(debugA, IndexArray(bestFitnesses, I_cu, J_cu));
//       TraceRegisterCU(debugA);

//     CUForEnd();
//   CUForEnd();


// }

// void cgp_printChosenGenomes(scExpr genomesArr){
//   DeclareApeMemVector(writeZone, Int, nbWords);
//   DeclareCUVar(I_cu, Int);
//   DeclareCUVar(J_cu, Int);

//   CUFor(I_cu, IntConst(0), IntConst(chipRows * chipCols-1), IntConst(1));
//     CUFor(J_cu, IntConst(0), IntConst(nbWords-1), IntConst(1));
//       Set(IndexVector(writeZone, J_cu), IndexArray(genomesArr, I_cu, J_cu));
//     CUForEnd();

//     TraceCgpGenomeOneApe(writeZone, 0, 0, nbGenomes);
//     TraceMemVectorOneApe(writeZone, 0, 0, integerFormat);

//   CUForEnd();

// }


// //callbacks for prints
// static int current_step_best_fit = 0;
// void cgp_data_callback_printBestFits(uint16_t* data, int size, void* extraData){
//   printf("--- REPORT NB %d ---", current_step_best_fit++);

//   printf("Training fitness : (");
//   for(int i = 0; i < chipCols*chipRows; i++) {
//     printf("%f", cvtFloat(data[i]));
//     if(i != chipCols*chipRows - 1) {
//       printf(", ");
//     }
//   }
//   printf(")\n");
// }

// void cgp_data_callback_printBestGenomes(uint16_t* data, int size, void* extraData){
//   int wPerChip =  size/(chipCols*chipRows);

//   /*printf("SIZE = %d\n", size);
//   printf("wPerChip = %d\n", wPerChip);*/

//   for (int i = 0; i < chipRows * chipCols; i++) {
//     printf("Vector %d: (", i);
//     for (int j = 0; j < wPerChip; j++) {
//       printf("%u", data[i * wPerChip + j]);
//       if(j != wPerChip - 1) {
//         printf(", ");
//       }
//     }
//     printf(")\n");
//   }

// }


// //callbacks to write result to files
// typedef struct {
//   char* filename;
// } cgp_FileWriteInfo;

// int write_best_fit_current_step=0;
// void cgp_data_callback_writeBestFits(uint16_t* data, int size, void* extraData){

//   printf("step %d \n", write_best_fit_current_step);

//   if(extraData == NULL){
//     return;
//   }

//   cgp_FileWriteInfo* info = (cgp_FileWriteInfo*) extraData;


//   FILE *file = fopen(info->filename, "a");
//   if (file == NULL) {
//     perror("File open failed");
//     return;
//   }

//   fprintf(file, "name=_train_fit, iteration=%d, value=(", write_best_fit_current_step++);

//   for(int i = 0; i < chipCols*chipRows; i++) {
//     fprintf(file, "%u", data[i]);
//     if(i != chipCols*chipRows - 1) {
//       fprintf(file, ", ");
//     }
//   }
//   fprintf(file, ")\n");

//   fclose(file);
//   return;

// }

// int write_best_genomes_current_step=0;
// void cgp_data_callback_writeBestGenomes(uint16_t* data, int size, void* extraData){

//   int wPerChip =  size/(chipCols*chipRows);

//   if(extraData == NULL){
//     return;
//   }

//   cgp_FileWriteInfo* info = (cgp_FileWriteInfo*) extraData;


//   FILE *file = fopen(info->filename, "a");
//   if (file == NULL) {
//     perror("File open failed");
//     return;
//   }

//   for (int i = 0; i < chipRows * chipCols; i++) {
//     fprintf(file, "name=best_genome, iteration=%d, chip=%d, value=(", write_best_genomes_current_step, i);
//     for (int j = 0; j < wPerChip; j++) {
//       fprintf(file, "%u", data[i * wPerChip + j]);
//       if(j != wPerChip - 1) {
//         fprintf(file, ", ");
//       }
//     }
//     fprintf(file, ")\n");
//   }

//   fclose(file);
//   write_best_genomes_current_step++;

//   return;

// }


// void cgp_findBestNeighbor(scExpr bestNeighbor){

//   //eApeC(apeGet, temp, fitness, getNorth);

//   DeclareApeVar(transferFitness, Approx);
//   DeclareApeVar(bestCurrentFitness, Approx);

//   Set(bestNeighbor, IntConst(4));
//   Set(bestCurrentFitness, fitness);


//   //North = 0
//   eApeC(apeGet, transferFitness, fitness, getNorth);
//   ApeIf(And(
//     Ne(gu_GetApeRow(), IntConst(0)),
//     Lt(transferFitness, bestCurrentFitness)
//   ));
//   Set(bestNeighbor, IntConst(getNorth));
//   Set(bestCurrentFitness, transferFitness);
//   ApeFi();

//   //South = 3
//   eApeC(apeGet, transferFitness, fitness, getSouth);
//   ApeIf(And(
//       Ne(gu_GetApeRow(), IntConst(chipRows * apeRows - 1)),
//       Lt(transferFitness, bestCurrentFitness)
//   ));
//   Set(bestNeighbor, IntConst(getSouth));
//   Set(bestCurrentFitness, transferFitness);
//   ApeFi();

//   //East = 1
//   eApeC(apeGet, transferFitness, fitness, getEast);
//   ApeIf(And(
//       Ne(gu_GetApeCol(), IntConst(chipCols * apeCols - 1)),
//       Lt(transferFitness, bestCurrentFitness)
//   ));
//   Set(bestNeighbor, IntConst(getEast));
//   Set(bestCurrentFitness, transferFitness);
//   ApeFi();


//   //West = 2
//   eApeC(apeGet, transferFitness, fitness, getWest);
//   ApeIf(And(
//     Ne(gu_GetApeCol(), IntConst(0)),
//     Lt(transferFitness, bestCurrentFitness)
//   ));
//   Set(bestNeighbor, IntConst(getWest));
//   Set(bestCurrentFitness, transferFitness);
//   ApeFi();

//   //Set(bestNeighbor, IntConst(getEast));

// }

// scExpr cgp_tournament(){

//   DeclareApeVar(bestNeighbor, Int);
//   cgp_findBestNeighbor(bestNeighbor);
//   cgp_copyGenome(bestNeighbor);
//   //cgp_resetFitness();

//   return bestNeighbor;

// }



// void cgp_mainLoop(int nbLines, uint32_t cuAddr_x, uint32_t cuAddr_y, int nb_genomes, int nb_loop, int report_every, int seed){

//   int nb_input = 8;
//   int nb_output = 1;
//   float mutationRate = 0.01f;
//   int sample_offset = 4;

//   int outer_loop_count = nb_loop/report_every;

//   DeclareCUMemVector(data_reserve, Approx, 9000);
//   DeclareCUMemArrayAtAddress(x_data, Approx, nbLines, nb_input, cuAddr_x);
//   DeclareCUMemArrayAtAddress(y_data, Approx, nbLines, nb_output, cuAddr_y)


//   //DeclareCUVar(debugACU, Approx);
//   //DeclareCUVar(debugICU, Int);

//   int filename_size_1 = snprintf(NULL, 0, "best_fits_%dgen_%dloop.txt", nb_genomes, nb_loop);
//   int filename_size_2 = snprintf(NULL, 0, "best_genomes_%dgen_%dloop.txt", nb_genomes, nb_loop);
//   int filename_size_3 = snprintf(NULL, 0, "all_fits_%dgen_%dloop.txt", nb_genomes, nb_loop);

//   char *fn1 = malloc(filename_size_1 + 1);
//   char *fn2 = malloc(filename_size_2 + 1);
//   char *fn3 = malloc(filename_size_3 + 1);

//   sprintf(fn1, "best_fits_%dgen_%dloop.txt", nb_genomes, nb_loop);
//   sprintf(fn2, "best_genomes_%dgen_%dloop.txt", nb_genomes, nb_loop);
//   sprintf(fn3, "all_fits_%dgen_%dloop.txt", nb_genomes, nb_loop);


//   cgp_FileWriteInfo* fwi1 = malloc(sizeof(cgp_FileWriteInfo));
//   fwi1->filename = fn1;

//   cgp_FileWriteInfo* fwi2 = malloc(sizeof(cgp_FileWriteInfo));
//   fwi2->filename = fn2;

//   cgp_FileWriteInfo* fwi3 = malloc(sizeof(cgp_FileWriteInfo));
//   fwi3->filename = fn3;


//   gr_setSeed(seed);

//   DeclareCUVar(I_main_1, Int);
//   DeclareCUVar(I_main_2, Int);
//   DeclareCUVar(J_main, Int);

//   //DeclareApeMem(debugA, Approx);
//   //DeclareApeMem(debugI, Int);

//   cgp_initGenome(nb_genomes, nb_input);
//   cgp_resetFitness();

//   DeclareApeVar(bestFitness, Approx);
//   DeclareApeVar(bestFitnessApeRow, Int);
//   DeclareApeVar(bestFitnessApeCol, Int);

//   DeclareCUMemVector(interFitnesses, Approx, (outer_loop_count + 1) * chipRows * chipCols);
//   DeclareCUVar(interFitnessWritePos, Int);
//   Set(interFitnessWritePos, IntConst(0));

//   DeclareCUMemArray(bestFitnesses, Approx, chipRows, chipCols);
//   DeclareCUMemArray(bestFitApeRows, Int, chipRows, chipCols);
//   DeclareCUMemArray(bestFitApeCols, Int, chipRows, chipCols);
//   DeclareCUMemArray(bestGenomes, Int, chipRows*chipCols, nbWords);
//   DeclareCUMemArray(allFitsSampled, Approx, chipRows*(apeRows/sample_offset), chipCols*(apeCols/sample_offset));

//   CUFor(I_main_1, IntConst(0), IntConst(outer_loop_count-1), IntConst(1));

//     CUFor(I_main_2, IntConst(0), IntConst(report_every-1), IntConst(1));

//       TraceMessage("--------\n");
//   	  TraceMessage("STEP_1 : ");
//       TraceRegisterCU(I_main_1);
//       TraceMessage("STEP_2 : ");
//       TraceRegisterCU(I_main_2);

//       cgp_mutateGenome(mutationRate);

//       cgp_resetFitness();

//       CUFor(J_main, IntConst(0), IntConst(nbLines-1), IntConst(1));
//         cgp_execAndComputeFitness(x_data, y_data, J_main, nb_output);

//       CUForEnd();

//       scExpr bestNeighor = cgp_tournament();

//       //reporting data at step 0
//       CUIf(Eq(Add(I_main_1, I_main_2), IntConst(0)));

//         gn_coordMin(cgp_getFitness(), bestFitness, bestFitnessApeRow, bestFitnessApeCol);
//         gu_readBackToCUTopLeftApeEachChip(bestFitnesses, bestFitness);

//         gu_readBackToCUTopLeftApeEachChip(bestFitApeRows, bestFitnessApeRow);
//         gu_readBackToCUTopLeftApeEachChip(bestFitApeCols, bestFitnessApeCol);

//         gu_readBackToCUVectorOnePerChip(bestFitApeRows, bestFitApeCols, gb_getFullVector(), bestGenomes);

//         gu_sampleFromBoardToCU(allFitsSampled, cgp_getFitness(), sample_offset);

//         gd_sendDataToHostData(bestFitnesses, cgp_data_callback_writeBestFits, fwi1);
//         gd_sendDataToHostData(bestGenomes, cgp_data_callback_writeBestGenomes, fwi2);
//         gd_sendDataToHostData(allFitsSampled, cgp_data_callback_writeBestGenomes, fwi3);

//       CUFi();

//     CUForEnd();

//     //reporting data every report_every steps
//     gn_coordMin(cgp_getFitness(), bestFitness, bestFitnessApeRow, bestFitnessApeCol);
//     gu_readBackToCUTopLeftApeEachChip(bestFitnesses, bestFitness);

//     gu_readBackToCUTopLeftApeEachChip(bestFitApeRows, bestFitnessApeRow);
//     gu_readBackToCUTopLeftApeEachChip(bestFitApeCols, bestFitnessApeCol);

//     gu_readBackToCUVectorOnePerChip(bestFitApeRows, bestFitApeCols, gb_getFullVector(), bestGenomes);

//     gu_sampleFromBoardToCU(allFitsSampled, cgp_getFitness(), sample_offset);

//     gd_sendDataToHostData(bestFitnesses, cgp_data_callback_writeBestFits, fwi1);
//     gd_sendDataToHostData(bestGenomes, cgp_data_callback_writeBestGenomes, fwi2);
//     gd_sendDataToHostData(allFitsSampled, cgp_data_callback_writeBestGenomes, fwi3);

//   CUForEnd();

//   /*gd_sendDataToHostData(bestFitnesses, cgp_data_callback_writeBestFits, fwi1);
//   gd_sendDataToHostData(bestGenomes, cgp_data_callback_writeBestGenomes, fwi2);*/


// }

// scExpr cgp_getFitness(){ //this is mostly for debug
//   return fitness;
// }

// scExpr cgp_getResults(){ //this is mostly for debug
//   return results;
// }

