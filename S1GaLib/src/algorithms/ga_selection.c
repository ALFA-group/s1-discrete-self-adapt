
#include <S1Core/S1Core.h>
#include <S1GaLib/S1GaLib.h>

extern int chipRows, chipCols, apeRows, apeCols;

void gs_copyGenome(scExpr bestNeighbor, int nbWords, bool is_local) {
    DeclareApeVar(data, Int);
    DeclareApeVar(transfer, Int);
    DeclareCUVar(I_cu, Int);
    Set(data, IntConst(0));

    CUFor(I_cu, IntConst(0), IntConst(nbWords-1), IntConst(1));
        Set(transfer, IndexVector(gb_getFullVector(), I_cu));
        if (is_local){
            eApeC(apeGet, data, transfer, bestNeighbor);
        }
        else{
            gu_ApeGetGlobal(transfer, data, bestNeighbor);
        }

        ApeIf(Ne(bestNeighbor, IntConst(4)));
            Set(IndexVector(gb_getFullVector(), I_cu), data);
        ApeFi();

    CUForEnd();
}

void gs_getValidNeighbors(scExpr ret_neighbors, scExpr ret_numNeighbors, bool is_local) {
    DeclareApeVar(apeRow, Int);
    DeclareApeVar(apeCol, Int);
    int r_edge_idx;
    int b_edge_idx;

    if(is_local){
        Set(apeRow, gu_GetApeRow());
        Set(apeCol, gu_GetApeCol());
        r_edge_idx = apeCols;
        b_edge_idx = apeRows;
    }
    else{
        Set(apeRow, gu_GetApeGlobalRow());
        Set(apeCol, gu_GetApeGlobalCol());
        r_edge_idx = apeCols*chipCols;
        b_edge_idx = apeRows*chipRows;
    }

  Set(ret_numNeighbors, IntConst(0));

    ApeIf(Ne(apeRow, IntConst(0)));
        Set(IndexVector(ret_neighbors, ret_numNeighbors), IntConst(getNorth));
        Set(ret_numNeighbors, Add(ret_numNeighbors, IntConst(1)));
    ApeFi();

    ApeIf(Ne(apeRow, Sub(IntConst(b_edge_idx), IntConst(1))));
        Set(IndexVector(ret_neighbors, ret_numNeighbors), IntConst(getSouth));
        Set(ret_numNeighbors, Add(ret_numNeighbors, IntConst(1)));
    ApeFi();

    ApeIf(Ne(apeCol, IntConst(0)));
        Set(IndexVector(ret_neighbors, ret_numNeighbors), IntConst(getWest));
        Set(ret_numNeighbors, Add(ret_numNeighbors, IntConst(1)));
    ApeFi();

    ApeIf(Ne(apeCol, Sub(IntConst(r_edge_idx), IntConst(1))));
        Set(IndexVector(ret_neighbors, ret_numNeighbors), IntConst(getEast));
        Set(ret_numNeighbors, Add(ret_numNeighbors, IntConst(1)));
    ApeFi();
}

scExpr gs_selectionElite(scExpr fitness, int is_min, bool is_local) {
    // Select the local (NEWS neighbors + self) elite individuals based on their fitness, local to own chip
    // returns the direction of the best neighbor (0 - North, 1 - East, 2 - West, 3 - South, 4 - self)

    DeclareApeVar(transferFitness, Approx);
    DeclareApeVar(bestCurrFitness, Approx);
    DeclareApeVar(ret, Int);
    Set(ret, IntConst(4)); // default best neighbor is self
    Set(bestCurrFitness, fitness);

    for(int i = 0; i < 4; i++){
        if(is_local){
            eApeC(apeGet, transferFitness, fitness, i); // i is the direction
        }
        else{
            gu_ApeGetGlobal(fitness, transferFitness, i);
        }
        
        // Check if we are doing a minimization or maximization of fitness
        if (is_min){
            ApeIf(Lt(transferFitness, bestCurrFitness));
                Set(ret, IntConst(i));
                Set(bestCurrFitness, transferFitness);
            ApeFi();
        }
        else{
            ApeIf(Gt(transferFitness, bestCurrFitness));
                Set(ret, IntConst(i));
                Set(bestCurrFitness, transferFitness);
            ApeFi();
        }
    }

    return ret;
}

scExpr gs_selectionTournament(scExpr fitness, int tourny_size, int is_min, bool is_local) {
    // Tournament selection with replacement, local chip or global board
    // returns the direction of the selected neighbor (0 - North, 1 - East, 2 - West, 3 - South, 4 - self)

    DeclareApeVar(transferFitness, Approx);
    DeclareApeVar(bestCurrFitness, Approx);
    DeclareApeVar(selectedNeighbor, Int);
    DeclareApeVar(ret, Int);

    Set(ret, IntConst(4)); // default best neighbor is self
    Set(bestCurrFitness, fitness);

    for(int i = 0; i < tourny_size; i++){
        Set(selectedNeighbor, gr_randInt8bit(5)); // select from neighbors for tourney
        ApeIf(Ne(selectedNeighbor, IntConst(4))); // dont need to do comparison against self
            if(is_local){
                eApeC(apeGet, transferFitness, fitness, selectedNeighbor); 
            }
            else{
                gu_ApeGetGlobal(fitness, transferFitness, selectedNeighbor);
            }
            
            if (is_min){
                ApeIf(Lt(transferFitness, bestCurrFitness));
                    Set(ret, IntConst(i));
                    Set(bestCurrFitness, transferFitness);
                ApeFi();
            }
            else{
                ApeIf(Gt(transferFitness, bestCurrFitness));
                    Set(ret, IntConst(i));
                    Set(bestCurrFitness, transferFitness);
                ApeFi();
            }
        ApeFi();
    }

    return ret;
}

