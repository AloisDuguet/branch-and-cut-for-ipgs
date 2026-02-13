//
// Created by Aloïs Duguet on 12/3/24.
//

#include "BranchingRuleMostFractional.h"

#include <cmath>

using namespace std;

BranchingRuleMostFractional::BranchingRuleMostFractional() {
    m_rule = "branch on most fractional variable";
}

indexx BranchingRuleMostFractional::getBranchingVariable(
        shared_ptr<Node> const& node) {
    vector<double> solution = node->getSolution().solution;
    vector<indexx> integralityConstraints =
        node->getSubproblem()->getIntegralityConstraints();
    indexx mostFractionalIndex = -1;
    double closestToHalf = 1.0;
    for (indexx index : integralityConstraints) {
        double fractionalValue = solution[index] - floor(solution[index]);
        double distToHalf = abs(fractionalValue - 0.5);
        if (!compareApproxEqual(distToHalf, 0.5)) {
            // do not take into account any integral value
            if (distToHalf < closestToHalf) {
                mostFractionalIndex = index;
                closestToHalf = distToHalf;
            }
        }
    }
    if (mostFractionalIndex != -1)
        return mostFractionalIndex;
    throw logic_error("There is no remaining integer variable that was not "
                      "branched on in "
                      "BranchingRuleFirstFractional::getBranchingVariable");
}
