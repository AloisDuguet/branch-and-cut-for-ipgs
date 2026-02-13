//
// Created by Aloïs Duguet on 12/3/24.
//

#ifndef BRANCHINGRULERANDOM_H
#define BRANCHINGRULERANDOM_H

#include "BranchingRule.h"
#include "CommonlyUsedFunctions.h"

class BranchingRuleRandom : public BranchingRule {
public:
    BranchingRuleRandom();

    // select an index for branching at random among nonsatisfied integralities
    indexx getBranchingVariable(std::shared_ptr<Node> const& node) override;
};



#endif //BRANCHINGRULERANDOM_H
