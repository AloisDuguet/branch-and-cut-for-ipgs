//
// Created by alois-duguet on 11/19/25.
//

#include <vector>

#include "BestMultiplicativeBranchOutput.h"

using namespace std;

ostream& operator<<(ostream& os, BestMultiplicativeBranchOutput const& output) {
    os << endl << "----------------------------------------" << endl;
    if (!output.solutionList.empty()) {
        os << output.solutionList.size() << " NE found:" << endl;
        os << "--" << endl;
        os << output.solutionList << endl;
    } else {
        os << "No Nash equilibria found" << endl;
    }
    os << "computation time: " << output.computationTime << " seconds" << endl;
    os << "number of nodes explored: " << output.numberOfNodesExplored << endl;
    os << "number of cuts added: " << output.numberOfCutsAdded << endl;

    if (output.bestDeltaFound == 1)
        os << "min factor: 1" << endl;
    else {
        if (output.minDeltaPossible == 1) {
            if (output.exactNEPossible)
                os << "bounds on min factor: [1," << output.bestDeltaFound << "]" << endl;
            else
                os << "bounds on min factor: ]1," << output.bestDeltaFound << "]" << endl;
        }
        else
            os << "bounds on min factor: ]" << output.minDeltaPossible << "," << output.bestDeltaFound << "]" << endl;
    }

    os << "anti cycling measures taken: ";
    if (output.antiCyclingMeasuresTaken)
        os << "true" << endl;
    else
        os << "false" << endl;
    os << "----------------------------------------" << endl;
    return os;
}