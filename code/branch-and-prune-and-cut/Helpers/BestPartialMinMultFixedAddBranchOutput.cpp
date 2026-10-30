//
// Created by alois-duguet on 3/30/26.
//

#include "BestPartialMinMultFixedAddBranchOutput.h"

#include "BestMultiplicativeAdditiveBranchOutput.h"

using namespace std;

void BestPartialMinMultFixedAddBranchOutput::oneLinePrint(
        ostream &os,
        string const& sep) const {
    BestMultiplicativeAdditiveBranchOutput::oneLinePrint(os, sep);
    os << sep;
    os << multToMinimize;
}

ostream& operator<<(ostream& os, BestPartialMinMultFixedAddBranchOutput const& output) {
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

    os << "which multiplicative approximation parameters to minimize: " << output.multToMinimize << endl;

    if (output.bestDeltaFound == 1)
        os << "min factor: 1" << endl;
    else {
        if (output.minDeltaPossible == 1) {
            if (output.exactNEPossible)
                os << "bounds on min factor: [1," << format("{:.{}g}", output.bestDeltaFound, getPrecisionFloatFormat(output.bestDeltaFound)) << "]" << endl;
            else
                os << "bounds on min factor: ]1," << format("{:.{}g}", output.bestDeltaFound, getPrecisionFloatFormat(output.bestDeltaFound)) << "]" << endl;
        }
        else
            os << "bounds on min factor: ]" << format("{:.{}g}", output.minDeltaPossible, getPrecisionFloatFormat(output.minDeltaPossible)) << "," << format("{:.{}g}", output.bestDeltaFound, getPrecisionFloatFormat(output.bestDeltaFound)) << "]" << endl;
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