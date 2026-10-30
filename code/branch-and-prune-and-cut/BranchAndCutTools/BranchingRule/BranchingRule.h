//
// Created by Aloïs Duguet on 10/22/24.
//

#ifndef BRANCHINGRULE_H
#define BRANCHINGRULE_H

#include <string>

#include "Node.h"
#include "CommonlyUsedFunctions.h"
#include "Constraint.h"

// abstract class
class BranchingRule {
public:
    // constructor
    BranchingRule();

    virtual ~BranchingRule();

    // print rule attribute
    void printRule() const;

    // produce index of variable to branch on
    // All OptimizationProblem infomation should be inside node.
    [[nodiscard]] virtual indexx getBranchingVariable(
        std::shared_ptr<Node> const& node,
        double integerFeasibilityTolerance) const;

    // produce the index of integer variable with the biggest range
    static VariableBound getBranchingVariableAmongIntegerValues(std::shared_ptr<Node> const& node);

    virtual std::vector<Constraint> buildBranchingConstraints(
        std::shared_ptr<Node> const& node,
        double integerFeasibilityTolerance,
        bool integerBranchingActive = false);

protected:
    // message explaining the branching rule
    std::string m_rule;
};

// return the variable with the biggest bound range
VariableBound getBiggestRangeVariable(std::vector<VariableBound> const& bounds);

#endif //BRANCHINGRULE_H
