//
// Created by Aloïs Duguet on 10/28/24.
//

#include "BranchingRuleFirstFractional.h"

#include <string>
#include <cmath>

#include "OptimizationProblem.h"
#include "Node.h"
#include "CommonlyUsedFunctions.h"

using namespace std;

BranchingRuleFirstFractional::BranchingRuleFirstFractional() {
    m_rule = "branch on first fractional variable";
}

indexx BranchingRuleFirstFractional::getBranchingVariable(
        shared_ptr<Node> const& node,
        double integerFeasibilityTolerance) const {
    vector<double> solution = node->getSolution().solution;
    vector<indexx> integralityConstraints =
        node->getSubproblem()->getIntegralityConstraints();
    for (indexx i = 0; i < integralityConstraints.size(); i++) {
        double fractionalValue = solution[i] - floor(solution[i]);
        double distToHalf = abs(fractionalValue - 0.5);
        if (!compareApproxEqual(distToHalf, 0.5, integerFeasibilityTolerance))
            return integralityConstraints[i];
    }
    throw logic_error("There is no remaining integer variable that was "
                      "not branched on");
}
