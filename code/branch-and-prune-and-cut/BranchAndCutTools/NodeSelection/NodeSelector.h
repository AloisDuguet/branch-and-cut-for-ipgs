//
// Created by Aloïs Duguet on 10/22/24.
//

#ifndef NODESELECTOR_H
#define NODESELECTOR_H

#include <vector>
#include <string>
#include <memory>

#include "Node.h"
#include "BranchingRule.h"
#include "NEPAbstractSolver.h"

class NodeSelector {
public:
    // constructor
    NodeSelector();

    // constructor with copy of solutionList
    explicit NodeSelector(std::vector<NEInfo> const& solutionList);

    virtual ~NodeSelector();

    // return a new instance with solutionList copied
    [[nodiscard]] virtual NodeSelector* createNewWithSolutionList() const;

    // return the size of the nodeList
    [[nodiscard]] int nodeListSize() const;

    // add a node to nodeList
    void incrementNodeList(std::shared_ptr<Node> const& node);

    // remove last node for specific reasons
    void removeLastNode();

    // return last node
    [[nodiscard]] std::shared_ptr<Node> getlastNode() const;

    // return the next Node of nodeList to explore and removes it from m_nodeList
    virtual std::shared_ptr<Node> nextNodeToExplore();

    // print rule to explain how the next node to be explored is chosen
    void printRule() const;

    // return true if nodeList is empty
    [[nodiscard]] bool isEmpty() const;

    // return the number of explored nodes
    [[nodiscard]] int getExploredNodeNumber() const;

    // increment the number of explored nodes
    void incrementExploredNodeNumber();

    // build a child of rootNode with no additional constraints.
    // useful so that the root node used has a father node that is not the null pointer
    std::shared_ptr<Node> buildChildOfRootNode(std::shared_ptr<Node> const& rootNode);

    // add 2 children nodes to nodeSelector.nodeList by branching with branchingRule
    void buildChildNodes(std::shared_ptr<BranchingRule> const& branchingRule,
                         std::shared_ptr<Node> const& node);

    // add a solution to the solution list
    void incrementSolutionList(std::shared_ptr<NEPAbstractSolver> const& NEP,
                               std::vector<double> solution,
                               double const& socialWelfare,
                               std::vector<double> const& differences,
                               double tolerance);

    // return the list of solutions found
    [[nodiscard]] std::vector<NEInfo> getSolutionList() const;

    // print all solutions in m_solutionList
    void printSolutionList(std::ostream& os = std::cout) const;

    // deep copy nodeList
    std::vector<std::shared_ptr<Node>> deepCopyNodelist();

    // make a deep copy of the instance, ie a deep copy of its attributes
    virtual std::unique_ptr<NodeSelector> clone();

    // get all nodes that are unexplored and parents of unexplored nodes
    std::vector<std::shared_ptr<Node>> getAllTreeNodes();

    // return the root node of the last node of m_nodeList
    std::shared_ptr<Node> getRootNodeOfLastNode() const;

    // reset exploredNodeNumber. To use when solving a new instance
    void resetExploredNodeNumber();

protected:
    // rule to select next node to explore
    std::string m_rule;

    // vector containing a pointer to all open nodes
    std::vector<std::shared_ptr<Node>> m_nodeList;

    // list of solutions found
    std::vector<NEInfo> m_solutionList;

    // number of explored nodes
    static int m_exploredNodeNumber;
};



#endif //NODESELECTOR_H
