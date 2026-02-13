//
// Created by alois-duguet on 11/19/25.
//

#ifndef BESTMULTIPLICATIVEBRANCHOUTPUT_H
#define BESTMULTIPLICATIVEBRANCHOUTPUT_H

#include "MinNEPBranchOutput.h"

class BestMultiplicativeBranchOutput : public MinNEPBranchOutput {
public:
    BestMultiplicativeBranchOutput(
                    std::string status,
                    std::vector<NEInfo> solutionList,
                    int numberOfNodesExplored,
                    int numberOfCutsAdded,
                    double computationTime,
                    double cutComputationTime,
                    std::shared_ptr<Node> finalNode,
                    bool antiCyclingMeasuresTaken,
                    bool exactNEPossible,
                    double minDeltaPossible,
                    double bestDeltaFound) :
    MinNEPBranchOutput(move(status),
                    move(solutionList),
                    numberOfNodesExplored,
                    numberOfCutsAdded,
                    computationTime,
                    cutComputationTime,
                    move(finalNode),
                    antiCyclingMeasuresTaken,
                    exactNEPossible,
                    minDeltaPossible,
                    bestDeltaFound) {
        minDeltaPossible = 1;
    }
};

std::ostream& operator<<(std::ostream& os, BestMultiplicativeBranchOutput const& output);



#endif //BESTMULTIPLICATIVEBRANCHOUTPUT_H
