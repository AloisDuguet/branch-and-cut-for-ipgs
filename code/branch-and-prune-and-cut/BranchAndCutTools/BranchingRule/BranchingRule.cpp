//
// Created by Aloïs Duguet on 10/22/24.
//

#include "BranchingRule.h"

#include <iostream>
#include <ostream>
#include <string>
#include <cmath>
#include <algorithm>

#include "Node.h"

using namespace std;

BranchingRule::BranchingRule() {
    m_rule = "Abstract branching rule";
}

BranchingRule::~BranchingRule() = default;

void BranchingRule::printRule() const {
    cout << m_rule << endl;
}

indexx BranchingRule::getBranchingVariable(
        shared_ptr<Node> const& node,
        double integerFeasibilityTolerance) const {
    cout << "Abstract function getBranchingVariable" << endl;
    return -1;
}

VariableBound BranchingRule::getBranchingVariableAmongIntegerValues(shared_ptr<Node> const& node) {
    // launched when solution is integer but branching is still interesting
    vector<indexx> integralityConstraints =
        node->getSubproblem()->getIntegralityConstraints();

    // find lower and upper bounds of integral variables and keep those not fixed
    node->initializeVariableBounds();
    vector<VariableBound> variableBounds;
    for (const auto [variableIndex, bounds] : node->getVariableBounds()) {
        if (bounds.lb != bounds.ub)
            variableBounds.push_back(VariableBound(variableIndex, bounds));
    }

    if (variableBounds.empty())
        throw logic_error("all integer variables are fixed: the node should have been discarded earlier");

    // select one deterministically: the one with the biggest range
    return getBiggestRangeVariable(variableBounds);
}

vector<Constraint> BranchingRule::buildBranchingConstraints(
        shared_ptr<Node> const& node,
        double integerFeasibilityTolerance,
        bool integerBranchingActive) {
    vector<double> solution = node->getSolution().solution;
    vector<Constraint> branchingConstraints;
    branchingConstraints.reserve(2);

    indexx indexToBranch = -1;
    try {
        indexToBranch = getBranchingVariable(node, integerFeasibilityTolerance);
        if (solution[indexToBranch] - floor(solution[indexToBranch]) <= 0.5) {
            // branch on floor first to hopefully find faster a feasible solution
            branchingConstraints.push_back(
                {{{-1.0}, {indexToBranch}},
                    {}, -ceil(solution[indexToBranch])});
            branchingConstraints.push_back(
                {{{1.0}, {indexToBranch}},
                    {}, floor(solution[indexToBranch])});
        } else {
            branchingConstraints.push_back(
                {{{1.0}, {indexToBranch}},
                    {}, floor(solution[indexToBranch])});
            branchingConstraints.push_back(
                {{{-1.0}, {indexToBranch}},
                    {}, -ceil(solution[indexToBranch])});
        }
    }
    catch (logic_error const&  e) {
        string s = e.what();
        if (integerBranchingActive and s == "There is no remaining integer variable that was not branched on") {
            cout << "integer branching in Node " << node->getNodeNumber() << endl;
            auto [indexToBranch, bounds] = getBranchingVariableAmongIntegerValues(node);
            // fill in greaterThanBound and lessThanBound that are used in the constraints:
            // -x <= greaterThanBound and
            // x <= lessThanBound
            double greaterThanLowerBound;
            if (solution[indexToBranch] - bounds.lb > bounds.ub - solution[indexToBranch]) {
                greaterThanLowerBound = solution[indexToBranch];
            } else {
                greaterThanLowerBound = solution[indexToBranch] + 1;
            }
            double lessThanUpperBound = greaterThanLowerBound - 1;

            if (lessThanUpperBound < bounds.lb or greaterThanLowerBound > bounds.ub) {
                cout << "solution[indexToBranch]: " << solution[indexToBranch] << endl;
                cout << "lessThan: " << lessThanUpperBound << "\tgreaterThan: " << greaterThanLowerBound << endl;
                cout << "bounds: [" << bounds.lb << ", " << bounds.ub << "]" << endl;
                throw logic_error("bounds for integer branching constraints inconsistent with bounds");
            }

            branchingConstraints.push_back(
                {{{-1.0}, {indexToBranch}},
                {}, -greaterThanLowerBound});
            branchingConstraints.push_back(
            {{{1.0}, {indexToBranch}},
                {}, lessThanUpperBound});
            // cout << "two branching constraints: " << endl;
            // cout << branchingConstraints[0] << endl;
            // cout << branchingConstraints[1] << endl;
        }
        else throw;
    }

    return branchingConstraints;
}

VariableBound getBiggestRangeVariable(
        vector<VariableBound> const &variableBounds) {
    indexx bestIndex = -1;
    auto bestBounds = Bounds(static_cast<int>(-1e100), static_cast<int>(1e100));
    int bestRange = 0;
    for (auto [variableIndex, bounds] : variableBounds) {
        double range = bounds.ub - bounds.lb;
        if (range > bestRange) {
            bestIndex = variableIndex;
            bestRange = static_cast<int>(floor(range));
            bestBounds = bounds;
        }
    }
    return VariableBound(bestIndex, bestBounds);
}