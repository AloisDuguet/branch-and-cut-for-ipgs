//
// Created by Aloïs Duguet on 2/20/25.
//

#include <string>
#include <fstream>
#include <cmath>

#include "BranchAndCutForGNEP.h"
#include "GameParserAndBuilder.h"
#include "BranchAndCutTools.h"

using namespace std;

bool launchSolve(string const& filename) {
    try {
        auto GNEP = createGNEPKnapsackInstance(filename, "fullInteger");
        bool globalCutSwitch = false;
        string modelTypeString = "manyEtasGurobi";
        string whichCutString = "intersectionCuts";
        bool enumerateNE = false;
        string algorithmVariant = "basicAlgorithm";
        string branchingRuleString = "mostFractional";
        double timeLimit = 1000;
        int verbosity = 0;
        string resultFilename = "toyResults.txt";
        auto branchingAlgorithm = BranchAndCutForGNEP(GNEP, "fullInteger",
            globalCutSwitch, modelTypeString, whichCutString,
            branchingRuleString, algorithmVariant,
            filename, enumerateNE, timeLimit, verbosity, resultFilename);
        auto NEPOutput = branchingAlgorithm.solve();
    }
    catch (std::logic_error const& e) { // almost useless catch statement for now
        cerr << "logic_error: " << e.what() << endl;
        if (static_cast<string>(e.what()) == "Due to a cut not cutting the point to cut, the algorithm will cycle")
            return true; // an instance producing the good output has been found, returning 1
    }
    catch (std::runtime_error const& e) { // almost useless catch statement for now
        // TODO: derive my own class from exception for input_error
        // such as two arguments that can't be handled at the same time
        // for now those errors are classified as runtime_error
        throw runtime_error(e.what());
    }

    return false;
}

void generateIntermediateKnapsackInstance(string fileInput,
                                          string fileOutput,
                                          int numberOfItems,
                                          indexx startingItem) {
    // generate an instance from knapsack instance fileInput to fileOutput
    // with numberOfItems items by copying items starting from startingItem
    // and readjusting capacity as in the original instance
}

void generateToyInstance(vector<int> itemsInside,
                         string const& filenameBase,
                         string const& filenameOutput,
                         int numberOfItems,
                         int initialNumberOfItems) {
    ifstream fileRead("../../../instances/knapsack_Dragotto23/" + filenameBase);
    string line,newLine;
    string temp;

    ofstream fileWrite("../../../instances/knapsack_Dragotto23/" + filenameOutput, ofstream::trunc);
    cout << "file to write: " << "../../../instances/knapsack_Dragotto23/" + filenameOutput << endl;

    // change number of item of first line
    getline(fileRead, line);
    istringstream iss(line);
    getline(iss,newLine,' ');
    fileWrite << newLine << " " << numberOfItems << endl;

    // copy second line
    getline(fileRead, line);
    fileWrite << line << endl;

    // copy only wanted next lines according to itemsInside
    for (indexx i = 0; i < initialNumberOfItems; ++i) {
        getline(fileRead, line);
        if (isValueInVector(i, itemsInside))
            fileWrite << line << endl;
    }

    fileRead.close();
    fileWrite.close();
}

void incrementSparseBinaryCount(vector<int> & items, int totalNumberOfItems) {
    indexx pos = static_cast<indexx>(items.size())-1;
    int maxValues = 0;
    while (true) {
        if (maxValues == items.size()) {
            items = {};
            break;
        }
        if (items[pos] == totalNumberOfItems - maxValues - 1) {
            maxValues += 1;
            pos = static_cast<indexx>(items.size()) - maxValues - 1;
        } else {
            items[pos] += 1;
            // then put all further values (blocked values) just after this one
            for (indexx j = 1; j <= maxValues; j++)
                items[pos+j] = items[pos] + j;
            break;
        }
    }

    // if incrementation is not possible, return empty vector
}

void testIncrement(int numberOfItems, int totalNumberOfItems) {
    vector<int> items = {};
    for (indexx i = 0; i < numberOfItems; i++) items.push_back(i);
    indexx count = 0;
    while (true) {
        incrementSparseBinaryCount(items, totalNumberOfItems);
        count++;
        // cout << count << endl;
        cout << "items: " << items << endl;
        if (items.empty())
            break;
    }
    cout << "final items: " << items << endl;
    cout << "final count: " << count << endl;
}

void iterativeSearchOfToyInstance(int numberOfItems,
                                  int initialNumberOfItems,
                                  string const& filenameBase) {
    // filenameBase is the instance of folder knapsack_Dragotto23 to modify
    // instance file written is in folder ../toy_knapsack_Dragotto23
    // relative to the knapsack_Dragotto23 folder

    // use binary representation of i to choose items
    // does not try all i for which there is not exactly numberOfItems 1 is the binary representation of i
    vector<int> itemsInside = {};
    for (indexx i = 0; i < numberOfItems; i++) itemsInside.push_back(i);

    for (indexx i = 0; i < pow(2, initialNumberOfItems); i++) {
        incrementSparseBinaryCount(itemsInside, initialNumberOfItems);
        if (itemsInside.empty())
            // stop iterations when all instances have been tested.
            // By convention it is found when itemsInside is empty.
            break;
        cout << "checking number " << i << " with items " << itemsInside << endl;
        string filenameOutput = "../toy_knapsack_Dragotto23/" + filenameBase.substr(0,filenameBase.size()-4)
            + "-" + to_string(numberOfItems) + ".txt";
        generateToyInstance(itemsInside, filenameBase, filenameOutput, numberOfItems, initialNumberOfItems);
        if (launchSolve(filenameOutput)) {
            cout << "instance found for " << numberOfItems << " items in filename " << filenameOutput << endl;
            cout  << "You should manually rename the instance" << endl;
            throw exception();
        }

    }
}
