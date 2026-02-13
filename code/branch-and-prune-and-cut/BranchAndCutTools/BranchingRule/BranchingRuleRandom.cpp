//
// Created by Aloïs Duguet on 12/3/24.
//

#include "BranchingRuleRandom.h"

#include <cmath>

using namespace std;

BranchingRuleRandom::BranchingRuleRandom() {
    m_rule = "branch on random unsatisfied integrality";
}

indexx BranchingRuleRandom::getBranchingVariable(shared_ptr<Node> const& node) {
    vector<double> solution = node->getSolution().solution;
    vector<indexx> integralityConstraints =
        node->getSubproblem()->getIntegralityConstraints();
    vector<indexx> fractionalIndices = {};
    for (indexx i = 0; i < integralityConstraints.size(); i++) {
        double fractionalValue = ceil(solution[integralityConstraints[i]])
                                 - floor(solution[integralityConstraints[i]]);
        if (!isApproxZero(fractionalValue))
            fractionalIndices.push_back(integralityConstraints[i]);
    }
    if (!fractionalIndices.empty()) {
        indexx r =
            randomUniformIntRange(0,static_cast<indexx>(fractionalIndices.size())-1);
        return fractionalIndices[r];
    }
    throw logic_error("There is no remaining integer variable that was not branched "
                      "on in BranchingRuleFirstFractional::getBranchingVariable");
}
