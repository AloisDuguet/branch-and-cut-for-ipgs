//
// Created by alois-duguet on 12/4/25.
//

#ifndef BESTMULTIPLICATIVEADDITIVEBRANCHOUTPUT_H
#define BESTMULTIPLICATIVEADDITIVEBRANCHOUTPUT_H

#include "BestMultiplicativeBranchOutput.h"

class BestMultiplicativeAdditiveBranchOutput : public BestMultiplicativeBranchOutput {
public:
    BestMultiplicativeAdditiveBranchOutput(
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
                    double bestDeltaFound,
                    double additiveApproxFixed) :
    BestMultiplicativeBranchOutput(move(status),
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
        this->additiveApproxFixed = additiveApproxFixed;
    };

    // one-line print
    void oneLinePrint(
        std::ostream& os,
        std::string const& sep) const override;

    double additiveApproxFixed;
};

std::ostream& operator<<(std::ostream& os, BestMultiplicativeAdditiveBranchOutput const& output);


#endif //BESTMULTIPLICATIVEADDITIVEBRANCHOUTPUT_H
