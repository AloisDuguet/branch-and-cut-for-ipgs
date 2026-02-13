//
// Created by alois-duguet on 11/19/25.
//

#ifndef MINNEPBRANCHOUTPUT_H
#define MINNEPBRANCHOUTPUT_H

#include "NEPBranchOutput.h"

class MinNEPBranchOutput : public NEPBranchOutput {
public:
    MinNEPBranchOutput(std::string status,
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
    NEPBranchOutput(move(status),
                    move(solutionList),
                    numberOfNodesExplored,
                    numberOfCutsAdded,
                    computationTime,
                    cutComputationTime,
                    move(finalNode),
                    antiCyclingMeasuresTaken) {
        this->exactNEPossible = exactNEPossible;
        this->minDeltaPossible = minDeltaPossible;
        this->bestDeltaFound = bestDeltaFound;
    }

    // one-line print
    void oneLinePrint(
        std::ostream& os,
        std::string const& sep) const override;

    // if an exact NE is possible
    bool exactNEPossible = true;

    // lower bound on the minimum delta such that a delta-NE exists
    double minDeltaPossible = 0;

    // upper bound on delta such that a delta-NE exists
    double bestDeltaFound = MY_INF;
};

std::ostream& operator<<(std::ostream& os, MinNEPBranchOutput const& output);



#endif //MINNEPBRANCHOUTPUT_H
