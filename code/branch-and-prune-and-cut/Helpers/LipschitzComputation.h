//
// Created by alois-duguet on 6/17/26.
//

#ifndef LIPSCHITZCOMPUTATION_H
#define LIPSCHITZCOMPUTATION_H

#include "TestInstances.h"
#include "BranchAndCutTools.h"

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

#endif //LIPSCHITZCOMPUTATION_H
