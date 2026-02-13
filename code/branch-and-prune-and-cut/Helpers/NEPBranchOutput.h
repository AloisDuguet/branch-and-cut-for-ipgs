//
// Created by alois-duguet on 11/19/25.
//

#ifndef NEPBRANCHOUTPUT_H
#define NEPBRANCHOUTPUT_H

#include <vector>
#include <memory>

#include "NEPAbstractSolver.h"
#include "Node.h"

// structure for output of a branching method for NEP
class NEPBranchOutput {
public:
    // "UNKNOWN" reserved to initialization
    // "NO_SOLUTION_FOUND" = no solution found
    // "SOLUTION_FOUND" = at least one solution found
    // "TIME_LIMIT_REACHED" = time limit reached
    NEPBranchOutput(std::string status,
                    std::vector<NEInfo> solutionList,
                    int numberOfNodesExplored,
                    int numberOfCutsAdded,
                    double computationTime,
                    double cutComputationTime,
                    std::shared_ptr<Node> finalNode,
                    bool antiCyclingMeasuresTaken) {
        this->status = move(status);
        this->solutionList = move(solutionList);
        this->numberOfNodesExplored = numberOfNodesExplored;
        this->numberOfCutsAdded = numberOfCutsAdded;
        this->computationTime = computationTime;
        this->cutComputationTime = cutComputationTime;
        this->finalNode = move(finalNode);
        this->antiCyclingMeasuresTaken = antiCyclingMeasuresTaken;
    }

    // one-line print
    virtual void oneLinePrint(
        std::ostream& os,
        std::string const& sep) const;

    std::string status = "UNKNOWN";

    std::vector<NEInfo> solutionList = {};

    // keep -1 to differentiate unchanged struct from other cases
    int numberOfNodesExplored = -1;

    // keep -1 to differentiate unchanged struct from other cases
    int numberOfCutsAdded = -1;

    // keep -1 to differentiate unchanged struct from other cases
    double computationTime = -1.0;
    double cutComputationTime = -1.0;

    std::shared_ptr<Node> finalNode = nullptr;
    bool antiCyclingMeasuresTaken = false;
};

std::ostream& operator<<(std::ostream& os, NEPBranchOutput const& output);

#endif //NEPBRANCHOUTPUT_H
