//
// Created by alois-duguet on 4/30/26.
//

#ifndef BESTADDITIVEMULTIPLICATIVEBRANCHOUTPUT_H
#define BESTADDITIVEMULTIPLICATIVEBRANCHOUTPUT_H

#include "BestMultiplicativeAdditiveBranchOutput.h"

class BestAdditiveMultiplicativeBranchOutput : public BestMultiplicativeAdditiveBranchOutput {
public:
    BestAdditiveMultiplicativeBranchOutput(
                    std::string status,
                    std::vector<NEInfo> solutionList,
                    int numberOfNodesExplored,
                    int numberOfCutsAdded,
                    double computationTime,
                    double cutComputationTime,
                    std::shared_ptr<Node> finalNode,
                    bool antiCyclingMeasuresTaken,
                    bool NEBypassedIntegerFeasibility,
                    bool exactNEPossible,
                    double minDeltaPossible,
                    double bestDeltaFound,
                    double multiplicativeApproxFixed) :
        BestMultiplicativeAdditiveBranchOutput(move(status),
                        move(solutionList),
                        numberOfNodesExplored,
                        numberOfCutsAdded,
                        computationTime,
                        cutComputationTime,
                        move(finalNode),
                        antiCyclingMeasuresTaken,
                        NEBypassedIntegerFeasibility,
                        exactNEPossible,
                        minDeltaPossible,
                        bestDeltaFound,
                        -1) {
        this->multiplicativeApproxFixed = multiplicativeApproxFixed;
    };

    // one-line print
    void oneLinePrint(
        std::ostream& os,
        std::string const& sep) const override;

    double multiplicativeApproxFixed;
};

std::ostream& operator<<(std::ostream& os, BestAdditiveMultiplicativeBranchOutput const& output);


#endif //BESTADDITIVEMULTIPLICATIVEBRANCHOUTPUT_H
