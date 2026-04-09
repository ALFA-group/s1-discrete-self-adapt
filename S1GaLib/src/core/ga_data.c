#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <S1Core/S1Core.h>
#include <S1GaLib/S1GaLib.h>

extern int chipRows, chipCols, apeRows, apeCols;

static int propDelay = 4;

static TransferEntry* transferRegistry = NULL;
static int MAX_TRANSFER_REGISTERY_ENTRIES = 100;
static int nextTransferId = 1;
static int systemInitialized = 0;

// CU variable to communicate transfer IDs
Declare(transferIdCU);

//Data output functions

void _gd_outputApeVarRange(scExpr var_name, FILE* f, range_t chipRange, range_t apeRange, FmtOptions options, FmtFunction fmt) {
    Declare(cuMemArr);
    
    int total_rows = (apeRange.row_end - apeRange.row_start + 1) * (chipRange.row_end - chipRange.row_start + 1);
    int total_cols = (apeRange.col_end - apeRange.col_start + 1) * (chipRange.col_end - chipRange.col_start + 1);
    
    ValueConverter conv = gd_getConverter(var_name);
    if (!conv) {
        printf("Invalid type for output. Must be Int or Approx.\n");
        exit(-1);
    }
    options.type_out_fn = conv;
    options.data_len = total_rows * total_cols;
    
    if (scType(var_name) == scTypeInt){CUMemArray(cuMemArr, Int, total_rows, total_cols);}
    else{CUMemArray(cuMemArr, Approx, total_rows, total_cols);}
    
    gd_cuGetApeVarChipRangeApeRange(var_name, chipRange, apeRange, cuMemArr);
    
    // Execute with chosen formatter and auto-detected converter
    gd_transferExecute(cuMemArr, f, fmt, options);
}

void gd_printApeVarOneChip(scExpr var_name, int chip_row, int chip_col){
    range_t chipRange = {chip_row, chip_row, chip_col, chip_col};
    range_t apeRange = {0, apeRows-1, 0, apeCols-1};
    FmtOptions options;
    options.entries_per_line = apeCols;
    _gd_outputApeVarRange(var_name, stdout, chipRange, apeRange, options, grid_fmt);
}

void gd_printApeVarOneChipRange(scExpr var_name, int chip_row, int chip_col, int ape_row_start, int ape_row_end, int ape_col_start, int ape_col_end){
    range_t chipRange = {chip_row, chip_row, chip_col, chip_col};
    range_t apeRange = {ape_row_start, ape_row_end-1, ape_col_start, ape_col_end-1};
    FmtOptions options;
    options.entries_per_line = ape_col_end - ape_col_start;
    _gd_outputApeVarRange(var_name, stdout, chipRange, apeRange, options, grid_fmt);
}

void gd_writeApeVarOneChipToCSV(scExpr var_name, FILE* f, int chip_row, int chip_col){
    range_t chipRange = {chip_row, chip_row, chip_col, chip_col};
    range_t apeRange = {0, apeRows-1, 0, apeCols-1};
    FmtOptions options;
    options.entries_per_line = apeCols;
    _gd_outputApeVarRange(var_name, f, chipRange, apeRange, options, csv_fmt);
}

void gd_printApeVarFullBoard(scExpr var_name){
    range_t chipRange = {0, chipRows-1, 0, chipCols-1};
    range_t apeRange = {0, apeRows-1, 0, apeCols-1};
    FmtOptions options;
    options.entries_per_line = apeCols*chipCols;
    _gd_outputApeVarRange(var_name, stdout, chipRange, apeRange, options, grid_fmt);
}

void gd_writeApeVarFullBoardToCSV(scExpr var_name, FILE* f){
    range_t chipRange = {0, chipRows-1, 0, chipCols-1};
    range_t apeRange = {0, apeRows-1, 0, apeCols-1};
    FmtOptions options;
    options.entries_per_line = apeCols*chipCols;
    _gd_outputApeVarRange(var_name, f, chipRange, apeRange, options, csv_fmt);
}

void gd_writeApeVarOneChipToRaw(scExpr var_name, FILE* f, int chip_row, int chip_col) {
    FmtFunction fmt = gd_getRawFormatter(var_name);
    if (!fmt) { printf("Invalid type for raw output.\n"); return; }

    range_t chipRange = {chip_row, chip_row, chip_col, chip_col};
    range_t apeRange  = {0, apeRows-1, 0, apeCols-1};
    FmtOptions options;
    options.entries_per_line = apeCols;
    options.data_len         = apeRows * apeCols;
    _gd_outputApeVarRange(var_name, f, chipRange, apeRange, options, fmt);
}

void gd_writeApeVarFullBoardToRaw(scExpr var_name, FILE* f) {
    FmtFunction fmt = gd_getRawFormatter(var_name);
    if (!fmt) { printf("Invalid type for raw output.\n"); return; }

    range_t chipRange = {0, chipRows-1, 0, chipCols-1};
    range_t apeRange  = {0, apeRows-1, 0, apeCols-1};
    FmtOptions options;
    options.entries_per_line = apeCols * chipCols;
    options.data_len         = apeRows * chipRows * apeCols * chipCols;
    _gd_outputApeVarRange(var_name, f, chipRange, apeRange, options, fmt);
}

static void _gd_writeApeMemVecRange(scExpr memVec, FILE* f, range_t chipRange, range_t apeRange) {
    int vec_len    = MemLength(memVec);
    int total_apes = (chipRange.row_end - chipRange.row_start + 1) *
                     (chipRange.col_end - chipRange.col_start + 1) *
                     (apeRange.row_end  - apeRange.row_start  + 1) *
                     (apeRange.col_end  - apeRange.col_start  + 1);

    FmtFunction fmt = gd_getRawFormatter(memVec);
    if (!fmt) {
        printf("Invalid type for raw output. Must be Int or Approx.\n");
        return;
    }

    Declare(cuMemArr);
    if (scType(memVec) == scTypeInt) { CUMemArray(cuMemArr, Int,    total_apes, 1); }
    else                             { CUMemArray(cuMemArr, Approx, total_apes, 1); }

    DeclareCUVar(k, Int);
    DeclareApeVar(wordTransfer, Int);

    eControl(controlOpReserveApeReg, apeR0);

    CUFor(k, IntConst(0), IntConst(vec_len - 1), IntConst(1));

        eCUX(cuSetRWAddress, _, _, MemAddress(cuMemArr));

        Set(wordTransfer, IndexVector(memVec, k));
        eApeX(apeSet, apeR0, _, wordTransfer);

        eCUC(cuSetHigh, cuRChipRow, _, 0);
        eCUC(cuSetHigh, cuRChipCol, _, 0);
        eCUC(cuSetHigh, cuRApeRow,  _, 0);
        eCUC(cuSetHigh, cuRApeCol,  _, 0);

        CUFor(cuRChipRow, IntConst(chipRange.row_start), IntConst(chipRange.row_end), IntConst(1));
            CUFor(cuRApeRow, IntConst(apeRange.row_start), IntConst(apeRange.row_end), IntConst(1));
                CUFor(cuRChipCol, IntConst(chipRange.col_start), IntConst(chipRange.col_end), IntConst(1));
                    CUFor(cuRApeCol, IntConst(apeRange.col_start), IntConst(apeRange.col_end), IntConst(1));
                        eCUC(cuRead, _, rwIgnoreMasks|rwUseCUMemory, (propDelay<<8)|apeR0);
                    CUForEnd();
                CUForEnd();
            CUForEnd();
        CUForEnd();

        FmtOptions options;
        options.data_len = total_apes;
        gd_transferExecute(cuMemArr, f, fmt, options);

    CUForEnd();

    eControl(controlOpReleaseApeReg, apeR0);
}

void gd_writeApeMemVecOneChipToRaw(scExpr memVec, FILE* f, int chip_row, int chip_col) {
    range_t chipRange = {chip_row, chip_row, chip_col, chip_col};
    range_t apeRange  = {0, apeRows-1, 0, apeCols-1};
    _gd_writeApeMemVecRange(memVec, f, chipRange, apeRange);
}

void gd_writeApeMemVecFullBoardToRaw(scExpr memVec, FILE* f) {
    range_t chipRange = {0, chipRows-1, 0, chipCols-1};
    range_t apeRange  = {0, apeRows-1, 0, apeCols-1};
    _gd_writeApeMemVecRange(memVec, f, chipRange, apeRange);
}

void gd_printMessage(char *message){
    DeclareCUMem(tmp, Int);
    FmtOptions options;
    options.text = message;

    gd_transferExecute(tmp, stdout, text_fmt, options);
}

void gd_printCUVar(scExpr cuVar, char *message){
    DeclareCUMem(tmp, Int);
    Set(tmp, cuVar);
    FmtOptions options;
    options.text = message;

    gd_transferExecute(tmp, stdout, cu_fmt, options);
}

// Data formatting functions

void gd_outputInt(FILE* out, uint16_t value) {
    fprintf(out, "%d", value);
}

void gd_outputApprox(FILE* out, uint16_t value) {
    fprintf(out, "%f", cvtFloat(value));
}

FmtFunction gd_getRawFormatter(scExpr var_name) {
    if (scType(var_name) == scTypeInt)    return raw_fmt;
    if (scType(var_name) == scTypeApprox) return raw_fmt_approx;
    return NULL;
}

ValueConverter gd_getConverter(scExpr var_name) {
    if (scType(var_name) == scTypeInt) return gd_outputInt;
    if (scType(var_name) == scTypeApprox) return gd_outputApprox;
    return NULL;
}

void grid_fmt(uint16_t* data, FILE* out, FmtOptions opts) {
    for (int i = 0; i < opts.data_len; i++) {
        if (i % opts.entries_per_line == 0) fprintf(out, "\n");
        opts.type_out_fn(out, data[i]);
        fprintf(out, ", ");
    }
    fprintf(out, "\n");
    fflush(out);
}

void csv_fmt(uint16_t* data, FILE* out, FmtOptions opts) {
    for (int i = 0; i < opts.data_len - 1; i++) {
        opts.type_out_fn(out, data[i]);
        fprintf(out, ", ");
    }
    opts.type_out_fn(out, data[opts.data_len - 1]);
    fprintf(out, "\n");
    fflush(out);
}

void text_fmt(uint16_t* data, FILE* out, FmtOptions opts) {
    fprintf(out, "%s", opts.text);
    fprintf(out, "\n");
    fflush(out);
}

void raw_fmt(uint16_t* data, FILE* out, FmtOptions opts) {
    fwrite(data, sizeof(uint16_t), opts.data_len, out);
    fflush(out);
}

void raw_fmt_approx(uint16_t* data, FILE* out, FmtOptions opts) {
    for (int i = 0; i < opts.data_len; i++) {
        float f = cvtFloat(data[i]);
        fwrite(&f, sizeof(float), 1, out);
    }
    fflush(out);
}

void cu_fmt(uint16_t* data, FILE* out, FmtOptions opts) {
    fprintf(out, "%s ", opts.text);
    fprintf(out, "%d", data[0]);
    fprintf(out, "\n");
    fflush(out);
}

// Transfer System Setup

void _gd_transferInit() {
    if (systemInitialized) {
        return;
    }
    
    transferRegistry = calloc(MAX_TRANSFER_REGISTERY_ENTRIES, sizeof(TransferEntry));
    
    // Declare the CU memory for transfer ID
    CUMem(transferIdCU, Int);
    Set(transferIdCU, IntConst(0));
    
    systemInitialized = 1;
    nextTransferId = 1;
}

int _gd_transferRegister(scExpr cuMem, FILE* output_to, FmtFunction fmt, FmtOptions options){
    if (!systemInitialized) {
        _gd_transferInit();
    }
    
    if (nextTransferId >= MAX_TRANSFER_REGISTERY_ENTRIES) {
        fprintf(stderr, "ERROR: Transfer registry full!\n");
        return -1;
    }
    
    // Create new entry
    int transferId = nextTransferId++; //Sets value of transferId, then increments nextTransferId
    transferRegistry[transferId].cuMem = cuMem;
    transferRegistry[transferId].output_to = output_to;
    transferRegistry[transferId].fmt = fmt;
    transferRegistry[transferId].options = options;
    
    // Store the transfer ID in CU memory
    Set(transferIdCU, IntConst(transferId));
    
    return transferId;
}

void gd_transferExecute(scExpr cuMem, FILE* output_to, FmtFunction fmt, FmtOptions options){
    // Register this transfer point
    _gd_transferRegister(cuMem, output_to, fmt, options);
    
    // Pause kernel execution and signal host
    eCUC(cuSetSignal, _, _, _);
    eCUC(cuWaitForClearSignal, _, _, _);
    
    // Reset transfer ID after completion
    Set(transferIdCU, IntConst(0));
}

void _gd_transferCleanup() {
    if (systemInitialized) {
        free(transferRegistry);
        transferRegistry = NULL;
        systemInitialized = 0;
        nextTransferId = 1;
    }
}

void gd_waitForTransfers() {
    if (!systemInitialized) {
        fprintf(stderr, "ERROR: Transfer system not initialized\n");
        return;
    }
    
    printf("Entering transfer wait loop...\n");
    
    while (1) {
        // Wait for kernel to signal a transfer
        scLLKernelWaitSignal();
        
        // Read the transfer ID
        uint16_t* idData = gd_hostReadCuMem(transferIdCU);
        int transferId = (int)idData[0];
        free(idData);
        
        // printf("Received transfer signal, ID=%d\n", transferId);
        
        // ID of 0 means kernel is done
        if (transferId == 0) {
            printf("Kernel execution complete\n");
            scClearCUSignal();
            _gd_transferCleanup();
            break;
        }
        
        // Validate transfer ID
        if (transferId < 1 || transferId >= nextTransferId) {
            fprintf(stderr, "ERROR: Invalid transfer ID %d\n", transferId);
            scClearCUSignal();
            continue;
        }
        
        // Execute the transfer
        TransferEntry* entry = &transferRegistry[transferId];
        uint16_t* data = gd_hostReadCuMem(entry->cuMem);
        int size = MemLength(entry->cuMem);
        
        // Call the callback
        if (entry->fmt) {
            entry->fmt(data, entry->output_to, entry->options);
        }
        
        free(data);
        
        // Signal kernel to continue
        scClearCUSignal();
    }
    
    printf("Transfer loop ended\n");
}

void gd_transferFinish() {
    if (!systemInitialized) {
        CUMem(transferIdCU, Int);
    }
    
    Set(transferIdCU, IntConst(0));
    eCUC(cuSetSignal, _, _, _);
    eCUC(cuWaitForClearSignal, _, _, _);
}

// Cu Memory Operations

uint16_t* gd_hostReadCuMem(scExpr cuMemData) {
    int memAddr = MemAddress(cuMemData);
    int nbWords = MemLength(cuMemData);

    int blockSize = (nbWords/4)*8;

    int startReadingAt = memAddr;
    int nbStartSkip = 0;

    if(memAddr % 4 != 0){
        //we should start to read a bit before and read one block more
        startReadingAt = (startReadingAt / 4)*4;
        nbStartSkip = memAddr - startReadingAt;
        blockSize+= 8;
    }

    if(nbWords % 4 != 0){
        //if the nbWords is not a multiple of 4 then we should add one more group of 4 words = 8 block
        blockSize+= 8;
    }

    //the block size is in byte => 2 block = 1 word this division should always give the correct result since blocksize is divisible
    uint16_t __attribute__ ((aligned (8))) CUData[blockSize/2];

    scReadCUDataMemoryBlock(
        blockSize, // byte count, must be multiple of 8 (4 words in a block, 16 bits each, so 8 bytes per block)
        (uintptr_t)CUData, // address in Host memory
        startReadingAt// CU Data Mem starting word address
    );

    uint16_t* retData = malloc(nbWords * sizeof(uint16_t));

    for(int i = 0; i < nbWords; i++){
        retData[i] = CUData[i + nbStartSkip];
    }

    return retData;
}

void gd_cuGetApeVarOneChip(scExpr var_name, int chip_row, int chip_col, scExpr ret_cuMemArr){
    range_t chipRange = {chip_row, chip_row, chip_col, chip_col};
    range_t apeRange = {0, apeRows-1, 0, apeCols-1};
    gd_cuGetApeVarChipRangeApeRange(var_name, chipRange, apeRange, ret_cuMemArr);
}

void gd_cuGetApeVarChipRangeApeRange(scExpr var_name, range_t chipRange, range_t apeRange, scExpr ret_cuMemArr){

    // Clear high bits of chip row and col registers
    eCUC(cuSetHigh, cuRChipRow, _, 0);
    eCUC(cuSetHigh, cuRChipCol, _, 0);
    eCUC(cuSetHigh, cuRApeRow, _, 0);
    eCUC(cuSetHigh, cuRApeCol, _, 0);

    eControl(controlOpReserveApeReg,apeR0);

    eApeX(apeSet, apeR0, _, var_name);

    eCUX(cuSetRWAddress, _, _, MemAddress(ret_cuMemArr));
    CUFor(cuRChipRow, IntConst(chipRange.row_start), IntConst(chipRange.row_end), IntConst(1));
        CUFor(cuRApeRow, IntConst(apeRange.row_start), IntConst(apeRange.row_end), IntConst(1));
            CUFor(cuRChipCol, IntConst(chipRange.col_start), IntConst(chipRange.col_end), IntConst(1));
                CUFor(cuRApeCol, IntConst(apeRange.col_start), IntConst(apeRange.col_end), IntConst(1));
                    //this should automatically increase the value from the cuSetRWAddress
                    eCUC(cuRead, _, rwIgnoreMasks|rwUseCUMemory, (propDelay<<8)|apeR0);
                CUForEnd();
            CUForEnd();
        CUForEnd();
    CUForEnd();

    eControl(controlOpReleaseApeReg,apeR0);
}

void gd_readBackToCUTopLeftApeEachChip(scExpr cuMemVecTransfer, scExpr apeVarToGet){
  //sends one var from each chip (on top left ape of each chip) to the CU
  //this should even be working by sending an array instead of a Vector

  /*DeclareApeMem(debug, Int);
  Set(debug, apeVarToGet);*/

  /*eCUC(cuSet, cuRChipRow, _, 0);*/
  eCUC(cuSetHigh, cuRChipRow, _, 0);
  /*eCUC(cuSet, cuRChipCol, _, 0);*/
  eCUC(cuSetHigh, cuRChipCol, _, 0);
  eCUC(cuSet, cuRApeRow, _, 0);
  eCUC(cuSetHigh, cuRApeRow, _, 0);
  eCUC(cuSet, cuRApeCol, _, 0);
  eCUC(cuSetHigh, cuRApeCol, _, 0);

  eControl(controlOpReserveApeReg,apeR0);

  eApeX(apeSet, apeR0, _, apeVarToGet);
  eCUX(cuSetRWAddress, _, _, MemAddress(cuMemVecTransfer));
  CUFor(cuRChipRow, IntConst(0), IntConst(chipRows-1), IntConst(1));
    CUFor(cuRChipCol, IntConst(0), IntConst(chipCols-1), IntConst(1));
      //this should automatically increase the value from the cuSetRWAddress
      eCUC(cuRead, _, rwIgnoreMasks|rwUseCUMemory, (propDelay<<8)|apeR0);
    CUForEnd();
  CUForEnd();

  eControl(controlOpReleaseApeReg,apeR0);

}

void gd_readBackToCUVectorOnePerChip(scExpr rowCoordsArrCU, scExpr colCoordsArrCU, scExpr vecToReadApe, scExpr outArrCU){
  //this function allow to get vector per chip from an Ape specified in the parameter and send it to a CU array
  //the target ape row and col can be different for each chip

  DeclareCUVar(colRowTransfer, Int);
  DeclareCUVar(I_cu, Int);
  DeclareCUVar(J_cu, Int);
  DeclareCUVar(K_cu, Int);

  DeclareCUVar(debug, Int);

  DeclareApeVar(dataTransfer, Int);

  eCUC(cuSetHigh, cuRChipRow, _, 0);
  eCUC(cuSetHigh, cuRChipCol, _, 0);
  eCUC(cuSetHigh, cuRApeRow, _, 0);
  eCUC(cuSetHigh, cuRApeCol, _, 0);
  eCUX(cuSetRWAddress, _, _, MemAddress(outArrCU));

  eControl(controlOpReserveApeReg,apeR0);
  CUFor(I_cu, IntConst(0), IntConst(chipRows-1), IntConst(1));
    eCUX(cuSet, cuRChipRow, _, I_cu);

    CUFor(J_cu, IntConst(0), IntConst(chipCols-1), IntConst(1));
      eCUX(cuSet, cuRChipCol, _, J_cu);

      Set(colRowTransfer, IndexArray(rowCoordsArrCU, I_cu, J_cu));
      eCUX(cuSet, cuRApeRow, _, colRowTransfer);
      Set(colRowTransfer, IndexArray(colCoordsArrCU, I_cu, J_cu));
      eCUX(cuSet, cuRApeCol, _, colRowTransfer);

      CUFor(K_cu, IntConst(0), IntConst(MemLength(vecToReadApe) - 1), IntConst(1));
        Set(dataTransfer, IndexVector(vecToReadApe, K_cu));
        eApeX(apeSet, apeR0, _, dataTransfer);

        eCUC(cuRead, _, rwIgnoreMasks|rwUseCUMemory, (propDelay<<8)|apeR0);

      CUForEnd();

    CUForEnd();
  CUForEnd();
  eControl(controlOpReleaseApeReg,apeR0);

}

// Data input functions

int _gd_getOffset(int nb_words){
    int offset = 0;

    if (nb_words % 4 == 0) {
        offset = nb_words / 4; // 4 words in a block, so divide by 4
    }
    else {
        offset = (nb_words / 4) + 1; // round up to next block
    }

    return offset*8;

}

void gd_loadDataset(char* fileName, int nbLines, int nbCols, uint32_t cuAddr, char delimiter){


    FILE *file = fopen(fileName, "r");
    if (!file) {
        perror("Unable to open file");
        return;
    }

    int currentLine = 0;
    int currentCol = 0;

    char line[nbLines];
    uint16_t nbs[nbLines][nbCols];

    while (fgets(line, sizeof(line), file)) {

        if(currentLine == nbLines){
            //printf("BREAK !! : %d\n", currentLine);
          break;
        }

        line[strcspn(line, "\n")] = 0; // Remove newline

        char *token = strtok(line, &delimiter);
        currentCol = 0;
        while (token) {
             if(currentCol == nbCols){
               //printf("BREAK !! : %d\n", currentCol);
               break;
             }

            int value = cvtApprox(atof(token));  // Convert token to float
            //printf("Float: %f\n", value);
            nbs[currentLine][currentCol] = value;
            //printf("value %f become %d\n", atof(token), value);

            token = strtok(NULL, &delimiter);

            currentCol++;
        }

        currentLine++;
    }

    fclose(file);

    int nb_words = nbLines*nbCols;
    int offset = _gd_getOffset(nb_words);

    //printf("Current offset: %d\n", offset);

    uint16_t __attribute__ ((aligned (8))) CUData[nb_words];


    for(int i = 0; i < nbLines; i++){
      for(int j = 0; j < nbCols; j++){
        //printf("CUData[%d] = %d\n", i*nbCols + j, nbs[i][j]);

        CUData[i*nbCols + j] = nbs[i][j];
      }
    }

    scWriteCUDataMemoryBlock(offset, (uintptr_t)CUData, cuAddr);


}

