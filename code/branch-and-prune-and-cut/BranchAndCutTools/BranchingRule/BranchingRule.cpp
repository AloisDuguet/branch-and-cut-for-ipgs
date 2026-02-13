//
// Created by Aloïs Duguet on 10/22/24.
//

#include "BranchingRule.h"

#include <iostream>
#include <ostream>
#include <string>
#include <cmath>

#include "Node.h"

using namespace std;

BranchingRule::BranchingRule() {
    m_rule = "Abstract branching rule";
}

BranchingRule::~BranchingRule() = default;

void BranchingRule::printRule() const {
    cout << m_rule << endl;
}

indexx BranchingRule::getBranchingVariable(shared_ptr<Node> const& node) {
    cout << "Abstract function getBranchingVariable" << endl;
    return -1;
}

vector<Constraint> BranchingRule::buildBranchingConstraints(
        shared_ptr<Node> const& node) {
    indexx indexToBranch = getBranchingVariable(node);
    vector<double> solution = node->getSolution().solution;
    vector<Constraint> branchingConstraints;
    branchingConstraints.reserve(2);
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

    return branchingConstraints;
}
