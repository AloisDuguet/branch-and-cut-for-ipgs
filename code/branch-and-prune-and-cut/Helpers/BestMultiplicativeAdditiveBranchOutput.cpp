//
// Created by alois-duguet on 12/4/25.
//

#include "BestMultiplicativeAdditiveBranchOutput.h"

#include "BestMultiplicativeApproxGNEP.h"

using namespace std;

void BestMultiplicativeAdditiveBranchOutput::oneLinePrint(
        ostream &os,
        string const& sep) const {
    MinNEPBranchOutput::oneLinePrint(os, sep);
    os << sep;
    os << additiveApproxFixed;
}

ostream& operator<<(ostream& os, BestMultiplicativeAdditiveBranchOutput const& output) {
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

    os << "additive approximation fixed: " << output.additiveApproxFixed << endl;

    os << "anti cycling measures taken: ";
    if (output.antiCyclingMeasuresTaken)
        os << "true" << endl;
    else
        os << "false" << endl;
    os << "----------------------------------------" << endl;
    return os;
}