//
// Created by Aloïs Duguet on 10/22/24.
//

#include "NodeSelector.h"

#include <memory>

#include "CommonlyUsedFunctions.h"
#include "Constraint.h"
#include "NEPAbstractSolver.h"

using namespace std;

int NodeSelector::m_exploredNodeNumber = 0;

NodeSelector::NodeSelector() : m_rule("Abstract class NodeSelector"),
                               m_nodeList({}),
                               m_solutionList({}) {
}

NodeSelector::NodeSelector(vector<NEInfo> const& solutionList) :
        m_rule("Abstract class NodeSelector"),
        m_nodeList({}),
        m_solutionList(solutionList) {
    }

NodeSelector::~NodeSelector() = default;

NodeSelector *NodeSelector::createNewWithSolutionList() const {
    return new NodeSelector(m_solutionList);
}


int NodeSelector::nodeListSize() const {
    return static_cast<int>(m_nodeList.size());
}

void NodeSelector::incrementNodeList(shared_ptr<Node> const& node) {
    m_nodeList.push_back(node);
}

void NodeSelector::removeLastNode() {
    m_nodeList.pop_back();
}

std::shared_ptr<Node> NodeSelector::getlastNode() const {
    return m_nodeList.back();
}

shared_ptr<Node> NodeSelector::nextNodeToExplore() {
    cout << "abstract function nextNodeToExplore" << endl;
    shared_ptr<Node> ptr(nullptr);
    return ptr;
}

void NodeSelector::printRule() const {
    cout << m_rule << endl;
}

bool NodeSelector::isEmpty() const {
    if (m_nodeList.empty()) {
        return true;
    }
    return false;
}

int NodeSelector::getExploredNodeNumber() const {
    return NodeSelector::m_exploredNodeNumber;
}

void NodeSelector::incrementExploredNodeNumber() {
    m_exploredNodeNumber++;
}

shared_ptr<Node> NodeSelector::buildChildOfRootNode(
        shared_ptr<Node> const& rootNode) {
    // auto* newNode = rootNode->buildChildOfRootNode();
    auto newNode = shared_ptr<Node>(rootNode->buildChildOfRootNode());
    newNode->setFatherNode(rootNode);
    newNode->setSubproblem(make_shared<OptimizationProblem>(rootNode->getSubproblem()->getNumberOfVariables()));
    m_nodeList.push_back(newNode);
    return m_nodeList[0];
}

void NodeSelector::buildChildNodes(shared_ptr<BranchingRule> const& branchingRule,
                                   shared_ptr<Node> const& node) {
    vector<Constraint> branchingConstraints =
        branchingRule->buildBranchingConstraints(node);
    // creating first node
    auto* newNode = node->buildChildWithBranch(&branchingConstraints[0]);
    newNode->setFatherNode(node);
    newNode->setSubproblem(make_shared<OptimizationProblem>(node->getSubproblem()->getNumberOfVariables()));
    newNode->setBranch(branchingConstraints[0]);
    m_nodeList.push_back(shared_ptr<Node>(newNode));
    // creating second node
    newNode = node->buildChildWithBranch(&branchingConstraints[1]);
    newNode->setFatherNode(node);
    newNode->setSubproblem(make_shared<OptimizationProblem>(node->getSubproblem()->getNumberOfVariables()));
    newNode->setBranch(branchingConstraints[1]);
    m_nodeList.push_back(shared_ptr<Node>(newNode));
}

void NodeSelector::incrementSolutionList(shared_ptr<NEPAbstractSolver> const& NEP,
                                         vector<double> solution,
                                         double const& socialWelfare,
                                         vector<double> const& differences,
                                         double tolerance) {
    solution = NEP->extractNashEquilibrium(solution, tolerance);
    auto bestResponses = NEP->getBestResponses();
    vector<double> bestResponseValues = {};
    for (SolveOutput const& bestResponse : bestResponses)
        bestResponseValues.push_back(bestResponse.objective);
    m_solutionList.push_back({solution,
                             socialWelfare,
                             bestResponseValues,
                             differences});
}

vector<NEInfo> NodeSelector::getSolutionList() const {
    return m_solutionList;
}

void NodeSelector::printSolutionList(ostream& os) const {
    os << m_solutionList;
}

vector<shared_ptr<Node>> NodeSelector::deepCopyNodelist() {
    vector<shared_ptr<Node>> newNodeList;
    vector<shared_ptr<Node>> hiddenNodes;

    // tricky part of deep copying the whole (useful)
    // tree structure from the unexplored node.
    // strategy: keep in mind all nodes treated in a vector;
    // and from each unexplored node, copy properly the node x
    // with the father node y unchanged, then create a new father node y,
    // and then only change father node for node x to y.
    // when a node is copied, its number is saved in copiedNodes and
    // a pointer to its copied father in fatherOfCopiedNodes
    // it should allow to find father nodes already copied from
    // an old ascent from an unexplored node to the root node
    vector<shared_ptr<Node>> copiedNodes;
    for (auto const& node : m_nodeList) {
        auto currentNode = node;
        vector<shared_ptr<Node>> nodesInAscent = {currentNode};
        // ascent to root node with saving of pointers
        while (currentNode->getFatherNode()) {
            currentNode = currentNode->getFatherNode();
            nodesInAscent.push_back(currentNode);
        }

        // check if currentNode has already been copied
        // if not, copy it into state and go to previous node
        // in nodesInAscent (which is a child of currentNode)
        // if yes, do not copy, set lastCurrentNode to currentNode
        // and go to previous node in nodesInAscent
        shared_ptr<Node> lastCurrentNode = nullptr;
        while (!nodesInAscent.empty()) {
            if (ranges::find(copiedNodes.begin(),
                             copiedNodes.end(),
                             currentNode) == copiedNodes.end()) {
                // currentNode is an unexplored node, so add it either
                // to the queue if it is node, or to the hiddenNodes
                if (currentNode == node) {
                    newNodeList.push_back(currentNode->clone());
                    newNodeList.back()->setFatherNode(lastCurrentNode);
                    lastCurrentNode = newNodeList.back();
                }
                else {
                    hiddenNodes.push_back(currentNode->clone());
                    hiddenNodes.back()->setFatherNode(lastCurrentNode);
                    lastCurrentNode = hiddenNodes.back();
                }
                copiedNodes.push_back(currentNode);
            } else {
                // save lastCurrentNode as the clone of currentNode in hiddenNodes
                // this node is recognizable by its node number which is the same
                // as the one of currentNode.
                // Then there are two cases:
                // 1) if node number == -1, then it has to be the real root node
                // and it can be checked by currentNode->getFatherNode() == nullptr
                // indeed, we are looking in hiddenNodes which contains only clones of explored nodes
                // and the real root node has number -1
                // 2) if node number >= 0, then this number is unique
                // because of the static attribute exploredNodeNumber
                int targetNodeNumber = currentNode->getNodeNumber();
                bool hiddenNodeFound = false;
                for (auto const& hiddenNode : hiddenNodes) {
                    if (hiddenNode->getNodeNumber() == targetNodeNumber) {
                        lastCurrentNode = hiddenNode;
                        hiddenNodeFound = true;
                        break;
                    }
                }
                if (!hiddenNodeFound) {
                    throw logic_error("hidden node clone of current node was not found");
                }
            }
            nodesInAscent.pop_back();
            if (!nodesInAscent.empty()) {
                // careful: when nodesInAscent is empty,
                // this line can cause undefined behaviour.
                // Do not reuse currentNode directly
                currentNode = nodesInAscent.back();
            }
        }
    }
    return newNodeList;
}


unique_ptr<NodeSelector> NodeSelector::clone() {
    auto state = make_unique<NodeSelector>(*this);
    // deep copy because there is no raw or smart pointer inside NEInfo
    state->m_solutionList = m_solutionList;
    state->m_nodeList = deepCopyNodelist();
    return state;
}

vector<shared_ptr<Node>> NodeSelector::getAllTreeNodes() {
    if (m_nodeList.empty())
        return {};
    vector<shared_ptr<Node>> treeNodes = {m_nodeList[0]};
    vector<shared_ptr<Node>> changedNodes = {};
    for (auto const& node : m_nodeList) {
        auto currentNode = node;
        vector<shared_ptr<Node>> nodesInAscent = {currentNode};
        // ascent to root node with saving of pointers
        while (currentNode->getFatherNode()) {
            if (ranges::find(treeNodes.begin(),
                             treeNodes.end(),
                             currentNode) == treeNodes.end())
                treeNodes.push_back(currentNode);
            currentNode = currentNode->getFatherNode();
        }
    }
    return treeNodes;
}

std::shared_ptr<Node> NodeSelector::getRootNodeOfLastNode() const {
    auto lastNode = m_nodeList.back();
    return lastNode->getRootNode();
}

void NodeSelector::resetExploredNodeNumber() {
    m_exploredNodeNumber = 0;
}

ostream& operator<<(ostream& os, NEInfo const& info) {
    info.print(os);
    return os;
}

ostream& operator<<(ostream& os, vector<NEInfo> const& infos) {
    for (auto const& info : infos) {
        info.print(os);
        os << "--" << endl;
    }
    return os;
}
