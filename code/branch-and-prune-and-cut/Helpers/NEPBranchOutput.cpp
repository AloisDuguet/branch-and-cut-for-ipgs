//
// Created by alois-duguet on 11/19/25.
//

#include <iostream>

#include "NEPBranchOutput.h"

using namespace std;

void NEPBranchOutput::oneLinePrint(
        ostream &os,
        string const &sep) const {
    os << status << sep;
    os << solutionList.size() << sep;
    os << numberOfNodesExplored << sep;
    os << numberOfCutsAdded << sep;
    os << computationTime << sep;
    os << cutComputationTime << sep;
    os << getWarningCode();
}

int NEPBranchOutput::getWarningCode() const {
    // sum up all warning codes together in a single integer number by
    // aggregating the values of active warning with separator 0
    // the method works as long as all warning codes are one-digit and
    // the max value is less than max int value

    // warning codes
    // antiCyclingMeasuresTaken: 1
    // NEBypassedIntegerFeasibility: 2

    int code = 0;
    if (antiCyclingMeasuresTaken)
        code += 1;
    if (NEBypassedIntegerFeasibility) {
        code *= 100;
        code += 2;
    }
    return code;
}

ostream& operator<<(ostream& os, NEPBranchOutput const& output) {
    os << endl << "----------------------------------------" << endl;
    if (!output.solutionList.empty()) {
        os << output.solutionList.size() << " NE found:" << endl;
        os << "--" << endl;
        os << output.solutionList << endl;
    } else {
        os << "No Nash equilibrium found" << endl;
    }
    os << "computation time: " << output.computationTime << " seconds" << endl;
    os << "cut computation time: " << output.cutComputationTime << " seconds" << endl;
    os << "number of nodes explored: " << output.numberOfNodesExplored << endl;
    os << "number of cuts added: " << output.numberOfCutsAdded << endl;
    os << "anti cycling measures taken: ";
    os << static_cast<string>(output.antiCyclingMeasuresTaken ? "true" : "false") << endl;
    os << "NE bypassed integer feasibility: ";
    os << static_cast<string>(output.NEBypassedIntegerFeasibility ? "true" : "false") << endl;
    os << "----------------------------------------" << endl;
    return os;
}