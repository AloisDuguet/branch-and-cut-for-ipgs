//
// Created by Aloïs Duguet on 10/28/24.
//

#include "DFS.h"

#include <memory>
#include <string>
#include <algorithm>

using namespace std;

DFS::DFS() {
    m_rule = "Depth First Search";
}

DFS::DFS(vector<NEInfo> const& solutionList) {
    m_rule = "Depth First Search";
    m_solutionList = solutionList;
}

DFS* DFS::createNewWithSolutionList() const {
    return new DFS(m_solutionList);
}

shared_ptr<Node> DFS::nextNodeToExplore() {
    // make a reference of last element of nodeList
    shared_ptr<Node> lastNode = m_nodeList.back();

    // destroys the old reference to the node
    m_nodeList.pop_back();

    lastNode->setNodeNumber(m_exploredNodeNumber);
    m_exploredNodeNumber++;

    // return the copied reference (shared_ptr) to the node
    return lastNode;
}

unique_ptr<NodeSelector> DFS::clone() {
    auto state = make_unique<DFS>(*this);
    state->m_exploredNodeNumber = m_exploredNodeNumber;
    // deep copy because there is no raw or smart pointer inside NEInfo
    state->m_solutionList = m_solutionList;
    state->m_nodeList = deepCopyNodelist();
    return state;
}
