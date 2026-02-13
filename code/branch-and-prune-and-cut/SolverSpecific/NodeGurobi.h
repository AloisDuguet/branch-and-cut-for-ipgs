//
// Created by Aloïs Duguet on 10/28/24.
//

#ifndef NODEGUROBI_H
#define NODEGUROBI_H

#include "BranchAndCutTools.h"

// child class of Node solving subproblem with Gurobi
class NodeGurobi : public Node {
public:
    NodeGurobi();

    // constructor from a father node
    NodeGurobi(std::shared_ptr<NodeGurobi> ptrnode, Constraint *constraint);

    // constructor
    explicit NodeGurobi(std::shared_ptr<NodeGurobi> ptrnode);

    // specific construction of a NodeGurobi child of a rootNode (m_subproblem defined and fatherNode == nullptr)
    NodeGurobi* buildChildOfRootNode() override;

    // call NodeGurobi(ptrnode, constraint) to make a copy with an additional constraint in m_branch
    NodeGurobi* buildChildWithBranch(Constraint *constraint) override;

    // return a shared_ptr containing a copy of *this.
    // The shared_ptr attributes are not deep copied
    // because the pointers are copied.
    std::shared_ptr<Node> clone() override;

    // print IIS of model
    void printIIS(std::shared_ptr<GRBModel> const& model, std::vector<double> const& point);

    // uses Gurobi to solve subproblem
    std::shared_ptr<GRBModel> solveModel(std::shared_ptr<GRBEnv> const& globalEnv,
                                         double solverTimeLimit,
                                         int verbosity,
                                         bool activateNumericFocus,
                                         double feasibilityTol = 1e-6) override;

    // uses Gurobi to determine feasibility of point for subproblem
    std::shared_ptr<GRBModel> checkPointFeasibility(std::vector<double> const& point, std::shared_ptr<GRBEnv> const& globalEnv, double solverTimeLimit, int verbosity, double feasibilityTol = 1e-6) override;
};

void printSolutionQuality(std::shared_ptr<GRBModel> const& model);


#endif //NODEGUROBI_H
