//
// Created by Aloïs Duguet on 12/3/24.
//

#ifndef BRANCHINGRULEMOSTFRACTIONAL_H
#define BRANCHINGRULEMOSTFRACTIONAL_H

#include "BranchingRule.h"

class BranchingRuleMostFractional : public BranchingRule {
public:
    BranchingRuleMostFractional();

    // return the most fractional variable of node->getSolution()
    // that should be integer
    indexx getBranchingVariable(std::shared_ptr<Node> const& node) override;
};



#endif //BRANCHINGRULEMOSTFRACTIONAL_H
