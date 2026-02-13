//
// Created by Aloïs Duguet on 11/7/24.
//

#ifndef TESTINSTANCES_H
#define TESTINSTANCES_H

#include "BranchAndCutTools.h"

struct NEPKnapsackInstance {
    int numberOfPlayers;
    int numberOfItems;
    std::vector<int> capacities;
    std::vector<std::vector<int>> profits;
    std::vector<std::vector<int>> weights;
    std::vector<std::vector<std::vector<int>>> interactionCoefficients;
    std::vector<int> itemsAvailable;
};

struct GNEPImplementationGameInstance {
    int numberOfPlayers;
    int numberOfNodes;
    int numberOfEdges;
    MySparseMatrix network;
    std::vector<std::vector<double>> capacities;
    std::vector<std::vector<double>> sourceSinks;
    std::vector<std::vector<double>> U;
    std::vector<double> demands;
    std::vector<double> pMax;
    std::vector<double> u;
};

struct Schwarze23Instance {
    int numberOfPlayers;
    int numberOfVariablesPerPlayer;
    std::vector<std::vector<bool>> costsConvexity;
    std::vector<std::vector<std::vector<double>>> constraintMatrices;
    std::vector<std::vector<double>> constraintB;
    std::vector<std::vector<std::vector<double>>> bounds;
    std::vector<std::vector<std::vector<double>>> objectiveC;
    std::vector<std::vector<std::vector<double>>> objectiveQ;
    std::vector<std::vector<double>> objectiveB;
};

// test for simpleBranching
OptimizationProblem testEasyMIQP(); // checked
OptimizationProblem testMILP(); // checked

// test for branchAndPruneAndCutForNEP
std::shared_ptr<NashEquilibriumProblem> testNEP1(); // checked
std::shared_ptr<NashEquilibriumProblem> testNEP2(); // checked
std::shared_ptr<NashEquilibriumProblem> testNEP3();  // checked
std::shared_ptr<NashEquilibriumProblem> testNEP4(); // checked, cuts derived
std::shared_ptr<NashEquilibriumProblem> testNEP5(); // checked
std::shared_ptr<NashEquilibriumProblem> testNEP6(); // checked
std::shared_ptr<NashEquilibriumProblem> testNEP7(); // not checked but quite long (6 nodes)
std::shared_ptr<NashEquilibriumProblem> testNEP8(); // checked
std::shared_ptr<NashEquilibriumProblem> testNEP9(); // checked
std::shared_ptr<NashEquilibriumProblem> testGNEP1(); // checked, no GNE
std::shared_ptr<NashEquilibriumProblem> testGNEP2(); // checked, no GNE
std::shared_ptr<NashEquilibriumProblem> testGNEP3();
std::shared_ptr<NashEquilibriumProblem> testGNEPConforti();
std::shared_ptr<NashEquilibriumProblem> testGNEPJulianICExample();
std::shared_ptr<NashEquilibriumProblem> testGNEPJulianICExampleModified();
std::shared_ptr<NashEquilibriumProblem> testGNEPtoNEPJulianICExample();
std::shared_ptr<NashEquilibriumProblem> testNEP4playerwithNE(); // compute manually the instance

// 'random' instance with more variable to feel the behaviour.
// seed == -1 means random seed should be used (non reproducable)
std::shared_ptr<NashEquilibriumProblem> testRandomNEP(
    int numberOfPlayers,
    UniformDistribInt distribVariables = UniformDistribInt(3,1),
    UniformDistribDouble distribC = UniformDistribDouble(0.0,5.0),
    UniformDistribDouble distribQ = UniformDistribDouble(0.0,2.0),
    UniformDistribDouble distribBounds = UniformDistribDouble(0.0,10.0),
    double probaIntegrality = 0.8, // proba of a variable to be integer
    double probaNonzero = 0.8,
    int seed = 123);

// hand-crafted toy example of NEP knapsack with 3 players and 7 objects
std::shared_ptr<NashEquilibriumProblem> testKnapsackNEP();

// toy example to obtain a non finite termination example -> not working
std::shared_ptr<NashEquilibriumProblem> testInfiniteNEP();

// toy example to obtain a non finite termination example -> not working
std::shared_ptr<NashEquilibriumProblem> testInfiniteNEP2();

// test for extreme ray computation with negative bounds to variables
// GNEP knapsack builder modified
std::shared_ptr<NashEquilibriumProblem> createGNEPInstanceNegativeBounds(
    std::string const& filename);

// call a specific GNEP knapsack instance file
std::shared_ptr<NashEquilibriumProblem> testGNEPNegativeBounds();

// GNEP instance induced from a famous bilevel example with infimum not attained
std::shared_ptr<NashEquilibriumProblem> testBilevelCounterexample();

#endif //TESTINSTANCES_H
