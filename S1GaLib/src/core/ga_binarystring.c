#include <S1Core/S1Core.h>
#include <S1GaLib/S1GaLib.h>

Declare(BS);
static int vectorSize = 0;

void gb_initBinaryString(int nb_words){
    ApeMemVector(BS, Int, nb_words);
    vectorSize = nb_words;
}

scExpr gb_get(scExpr startPos, int size){
    //TODO add check that size is 16 or less
    DeclareApeVar(inVectorBegin, Int);
    DeclareApeVar(ret, Int);
    Set(inVectorBegin, Sub(startPos, Asl(Asr(startPos, IntConst(4)), IntConst(4))));

    //if inVectorBegin + size > 16 --> the result is in multiple vectors
    ApeIf(Not(Gt(Add(inVectorBegin, IntConst(size)), IntConst(16))));
        //this is the case where what we want to recover lies in a single vector
        //ret <- (BS[startPos//16] >> (16-size-inVectorBegin)) and (1 << size) - 1
        //the part "(1 << size) - 1" makes an only ones string of length "size"
        Set(
            ret,
            And(
                Asr(
                    IndexVector(BS,Asr(startPos, IntConst(4))),
                    Sub(IntConst(16-size), inVectorBegin)
                ),
                IntConst((1 << size) - 1)
            )
        );
    ApeElse(); 
        //this is in the scenarion where the result is spread in two separate vectors v1 and v2 :
        //let's imagine the vectors as displayed                                        v1                   v2
        //imagine we would like to take the part of the vector with x and y ...|0000'0000'0000'xxxx|yyyy'yyyy'0000'0000|....
        //it means we have to build in ret (v1) << length(yyyy'yyyy) = 8 to make room for y's and then concatenate the y
        //the y's and first shifted (v2 >> 16-length(yyyy'yyyy)) and then concat with or (v1 << .... OR v2 >> ....)
        //the formula could be written the following way:
        //	ret <- [(v2 >> right_shift) AND (1's of length(y's))] OR [(v1 << length(y's)) AND ((1's of length(x's)) << length(y's))]
        Set(
            ret,
            Or(
                And( //first and --> we shift right the part in BS[startPos//16] + 1
                    Asr(
                        IndexVector(BS, Add(Asr(startPos, IntConst(4)), IntConst(1))),
                        Sub(IntConst(32-size), inVectorBegin) //32 - size - inVectorBegin => the amount of shift the part in BS[startPos//16] + 1
                    ),
                    Sub(Asl(IntConst(1), Add(IntConst(size-16), inVectorBegin)), IntConst(1)) //we create a string of 1's of length size - (16 - inVectorBegin)
                ),
                And( //second and -->we shift left the part in BS[startPos//16]
                    Asl(IndexVector(BS, Asr(startPos, IntConst(4))), Add(IntConst(size-16), inVectorBegin)),
                    Asl(Sub(Asl(IntConst(1), Sub(IntConst(16), inVectorBegin)), IntConst(1)), Add(IntConst(size-16), inVectorBegin))
                )
            )
        );
    ApeFi();

    return ret;
}

void gb_set(scExpr startPos, int size, scExpr newValue){
    //TODO add check that size is 16 or less
    DeclareApeVar(inVectorBegin, Int);

    Set( //this variable gives the first bit inside of the vector element
        inVectorBegin,
        Sub(startPos, Asl(Asr(startPos, IntConst(4)), IntConst(4)))
    );

    //if inVectorBegin + size > 16 --> the result is in multiple vectors
    ApeIf(Not(Gt(Add(inVectorBegin, IntConst(size)), IntConst(16))));
        //here this is the case where the whole value that we write is inside only one vector element
        //we first the part where our no value will go to zero for that the formula look like this
        //to do som we built a mask that has 0 where our data will be (if the data is 0000'xxx0 then the mask is 1111'0001)
        //BS[startPos//16] <-- BS[startPos//16] AND left_mask AND right_mask
        //BS[startPos//16] <-- BS[startPos//16] AND {[(1 << inVectorBegin) - 1] << (16 - inVectorBegin)} AND {[1 << (16-inVectorBegin-size)] -1}
        Set(
            IndexVector(BS, Asr(startPos, IntConst(4))), //
            And(
                IndexVector(BS, Asr(startPos, IntConst(4))),
                Or(
                    Asl(
                        Sub(Asl(IntConst(1), inVectorBegin), IntConst(1)),
                        Sub(IntConst(16), inVectorBegin)
                    ),
                    Sub(
                        Asl(IntConst(1), Sub(IntConst(16-size), inVectorBegin)),
                        IntConst(1)
                    )
                )
            )
        );

    //then we do an OR to put something at the same place with our shifted data
    //to insert the data we do 2 things :
    //1. do a mask on our data to ensure that what we will write is not bigger than it should
    //2. shift the data
    //this is done with the following formula : BS[startPos//16] <-- BS[startPos//16] OR {[newValue << (16 - inVectorBegin)] AND [(1 << size) - 1]}
    Set(
        IndexVector(BS, Asr(startPos, IntConst(4))),
        Or(
            Asl(
                And(newValue, Sub(Asl(IntConst(1), IntConst(size)), IntConst(1))),
                Sub(IntConst(16-size), inVectorBegin)
            ),
            IndexVector(BS, Asr(startPos, IntConst(4)))
        )
    );

    ApeElse();
        //this is the scenario where what we want to write to the bit string has to be written accross 2 vector elements
        //the process here is the same idea than before but we have to do it twice : one time for each element
        //we first handle the part that goes in BS[startPos//16] and then in BS[startPos//16 + 1]

        //we first handle the part that will be in the first vector BS[startPos//16]

        //we zero where our data will come in BS[startPos//16]
        //the formula to do it is the following : BS[startPos//16] <-- BS[startPos//16] AND NOT[(1 << (16-inVectorBegin)) - 1]
        Set(
            IndexVector(BS, Asr(startPos, IntConst(4))),
            And(
                IndexVector(BS, Asr(startPos, IntConst(4))),
                Not(Sub(Asl(IntConst(1), Sub(IntConst(16), inVectorBegin)),IntConst(1)))
            )
        );

        //now the part we would like to write it is empty we can do an OR with our newValue that has been firstly shifted to be written on the
        //spot. Also in order to not write something too big for the spot we also first put a mask to newValue
        //the formula is the following : BS[startPos//16] <-- BS[startPos//16] OR {[newValue >> (size - 16 + inVectorBegin)] AND [(1 << (16 - inVectorBegin)) - 1]}
        Set(
            IndexVector(BS, Asr(startPos, IntConst(4))),
            Or(
                IndexVector(BS, Asr(startPos, IntConst(4))),
                And(
                    Asr(newValue, Add(IntConst(size-16), inVectorBegin)),
                    Sub(Asl(IntConst(1), Sub(IntConst(16), inVectorBegin)), IntConst(1))
                )
            )
        );

        //we handle now the part that will be written in the second vector BS[startPos//16] + 1
        //we first zero the spot we would like to write in
        //the formula for it is the following : BS[startPos//16 + 1] <-- BS[startPos//16 + 1] AND [(1 << (32 - inVectorBegin - size))- 1]
        Set(
            IndexVector(BS, Add(Asr(startPos, IntConst(4)), IntConst(1))),
            And(
                IndexVector(BS, Add(Asr(startPos, IntConst(4)), IntConst(1))),
                Sub(Asl(IntConst(1), Sub(IntConst(32-size), inVectorBegin)), IntConst(1))
            )
        );

        //now we fill at the place in the vector as before we shift but also mask to make sure the data inserted is not too big
        //the formula is the following : BS[startPos//16 + 1] <-- BS[startPos//16 + 1] OR {[newValue << (32 - inVectorBegin - size)] AND NOT[(1 << (32 - inVectorBegin - size)) - 1]}
        Set(
            IndexVector(BS, Add(Asr(startPos, IntConst(4)), IntConst(1))),
            Or(
                IndexVector(BS, Add(Asr(startPos, IntConst(4)), IntConst(1))),
                And(
                    Asl(newValue, Sub(IntConst(32-size), inVectorBegin)),
                    Not(Sub(Asl(IntConst(1), Sub(IntConst(32-size), inVectorBegin)),IntConst(1)))
                )
            )
        );
    ApeFi();

}

void gb_setVectorEntryExpr(scExpr idx, scExpr val){
    Set(IndexVector(BS, idx), val);
}

scExpr gb_getVectorEntryExpr(scExpr idx){
    return IndexVector(BS, idx);
}

void gb_setVectorEntry(int idx, scExpr val){
    gb_setVectorEntryExpr(IntConst(idx), val);
}

scExpr gb_getVectorEntry(int idx){
    return gb_getVectorEntryExpr(IntConst(idx));
}

scExpr gb_getFullVector(){
  return BS;
}

int gb_getNbWords(){
  return vectorSize;
}

