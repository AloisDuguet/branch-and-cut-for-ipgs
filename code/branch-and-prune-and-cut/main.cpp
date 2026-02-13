//
// Created by Aloïs Duguet on 05/11/24.
//

#include <iostream>
#include <stdexcept>
#include "gurobi_c++.h"

#include "BranchAndCutTools.h"
#include "BranchAndCutForGNEP.h"
#include "BuildGameFromInstance.h"
#include "TestInstances.h"

using namespace std;

int main(int argc, char *argv[]) {
    // basic setting for test purposes, not using argc and argv
    string filename;
    // string gameTypeString = "NEP-fullInteger";
    // gameTypeString in NEP-fullInteger NEP-mixedInteger GNEP-fullInteger
    // implementationGame NEPdiscrete-Schwarze23
    string gameTypeString = "NEPdiscrete-Schwarze23";
    string algorithmVariant = "basicAlgorithm";

    // decide the cut used with respect to the game type
    string whichCutString = "eqCuts"; // for NEP
    if (gameTypeString == "GNEP-fullInteger" or gameTypeString == "implementationGame")
        whichCutString = "intersectionCuts"; // for GNEP

    // decide if the cut is local or global with the type of cut used
    bool globalCutSwitch = false;
    // equilibrium cuts are globally valid in the NEP setting
    if (whichCutString == "eqCuts")
        globalCutSwitch = true;
    if (whichCutString == "noGoodCuts")
        globalCutSwitch = true;

    bool enumerateNE = false;
    string modelTypeString = "manyEtasGurobi";
    string branchingRuleString = "mostFractional";
    double timeLimit = 3600;
    int verbosity = 3;
    string resultFilename = "noWriteResult";

    // specific setting given by argc and argv
    if (argc >= 2)
        filename = argv[1];
    else {
        // in case no filename was provided to the call to main
        try {
            filename = getFilenameInstance(gameTypeString);
        } catch (...) {
            // if getFilenameInstance throws an error, it means gameTypeString is
            // the name of an instance defined in testInstances.h
            filename = gameTypeString;
        }
    }
    if (argc >= 3) gameTypeString = argv[2];

    // build instance from instance file
    // filename = "testGNEPJulianICExample"; // for debug purpose
    shared_ptr<NashEquilibriumProblem> NEP = buildGameFromInstance(gameTypeString, filename);

    if (argc >= 4) algorithmVariant = argv[3];
    if (argc >= 5) whichCutString = argv[4];
    if (argc >= 6) globalCutSwitch = handleBoolErrorAsMainArgument(argv[5], 5);
    if (argc >= 7) enumerateNE = handleBoolErrorAsMainArgument(argv[6], 6);
    if (argc >= 8) modelTypeString = argv[7];
    if (argc >= 9) branchingRuleString = argv[8];
    if (argc >= 10) timeLimit = stod(argv[9]);
    if (argc >= 11) verbosity = stoi(argv[10]);
    if (argc >= 12) resultFilename = argv[11];

    // write arguments
    if (verbosity >= 1) {
        cout << "argc = " << argc << endl;
        for (int i = 0; i < argc; i++) {
            cout << "argv[" << i << "] = " << argv[i] << endl;
        }
    }

    try {
        auto branchingAlgorithm = BranchAndCutForGNEP(
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
        auto NEPOutput = branchingAlgorithm.solve();
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
