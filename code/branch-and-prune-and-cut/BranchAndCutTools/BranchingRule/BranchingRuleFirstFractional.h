//
// Created by Aloïs Duguet on 10/28/24.
//

#ifndef BRANCHINGRULEFIRSTFRACTIONAL_H
#define BRANCHINGRULEFIRSTFRACTIONAL_H

#include "BranchingRule.h"
#include "CommonlyUsedFunctions.h"

class BranchingRuleFirstFractional : public BranchingRule {
public:
    // constructor
    BranchingRuleFirstFractional();

    // return the first fractional variable of
    // node->getSolution() that should be integer
    indexx getBranchingVariable(std::shared_ptr<Node> const& node) override;

};



#endif //BRANCHINGRULEFIRSTFRACTIONAL_H
