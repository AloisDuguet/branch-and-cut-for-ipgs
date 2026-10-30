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
#include "BestAdditiveMultiplicativeApproxGNEP.h"
#include "BestPartialMinMultFixedAdd.h"

using namespace std;

int main(int argc, char *argv[]) {
    // basic setting for test purposes, not using argc and argv
    // problemVariant in multiplicativeNE addMultNE minMultiplicativeNE
    // minMultFixedAddNE minPartialMultFixedAddNE minAdditiveFixedMultNE
    string problemVariant = "minPartialMultFixedAddNE";
    string filename;
    // gameTypeString in NEP-fullInteger NEP-mixedInteger GNEP-fullInteger
    // NEPImplementationGame GNEP-maxFlowGame GNEPQuadraticKnapsack
    // randomGNEP randomGNEPQuadratic
    string gameTypeString = "NEPImplementationGame";
    vector<double> multiplicativeApprox = {10};
    // first vector is for the multiplicative approx, second for additive
    vector<vector<double>> multiplicativeAdditiveApprox = {{10},{1}};
    string algorithmVariant = "basicAlgorithm";
    if ((problemVariant == "minMultiplicativeNE"
        or problemVariant == "minMultFixedAddNE"
        or problemVariant == "minPartialMultFixedAddNE"
        or problemVariant == "minAdditiveFixedMultNE") and
        (gameTypeString != "GNEP-fullInteger"
            and gameTypeString != "GNEP-maxFlowGame"
            and gameTypeString != "GNEPQuadraticKnapsack"
            and gameTypeString != "randomGNEP"
            and gameTypeString != "randomGNEPQuadratic")){
        // algorithmVariant in basicAlgorithm reuseTreeSearch reuseWithoutCuts
        // last two options valid only with globally valid cuts
        algorithmVariant = "reuseTreeSearch";
    }

    bool enumerateNE = false;
    string modelTypeString = "manyEtasGurobi";
    string branchingRuleString = "mostFractional";
    double timeLimit = 3600;
    int verbosity = 3;
    string resultFilename = "noWriteResult";

    // decide the cut used with respect to the game type
    // whichCutString in intersectionCutsXiConvex intersectionCutsXiConcave intersectionCutsLipschitz
    string whichCutString = "eqCuts"; // for NEP
    if (gameTypeString == "GNEP-fullInteger" or gameTypeString == "GNEP-maxFlowGame"
        or gameTypeString == "GNEPQuadraticKnapsack") {
        // for GNEP with convex cost function in x
        whichCutString = "intersectionCutsXiConvex";
    }
    else if (gameTypeString == "randomGNEP" or gameTypeString == "randomGNEPQuadratic")
        whichCutString = "intersectionCutsXiConcave";
    else if (gameTypeString == "implementationGame") {
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
        // produce a correct (or intermediary) string with the approximation parameters
        if (problemVariant == "multiplicativeNE" or problemVariant == "minMultiplicativeNE") {
            auto multiplicativeApproxString = string(argv[3]);
            multiplicativeApprox = buildVectorFromString(multiplicativeApproxString);
        } else if (problemVariant == "addMultNE" or problemVariant == "minMultFixedAddNE"
            or problemVariant == "minPartialMultFixedAddNE" or problemVariant == "minAdditiveFixedMultNE") {
            auto multiplicativeAdditiveApproxString = string(argv[3]);
            multiplicativeAdditiveApprox = buildMultiplicativeAdditiveApproxVector(multiplicativeAdditiveApproxString);
            if (problemVariant == "minPartialMultFixedAddNE" and
                    (gameTypeString == "NEPImplementationGame" or gameTypeString == "implementationGame") and
                    multiplicativeAdditiveApprox[0].size() != 1) {
                throw logic_error("for now, argv[3] has to be exactly one double in string format");
            }
        }
    }

    if (argc >= 5) gameTypeString = argv[4];

    // build instance from instance file
    shared_ptr<NashEquilibriumProblem> NEP = buildGameFromInstance(gameTypeString, filename);

    // specific case in which the last player has a 1 for multiplicative and 0 for additive approximation
    // because he should play exact best responses to enforce the implementation property
    if (problemVariant == "minPartialMultFixedAddNE" and (gameTypeString == "NEPImplementationGame" or gameTypeString == "implementationGame")) {
        // enforce that there is one value per player for the later step
        int n = NEP->getNumberOfPlayer();
        if (multiplicativeAdditiveApprox[0].size() != n)
            multiplicativeAdditiveApprox[0] = expandToVector(multiplicativeAdditiveApprox[0][0], n);
        if (multiplicativeAdditiveApprox[1].size() != n)
            multiplicativeAdditiveApprox[1] = expandToVector(multiplicativeAdditiveApprox[1][0], n);
        // changes the approximation value of the last player
        multiplicativeAdditiveApprox[0][multiplicativeAdditiveApprox[0].size()-1] = 1;
        multiplicativeAdditiveApprox[1][multiplicativeAdditiveApprox[1].size()-1] = 0;
    }

    // build multToMinimize after knowing the instance because it gives the number of players
    vector<bool> multToMinimize = {};
    if (problemVariant == "minPartialMultFixedAddNE") {
        if (gameTypeString == "NEPImplementationGame" or gameTypeString == "implementationGame")
            multToMinimize = allButLastTrue(NEP->getNumberOfPlayer());
    }

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
        } else if (problemVariant == "minAdditiveFixedMultNE") {
            branchingAlgorithm = make_shared<BestAdditiveMultiplicativeApproxGNEP>(
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
        } else if (problemVariant == "minPartialMultFixedAddNE") {
            branchingAlgorithm = make_shared<BestPartialMinMultFixedAdd>(
                                    multToMinimize,
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
        // TODO: derive my own class from exception for input_error
        // such as two arguments that can't be handled at the same time
        cerr << "runtime_error: " << e.what() << endl;
    }
    return 0;
}