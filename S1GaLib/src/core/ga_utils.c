#include <stdio.h>

#include <S1Core/S1Core.h>
#include <S1GaLib/S1GaLib.h>

Declare(ApeRow);
Declare(ApeCol);
Declare(ApeRowG);
Declare(ApeColG);
Declare(ApeRowApprox);
Declare(ApeColApprox);

Declare(apeMemTransfer);
Declare(cuMemTransfer);

const unsigned short MAX_INT_VALUE = 0xFFFF;

static int isApeRowInit = 0;
static int isApeColInit = 0;
static int isApeRowApproxInit = 0;
static int isApeColApproxInit = 0;
static int isApeRowGInit = 0;
static int isApeColGInit = 0;

extern int chipRows, chipCols, apeRows, apeCols;

scExpr gu_GetApeRow(){
  if (isApeRowInit != 1) { 
    ApeVar(ApeRow, Int);
    Set(ApeRow,IntConst(0));

    for (int m=0; m<apeRows * chipRows; m++){
        eApeC(apeGet, ApeRow, ApeRow, getNorth);
        Set(ApeRow, Add(ApeRow,IntConst(1)));
    }
    Set(ApeRow, Sub(ApeRow,IntConst(1)));
  }
  return ApeRow;
}

scExpr gu_GetApeCol(){
  if (isApeColInit != 1) {
    ApeVar(ApeCol, Int);
    Set(ApeCol,IntConst(0));

    for (int m=0; m<apeCols * chipCols; m++){
        eApeC(apeGet, ApeCol, ApeCol, getWest);
        Set(ApeCol, Add(ApeCol,IntConst(1)));
    }
    Set(ApeCol, Sub(ApeCol,IntConst(1)));
  }
  return ApeCol;
}

scExpr gu_GetApeGlobalRow(){
  if(isApeRowGInit != 1){
    ApeVar(ApeRowG, Int);
    DeclareApeMem(apeMemTransfer, Int);
    DeclareCUMem(cuMemTransfer, Int);
    Set(apeMemTransfer, IntConst(0));

    scExpr apeRow = gu_GetApeRow();

    eCUC(cuSet, cuRChipCol, _, 0);
    eCUC(cuSetHigh, cuRChipCol, _, 0x8000);
    eCUC(cuSet, cuRApeRow, _, 0);
    eCUC(cuSetHigh, cuRApeRow, _, 0x8000);
    eCUC(cuSet, cuRApeCol, _, 0);
    eCUC(cuSetHigh, cuRApeCol, _, 0x8000);

    //we don't need to start at zero
    for(int chRow=1; chRow<chipRows; chRow++){ //TODO make this a CUFor loop
        Set(cuMemTransfer, IntConst(chRow*apeRows));
        eCUC(cuSet, cuRChipRow, _, chRow);
        eCUC(cuSetHigh, cuRChipRow, _, 0);
        //we put this in the loop since cuWrite increments this value by default
        eCUX(cuSetRWAddress, _, _, MemAddress(cuMemTransfer));
        eCUX(cuWrite, _, rwIgnoreMasks|rwUseCUMemory, MemAddress(apeMemTransfer));
    }
    Set(ApeRowG, Add(apeMemTransfer, apeRow));
    isApeRowGInit = 1;
  }

  return ApeRowG;
}

scExpr gu_GetApeGlobalCol(){
  if(isApeColGInit != 1){
    ApeVar(ApeColG, Int);
    DeclareApeMem(apeMemTransfer, Int);
    DeclareCUMem(cuMemTransfer, Int);
    Set(apeMemTransfer, IntConst(0));

    scExpr apeCol = gu_GetApeCol();

    eCUC(cuSet, cuRChipRow, _, 0);
    eCUC(cuSetHigh, cuRChipRow, _, 0x8000);
    eCUC(cuSet, cuRApeRow, _, 0);
    eCUC(cuSetHigh, cuRApeRow, _, 0x8000);
    eCUC(cuSet, cuRApeCol, _, 0);
    eCUC(cuSetHigh, cuRApeCol, _, 0x8000);

    //we don't need to start at zero
    for(int chCol=1; chCol<chipRows; chCol++){
        Set(cuMemTransfer, IntConst(chCol*apeCols));
        eCUC(cuSet, cuRChipCol, _, chCol);
        eCUC(cuSetHigh, cuRChipCol, _, 0);

        //we put this in the loop since cuWrite increments this value by default
        eCUX(cuSetRWAddress, _, _, MemAddress(cuMemTransfer));
        eCUX(cuWrite, _, rwIgnoreMasks|rwUseCUMemory, MemAddress(apeMemTransfer));
    }
    Set(ApeColG, Add(apeMemTransfer, apeCol));
    isApeColGInit = 1;
  }

  return ApeColG;
}

void gu_ApeGetGlobal(scExpr srcVar, scExpr dstVar, scExpr direction){
    DeclareCUVar(I_cu, Int);

    //we must use a temp var as we might override the source if the dst and src are the same var
    DeclareApeVar(dstVarTemp, Int);
    Set(dstVarTemp, IntConst(0));

    eApeX(apeGetGStart, _, _, direction);
    eApeX(apeGetGStartDone, _, srcVar, _);

    CUFor(I_cu, IntConst(0), IntConst(15), IntConst(1));
        eApeC(apeGetGMove, _, _, _);
    CUForEnd();

    eApeX(apeGetGMoveDone, _, _, _);
    eApeX(apeGetGEnd, dstVar, srcVar, direction);
}

scExpr gu_isEdge(scExpr direction){
    DeclareApeVar(res, Bool);
    // Set(res, IntConst(0));
    DeclareApeVar(dir, Int);
    Set(dir, IntConst(direction));
    scExpr apeRow = gu_GetApeRow();
    scExpr apeCol = gu_GetApeCol();
    
    Set(res, // check all directions for an edge, return true if any are
        Or(
            Or(
                And(Eq(apeRow, IntConst(0)), Eq(dir, IntConst(getNorth))),
                And(Eq(apeCol, IntConst(apeCols-1)), Eq(dir, IntConst(getEast)))
            ),
            Or(
                And(Eq(apeRow, IntConst(apeRows-1)), Eq(dir, IntConst(getSouth))),
                And(Eq(apeCol, IntConst(0)), Eq(dir, IntConst(getWest)))
            )
        )
    );

    // TraceMessage("DIR : ")
    // TraceOneRegisterOneApe(dir, 0, 2);
    // Set(dir, IntConst(getWest));
    return res;
}

scExpr gu_countOneBitsSWAR(scExpr num){
    DeclareApeVar(ret, Int);
    // Compute number of 1s (for scoring) using SWAR algorithm
    Set(ret, num);
    // ret = ret - ((ret >> 1) & 0x5555)
    Set(ret, Sub(ret, And(And(Asr(ret, IntConst(1)), IntConst(0x7FFF)), IntConst(0x5555))));
    // ret = (ret & 0x3333) + ((ret >> 2) & 0x3333)
    Set(ret, Add(And(ret, IntConst(0x3333)), And(And(Asr(ret, IntConst(2)), IntConst(0x3FFF)), IntConst(0x3333))));
    // ret = (ret + (ret >> 4)) & 0x0F0F
    Set(ret, And(Add(ret, And(Asr(ret, IntConst(4)), IntConst(0x0FFF))), IntConst(0x0F0F)));
    // ret = (ret & 0xFF) + (ret >> 8)
    Set(ret, Add(And(ret, IntConst(0x00FF)), And(Asr(ret, IntConst(8)), IntConst(0x00FF))));

    return ret;
}

unsigned short gu_next_power_of_2(unsigned short n) {
  if (n == 0) return 1;
  n--;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n++;
  return n;
}

unsigned short gu_bits_needed(unsigned short N) {
  if (N == 0) return 0;
  N--; // Decrement N to handle the case where N is a power of 2
  unsigned short bits = 0;
  while (N >>= 1) {
      bits++;
  }
  return bits + 1;
}
