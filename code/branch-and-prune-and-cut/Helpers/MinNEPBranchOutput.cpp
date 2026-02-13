//
// Created by alois-duguet on 11/19/25.
//

#include <vector>

#include "MinNEPBranchOutput.h"

using namespace std;

void MinNEPBranchOutput::oneLinePrint(
        ostream &os,
        string const &sep) const {
    NEPBranchOutput::oneLinePrint(os, sep);
    os << sep;
    os << exactNEPossible << sep;
    os << minDeltaPossible << sep;
    os << bestDeltaFound;
}


ostream& operator<<(ostream& os, MinNEPBranchOutput const& output) {
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

    // part specific to min additive approximation search
    if (output.bestDeltaFound == 0)
        os << "min delta: 0" << endl;
    else {
        if (output.minDeltaPossible == 0) {
            if (output.exactNEPossible)
                os << "bounds on min delta: [0," << output.bestDeltaFound << "]" << endl;
            else
                os << "bounds on min delta: ]0," << output.bestDeltaFound << "]" << endl;
        }
        else
            os << "bounds on min delta: ]" << output.minDeltaPossible << "," << output.bestDeltaFound << "]" << endl;
    }

    os << "anti cycling measures taken: ";
    if (output.antiCyclingMeasuresTaken)
        os << "true" << endl;
    else
        os << "false" << endl;
    os << "----------------------------------------" << endl;
    return os;
}