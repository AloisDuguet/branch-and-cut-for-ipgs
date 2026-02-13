//
// Created by Aloïs Duguet on 10/22/24.
//

#ifndef NODE_H
#define NODE_H

#include <vector>
#include <memory>

#include "SolveOutput.h"
#include "OptimizationProblem.h"
#include "CommonlyUsedFunctions.h"
#include "Constraint.h"

class Node {
public:
    // constructor
    Node();

    virtual ~Node();

    // constructor specific to making child nodes while branching
    Node(std::shared_ptr<Node> const& ptrnode,
         Constraint const& constraint);

    // set m_nodeNumber to nodeNumber
    void setNodeNumber(int nodeNumber);

    [[nodiscard]] int getNodeNumber() const;

    // print name of the node
    void printNodeNumber(bool printEndl = false) const;

    // return shared_ptr<Node> of rootNode
    [[nodiscard]] std::shared_ptr<Node> getRootNode() const;

    // shared pointer to the father node
    [[nodiscard]] std::shared_ptr<Node> getFatherNode() const;

    // set m_fatherNode to fatherNode
    void setFatherNode(std::shared_ptr<Node> const& fatherNode);

    // return m_branch
    [[nodiscard]] Constraint getBranch() const;

    // set m_branch
    void setBranch(Constraint const& constraint);

    [[nodiscard]] std::shared_ptr<OptimizationProblem> getSubproblem() const;

    void setSubproblem(std::shared_ptr<OptimizationProblem> const& subproblem);

    // compute the difference in subproblem between this node and its father.
    // Maybe a list of constraints is more relevant
    [[nodiscard]] std::vector<Constraint> getModelDiff() const;

    // build m_model by going through all father nodes
    // to collect root node model and constraints
    void buildModel();

    // add the last cut added to current node to the model,
    // so that using buildModel() is not necessary
    // after a valid cut has been added
    void addCutToModel() const;

    // solve m_model and populates m_isSolved
    // with true for solution found, else false;
    // also populates m_solution if found
    virtual std::shared_ptr<GRBModel> solveModel(
        std::shared_ptr<GRBEnv> const& globalEnv,
        double solverTimeLimit,
        int verbosity,
        bool activateNumericFocus,
        double feasibilityTol);

    virtual std::shared_ptr<GRBModel> checkPointFeasibility(
        std::vector<double> const& point,
        std::shared_ptr<GRBEnv> const& globalEnv,
        double solverTimeLimit,
        int verbosity,
        double feasibilityTol);

    // return bool m_isSolved
    [[nodiscard]] bool isSolved() const;

    // set attribute m_isSolved to false
    void setUnsolved();

    // print m_solution
    void printSolution() const;

    // return m_solution
    [[nodiscard]] SolveOutput getSolution() const;

    // return bool for the integrality constraints satisfaction
    [[nodiscard]] bool isIntegerSolution(double const &tolerance) const;

    // virtual function to build a child node from the root node
    virtual Node* buildChildOfRootNode();

    // virtual function to build a child node from
    // the father node and the additional branching constraint
    virtual Node* buildChildWithBranch(Constraint *constraint);

    // return m_cuts
    [[nodiscard]] std::vector<Cut> getCuts() const;

    // used to add a cut to the rootNodeProblem so that it acts as a global cut
    void addCutToRootNode(Cut const& cut);

    // do two things: add cut to m_cuts (for child nodes)
    // and add cut to subproblem (for new iteration with this node)
    void addCut(Cut const& cut,
                bool globalCutSwitch = false);

    // forget about the model to save memory
    void resetSubproblem(std::shared_ptr<OptimizationProblem> const& rootNodeProblem);

    // return constraints in root node
    [[nodiscard]] Constraint getConstraintFromRootNode(indexx const& index) const;

    // return a shared_ptr containing a copy of *this.
    // The shared_ptr attributes are not deep copied
    // because the pointers are copied.
    virtual std::shared_ptr<Node> clone();

    // increase rhs of cuts by amount rhsIncrease
    void updateRhsCuts(double rhsIncrease);

    // increase rhs of the last numberOfCuts cuts of root node by rhsIncrease
    void updateRootNodeRhsCuts(double rhsIncrease, int numberOfCuts) const;

    // remove nonlinear cuts
    void removeNonlinearCuts();

    // remove nonlinear cuts in the root node as constraints and not cuts
    int removeRootNodeNonlinearCuts(int numberOfCuts) const;

    // return the number of cuts in this node and its parents
    int getNumberOfCutsInNodeProblem() const;

    // return depth, starting from 1
    int getDepth() const;

    // check that point is feasible for the branching constraints of this node
    bool checkPointSatisfyBranchingConstraints(std::vector<double> const& point);

    // print all constraints of the node model together with the evaluation of the lhs by the node solution
    void printConstraintsSatisfaction() const;

protected:
    // attribute that should be kept even after treating the node:

    // given at the exploration, starting with node 0
    int m_nodeNumber;

    // shared pointer to the father node
    std::shared_ptr<Node> m_fatherNode;

    // contains the last branching decision or all branching decisions depending on the design chosen
    Constraint m_branch;

    // contains the cuts added to this node
    std::vector<Cut> m_cuts;

    // attributes that should be discarded after treating the node:

    // reference to an OptimizationProblem corresponding to this node
    std::shared_ptr<OptimizationProblem> m_subproblem;

    // true if m_model has been solved, else false
    bool m_isSolved;

    // solution of the node problem
    SolveOutput m_solution;
};

#endif //NODE_H
