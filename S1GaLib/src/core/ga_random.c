#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <S1Core/S1Core.h>
#include <S1GaLib/S1GaLib.h>

Declare(APEkissState);

static int isRandomInit = 0;

void emitNextKISS16 (scExpr result, int stateAddr) {
    int xLAddr = stateAddr;
    int yLAddr = stateAddr+2;
    int zLAddr = stateAddr+4;
    int wLAddr = stateAddr+6;
    int cAddr = stateAddr+8;

    int apeRah = apeR0;
    int apeRal = apeR4;
    int apeRbh = apeR5;
    int apeRbl = apeR6;
    int apeRc  = apeR2;
    eControl(controlOpReserveApeReg, apeRah);
    eControl(controlOpReserveApeReg, apeRal);
    eControl(controlOpReserveApeReg, apeRbh);
    eControl(controlOpReserveApeReg, apeRbl);
    eControl(controlOpReserveApeReg, apeRc);

    // For shifting, keep y in Rah/Ral, and use Rbh/Rbl as extra space
    // [Rah,Ral] = y
    eApeC(apeLoad, apeRah, _, yLAddr+1);
    eApeC(apeLoad, apeRal, _, yLAddr);

    // y ^= (y<<5);
    eApeR(apeSet,  apeRbh, _,      apeRah);  // [Rbh,Rbl] = [Rah,Ral]
    eApeR(apeSet,  apeRbl, _,      apeRal);
    eApeC(apeAsl,  apeRbh, apeRbh, 5);       // Rbh <<= 5
    eApeC(apeRol,  apeRc,  apeRbl, 5);       // Rc = Rbl Rol 5
    eApeC(apeAnd,  apeRc,  apeRc,  0x1F);    // Rc &= 0x1F
    eApeR(apeOr,   apeRbh, apeRbh, apeRc);   // Rbh |= Rc
    eApeC(apeAsl,  apeRbl, apeRbl, 5);       // Rbl <<= 5
    eApeR(apeXor,  apeRal, apeRal, apeRbl);  // Ral ^= Rbl
    eApeR(apeXor,  apeRah, apeRah, apeRbh);  // Rah ^= Rbh

    // y ^= (y>>7);
    eApeR(apeSet,  apeRbh, _,      apeRah);  // [Rbh,Rbl] = [Rah,Ral]
    eApeR(apeSet,  apeRbl, _,      apeRal);
    eApeC(apeAsr,  apeRbl, apeRbl, 7);       // Rbl >>= 7
    eApeC(apeAnd,  apeRbl, apeRbl, 0x1FF);   // Rbl &= 0x1FF
    eApeC(apeAsl,  apeRc,  apeRbh,  9);      // Rc = Rbh << 9
    eApeR(apeOr,   apeRbl, apeRbl, apeRc);   // Rbl |= Rc
    eApeC(apeAsr,  apeRbh, apeRbh, 7);       // Rbh >>= 7
    eApeC(apeAnd,  apeRbh, apeRbh, 0x1FF);   // Rbh &= 0x1FF
    eApeR(apeXor,  apeRal, apeRal, apeRbl);  // Ral ^= Rbl
    eApeR(apeXor,  apeRah, apeRah, apeRbh);  // Rah ^= Rbh

    // y ^= (y<<22);
    eApeC(apeAsl,  apeRbh, apeRal, 6);       // Rbh = Ral << 22-16
    eApeR(apeXor,  apeRah, apeRah, apeRbh);  // Rah ^= Rbh

    // store y = [Rah,Ral]
    eApeC(apeStore, apeRah, _, yLAddr+1);
    eApeC(apeStore, apeRal, _, yLAddr);

    // t = z+w+c;
    eApeC(apeLoad, apeRah, _,      zLAddr+1);    // [Rah,Ral] = z
    eApeC(apeLoad, apeRal, _,      zLAddr);
    eApeC(apeLoad, apeRbh, _,      wLAddr+1);    // [Rbh,Rbl] = w
    eApeC(apeLoad, apeRbl, _,      wLAddr);
    eApeR(apeAdd,  apeRal, apeRal, apeRbl);      // Ral += Rbl
    eApeR(apeAddL, apeRah, apeRah, apeRbh);      // Rah += Rbh + Linc
    eApeC(apeLoad, apeRc,  _,      cAddr);       // Rc = c
    eApeC(apeAnd,  apeRc,  apeRc,  1);           // in case c initialized with >1 bit
    eApeR(apeAdd,  apeRal, apeRal, apeRc);       // Ral += Rc
    eApeC(apeAddL, apeRah, apeRah, 0);           // Rah += Linc

    // z = w;
    eApeC(apeStore, apeRbh, _, zLAddr+1);
    eApeC(apeStore, apeRbl, _, zLAddr);

    // c = (t & 0x80000000) != 0;
    eApeC(apeAsr,   apeRc, apeRah, 15);
    eApeC(apeAnd,   apeRc, apeRc,  1);
    eApeC(apeStore, apeRc, _,      cAddr);

    // w = t & 0x7FFFFFFF;
    eApeC(apeAnd,   apeRah, apeRah, 0x7FFF);
    eApeC(apeStore, apeRah, _,      wLAddr+1);
    eApeC(apeStore, apeRal, _,      wLAddr);

    // x += 1411392427;
    eApeC(apeLoad,  apeRbh, _,      xLAddr+1);
    eApeC(apeLoad,  apeRbl, _,      xLAddr);
    eApeC(apeAdd,   apeRbl, apeRbl, 1411392427 & 0xFFFF);
    eApeC(apeAddL,  apeRbh, apeRbh, (1411392427>>16) & 0xFFFF);
    eApeC(apeStore, apeRbh, _,      xLAddr+1);
    eApeC(apeStore, apeRbl, _,      xLAddr);

    // return low 16 bits of x + y + w;
    eApeR(apeAdd,  apeRal, apeRal, apeRbl);      // Ral += Rbl
    eApeC(apeLoad, apeRbl, _,      yLAddr);      // Rbl = low 16 bits of y
    eApeR(apeAdd, result, apeRal, apeRbl);      // result = Ral + Rbl*/

    eControl(controlOpReleaseApeReg, apeRc);
    eControl(controlOpReleaseApeReg, apeRah);
    eControl(controlOpReleaseApeReg, apeRbh);
    eControl(controlOpReleaseApeReg, apeRal);
    eControl(controlOpReleaseApeReg, apeRbl);
}

void gr_setSeed(int seed){
    Declare(ApeRowG);
    Declare(ApeColG);
    ApeRowG = gu_GetApeGlobalRow();
    ApeColG = gu_GetApeGlobalCol();

    ApeMemVector(APEkissState, Int, 9);

    DeclareApeVar(APErowcol,Int);
    Set(APErowcol, Xor(Asl(ApeRowG, IntConst(8)), ApeColG));
    Set(APErowcol, Add(APErowcol, IntConst(1)));
    for (int i = 0; i < MemLength(APEkissState); i++) {
        Set(IndexVector(APEkissState, IntConst(i)), Add(APErowcol, IntConst(i+seed)));
    }

    isRandomInit = 1;
}

scExpr gr_genRandomNumber(){
    DeclareApeVar(rand_num, Int);
    if(isRandomInit != 1){
        srand(time(NULL));   // Initialization, should only be called once.
        int rand_seed = rand() % 1000; //TODO is this ok? gotta fit into ape register so cant be too big
        printf("S1 RNG initialized without a random seed set. Setting seed to %d...\n", rand_seed);
        gr_setSeed(rand_seed);
    }

    emitNextKISS16(rand_num, MemAddress(APEkissState));

    return rand_num;
}

scExpr gr_randIntModExpr(scExpr upper_bound){
    //this is made to sample an int random number in [0, nb_gens[ using modulo operation
    DeclareApeVar(rand_num, Int);
    DeclareApeVar(quotient, Int);
    DeclareApeVar(remainder, Int);
    DeclareApeVar(denom, Int);
    Set(denom, upper_bound);
    Set(rand_num, gr_genRandomNumber());
    gn_intDiv(rand_num, denom, quotient, remainder);
    
    return remainder;
}

scExpr gr_randIntMod(int upper_bound){
    DeclareApeVar(remainder, Int);
    Set(remainder, gr_randIntModExpr(IntConst(upper_bound)));
    return remainder;
}

scExpr gr_randInt8bit(int upper_bound){
    // Generates random int in [0, upper_bound) for upper_bound <= 256
    // Fastest method for small ranges using bitwise scaling
    DeclareApeVar(rand_num, Int);
    DeclareApeVar(bounded_num, Int);
    Set(rand_num, gr_genRandomNumber());
    Set(bounded_num, gn_bitwiseRangeScaling(rand_num, upper_bound));

    return bounded_num;
}

void gr_initRandomArray(scExpr dstArray, int rows, int cols){
  DeclareCUVar(I_cu, Int);
  DeclareCUVar(J_cu, Int);
  DeclareApeMem(transfer, Int);

  eControl(controlOpReserveApeReg, apeR1);
  CUFor(I_cu, IntConst(0), IntConst(rows-1), IntConst(1));
  	CUFor(J_cu, IntConst(0), IntConst(cols-1), IntConst(1));
    	Set(transfer, gr_genRandomNumber());
  		Set(IndexArray(dstArray, I_cu, J_cu), transfer);
  	CUForEnd();
  CUForEnd();

  eControl(controlOpReleaseApeReg, apeR1);

}