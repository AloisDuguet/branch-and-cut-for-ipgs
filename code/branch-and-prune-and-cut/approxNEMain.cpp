//
// Created by alois-duguet on 7/22/25.
//

#include <iostream>
#include <stdexcept>
#include <string>

#include "gurobi_c++.h"

#include "BranchAndCutTools.h"
#include "BranchAndCutMultiplicativeGNEP.h"
#include "BestMultiplicativeApproxGNEP.h"
#include "BCMultiplicativeAdditiveApproxGNEP.h"
#include "BuildGameFromInstance.h"
#include "BestMultiplicativeAdditiveApproxGNEP.h"

using namespace std;

int main(int argc, char *argv[]) {
    // basic setting for test purposes, not using argc and argv
    // problemVariant in multiplicativeNE addMultNE minMultiplicativeNE minMultFixedAddNE
    string problemVariant = "addMultNE";
    string filename;
    // gameTypeString in NEP-fullInteger NEP-mixedInteger GNEP-fullInteger NEPImplementationGame GNEP-maxFlowGame
    string gameTypeString = "NEPImplementationGame";
    double delta = 0;
    vector<double> multiplicativeApprox = {10};
    // first vector is for the multiplicative approx, second for additive
    vector<vector<double>> multiplicativeAdditiveApprox = {{4},{25}};
    string algorithmVariant = "basicAlgorithm";
    if (problemVariant == "minMultiplicativeNE") {
        // algorithmVariant in reuseTreeSearch reuseWithoutCuts
        algorithmVariant = "reuseWithoutCuts";
    }

    bool enumerateNE = false;
    string modelTypeString = "manyEtasGurobi";
    string branchingRuleString = "mostFractional";
    double timeLimit = 3600000;
    int verbosity = 1;
    string resultFilename = "allResults.txt";

    // decide the cut used with respect to the game type
    string whichCutString = "eqCuts"; // for NEP
    if (gameTypeString == "GNEP-fullInteger" or gameTypeString == "GNEP-maxFlowGame") {
        // for GNEP with concave cost function in x and linear in x_-i
        // whichCutString in intersectionCutsXiConvex intersectionCutsXiConcave intersectionCutsLipschitz
        // for GNEP with convex cost function in x
        whichCutString = "intersectionCutsXiConvex";
    }
    if (gameTypeString == "implementationGame") {
        whichCutString = "intersectionCutsLipschitz";
    }

    // decide if the cut is local or global with the type of cut used
    bool globalCutSwitch = false;
    // equilibrium cuts are globally valid in the NEP setting
    if (whichCutString == "eqCuts")
        globalCutSwitch = true;
    // no-good cuts are globally valid because they remove
    // one feasible strategy profile that is not an NE
    if (whichCutString == "noGoodCuts")
        globalCutSwitch = true;

    if (argc >= 2)
        problemVariant = argv[1];

    // specific setting given by argc and argv
    if (argc >= 3)
        filename = argv[2];
    else {
        // in case no filename was provided to the call to main
        filename = getFilenameInstance(gameTypeString);
    }
    if (argc >= 4) {
        if (problemVariant == "multiplicativeNE" or problemVariant == "minMultiplicativeNE") {
            string multiplicativeApproxString = string(argv[3]);
            multiplicativeApprox = buildVectorFromString(multiplicativeApproxString);
        } else if (problemVariant == "addMultNE" or problemVariant == "minMultFixedAddNE") {
            string multiplicativeAdditiveApproxString = string(argv[3]);
            multiplicativeAdditiveApprox = buildMultiplicativeAdditiveApproxVector(multiplicativeAdditiveApproxString);
        }
    }
    if (argc >= 5) gameTypeString = argv[4];

    // build instance from instance file
    shared_ptr<NashEquilibriumProblem> NEP = buildGameFromInstance(gameTypeString, filename);

    if (argc >= 6) algorithmVariant = argv[5];
    if (argc >= 7) whichCutString = argv[6];
    if (argc >= 8) globalCutSwitch = handleBoolErrorAsMainArgument(argv[7], 7);
    if (argc >= 9) enumerateNE = handleBoolErrorAsMainArgument(argv[8], 8);
    if (argc >= 10) modelTypeString = argv[9];
    if (argc >= 11) branchingRuleString = argv[10];
    if (argc >= 12) timeLimit = stod(argv[11]);
    if (argc >= 13) verbosity = stoi(argv[12]);
    if (argc >= 14) resultFilename = argv[13];

    // write arguments
    if (verbosity >= 1) {
        cout << "argc = " << argc << endl;
        for (int i = 0; i < argc; i++) {
            cout << "argv[" << i << "] = " << argv[i] << endl;
        }
    }

    try {
        shared_ptr<BranchAndCutForGNEP> branchingAlgorithm;
        if (problemVariant == "multiplicativeNE") {
            branchingAlgorithm = make_shared<BranchAndCutMultiplicativeGNEP>(
                                     multiplicativeApprox,
                                     NEP,
                                     gameTypeString,
                                     globalCutSwitch,
                                     modelTypeString,
                                     whichCutString,
                                     branchingRuleString,
                                     algorithmVariant,
                                     filename,
                                     enumerateNE,
                                     timeLimit,
                                     verbosity,
                                     resultFilename);
        } else if (problemVariant == "minMultiplicativeNE") {
            branchingAlgorithm = make_shared<BestMultiplicativeApproxGNEP>(
                                     multiplicativeApprox,
                                     NEP,
                                     gameTypeString,
                                     globalCutSwitch,
                                     modelTypeString,
                                     whichCutString,
                                     branchingRuleString,
                                     algorithmVariant,
                                     filename,
                                     enumerateNE,
                                     timeLimit,
                                     verbosity,
                                     resultFilename);
        } else if (problemVariant == "addMultNE") {
            branchingAlgorithm = make_shared<BCMultiplicativeAdditiveApproxGNEP>(
                                     multiplicativeAdditiveApprox.front(),
                                     multiplicativeAdditiveApprox.back(),
                                     NEP,
                                     gameTypeString,
                                     globalCutSwitch,
                                     modelTypeString,
                                     whichCutString,
                                     branchingRuleString,
                                     algorithmVariant,
                                     filename,
                                     enumerateNE,
                                     timeLimit,
                                     verbosity,
                                     resultFilename);
        } else if (problemVariant == "minMultFixedAddNE") {
            branchingAlgorithm = make_shared<BestMultiplicativeAdditiveApproxGNEP>(
                                    multiplicativeAdditiveApprox.front(),
                                    multiplicativeAdditiveApprox.back(),
                                     NEP,
                                     gameTypeString,
                                     globalCutSwitch,
                                     modelTypeString,
                                     whichCutString,
                                     branchingRuleString,
                                     algorithmVariant,
                                     filename,
                                     enumerateNE,
                                     timeLimit,
                                     verbosity,
                                     resultFilename);
        } else {
            throw logic_error("problem variant " + problemVariant + " not supported");
        }
        auto NEPOutput = branchingAlgorithm->solve();
        delete NEPOutput;
    }
    catch (std::logic_error const&  e) {
        cerr << "logic_error: " << e.what() << endl;
    }
    catch (std::runtime_error const& e) {
        cerr << "runtime_error: " << e.what() << endl;
    }
    catch (GRBException const& e) {
        cerr << "GRBException: " << e.getMessage() << endl;
    }
    return 0;
}
