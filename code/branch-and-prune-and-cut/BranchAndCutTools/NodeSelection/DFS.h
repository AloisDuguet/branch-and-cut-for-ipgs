//
// Created by Aloïs Duguet on 10/28/24.
//

#ifndef DFS_H
#define DFS_H

#include "NodeSelector.h"

class DFS : public NodeSelector {
public:
    DFS();

    // constructor with copy of solutionList
    explicit DFS(std::vector<NEInfo> const& solutionList);

    // return a new instance with solutionList copied
    [[nodiscard]] DFS* createNewWithSolutionList() const override;

    std::shared_ptr<Node> nextNodeToExplore() override;

    // make a deep copy of the instance, ie a deep copy of its attributes
    std::unique_ptr<NodeSelector> clone() override;
};



#endif //DFS_H
