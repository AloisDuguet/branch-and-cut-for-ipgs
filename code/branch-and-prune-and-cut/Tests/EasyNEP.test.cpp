//
// Created by Aloïs Duguet on 11/12/24.
//

#include <catch2/catch_all.hpp>
#include "catch2/catch_test_macros.hpp"
#include "catch2/internal/catch_compiler_capabilities.hpp"
#include "catch2/internal/catch_test_registry.hpp"
#include "catch2/internal/catch_result_type.hpp"
#include "catch2/internal/catch_test_macro_impl.hpp"
#include <memory>
#include <fstream>

#include "TestInstances.h"
#include "BranchAndCutForGNEP.h"
#include "BranchAndCutTools.h"
#include "BuildGameFromInstance.h"
#include "Constraint.h"
#include "BestMultiplicativeApproxGNEP.h"
#include "BCMultiplicativeAdditiveApproxGNEP.h"

using namespace std;

struct NEPTestInstances {
    vector<shared_ptr<NashEquilibriumProblem>> NEPs;
    vector<string> NEsFound;
    vector<vector<vector<double>>> solutionsExpected;
};

NEPTestInstances buildTestInstances() {
    vector<shared_ptr<NashEquilibriumProblem>> NEPs = {};
    NEPs.push_back(testNEP1());
    NEPs.push_back(testNEP2());
    NEPs.push_back(testNEP3());
    NEPs.push_back(testNEP4());
    NEPs.push_back(testNEP5());
    NEPs.push_back(testNEP6()); // no testNEP7 because it was not manually checked
    NEPs.push_back(testNEP8());
    NEPs.push_back(testNEP9());
    NEPs.push_back(
        testRandomNEP(3,
        UniformDistribInt(3,1),
        UniformDistribDouble(0.0,5.0),
        UniformDistribDouble(0.0,2.0),
        UniformDistribDouble(0.0,10.0),
        0.8,
        0.8,
        123));

    vector<string> optimizationStatuses = {"NO_SOLUTION_FOUND","SOLUTION_FOUND"};
    vector<string> NEsFound = {optimizationStatuses[1], optimizationStatuses[0],
        optimizationStatuses[1], optimizationStatuses[1],
        optimizationStatuses[1], optimizationStatuses[0],
        optimizationStatuses[1], optimizationStatuses[1],
        optimizationStatuses[1]};

    vector<vector<vector<double>>> solutionsExpected = {};
    solutionsExpected.push_back({{0.0,0.0}});
    solutionsExpected.push_back({{}});
    solutionsExpected.push_back({{0.0,0.0}});
    solutionsExpected.push_back({{0.0,0.0}});
    solutionsExpected.push_back({{-1.0,1.0}});
    solutionsExpected.push_back({{}});
    solutionsExpected.push_back({{1.0,1.4}});
    solutionsExpected.push_back({{1.0,1.0}});
    solutionsExpected.push_back({{6, 7, 8, 8, -5, 5, -4, -3.8243273, 3.3162618, -1, 0}, {-7, -6, -8, -6, -8, 3, -1, -3.824327, -6.544961, -1, 6}});

    auto instances  = NEPTestInstances({NEPs, NEsFound, solutionsExpected});
    return instances;
}

NEPTestInstances buildKnapsackTestInstances(string const gameTypeString) {
    string folder_NEP_knapsack = "../../../instances/NEP_knapsack_instances/";
    string folder_GNEP_knapsack = "../../../instances/GNEP_knapsack_instances/";
    string folder_implementation_game = "../../../instances/implementation_games/";

    vector<shared_ptr<NashEquilibriumProblem>> NEPs = {};
    vector<string> optimizationStatuses = {"NO_SOLUTION_FOUND","SOLUTION_FOUND"};
    vector<string> NEsFound = {};
    vector<vector<vector<double>>> solutionsExpected = {};

    if (gameTypeString == "NEP-mixedInteger") {
        NEPs.push_back(buildGameFromInstance(gameTypeString, folder_NEP_knapsack+"2-5-5-uncorr-0.txt"));
        NEPs.push_back(buildGameFromInstance(gameTypeString, folder_NEP_knapsack+"2-5-5-weakcorr-0.txt"));
        NEsFound = {optimizationStatuses[1], optimizationStatuses[1]};
        solutionsExpected.push_back({{0,1,1,0.434783,1,0,1,1,1,0}});
        solutionsExpected.push_back({{1,1,0,0.0659341,1,1,0.60241,0,0,1}, {0,1,0,0.417582,1,0,1,0,0.447368,1}});
    }
    else if (gameTypeString == "NEP-fullInteger") {
        NEPs.push_back(buildGameFromInstance(gameTypeString, folder_NEP_knapsack+"2-5-5-uncorr-0.txt"));
        NEPs.push_back(buildGameFromInstance(gameTypeString, folder_NEP_knapsack+"2-5-5-weakcorr-0.txt"));
        NEsFound = {optimizationStatuses[1], optimizationStatuses[1]};
        solutionsExpected.push_back({{0,1,1,0,1,0,1,1,1,0}});
        solutionsExpected.push_back({{1,1,1,0,0,1,1,0,0,0}, {0,1,1,0,1,0,1,1,1,0}, {1,1,0,0,1,0,1,0,0,1}});
    }
    else if (gameTypeString == "GNEP-fullInteger") {
        NEPs.push_back(buildGameFromInstance(gameTypeString, folder_GNEP_knapsack+"2-5-5-uncorr-0.txt"));
        NEPs.push_back(buildGameFromInstance(gameTypeString, folder_GNEP_knapsack+"2-5-5-weakcorr-0.txt"));
        NEsFound = {optimizationStatuses[1], optimizationStatuses[1]};
        solutionsExpected.push_back({{1,0,1,0,1,0,1,1,1,0}});
        solutionsExpected.push_back({{0,0,0,1,0,0,0,1,1,0}});
    }
    else if (gameTypeString == "implementationGame") {
        NEPs.push_back(buildGameFromInstance(gameTypeString, folder_implementation_game+"I_2_10_mm_1_10.txt")); // NE exists
        NEPs.push_back(buildGameFromInstance(gameTypeString, folder_implementation_game+"I_2_10_mm_1_1.txt")); // no NE exists
        NEsFound = {optimizationStatuses[1], optimizationStatuses[0]};
        solutionsExpected.push_back({{1,1,1,0,1,1,0,1,0,1,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0}});
        solutionsExpected.push_back({{}});
    }

    auto instances  = NEPTestInstances({NEPs, NEsFound, solutionsExpected});
    return instances;
}

void checkGoodOutput(
        NEPBranchOutput NEPOutput,
        NEPTestInstances instances,
        indexx i,
        int nodeNumber = -1,
        int cutNumber = -1) {
    CHECK(NEPOutput.status == instances.NEsFound[i]);
    if (instances.NEsFound[i] == "SOLUTION_FOUND") {
        bool equalNE = false;
        for (auto const& solutionExpected : instances.solutionsExpected[i]) {
            for (int indexNE = 0; indexNE < NEPOutput.solutionList.size(); indexNE++) {
                double dist = distanceMeasure(NEPOutput.solutionList[indexNE].NE, solutionExpected);
                if(isApproxZero(dist, 1e-4)) {
                    cout << "NE found: " << NEPOutput.solutionList[indexNE]
                    << " is (roughly) equal to the solution expected: " << solutionExpected << endl;
                    equalNE = true;
                    break;
                }
                else {
                    cout << "NE found: " << NEPOutput.solutionList[indexNE]
                    << " is not equal to the solution expected: " << solutionExpected << endl;
                }
            }
        }
        CHECK(equalNE);
        if (!equalNE) {
            cerr << "instance index " << i << endl;
            cerr << "there may still be NEs that have not been added to solutionsExpected" << endl;
            cerr << "solutions expected:" << endl;
            for (auto const& solutionExpected : instances.solutionsExpected[i])
                cerr << solutionExpected << endl;
            cerr << "NEs found:" << endl;
            for (int indexNE = 0; indexNE < NEPOutput.solutionList.size(); indexNE++)
                cerr << NEPOutput.solutionList[indexNE].NE << endl;
        } else {
            // same NE, now checking if same number of nodes and cuts if provided
            if (nodeNumber != -1) {
                CHECK(nodeNumber == NEPOutput.numberOfNodesExplored);
            }
            if (cutNumber != -1) {
                CHECK(cutNumber == NEPOutput.numberOfCutsAdded);
            }
        }
    }
}

void checkGoodOutputMinMultApprox(
        BestMultiplicativeBranchOutput NEPOutput,
        NEPTestInstances instances,
        indexx i,
        int nodeNumber = -1,
        int cutNumber = -1) {
    CHECK(NEPOutput.status == instances.NEsFound[i]);
    if (instances.NEsFound[i] == "SOLUTION_FOUND") {
        bool equalNE = false;
        for (auto const& solutionExpected : instances.solutionsExpected[i]) {
            for (int indexNE = 0; indexNE < NEPOutput.solutionList.size(); indexNE++) {
                double dist = distanceMeasure(NEPOutput.solutionList[indexNE].NE, solutionExpected);
                cout << dist << endl;
                if (dist < 0.002)
                    cout << "gna" << endl;
                if(isApproxZero(dist)) {
                    cout << "NE found: " << NEPOutput.solutionList[indexNE]
                    << " is (roughly) equal to the solution expected: " << solutionExpected << endl;
                    equalNE = true;
                    break;
                }
            }
        }
        CHECK(equalNE);
        if (!equalNE) {
            cerr << "instance index " << i << endl;
            cerr << "there may still be NEs that have not been added to solutionsExpected" << endl;
            cerr << "solutions expected:" << endl;
            for (auto const& solutionExpected : instances.solutionsExpected[i])
                cerr << solutionExpected << endl;
            cerr << "NEs found:" << endl;
            for (int indexNE = 0; indexNE < NEPOutput.solutionList.size(); indexNE++)
                cerr << NEPOutput.solutionList[indexNE].NE << endl;
        } else {
            // same NE, now checking if same number of nodes and cuts if provided
            if (nodeNumber != -1) {
                CHECK(nodeNumber == NEPOutput.numberOfNodesExplored);
            }
            if (cutNumber != -1) {
                CHECK(cutNumber == NEPOutput.numberOfCutsAdded);
            }
        }
    }
}

void launchAndCheckInstances(
    NEPTestInstances const& instances,
    bool globalCutSwitch,
    string const& gameTypeString,
    string const& modelTypeString,
    string const& whichCutString,
    string const& branchingRuleString,
    string const& algorithmVariant,
    vector<int> const& nodeNumbers = {},
    vector<int> const& cutNumbers = {}) {
    // int cumulatedNodeExplored = 0;
    for (int i = 0; i < instances.NEsFound.size(); i++) {
        shared_ptr<NashEquilibriumProblem> const& NEP = instances.NEPs[i];
        auto branchingAlgorithm = BranchAndCutForGNEP(NEP, gameTypeString, globalCutSwitch,
        modelTypeString, whichCutString, branchingRuleString, algorithmVariant);
        // correction of node numbers because it is affected by the staticity of m_exploredNodeNumber in NodeSelector
        branchingAlgorithm.resetExploredNodeNumber();
        auto NEPOutput = *branchingAlgorithm.solve();
        if (nodeNumbers.size() == instances.NEsFound.size()) {
            // NEPOutput.numberOfNodesExplored -= cumulatedNodeExplored;
            // cumulatedNodeExplored += NEPOutput.numberOfNodesExplored;
            if (cutNumbers.size() == instances.NEsFound.size()) {
                // node and cut numbers given
                checkGoodOutput(NEPOutput, instances, i, nodeNumbers[i], cutNumbers[i]);
            }
            else {
                // only node number given
                checkGoodOutput(NEPOutput, instances, i, nodeNumbers[i]);
            }
        } else {
            if (cutNumbers.size() == instances.NEsFound.size()) {
                // only cut numbers given
                checkGoodOutput(NEPOutput, instances, i, -1, cutNumbers[i]);
            } else {
                // neither node nor cut numbers given
                checkGoodOutput(NEPOutput, instances, i);
            }
        }
    }
}

void launchAndCheckMultAddInstances(
        NEPTestInstances const& instances,
        bool globalCutSwitch,
        string const& gameTypeString,
        string const& modelTypeString,
        string const& whichCutString,
        string const& branchingRuleString,
        string const& algorithmVariant,
        vector<vector<double>> & multiplicativeApproxs,
        vector<vector<double>> & additiveApproxs,
        vector<int> const& nodeNumbers = {},
        vector<int> const& cutNumbers = {}) {
    for (int i = 0; i < instances.NEsFound.size(); i++) {
        shared_ptr<NashEquilibriumProblem> const& NEP = instances.NEPs[i];
        auto branchingAlgorithm = BCMultiplicativeAdditiveApproxGNEP(
            multiplicativeApproxs[i], additiveApproxs[i],
            NEP, gameTypeString, globalCutSwitch,
            modelTypeString, whichCutString, branchingRuleString, algorithmVariant);
        // correction of node numbers because it is affected by the staticity of m_exploredNodeNumber in NodeSelector
        branchingAlgorithm.resetExploredNodeNumber();
        auto NEPOutput = *branchingAlgorithm.solve();
        if (nodeNumbers.size() == instances.NEsFound.size()) {
            if (cutNumbers.size() == instances.NEsFound.size()) {
                // node and cut numbers given
                checkGoodOutput(NEPOutput, instances, i, nodeNumbers[i], cutNumbers[i]);
            }
            else {
                // only node number given
                checkGoodOutput(NEPOutput, instances, i, nodeNumbers[i]);
            }
        } else {
            if (cutNumbers.size() == instances.NEsFound.size()) {
                // only cut numbers given
                checkGoodOutput(NEPOutput, instances, i, -1, cutNumbers[i]);
            } else {
                // neither node nor cut numbers given
                checkGoodOutput(NEPOutput, instances, i);
            }
        }
    }
}

void launchAndCheckInstancesMinMultApprox(
    NEPTestInstances const& instances,
    bool globalCutSwitch,
    string const& gameTypeString,
    string const& modelTypeString,
    string const& whichCutString,
    string const& branchingRuleString,
    string const& algorithmVariant,
    vector<int> const& nodeNumbers = {},
    vector<int> const& cutNumbers = {}) {
    // int cumulatedNodeExplored = 0;
    for (int i = 0; i < instances.NEsFound.size(); i++) {
        shared_ptr<NashEquilibriumProblem> const& NEP = instances.NEPs[i];
        vector<double> multiplicativeApprox = {10};
        auto branchingAlgorithm = BestMultiplicativeApproxGNEP(
                                     multiplicativeApprox,
                                     NEP,
                                     gameTypeString,
                                     globalCutSwitch,
                                     modelTypeString,
                                     whichCutString,
                                     branchingRuleString,
                                     algorithmVariant,
                                     "UNK",
                                     false,
                                     3600,
                                     0);
        // correction of node numbers because it is affected by the staticity of m_exploredNodeNumber in NodeSelector
        branchingAlgorithm.resetExploredNodeNumber();
        auto NEPOutput = *branchingAlgorithm.solve();
        if (nodeNumbers.size() == instances.NEsFound.size()) {
            // NEPOutput.numberOfNodesExplored -= cumulatedNodeExplored;
            // cumulatedNodeExplored += NEPOutput.numberOfNodesExplored;
            if (cutNumbers.size() == instances.NEsFound.size()) {
                // node and cut numbers given
                checkGoodOutputMinMultApprox(NEPOutput, instances, i, nodeNumbers[i], cutNumbers[i]);
            }
            else {
                // only node number given
                checkGoodOutputMinMultApprox(NEPOutput, instances, i, nodeNumbers[i]);
            }
        } else {
            if (cutNumbers.size() == instances.NEsFound.size()) {
                // only cut numbers given
                checkGoodOutputMinMultApprox(NEPOutput, instances, i, -1, cutNumbers[i]);
            } else {
                // neither node nor cut numbers given
                checkGoodOutputMinMultApprox(NEPOutput, instances, i);
            }
        }
    }
}


TEST_CASE("GNEP with intersection cuts on Julian's IC example") {
    auto NEP = testGNEPJulianICExample();
    auto branchingAlgorithm = BranchAndCutForGNEP(
        NEP, "JulianICExample",
        false,
        "manyEtasGurobi",
        "intersectionCuts",
        "mostFractional",
        "basicAlgorithm",
        "JulianICExample",
        false,
        3600,
        3);

    // correction of node numbers because it is affected by the staticity of m_exploredNodeNumber in NodeSelector
    branchingAlgorithm.resetExploredNodeNumber();
    auto NEPOutput = *branchingAlgorithm.solve();
    vector<vector<double>> solutionExpected = {{1,0,0,1,0,1}};
    auto instances = NEPTestInstances({{NEP},
        {"SOLUTION_FOUND"},
        {solutionExpected}});
    checkGoodOutput(NEPOutput, instances, 0);

    // check number of cuts and nodes
    CHECK(NEPOutput.numberOfCutsAdded == 3);
    CHECK(NEPOutput.numberOfNodesExplored == 1);

    // check which ICs
    cout << NEPOutput.finalNode->getCuts()[0] << endl;
    cout << NEPOutput.finalNode->getCuts()[1] << endl;
    cout << NEPOutput.finalNode->getCuts()[2] << endl;
    auto cut1 = NEPOutput.finalNode->getCuts()[0];
    auto cut2 = NEPOutput.finalNode->getCuts()[1];
    auto cut3 = NEPOutput.finalNode->getCuts()[2];
    MySparseVector ax1 = {{-0.5,-0.5,1},{2,4,6}};
    Cut realCut1 = {ax1,{},-1};
    MySparseVector ax2 = {{-0.5,0.5},{0,7}};
    Cut realCut2 = {ax2,{},-0.5};
    MySparseVector ax3 = {{-0.5,0.5},{0,8}};
    Cut realCut3 = {ax3,{},-0.5};
    CHECK(cut1 == realCut1);
    CHECK(cut2 == realCut2);
    CHECK(cut3 == realCut3);
}

TEST_CASE("GNEP with intersection cuts on 2-5-5-uncorr-0", "IC") {
    string gameTypeString = "GNEP-fullInteger";
    string folder_GNEP_knapsack = "../../../instances/GNEP_knapsack_instances/";
    auto NEP = buildGameFromInstance(gameTypeString, folder_GNEP_knapsack+"2-5-5-uncorr-0.txt");
    auto branchingAlgorithm = BranchAndCutForGNEP(
        NEP, gameTypeString,
        false,
        "manyEtasGurobi",
        "intersectionCuts",
        "mostFractional",
        "basicAlgorithm",
        "2-5-5-uncorr-0.txt",
        false,
        3600,
        3);

    branchingAlgorithm.resetExploredNodeNumber();
    auto NEPOutput = *branchingAlgorithm.solve();
    vector<vector<double>> solutionExpected = {{1,0,1,0,1,0,1,1,1,0}};
    auto instances = NEPTestInstances({{NEP},
        {"SOLUTION_FOUND"},
        {solutionExpected}});
    checkGoodOutput(NEPOutput, instances, 0);

    // check number of cuts and nodes
    CHECK(NEPOutput.numberOfCutsAdded == 18);
    CHECK(NEPOutput.numberOfNodesExplored == 57);

    vector<Constraint> constraints = NEPOutput.finalNode->getModelDiff();
    auto model = NEPOutput.finalNode->getSubproblem();
    cout << *model << endl;
}

TEST_CASE("Intersection cut focused: knapsack integer GNEPs", "IC") {
    string gameTypeString = "GNEP-fullInteger";
    string folder_GNEP_knapsack = "../../../instances/GNEP_knapsack_instances/";

    vector<shared_ptr<NashEquilibriumProblem>> NEPs = {};
    vector<string> optimizationStatuses = {"NO_SOLUTION_FOUND","SOLUTION_FOUND"};
    vector<string> NEsFound = {};
    vector<vector<vector<double>>> solutionsExpected = {};

    NEPs.push_back(buildGameFromInstance(gameTypeString, folder_GNEP_knapsack+"2-5-5-uncorr-0.txt"));
    NEPs.push_back(buildGameFromInstance(gameTypeString, folder_GNEP_knapsack+"2-5-5-weakcorr-0.txt"));
    NEPs.push_back(buildGameFromInstance(gameTypeString, folder_GNEP_knapsack+"2-20-2-uncorr-1.txt"));
    NEPs.push_back(buildGameFromInstance(gameTypeString, folder_GNEP_knapsack+"2-20-2-uncorr-2.txt"));
    NEsFound = {optimizationStatuses[1], optimizationStatuses[1], optimizationStatuses[1], optimizationStatuses[1]};
    solutionsExpected.push_back({{1,0,1,0,1,0,1,1,1,0}});
    solutionsExpected.push_back({{0,0,0,1,0,0,0,1,1,0}});
    solutionsExpected.push_back({{0,0,0,0,0,0,1,1,1,0,0,0,1,0,0,0,0,1,0,0,1,1,0,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,0,0}});
    solutionsExpected.push_back({{1,0,0,0,0,0,0,0,1,1,0,0,0,1,0,1,0,0,1,1,1,1,1,0,1,1,0,0,0,0,0,0,0,0,0,0,1,1,0,1}});

    auto instances  = NEPTestInstances({NEPs, NEsFound, solutionsExpected});

    vector<int> nodeNumbers = {57,7,110,3779};
    vector<int> cutNumbers = {18,0,10,914};

    launchAndCheckInstances(instances,
        false,
        gameTypeString,
        "manyEtasGurobi",
        "intersectionCuts",
        "mostFractional",
        "basicAlgorithm",
        nodeNumbers,
        cutNumbers);
}

TEST_CASE("Intersection cut focused: implementation game GNEPs", "IC2") {
    string gameTypeString = "implementationGame";
    string folder_implementation_game = "../../../instances/implementation_games/";

    vector<shared_ptr<NashEquilibriumProblem>> NEPs = {};
    vector<string> optimizationStatuses = {"NO_SOLUTION_FOUND","SOLUTION_FOUND"};
    vector<string> NEsFound = {};
    vector<vector<vector<double>>> solutionsExpected = {};

    NEPs.push_back(buildGameFromInstance(gameTypeString, folder_implementation_game+"I_2_20_mm_10_62.txt"));
    NEPs.push_back(buildGameFromInstance(gameTypeString, folder_implementation_game+"I_2_15_mm_1_6.txt"));
    NEPs.push_back(buildGameFromInstance(gameTypeString, folder_implementation_game+"I_2_10_ss_10_5.txt"));
    NEsFound = {optimizationStatuses[0], optimizationStatuses[1], optimizationStatuses[1]};
    // solutionsExpected.push_back({});
    solutionsExpected.push_back({});
    solutionsExpected.push_back({{0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,1,0,0,1,0,1,0,0,0,1,1,0,0,1,1,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,25,0,0,0,12,0,0,0,0,0,0,0,0,0,0,0,0,0},
                                        {0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,1,0,0,1,0,1,0,0,0,1,1,0,0,1,1,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12,0,0,0,0,0,0,0,0,0,0,0,0,0},
                                        {0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,1,0,0,1,0,1,0,0,0,1,1,0,0,1,1,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,25,21,0,0,15,0,0,0,0,0,0,0,0,0,0,0,0,0},
                                        {0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,1,0,0,1,0,1,0,0,0,1,1,0,0,1,1,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,11,25,0,0,0,12,0,0,0,0,0,0,0,0,0,0,0,0,0}});
    solutionsExpected.push_back({{0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,3,1,1,7,3,4,3,0,0,5,0,0,0,0,0,0,0,0,0,0,0,25,0,0,0},
                                {4,0,0,0,3,0,4,8,3,5,3,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,27,0,0,0,12,0,0,0}});

    auto instances  = NEPTestInstances({NEPs, NEsFound, solutionsExpected});

    vector<int> nodeNumbers = {-1,-1,-1};
    vector<int> cutNumbers = {-1,-1,-1};

    launchAndCheckInstances(instances,
        false,
        gameTypeString,
        "manyEtasGurobi",
        "intersectionCuts",
        "mostFractional",
        "basicAlgorithm",
        nodeNumbers,
        cutNumbers);
}

TEST_CASE("NEP implementation games with multiplicative min-approx NE","minMultApproxNEP") {
    string gameTypeString = "NEPImplementationGame";
    string folder_implementation_game = "../../../instances/implementation_games/";

    vector<shared_ptr<NashEquilibriumProblem>> NEPs = {};
    vector<string> optimizationStatuses = {"NO_SOLUTION_FOUND","SOLUTION_FOUND"};
    vector<string> NEsFound = {};
    vector<vector<vector<double>>> solutionsExpected = {};

    string algorithmVariant = "reuseTreeSearch";
    NEPs.push_back(buildGameFromInstance(gameTypeString, folder_implementation_game+"I_2_20_mm_10_34.txt"));
    NEPs.push_back(buildGameFromInstance(gameTypeString, folder_implementation_game+"I_2_20_mm_10_62.txt"));
    NEPs.push_back(buildGameFromInstance(gameTypeString, folder_implementation_game+"I_2_20_mm_10_11.txt"));
    NEsFound = {optimizationStatuses[1], optimizationStatuses[1], optimizationStatuses[1]};
    solutionsExpected.push_back({{2,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,2,2,0,0,0,0,0,0,0,0,2,5,2,3,0,0,2,0,2,0,0,0,3,0,0,0,0,5,2,0,0,0,0,0,0,2,0,0,0,0,73,0,0,0,-0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
                                     {2,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,2,2,0,0,0,0,0,0,0,0,2,5,2,3,0,0,2,0,2,0,0,0,3,0,0,0,0,5,2,0,0,0,0,0,0,2,0,10,0,0,73,0,0,0,-0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}});
    solutionsExpected.push_back({{4,5,0,9,-0,0,0,0,4,5,9,0,0,9,0,0,0,0,9,0,0,0,-0,-0,0,0,0,1,-0,0,0,0,0,0,1,0,0,0,0,1,0,1,1,0,0,0,-0,0,0,0,0,0,-0,0,0,0,0,108,0,0,0,0,0,0,0,0,0,0,0,0,-0},
                                     {4,5,0,9,0,0,0,0,4,5,9,0,0,9,0,0,0,0,9,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,1,0,0,0,0,1,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,109,0,0,0,0,0,0,0,0,0,0,0,0,0},
                                     {4,5,0,9,0,0,0,0,4,5,9,0,0,9,0,0,0,0,9,0,0,0,0,-0,0,0,0,1,0,0,0,0,0,0,1,0,0,0,0,1,0,1,1,0,0,0,0,0,0,0,0,0,-0,0,0,0,0,109.199822,0,0,0,0,0,0,0,0,0,0,0,0,-0},
                                     {4,5,0,9,-0,0,0,0,4,5,9,0,0,9,0,0,0,0,9,0,0,0,-0,-0,0,0,0,1,-0,0,0,0,0,0,1,0,0,0,0,1,0,1,1,0,0,0,-0,0,0,0,0,0,-0,0,0,0,0,109.198795,0,0,0,0,0,0,0,0,0,0,0,0,-0}});
    solutionsExpected.push_back({{3,0,3,3,0,0,7,0,0,7,0,0,0,0,7,3,4,0,0,0,0,0,0,0,0,0,0,-0,0,0,0,0,2,0,0,0,0,0,2,0,0,0,2,2,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,31,0,0,0,0,0,0,0,0,0,0,-0}});

    auto instances  = NEPTestInstances({NEPs, NEsFound, solutionsExpected});

    vector<int> nodeNumbers = {-1,-1,-1};
    vector<int> cutNumbers = {7,-1,-1};

    vector<double> lowerBounds = {1,5.0077,1};
    vector<double> upperBounds = {1,5.08333,1};

    launchAndCheckInstancesMinMultApprox(instances,
        true,
        gameTypeString,
        "manyEtasGurobi",
        "eqCuts",
        "mostFractional",
        algorithmVariant,
        nodeNumbers,
        cutNumbers);
}

TEST_CASE("GNEP knapsack with quadratic costs with multiplicative min-approx NE","minMultApproxGNEP2") {
    // artificial instances of GNEP knapsack with additional quadratic convex costs
    // just to have examples of GNEP instances with quadratic costs in the tests
    // the solutions expected are just the output of those artificial instances
    // when the test was implemented, so if one day those tests do not pass,
    // do not expect the solutions expected to be the valid ones

    string gameTypeString = "GNEP-fullInteger";
    string folder_GNEP_knapsack = "../../../instances/GNEP_knapsack_instances/";

    vector<shared_ptr<NashEquilibriumProblem>> NEPs = {};
    vector<string> optimizationStatuses = {"NO_SOLUTION_FOUND","SOLUTION_FOUND"};
    vector<string> NEsFound = {};
    vector<vector<vector<double>>> solutionsExpected = {};

    string algorithmVariant = "reuseTreeSearch";
    NEPs.push_back(buildGameFromInstance(gameTypeString, folder_GNEP_knapsack+"2-5-5-uncorr-0.txt"));
    NEPs.push_back(buildGameFromInstance(gameTypeString, folder_GNEP_knapsack+"2-5-5-weakcorr-0.txt"));
    NEPs.push_back(buildGameFromInstance(gameTypeString, folder_GNEP_knapsack+"2-10-2-uncorr-1.txt"));

    // adding quadratic convex terms to cost functions
    for (auto const& nep : NEPs) {
        for (indexx player = 0; player < nep->getNumberOfPlayer(); player++) {
            auto optProblem = nep->getBestResponseProblems()[player];
            auto Q = optProblem->getQ();
            indexx firstPlayerIndex = nep->getStartingPlayerVariableIndex(player);
            for (indexx var = 0; var < optProblem->getNumberOfVariables(); var++) {
                Q.addCoefficient(1, var+firstPlayerIndex, var+firstPlayerIndex);
            }
            nep->getBestResponseProblems()[player]->setQ(Q);
        }
    }

    NEsFound = {optimizationStatuses[1], optimizationStatuses[1], optimizationStatuses[1]};
    solutionsExpected.push_back({{1,0,1,0,1,0,1,1,1,0}});
    solutionsExpected.push_back({{0,0,0,1,0,0,0,1,1,0}});
    solutionsExpected.push_back({{0,0,0,0,1,0,1,1,0,1,0,1,0,1,0,0,0,0,0,1}});

    auto instances  = NEPTestInstances({NEPs, NEsFound, solutionsExpected});

    vector<int> nodeNumbers = {-1,-1,-1};
    vector<int> cutNumbers = {-1,-1,-1};

    vector<double> lowerBounds = {1,1};
    vector<double> upperBounds = {1,1};

    launchAndCheckInstancesMinMultApprox(instances,
        false,
        gameTypeString,
        "manyEtasGurobi",
        "intersectionCutsXiConvex",
        "mostFractional",
        algorithmVariant,
        nodeNumbers,
        cutNumbers);
}

TEST_CASE("NEP implementation games with multiplicative-additive approx NEs","MultAddApproxNEP") {
    string gameTypeString = "NEPImplementationGame";
    string folder_implementation_game = "../../../instances/implementation_games/";

    vector<shared_ptr<NashEquilibriumProblem>> NEPs = {};
    vector<string> optimizationStatuses = {"NO_SOLUTION_FOUND","SOLUTION_FOUND"};
    vector<string> NEsFound = {};
    vector<vector<vector<double>>> solutionsExpected = {};

    string algorithmVariant = "basicAlgorithm";
    NEPs.push_back(buildGameFromInstance(gameTypeString, folder_implementation_game+"I_2_20_mm_10_62.txt"));
    NEPs.push_back(buildGameFromInstance(gameTypeString, folder_implementation_game+"I_2_20_mm_10_62.txt"));
    NEPs.push_back(buildGameFromInstance(gameTypeString, folder_implementation_game+"I_2_20_ss_1_7.txt"));

    vector<vector<double>> multiplicativeApproxs = {{10},{5},{5}};
    vector<vector<double>> additiveApproxs = {{0},{0},{0}};

    NEsFound = {optimizationStatuses[1], optimizationStatuses[0], optimizationStatuses[1]};
    solutionsExpected.push_back({{4,5,0,9,0,0,0,0,4,5,9,0,0,9,0,0,0,0,9,0,0,0,0,-0,0,0,0,1,0,0,0,0,0,0,1,0,0,0,0,1,0,1,1,0,0,0,0,0,4,22,4,0,-0,0,0,0,0,79,0,3,0,0,0,0,0,0,0,0,0,0,-0},
                                  {4,5,0,9,0,0,0,0,4,5,9,0,0,9,0,0,0,0,9,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,1,0,0,0,0,1,0,1,1,0,0,0,0,0,4,14,4,0,0,0,0,0,0,79,0,3,0,0,0,0,0,0,0,3,0,0,0},
                                  {4,5,0,9,0,0,0,0,4,5,9,0,0,9,0,0,0,0,9,0,0,0,0,-0,0,0,0,1,0,0,0,0,0,0,1,0,0,0,0,1,0,1,1,0,0,0,0,0,1.25037,4.17593,0,0,-0,0,0,0,0.808188,95.2423,0,0,0,0.598657,0,0,0,0,0,0,0,0,-0}});
    solutionsExpected.push_back({{}});
    solutionsExpected.push_back({{0,0,1,0,1,0,0,1,1,1,1,1,0,0,0,0,0,1,1,0,2,2,0,0,0,0,2,1,0,0,0,2,0,0,0,0,2,0,0,1,1,0,-0,1,0,1,0,1,0,0,1,1,1,0,1,0,0,0,0,0,2,1,0,1,2,0,0,0,0,2,0,1,0,0,2,1,0,0,0,2,0,0,1,2,0,-0,-0,0,-0,0,23,0,0,0,0,26,0,-0,0,0,0,0,0,-0,12,0,-0,35,0,0,0,0,-0,0,-0,0,0,-0,0,0,0,0,-0,0,0,0,3,0},
                                     {0,0,2,0,0,0,0,1,1,1,0,1,1,0,0,0,0,1,1,0,2,2,0,0,0,0,1,0,0,0,1,1,0,0,0,0,2,0,0,1,2,0,-0,0,1,2,0,0,1,0,1,1,0,0,1,0,0,0,0,0,2,1,1,1,1,0,0,0,0,1,0,1,0,0,1,1,0,0,0,1,0,0,0,2,0,-0,0,0,0,0,0,0,0,0,0,0.599956,0,0,0,0,0,0,0,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,49.3304,0},
                                     {0,0,2,0,1,0,0,1,1,1,1,1,0,0,0,0,0,1,1,0,2,2,0,0,0,0,2,0,0,0,1,2,0,0,0,0,2,0,0,1,2,0,-0,0,1,2,0,0,1,0,1,1,0,0,1,0,0,0,0,0,1,1,1,1,1,1,0,0,0,2,0,1,0,0,2,0,0,0,1,2,0,0,0,2,0,-0,7.81769e-08,0,2.37682,7.4268e-08,0.202345,3.80862e-08,0,7.4268e-08,7.4268e-08,0.202554,1.48535e-06,4.18528,7.4268e-08,7.4268e-08,7.4268e-08,7.4268e-08,7.4268e-08,9.9024e-08,3.20208,7.81769e-08,5,0.1951,3.7134e-08,0,0,3.7134e-08,2.40999,0,9.90241e-08,0,5.94144e-08,2.21968,4.24389e-08,0,0,3.7134e-08,2.34673,0,0,7.4268e-08,51,7.4268e-08}});

    auto instances  = NEPTestInstances({NEPs, NEsFound, solutionsExpected});

    vector<int> nodeNumbers = {-1,389,-1};
    vector<int> cutNumbers = {-1,6,3};


    launchAndCheckMultAddInstances(
        instances,
        true,
        gameTypeString,
        "manyEtasGurobi",
        "eqCuts",
        "mostFractional",
        algorithmVariant,
        multiplicativeApproxs,
        additiveApproxs,
        nodeNumbers,
        cutNumbers);
}

TEST_CASE("global equilibrium cuts, many etas model and basicAlgorithm") {
    auto instances = buildTestInstances();

    bool globalCutSwitch = true;
    string const modelTypeString = "manyEtasGurobi";
    string const whichCutString = "eqCuts";
    string branchingRuleString = "mostFractional";
    string algorithmVariant = "basicAlgorithm";

    launchAndCheckInstances(instances, globalCutSwitch, "different games", modelTypeString, whichCutString, branchingRuleString, algorithmVariant);
}

TEST_CASE("global equilibrium cuts, many etas model and cuttingBeforeBranching") {
    auto instances = buildTestInstances();

    bool globalCutSwitch = true;
    string const modelTypeString = "manyEtasGurobi";
    string const whichCutString = "eqCuts";
    string branchingRuleString = "mostFractional";
    string algorithmVariant = "cuttingBeforeBranching";

    launchAndCheckInstances(instances, globalCutSwitch, "different games", modelTypeString, whichCutString, branchingRuleString, algorithmVariant);
}