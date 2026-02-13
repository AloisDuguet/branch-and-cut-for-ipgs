//
// Created by Aloïs Duguet on 2/20/25.
//

#ifndef GAMEPARSERANDBUILDER_H
#define GAMEPARSERANDBUILDER_H

#include "TestInstances.h"
#include "BranchAndCutTools.h"

// NEP and GNEP knapsack instances

// parse NEP/GNEP knapsack game instance from Dragotto23
NEPKnapsackInstance parseNEPKnapsack(std::string const& filename);

// build a NEP with knapsack instance with infos from file filename
std::shared_ptr<NashEquilibriumProblem> createNEPKnapsackInstance(
    std::string const& filename,
    std::string const& optionIntegrality = "fullInteger");

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

// compute all Lipschitz constants of pi_i for the implementation game
std::vector<double> computeAllLipschitzConstantImplementationGame(
        std::string const& filename,
        std::vector<std::shared_ptr<OptimizationProblem>> const& bestResponseProblems);

// compute local Lipschitz constant of pi_i wrt x for the implementation game
double computeLocalLipschitzConstantImplementationGame(
        indexx player,
        GNEPImplementationGameInstance const& gameInstance,
        std::vector<std::shared_ptr<OptimizationProblem>> const& bestResponseProblems,
        std::vector<double> const& point,
        double numerator);

// compute Lipschitz constant of pi_i wrt x for the implementation game
double computeLipschitzConstantImplementationGame(
        indexx player,
        std::shared_ptr<OptimizationProblem> const& playerProblem,
        int numberOfEdges,
        int numberOfRealPlayers,
        std::vector<std::vector<double>> const& U,
        std::vector<std::vector<double>> const& capacities,
        std::vector<double> const& pMax,
        std::vector<double> const& u,
        std::string const& mode = "global",
        std::vector<double> const& point = {},
        double ballSize = 0);

// create implementation game from instance file
std::shared_ptr<NashEquilibriumProblem> createGNEPImplementationGameInstance(
    std::string const& filename);

// create implementation game without capacity constraints (NEP and not GNEP)
std::shared_ptr<NashEquilibriumProblem> createNEPImplementationGameInstance(
    std::string const& filename);

// create max-flow min-load-cost game from implementation game instance file
std::shared_ptr<NashEquilibriumProblem> createMaxFlowMinLoadCostInstance(
    std::string const& filename);

#endif //GAMEPARSERANDBUILDER_H
