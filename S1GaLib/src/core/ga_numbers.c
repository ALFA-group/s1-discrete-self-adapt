#include <stdio.h>

#include <S1Core/S1Core.h>
#include <S1GaLib/S1GaLib.h>

extern int chipRows, chipCols, apeRows, apeCols;

scExpr gn_intToApprox(scExpr uintX){
  DeclareApeVar(approxX, Approx);
  // For each set bit i in the input, add Approx(2^i) to the output
  int xabs = apeR4;
  int aX = apeR5;
  int tempReg = apeR6;
  eControl(controlOpReserveApeReg, xabs);
  eApeX(apeSet, xabs, _, uintX);

  eControl(controlOpReserveApeReg, aX);
  eControl(controlOpReserveApeReg, tempReg);

  eApeC(apeSet, aX, _, approx0);

  int i;
  for (i = 0; i < 16; i++) {
    eApeC(apeAnd, tempReg, xabs, 1<<i);
    eApeC(apeRor, tempReg, tempReg, i);
    eApeC(apeRor, tempReg, tempReg, 2);  //assumes register is either all zeroes -> goes to approx0, or register is all zeroes and bottom bit 1 -> goes to approx 1

    eApeC(apeMAddA, aX, tempReg, cvtApprox(pow(2, i))); // ax += tempReg * Approx(2^i)
  }

  eApeR(apeSet, approxX, _, aX);

  eControl(controlOpReleaseApeReg, xabs);
  eControl(controlOpReleaseApeReg, aX);
  eControl(controlOpReleaseApeReg, tempReg);

  return approxX;
}

scExpr gn_powApprox(scExpr base, scExpr exp) {
  DeclareApeVar(res, Approx);
  Set(res, AConst(1));
  DeclareCUVar(I_cu, Int);
  CUFor(I_cu, IntConst(1), exp, IntConst(1));
    Set(res, Mul(res, base));
  CUForEnd();

  return res;
}

void gn_intDiv(scExpr num, scExpr denom, scExpr quotient, scExpr remainder){
  gn_intDivFast(num, denom, 16, quotient, remainder);
}

void gn_intDivFast(scExpr numer, scExpr denomin, int num_bits, scExpr quotient, scExpr remainder){
  DeclareApeVar(num, Int);
  DeclareApeVar(denom, Int);
  Set(num, numer);
  Set(denom, denomin);
  DeclareCUVar(I_cu, Int);
  DeclareApeVar(mask, Int);
  DeclareApeVar(mask_denom, Int);
  Set(quotient, IntConst(0));
  Set(remainder, IntConst(0));

  // If numerator is negative, take absolute value
  Set(mask, Asr(num, IntConst(15)));
  Set(num, Sub(Xor(num, mask), mask));

  // If denominator is negative, take absolute value
  Set(mask_denom, Asr(denom, IntConst(15)));
  Set(denom, Sub(Xor(denom, mask_denom), mask_denom));

  // If either num or denom is negative, then result is negative. If neither are or both are, result is positive
  Set(mask, Xor(mask, mask_denom));

  ApeIf(Ne(denom, IntConst(0)));
    DeclareApeVar(temp, Int);
    
    // The long division algorithm processes each bit of the dividend, updating the remainder and quotient step by step
    CUFor(I_cu, IntConst(16 - num_bits), IntConst(15), IntConst(1));
      Set(temp, Sub(IntConst(15), I_cu));
      Set(remainder, Asl(remainder, IntConst(1))); // R << 1
      Set(remainder, Or(remainder, And(Asr(num, temp), IntConst(1)))); // R = (R << 1) | ((N >> i) & 1);
      ApeIf(Ge(remainder, denom));
        Set(remainder, Sub(remainder, denom));
        Set(quotient, Or(quotient, Asl(IntConst(1), temp)));
      ApeFi();
    CUForEnd();

    // Set the sign bit
    ApeIf(Eq(mask, IntConst(0xFFFF)));
      Set(quotient, Add(Not(quotient), IntConst(1)));
    ApeFi();
  ApeFi();
}

void gn_uintMul(scExpr nbA, scExpr nbB, scExpr ret_lo, scExpr ret_hi) {
  // Declare 16-bit working variables
  DeclareApeVar(intA, Int);
  DeclareApeVar(intB, Int);
  Set(intA, nbA);
  Set(intB, nbB);

  DeclareApeVar(nbAcp, Int);
  DeclareApeVar(nbBcp, Int);
  DeclareCUVar(I_cu, Int);

  // Choose smaller multiplier for fewer iterations (optional optimization)
  ApeIf(Gt(intA, intB));
    Set(nbAcp, intB);
    Set(nbBcp, intA);
  ApeElse();
    Set(nbAcp, intA);
    Set(nbBcp, intB);
  ApeFi();

  // Initialize result
  Set(ret_lo, IntConst(0));
  Set(ret_hi, IntConst(0));

  CUFor(I_cu, IntConst(0), IntConst(16), IntConst(1));
    // If current LSB of B is 1 → add A to result
    ApeIf(Eq(IntConst(1), And(nbBcp, IntConst(0x0001))));
      // Add nbAcp to the 32-bit result (ret_hi:ret_lo)
      DeclareApeVar(sum, Int);
      DeclareApeVar(carry, Int);
      
      // Add to low part
      Set(sum, Add(ret_lo, nbAcp));
      
      // carry = (sum < ret_lo) ? 1 : 0
      ApeIf(Lt(sum, ret_lo));
        Set(carry, IntConst(1));
      ApeElse();
        Set(carry, IntConst(0));
      ApeFi();

      // Update result
      Set(ret_lo, sum);
      Set(ret_hi, Add(ret_hi, carry));
    ApeFi();

    // Shift A left (<< 1) and B right (>> 1)
    Set(nbAcp, Asl(nbAcp, IntConst(1)));
    Set(nbBcp, Asr(nbBcp, IntConst(1))); // If unsigned, replace with logical shift if possible
  CUForEnd();


}

scExpr gn_intMulCU(scExpr nbA, scExpr nbB) {
  DeclareCUVar(ret, Int);
  DeclareCUVar(I_cu, Int);
  Set(ret, IntConst(0));
  CUFor(I_cu, IntConst(0), nbA, IntConst(1));
    Set(ret, Add(ret, nbB));
  CUForEnd();

  return ret;
}

scExpr gn_bitwiseRangeScaling(scExpr num, int upper_bound){
  /**
   * scaled = (num * upper_bound + ((2^nb_bits-1)-1)))/ (2^nb_bits)
   */
  int nb_bits = gu_bits_needed(upper_bound);
  if (nb_bits > 8){
    printf("ERROR: Upper bound can not be greater than 255. Your upper bound: %d\n", upper_bound);
    exit(1);
  }

  // Only operate on number of bits needed to represent upper bound
  DeclareApeVar(mask, Int);
  Set(mask, Sub(Asl(IntConst(1), IntConst(nb_bits)), IntConst(1)));
  Set(num, And(num, mask));

  DeclareCUVar(I_cu, Int);
  DeclareApeVar(product, Int);
  Set(product, IntConst(0));

  // Multiply num by upper_bound
  CUFor(I_cu, IntConst(0), IntConst(nb_bits - 1), IntConst(1));
    ApeIf(Asr(And(IntConst(upper_bound), Asl(IntConst(1), I_cu)), I_cu));
      Set(product, Add(product, Asl(num, I_cu)));
    ApeFi();
  CUForEnd();
  
  // Approximate division by 2^nb_bits using shift and add:
  // (product + 2^(nb_bits-1) - 1) / 2^nb_bits ≈ (product + (product >> nb_bits) + 2^(nb_bits-1) - 1) >> nb_bits
  Set(product, Add(product, Asr(product, IntConst(nb_bits))));
  DeclareApeVar(round_val, Int);
  Set(round_val, Sub(Asl(IntConst(1), IntConst(nb_bits - 1)), IntConst(1))); // compute 2^(nb_bits -1)-1
  Set(product, Asr(Add(product, round_val), IntConst(nb_bits)));

  return product;
}

//TODO Clean up code for below func
void gn_coordMin(scExpr fitness, scExpr bestFitnessTransfer, scExpr apeRowBestFitness, scExpr apeColBestFitness) {

  //in the context of GA the target is the fitness
  DeclareCUVar(I_cu, Int);
  DeclareApeVar(fitnessTr, Approx);
  DeclareApeVar(apeColRowTransfer, Int);
  /*DeclareApeVar(bestFitnessTransfer, Approx);
  DeclareApeVar(apeRowBestFitness, Int);
  DeclareApeVar(apeColBestFitness, Int);*/

  /*DeclareApeMem(debug, Approx);
  DeclareApeMem(debugI, Int);*/

  scExpr apeCol = gu_GetApeCol();
  scExpr apeRow = gu_GetApeRow();

  Set(apeColBestFitness, apeCol);
  Set(apeRowBestFitness, apeRow);
  Set(bestFitnessTransfer, fitness);

  /*Set(debug, bestFitnessTransfer);
  TraceMemRangeApe(debug,40, 40, 49, 45,  approxFormat);*/

  //TraceMessage("---------------------\n");
  //data should arrive in left most col
  CUFor(I_cu, IntConst(0), IntConst(apeCols -1 /*15*/), IntConst(1));
    //we move the data to the left
    eApeC(apeGet, bestFitnessTransfer, bestFitnessTransfer, getEast);
    eApeC(apeGet, apeColBestFitness, apeColBestFitness, getEast);

    ApeIf(Eq(
      apeCol, IntConst(apeCols - 1)
    ));
      Set(apeColBestFitness, apeCol);
      Set(bestFitnessTransfer, fitness);
    ApeFi();

    //check it's NOT the most right col
    ApeIf(
      Lt(fitness, bestFitnessTransfer)
    );
      Set(apeColBestFitness, apeCol);
      Set(bestFitnessTransfer, fitness);

    ApeFi();

    /*Set(debug, bestFitnessTransfer);
    TraceMemRangeApe(debug,  0, 0, 10, 10, approxFormat);*/

  CUForEnd();

  /*Set(debugI, apeColBestFitness);
  TraceMemRangeApe(debugI,  0, 0, 10, 10, integerFormat);*/

  //we now do the same to retrieve the data in the left most column

  /*Set(debug, bestFitnessTransfer);
  TraceMemRangeApe(debug, 0, 0, 10, 10, approxFormat);
  Set(debugI, apeRowBestFitness);
  TraceMemRangeApe(debugI, 0, 0, 10, 10, integerFormat);
  Set(debugI, apeColBestFitness);
  TraceMemRangeApe(debugI, 0, 0, 10, 10, integerFormat);
  TraceMessage("\n----\n----\n----\n");*/

  Set(fitnessTr, bestFitnessTransfer);

  CUFor(I_cu, IntConst(0), IntConst(apeRows -1), IntConst(1));

  	//eApeX(apeGetGEnd, dstVarTemp, srcVar, i);
  	eApeC(apeGet, fitnessTr, bestFitnessTransfer, getSouth);
    eApeC(apeGet, apeColRowTransfer, apeRowBestFitness, getSouth);

    ApeIf(And(
      Ne(apeRow, IntConst(apeRows -1)),
      Lt(fitnessTr, bestFitnessTransfer)
    )); //fitnessTr < bestFitnessTransfer ==> the fit from the south ape is smaller

      Set(apeRowBestFitness, apeColRowTransfer);
    ApeFi();

    eApeC(apeGet, apeColRowTransfer, apeColBestFitness, getSouth);

    ApeIf(And(
      Ne(apeRow, IntConst(apeRows -1)),
      Lt(fitnessTr, bestFitnessTransfer)
    )); //fitnessTr < bestFitnessTransfer ==> the fit from the south ape is smaller
      Set(apeColBestFitness, apeColRowTransfer);
      Set(bestFitnessTransfer, fitnessTr);
    ApeFi();


    /*TraceMessage("-----\n")
  	Set(debug, bestFitnessTransfer);
  	TraceMemRangeApe(debug, 0, 0, 10, 10, approxFormat);
  	Set(debugI, apeRowBestFitness);
  	TraceMemRangeApe(debugI, 0, 0, 10, 10, integerFormat);
  	Set(debugI, apeColBestFitness);
  	TraceMemRangeApe(debugI, 0, 0, 10, 10, integerFormat);*/

  CUForEnd();

}

//TODO Clean up code for below func
void gn_coordMax(scExpr fitness, scExpr bestFitnessTransfer, scExpr apeRowBestFitness, scExpr apeColBestFitness) {

  //in the context of GA the target is the fitness
  DeclareCUVar(I_cu, Int);
  DeclareApeVar(fitnessTr, Int);
  DeclareApeVar(apeColRowTransfer, Int);

  scExpr apeCol = gu_GetApeCol();
  scExpr apeRow = gu_GetApeRow();

  Set(apeColBestFitness, apeCol);
  Set(apeRowBestFitness, apeRow);
  Set(bestFitnessTransfer, fitness);

  //data should arrive in left most col
  CUFor(I_cu, IntConst(0), IntConst(apeCols -1), IntConst(1));
    //we move the data to the left
    eApeC(apeGet, bestFitnessTransfer, bestFitnessTransfer, getEast);
    eApeC(apeGet, apeColBestFitness, apeColBestFitness, getEast);

    ApeIf(Eq(
      apeCol, IntConst(apeCols - 1)
    ));
      Set(apeColBestFitness, apeCol);
      Set(bestFitnessTransfer, fitness);
    ApeFi();

    //check it's NOT the most right col
    ApeIf(
      Gt(fitness, bestFitnessTransfer)
    );
      Set(apeColBestFitness, apeCol);
      Set(bestFitnessTransfer, fitness);

    ApeFi();

  CUForEnd();

  Set(fitnessTr, bestFitnessTransfer);

  CUFor(I_cu, IntConst(0), IntConst(apeRows -1), IntConst(1));

  	eApeC(apeGet, fitnessTr, bestFitnessTransfer, getSouth);
    eApeC(apeGet, apeColRowTransfer, apeRowBestFitness, getSouth);

    ApeIf(And(
      Ne(apeRow, IntConst(apeRows -1)),
      Gt(fitnessTr, bestFitnessTransfer)
    )); //fitnessTr < bestFitnessTransfer ==> the fit from the south ape is larger

      Set(apeRowBestFitness, apeColRowTransfer);
    ApeFi();

    eApeC(apeGet, apeColRowTransfer, apeColBestFitness, getSouth);

    ApeIf(And(
      Ne(apeRow, IntConst(apeRows -1)),
      Gt(fitnessTr, bestFitnessTransfer)
    )); //fitnessTr < bestFitnessTransfer ==> the fit from the south ape is larger
      Set(apeColBestFitness, apeColRowTransfer);
      Set(bestFitnessTransfer, fitnessTr);
    ApeFi();


  CUForEnd();




}