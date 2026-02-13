//
// Created by Aloïs Duguet on 10/22/24.
//

#include "Node.h"

#include <vector>
#include <memory>
#include <cmath>
#include "gurobi_c++.h"

#include "NodeSelector.h"
#include "OptimizationProblem.h"
#include "CommonlyUsedFunctions.h"
#include "Constraint.h"

using namespace std;

Node::Node() {
    // convention for 'unset' is -1, because this number
    // is given when the node is solved,
    // and not created and added to the node list
    m_nodeNumber = -1;

    // a Node must have a nullptr here only if it is the root node
    m_fatherNode = nullptr;

    // Make sure to restrict the use of this constructor to the root node
    // because of the initialization of m_fatherNode
    m_branch = {};

    m_cuts = {};
    m_subproblem = shared_ptr<OptimizationProblem>();
    m_isSolved = false;
    m_solution = {};
}

Node::~Node() = default;

Node::Node(shared_ptr<Node> const& ptrnode,
           Constraint const& constraint) {
    // convention for 'unset' is -1, because this number
    // is given when the node is solved,
    // and not created and added to the node list
    m_nodeNumber = -1;

    m_fatherNode = ptrnode;
    m_branch = constraint;
    m_cuts = {};
    m_subproblem = shared_ptr<OptimizationProblem>();
    m_isSolved = false;
    m_solution = {};
}

void Node::setNodeNumber(int const nodeNumber) {
    m_nodeNumber = nodeNumber;
}

int Node::getNodeNumber() const {
    return m_nodeNumber;
}

void Node::printNodeNumber(bool const printEndl) const {
        cout << m_nodeNumber << " ";
        if (printEndl)
            cout << endl;
}

shared_ptr<Node> Node::getRootNode() const {
    shared_ptr<Node> currentNode = make_shared<Node>(*this);
    // find the root node
    while (currentNode->getFatherNode()) {
        currentNode = currentNode->getFatherNode();
    }
    return currentNode;
}

shared_ptr<Node> Node::getFatherNode() const {
    return m_fatherNode;
}

void Node::setFatherNode(std::shared_ptr<Node> const& fatherNode) {
    m_fatherNode = fatherNode;
}

Constraint Node::getBranch() const {
    return m_branch;
}

void Node::setBranch(Constraint const& constraint) {
    m_branch = constraint;
}


shared_ptr<OptimizationProblem> Node::getSubproblem() const {
    return m_subproblem;
}

void Node::setSubproblem(shared_ptr<OptimizationProblem> const& subproblem) {
    m_subproblem = subproblem;
}

vector<Constraint> Node::getModelDiff() const {
    vector<Constraint> constraints = m_cuts;
    if (m_branch.getAx().size() > 0)
        constraints.push_back(m_branch);
    return constraints;
}

void Node::buildModel() {
    // a lot of work to do such as collect
    // model differences up to the root node
    // 2 steps
    // 1 - collect constraints from m_branch and
    //     m_cuts from child to father
    //     until the root node is reached
    // 2 - extend the constraints of the root node
    //     model with the constraints collected
    //     along the way to the root node

    // 1 - collect constraints
    // (currentNode should end on a pointer to the root node)
    Node* currentNode = this;
    vector<Constraint> constraints = getModelDiff();
    while (currentNode->getFatherNode()) {
        currentNode = currentNode->getFatherNode().get();
        vector<Constraint> newConstraints = currentNode->getModelDiff();
        constraints.insert(constraints.end(),
                           newConstraints.begin(),
                           newConstraints.end());
    }
    // currentNode is now pointing to the root node
    // because currentNode->getFatherNode() == nullptr

    // 2 - extend the constraints of the root node model and copy c and Q
    vector<Constraint> nodeConstraints =
        currentNode->m_subproblem->getConstraints();
    if (!constraints.empty())
        nodeConstraints.insert(nodeConstraints.end(),
                               constraints.begin(),
                               constraints.end());
    m_subproblem->setConstraints(nodeConstraints);

    m_subproblem->setC(currentNode->m_subproblem->getC());
    m_subproblem->setQ(currentNode->m_subproblem->getQ());
    m_subproblem->setIntegralityConstraints(
        currentNode->m_subproblem->getIntegralityConstraints());
}

void Node::addCutToModel() const {
    m_subproblem->AddConstraint(m_cuts[m_cuts.size()-1]);
}


shared_ptr<GRBModel> Node::solveModel(shared_ptr<GRBEnv> const& globalEnv,
                                      double solverTimeLimit,
                                      int verbosity,
                                      bool activateNumericFocus,
                                      double feasibilityTol) {
    cout << "entering Node::solveModel but it does nothing" << endl;
    return {};
}

std::shared_ptr<GRBModel> Node::checkPointFeasibility(
        std::vector<double> const &point,
        std::shared_ptr<GRBEnv> const &globalEnv,
        double solverTimeLimit,
        int verbosity,
        double feasibilityTol) {
    cout << "entering Node::checkFeasiblePoint but it does nothing" << endl;
    return {};
}


bool Node::isSolved() const {
    return m_isSolved;
}

void Node::setUnsolved() {
    m_isSolved = false;
}


void Node::printSolution() const {
    cout << m_solution.solution << endl;
}

SolveOutput Node::getSolution() const {
    return m_solution;
}

bool Node::isIntegerSolution(double const &tolerance) const {
    vector<indexx> integralityConstraints = m_subproblem->getIntegralityConstraints();
    for (indexx const ind : integralityConstraints) {
        double val = m_solution.solution[ind];
        if (!(val <= floor(val) + tolerance or ceil(val) <= val + tolerance)) {
            return false;
        }
    }
    // all integrality constraints are satisfied
    return true;
}

Node *Node::buildChildOfRootNode() {
    cout << "entering Node::buildChildOfRootNode that is a virtual function"
         << endl;
    return nullptr;
}


Node *Node::buildChildWithBranch(Constraint *constraint) {
    cout << "entering Node::buildChildWithBranch that is a virtual function"
         << endl;
    return nullptr;
}

vector<Cut> Node::getCuts() const {
    return m_cuts;
}

void Node::addCutToRootNode(Cut const& cut) {
    getRootNode()->getSubproblem()->AddConstraint(cut);
}

void Node::addCut(Cut const& cut, bool globalCutSwitch) {
    // adds a cut to the current problem,
    // ie the problem that will be solved next iteration
    m_subproblem->AddConstraint(cut);
    if (!globalCutSwitch) {
        // adds a cut to the list of cuts of this node
        m_cuts.push_back(cut);
    }
    else {
        // adds a cut to the root node problem
        addCutToRootNode(cut);
    }
}

void Node::resetSubproblem(shared_ptr<OptimizationProblem> const& rootNodeProblem) {
    // it must stay different from nullptr
    // because finding the rootNode is based on that
    m_subproblem = make_shared<OptimizationProblem>(
        rootNodeProblem->getNumberOfVariables());
}

Constraint Node::getConstraintFromRootNode(indexx const& index) const {
    auto rootNode = getRootNode();
    return rootNode->m_subproblem->getConstraints()[index];
}

shared_ptr<Node> Node::clone() {
    auto node = make_shared<Node>(*this);
    node->setSubproblem(m_subproblem->clone());
    return node;
}

void Node::updateRhsCuts(double const rhsIncrease) {
    for (auto& cut : m_cuts)
        cut.increaseB(rhsIncrease);
}

void Node::updateRootNodeRhsCuts(double const rhsIncrease,
                                 int const numberOfCuts) const {
    // find root node
    shared_ptr<Node> currentNode = getRootNode();
    // update last numberOfCuts cuts
    currentNode->getSubproblem()->updateRhsCuts(rhsIncrease, numberOfCuts);
}

void Node::removeNonlinearCuts() {
    // // slower code because of multiple erase
    // indexx numCut = 0;
    // while (numCut < m_cuts.size()) {
    //     if (m_cuts[numCut].getXQx().size() >= 1) {
    //         m_cuts.erase(m_cuts.begin() + numCut);
    //     } else
    //         numCut++;
    // }

    // avoid multiple erase calls that are costly
    if (m_cuts.size() > 0) {
        vector<Cut> newCuts = {};
        newCuts.reserve(m_cuts.size());
        for (auto const& cut : m_cuts) {
            if (cut.getXQx().size() == 0) {
                newCuts.push_back(cut);
            }
        }
        m_cuts = move(newCuts);
    }
}

int Node::removeRootNodeNonlinearCuts(int numberOfCuts) const {
    // find root node
    shared_ptr<Node> currentNode = getRootNode();
    // remove nonlinear cuts that are constraints of the root node
    return currentNode->getSubproblem()->removeNonlinearCutsAsConstraints(numberOfCuts);
}

int Node::getNumberOfCutsInNodeProblem() const {
    int count = static_cast<int>(m_cuts.size());
    shared_ptr<Node> currentNode = make_shared<Node>(*this);
    // find the root node
    while (currentNode->getFatherNode()) {
        currentNode = currentNode->getFatherNode();
        count += static_cast<int>(currentNode->m_cuts.size());
    }
    return count;
}

int Node::getDepth() const {
    int count = 0;
    shared_ptr<Node> currentNode = make_shared<Node>(*this);
    // find the root node
    while (currentNode->getFatherNode()) {
        currentNode = currentNode->getFatherNode();
        count += 1;
    }
    return count;
}

bool Node::checkPointSatisfyBranchingConstraints(vector<double> const &point) {
    // 1 - collect all branching constraints
    shared_ptr<Node> currentNode = make_shared<Node>(*this);
    vector<Constraint> branchingConstraints;
    if (currentNode->m_branch.getAx().size() > 0)
        branchingConstraints.push_back(currentNode->m_branch);
    // find the root node
    while (currentNode->getFatherNode()) {
        currentNode = currentNode->getFatherNode();
        if (currentNode->m_branch.getAx().size() > 0)
            branchingConstraints.push_back(currentNode->m_branch);
    }
    // 2 - check if point satisfies them
    for (auto const& branch : branchingConstraints) {
        cout << branch << endl;
        if(branch.checkConstraintWithPoint(point,0)) {
            // for printing information:
            // bool temp = branch.checkConstraintWithPoint(point,3);
            cout << "point not feasible for the local node: " << branch << endl;
            return false;
        }
    }
    cout << "point feasible for the local node" << endl;
    return true;
}

void Node::printConstraintsSatisfaction() const {
    // check that the node was solved
    if (m_solution.status == 2) {
        m_subproblem->printConstraintsAndLhsEval(m_solution.solution);
    }
}
