//
// Created by Aloïs Duguet on 10/30/24.
//

#ifndef NASHEQUILIBRIUMPROBLEM_H
#define NASHEQUILIBRIUMPROBLEM_H

#include <vector>
#include <memory>
#include <iostream>

#include "OptimizationProblem.h"
#include "Constraint.h"

class NashEquilibriumProblem {
public:
    NashEquilibriumProblem();

    explicit NashEquilibriumProblem(
        std::vector<std::shared_ptr<OptimizationProblem>> const& bestResponseProblems);

    // return true if instance is a GNEP, false if not
    [[nodiscard]] bool detectGNEP() const;

    virtual void print(std::ostream& os = std::cout) const;

    [[nodiscard]] int getNumberOfPlayer() const;

    // number of variables of player player
    [[nodiscard]] int getNumberOfVariablesOfPlayer(indexx player) const;

    // vector of number of variables per player
    [[nodiscard]] std::vector<int> getNumberOfVariablesPerPlayer() const;

    // sum of number of variables for all player
    [[nodiscard]] int getTotalNumberOfVariablesOfPlayers() const;

    // starting index of variables of player player
    [[nodiscard]] indexx getStartingPlayerVariableIndex(indexx player) const;

    [[nodiscard]] std::vector<std::shared_ptr<OptimizationProblem>> getBestResponseProblems() const;

    [[nodiscard]] bool getIsGNEP() const;

protected:
    int m_numberOfPlayers;

    // vector with the list of players' optimization problem
    std::vector<std::shared_ptr<OptimizationProblem>> m_bestResponseProblems;

    bool m_isGNEP;
};

std::ostream& operator<<(std::ostream& os, NashEquilibriumProblem const& NEP);

#endif //NASHEQUILIBRIUMPROBLEM_H
