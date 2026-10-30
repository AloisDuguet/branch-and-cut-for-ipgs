//
// Created by alois-duguet on 4/30/26.
//

#include "BestAdditiveMultiplicativeBranchOutput.h"

using namespace std;

void BestAdditiveMultiplicativeBranchOutput::oneLinePrint(
        ostream &os,
        string const &sep) const {
    MinNEPBranchOutput::oneLinePrint(os, sep);
    os << sep << multiplicativeApproxFixed;
}

ostream& operator<<(ostream& os, BestAdditiveMultiplicativeBranchOutput const& output) {
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

    if (output.bestDeltaFound == 0)
        os << "min additive approximation: 0" << endl;
    else {
        if (output.minDeltaPossible == 0) {
            if (output.exactNEPossible)
                os << "bounds on min additive approximation: [0," << format("{:.{}g}", output.bestDeltaFound, getPrecisionFloatFormat(output.bestDeltaFound)) << "]" << endl;
            else
                os << "bounds on min additive approximation: ]0," << format("{:.{}g}", output.bestDeltaFound, getPrecisionFloatFormat(output.bestDeltaFound)) << "]" << endl;
        }
        else
            os << "bounds on min additive approximation: ]" << format("{:.{}g}", output.minDeltaPossible, getPrecisionFloatFormat(output.minDeltaPossible)) << "," << format("{:.{}g}", output.bestDeltaFound, getPrecisionFloatFormat(output.bestDeltaFound)) << "]" << endl;
    }

    os << "multiplicative approximation fixed: " << output.multiplicativeApproxFixed << endl;

    os << "anti cycling measures taken: ";
    if (output.antiCyclingMeasuresTaken)
        os << "true" << endl;
    else
        os << "false" << endl;
    os << "----------------------------------------" << endl;
    return os;
}