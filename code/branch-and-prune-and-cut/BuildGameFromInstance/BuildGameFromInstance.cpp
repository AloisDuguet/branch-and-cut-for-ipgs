//
// Created by Aloïs Duguet on 7/8/25.
//

#include "GameParserAndBuilder.h"
#include "BuildGameFromInstance.h"

using namespace std;

shared_ptr<NashEquilibriumProblem> buildGameFromInstance(string const& gameTypeString, string const& filename) {
    shared_ptr<NashEquilibriumProblem> NEP;
    if (filename.find(".txt") != string::npos) {
        if (gameTypeString == "NEP-fullInteger")
            NEP = createNEPKnapsackInstance(filename, "fullInteger");
        else if (gameTypeString == "NEP-mixedInteger")
            NEP = createNEPKnapsackInstance(filename, "halfInteger");
        else if (gameTypeString == "GNEP-fullInteger")
            NEP = createGNEPKnapsackInstance(filename, "fullInteger");
        else if (gameTypeString == "implementationGame")
            NEP = createGNEPImplementationGameInstance(filename);
        else if (gameTypeString == "NEPImplementationGame")
            NEP = createNEPImplementationGameInstance(filename);
        else if (gameTypeString == "GNEP-maxFlowGame")
            NEP = createMaxFlowMinLoadCostInstance(filename);
        else if (gameTypeString == "NEPdiscrete-Schwarze23")
            NEP = createSchwarze23DiscreteNEPInstance(filename);
        else {
            throw runtime_error("Input error: unknown modelType '" + gameTypeString + "'");
        }
    } else if (filename == "testGNEPtoNEPJulianICExample") {
        NEP = testGNEPtoNEPJulianICExample();
    } else if (filename == "testGNEP1") {
        NEP = testGNEP1();
    } else if (filename == "testGNEP2") {
        NEP = testGNEP2();
    } else if (filename == "testGNEP3") {
        NEP = testGNEP3();
    } else if (filename == "testGNEPConforti") {
        NEP = testGNEPConforti();
    } else if (filename == "testGNEPJulianICExample") {
        NEP = testGNEPJulianICExample();
    } else if (filename == "testGNEPJulianICExampleModified") {
        NEP = testGNEPJulianICExampleModified();
    } else if (filename == "testInfiniteNEP") {
        NEP = testInfiniteNEP();
    } else if (filename == "testGNEPNegativeBounds") {
        NEP = testGNEPNegativeBounds();
    } else if (filename == "testBilevelCounterexample") {
        NEP = testBilevelCounterexample();
    } else {
        throw runtime_error("Input error: filename argument '" + filename + "' unknown");
    }
    return NEP;
}

string getFilenameInstance(string const& gameTypeString) {
    string filename;
    string folder_Dragotto = "../../../instances/knapsack_Dragotto23/";
    string folder_Pisinger = "../../../instances/knapsack_instances_Pisinger/";
    string folder_Pisinger_mini = "../../../instances/knapsack_instances_Pisinger_mini/";
    string folder_implementation_game = "../../../instances/implementation_games/";
    string folder_GNEP_knapsack = "../../../instances/GNEP_knapsack_instances/";
    string folder_NEP_knapsack = "../../../instances/NEP_knapsack_instances/";
    string folder_Schwarze23 = "../../../instances/Schwarze23DiscreteNEP/";
    if (gameTypeString == "NEP-fullInteger") {
        filename = folder_NEP_knapsack+"2-5-5-uncorr-1.txt";
        filename = folder_NEP_knapsack+"2-5-5-weakcorr-0.txt";
        // filename = folder_NEP_knapsack+"2-10-5-uncorr-0.txt";
        // filename = folder_NEP_knapsack+"4-20-2-uncorr-4.txt";
        filename = folder_NEP_knapsack+"2-15-5-uncorr-1.txt";
        // filename = folder_NEP_knapsack+"2-40-5-weakcorr-2.txt";
        filename = folder_Dragotto+"3-25-8-pot.txt";
        // filename = folder_NEP_knapsack+"2-50-2-weakcorr-0.txt";
        // filename = folder_NEP_knapsack+"2-30-5-uncorr-0.txt";
        // filename = folder_Dragotto+"2-25-5-cij-n.txt";
        // filename = folder_NEP_knapsack+"2-10-5-uncorr-0.txt";
        // filename = folder_NEP_knapsack+"4-50-5-uncorr-0.txt";
        // filename = folder_NEP_knapsack+"2-60-2-weakcorr-2.txt";
        // filename = folder_NEP_knapsack+"2-40-5-weakcorr-2.txt";
    }
    else if (gameTypeString == "NEP-mixedInteger") {
        filename = folder_NEP_knapsack+"2-5-5-uncorr-0.txt";
        // filename = folder_NEP_knapsack+"2-5-5-weakcorr-0.txt";
        // filename = folder_NEP_knapsack+"3-10-8-weakcorr-3.txt";
        // filename = folder_NEP_knapsack+"2-20-5-weakcorr-0.txt";
        // filename = folder_NEP_knapsack+"2-20-5-strongcorr-2.txt";
        filename = folder_NEP_knapsack+"4-5-2-weakcorr-1.txt";
        // filename = folder_NEP_knapsack+"4-20-2-uncorr-2.txt";
    }
    else if (gameTypeString == "GNEP-fullInteger") {
        filename = folder_GNEP_knapsack+"2-5-5-uncorr-0.txt";
        filename = folder_GNEP_knapsack+"4-50-2-strongcorr-8.txt";
        // filename = folder_GNEP_knapsack+"2-20-2-uncorr-1.txt";
        // filename = folder_GNEP_knapsack+"2-20-2-uncorr-2.txt";
        // filename = folder_GNEP_knapsack+"4-30-5-weakcorr-4.txt";
        // filename = folder_GNEP_knapsack+"4-15-5-weakcorr-7.txt";
        // filename = "testGNEPNegativeBounds";
    }
    else if (gameTypeString == "implementationGame") {
        filename = folder_implementation_game+"I_2_10_mm_10_1.txt";
        // filename = folder_implementation_game+"I_2_15_mm_1_6.txt";
        // filename = folder_implementation_game+"I_2_10_mm_10_10.txt"; // no NE exists
        // filename = folder_implementation_game+"I_2_10_ss_10_7.txt"; // no NE exists
        // filename = folder_implementation_game+"I_2_15_ss_10_2.txt"; // no NE exists
        filename = folder_implementation_game+"I_2_20_mm_10_62.txt"; // no NE exists
        // filename = folder_implementation_game+"I_2_20_mm_10_74.txt"; // no NE exists
        // filename = folder_implementation_game+"I_10_20_ss_10_5.txt";
        filename = folder_implementation_game+"I_2_15_ss_1_3.txt";
    } else if (gameTypeString == "NEPImplementationGame") {
        filename = folder_implementation_game+"I_2_10_mm_10_10.txt";
        // filename = folder_implementation_game+"I_2_10_mm_10_1.txt";
        filename = folder_implementation_game+"I_2_10_mm_10_10.txt";
        // filename = folder_implementation_game+"I_2_20_mm_10_62.txt";
        // filename = folder_implementation_game+"I_2_20_mm_10_26.txt";
        // filename = folder_implementation_game+"I_2_10_ss_10_7.txt";

        // filename = folder_implementation_game+"I_10_10_mm_10_10.txt";
        // filename = folder_implementation_game+"I_2_20_mm_10_1.txt";

        // filename = folder_implementation_game+"I_2_20_mm_10_34.txt";
        // filename = folder_implementation_game+"I_2_20_mm_10_11.txt";
        // filename = folder_implementation_game+"I_4_10_mm_10_1.txt";
    } else if (gameTypeString == "GNEP-maxFlowGame") {
        filename = folder_implementation_game+"I_2_10_mm_10_1.txt";
        // filename = folder_implementation_game+"I_2_15_mm_1_6.txt";
        filename = folder_implementation_game+"I_2_10_ss_10_7.txt";
        filename = folder_implementation_game+"I_2_20_mm_10_62.txt";
        filename = folder_implementation_game+"I_10_10_mm_10_4.txt";
        // filename = folder_implementation_game+"I_10_20_ss_10_5.txt";
        filename = folder_implementation_game+"I_4_15_mm_1_10.txt";
    } else if (gameTypeString == "NEPdiscrete-Schwarze23") {
        filename = folder_Schwarze23+"rg22_1.txt";
        filename = folder_Schwarze23+"RNC33_1.txt";
    }
    else {
        throw runtime_error("Input error: unknown modelType '" + gameTypeString + "'");
    }
    return filename;
}

vector<vector<double>> buildMultiplicativeAdditiveApproxVector(string& s) {
    // split string around '-'
    string s1,s2;
    stringstream ss(s);
    std::getline(ss, s1, '-');
    std::getline(ss, s2, '-');

    // fulfill multiplicative part
    string temp;
    vector<double> multVec = {};
    stringstream ss1(s1);
    while (std::getline(ss1, temp, ',')) {
        multVec.push_back(stod(temp));
    }

    // fulfill additive part
    vector<double> addVec = {};
    stringstream ss2(s2);
    while (std::getline(ss2, temp, ',')) {
        addVec.push_back(stod(temp));
    }
    return {multVec, addVec};
}
