//
// Created by alois-duguet on 3/30/26.
//

#ifndef BESTPARTIALMINMULTFIXEDADDBRANCHOUTPUT_H
#define BESTPARTIALMINMULTFIXEDADDBRANCHOUTPUT_H

#include "BestMultiplicativeAdditiveBranchOutput.h"

class BestPartialMinMultFixedAddBranchOutput : public BestMultiplicativeAdditiveBranchOutput {
public:
    BestPartialMinMultFixedAddBranchOutput(
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
        double additiveApproxFixed,
        std::vector<bool> multToMinimize) :
            BestMultiplicativeAdditiveBranchOutput(
                move(status),
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
                additiveApproxFixed) {
        this->multToMinimize = move(multToMinimize);
    };

    // one-line print
    void oneLinePrint(
        std::ostream& os,
        std::string const& sep) const override;

    std::vector<bool> multToMinimize;
};

std::ostream& operator<<(std::ostream& os, BestPartialMinMultFixedAddBranchOutput const& output);

#endif //BESTPARTIALMINMULTFIXEDADDBRANCHOUTPUT_H
