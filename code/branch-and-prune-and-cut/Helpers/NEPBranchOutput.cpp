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
    os << antiCyclingMeasuresTaken;
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
    if (output.antiCyclingMeasuresTaken)
        os << "true" << endl;
    else
        os << "false" << endl;
    os << "----------------------------------------" << endl;
    return os;
}