//
// Created by Aloïs Duguet on 2/20/25.
//

#ifndef GAMEPARSERANDBUILDER_H
#define GAMEPARSERANDBUILDER_H

#include "TestInstances.h"
#include "BranchAndCutTools.h"

struct randomPlayerProblem {
    indexx playerIndex;
    indexx firstVariable;
    int numberOfVariables;
    int numberOfConstraints;
    MySparseVector objective;
    std::vector<Constraint> constraints;
};

struct randomGNEP {
    indexx numberOfPlayers;
    indexx numberOfConstraints;
    indexx numberOfVariables;
    std::vector<randomPlayerProblem> problems;
};

// NEP and GNEP knapsack instances

// parse NEP/GNEP knapsack game instance from Dragotto23
NEPKnapsackInstance parseNEPKnapsack(std::string const& filename);

// build a NEP with knapsack instance with infos from file filename
std::shared_ptr<NashEquilibriumProblem> createNEPKnapsackInstance(
    std::string const& filename,
    std::string const& optionIntegrality = "fullInteger");
// build a NEP with a binary knapsack instance where the quadratic terms
// between binary variables are reformulated into linear constraints
std::shared_ptr<NashEquilibriumProblem> createNEPKnapsackInstanceWithQuadraticTermsReformulated(
    std::string const& filename);

// build a GNEP with knapsack instance with infos from file filename
std::shared_ptr<NashEquilibriumProblem> createGNEPKnapsackInstance(
    std::string const& filename,
    std::string const& optionIntegrality = "fullInteger");

// parse implementation game from instance file
GNEPImplementationGameInstance parseImplementationGameGNEP(
    std::string const& filename);

// parse instance of discrete NEP in article Schwarze and Stein, 2023
// Github repo here: https://github.com/schwarze-st/nep_pruning/
Schwarze23Instance parseSchwarze23DiscreteNEPInstance(
    std::string const& filename);

// create NEP instance from discrete NEP in article Schwarze and Stein, 2023
std::shared_ptr<NashEquilibriumProblem> createSchwarze23DiscreteNEPInstance(
    std::string const& filename);

// create implementation game from instance file
std::shared_ptr<NashEquilibriumProblem> createGNEPImplementationGameInstance(
    std::string const& filename);

// create implementation game without capacity constraints (NEP and not GNEP)
std::shared_ptr<NashEquilibriumProblem> createNEPImplementationGameInstance(
    std::string const& filename);

// create max-flow min-load-cost game from implementation game instance file
std::shared_ptr<NashEquilibriumProblem> createMaxFlowMinLoadCostInstance(
    std::string const& filename);

// create implementation game from instance file
std::shared_ptr<NashEquilibriumProblem> createGNEPQuadraticKnapsackInstance(
    std::string const& filename);

// add a fake constant term to each player, for example to ensure nonnegativity
std::shared_ptr<NashEquilibriumProblem> addFakeConstantTerm(
    std::shared_ptr<NashEquilibriumProblem> GNEP,
    std::vector<double> const& values);

// parse and build random GNEP instance
randomGNEP parseRandomGNEPInstance(std::string const& filename);
std::shared_ptr<NashEquilibriumProblem> createGNEPRandomInstance(
        std::string const& filename);
// objective is concave quadratic in x_i
std::shared_ptr<NashEquilibriumProblem> createGNEPRandomQuadraticInstance(
        std::string const& filename);

#endif //GAMEPARSERANDBUILDER_H
