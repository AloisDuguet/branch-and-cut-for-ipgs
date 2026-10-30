//
// Created by Aloïs Duguet on 3/5/25.
//

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <vector>

typedef int indexx;

using namespace std;

int generateKnapsackGameInstance(string const& instanceName, string & folderName, unsigned int seed, int playerNumber, int itemNumber, int maxCoef, int type, int ratioCapacityNumerator, int ratioCapacityDenominator) {
    // function generating one knapsack instance with arguments as characteristic
    srand(seed);

    // one element per player, which itself is a list of itemNumber values
    vector<vector<int>> profitsList = {};

    // same for weights
    vector<vector<int>> weightsList = {};

    // same for capacities
    vector<int> capacities = {};

    // generate all knapsack problem in relevant filenames
    for (indexx player = 0; player < playerNumber; player++) {
        profitsList.push_back({});
        weightsList.push_back({});
        // call to knapsack generator
        string commandLine = "./../InstanceGenerator/PisingerKnapsackGeneratorHarmlesslyModified "
        + to_string(itemNumber) + " "
        + to_string(maxCoef) + " "
        + to_string(type) + " "
        + to_string(ratioCapacityNumerator) + " "
        + to_string(ratioCapacityDenominator) + " "
        + to_string(seed);
        cout << "command: " << commandLine << endl;
        system(commandLine.data());

        // increment the seed so that all players don't have the same problem
        seed++;

        // parse test.in
        ifstream fileRead("test.in");
        string line,word;

        // line with seed
        getline(fileRead, line);

        // line with number of items
        getline(fileRead, line);

        // line with capacity
        getline(fileRead, line);

        istringstream iss(line);
        iss >> word; // capacity
        capacities.push_back(atoi(word.c_str()));
        for (indexx item = 0; item < itemNumber; item++) {
            getline(fileRead, line); // line with information of item item for player player
            istringstream iss(line);
            iss >> word; // index of item
            iss >> word; // profit of item
            profitsList[player].push_back(atoi(word.c_str()));
            iss >> word; // weight of item
            weightsList[player].push_back(atoi(word.c_str()));
        }
        fileRead.close();
    }

    // produce interaction coefficients for each item of each player of each other player
    vector<vector<vector<int>>> interactionCoefficientList = {};
    for (indexx player = 0; player < playerNumber; player++) {
        interactionCoefficientList.push_back({});
        for (indexx item = 0; item < itemNumber; item++) {
            interactionCoefficientList[player].push_back({});
            // produce one interaction coefficient for each other player per player per item
            for (indexx otherPlayer = 0; otherPlayer < playerNumber-1; otherPlayer++)
                // random between 1 and maxCoef (positive)
                interactionCoefficientList[player][item].push_back(rand()%(maxCoef)+1);
        }
    }

    // produce number of items available
    vector<int> itemsAvailable = {};
    for (indexx item = 0; item < itemNumber; item++)
        // rand between 1 and playerNumber
        itemsAvailable.push_back(rand()%playerNumber+1);

    // write instance with profitsList, weightsList, interaction coefficients and
    // items available to get one knapsack game
    if (folderName[folderName.size()-1] != '/')
        folderName = folderName + '/';
    string filename = folderName + instanceName + ".txt";
    ofstream fileWrite(filename);
    cout << "writing in filename " << filename << endl;
    fileWrite << playerNumber << " " << itemNumber << endl;
    for (indexx player = 0; player < playerNumber; player++)
        fileWrite << capacities[player] << " ";
    fileWrite << endl;
    // write item lines
    for (indexx item = 0; item < itemNumber; item++) {
        fileWrite << item << " ";
        for (indexx player = 0; player < playerNumber; player++)
            fileWrite << profitsList[player][item] << " " << weightsList[player][item] << " ";
        for (indexx player = 0; player < playerNumber; player++) {
            for (indexx otherPlayer = 0; otherPlayer < playerNumber-1; otherPlayer++)
                fileWrite << interactionCoefficientList[player][item][otherPlayer] << " ";
        }
        fileWrite << itemsAvailable[item];
        fileWrite << endl;
    }
    fileWrite.close();

    return seed;
}

int main() {
    // generate knapsack game instances from knapsack problems with Pisinger's generator

    int seed = 1234;
    int maxCoef = 100; // keep like that
    for (int playerNumber : {2,3,4}) {
        for (int itemNumber : {5,10,15,20,30,40,50,60,70,80}) {
            for (int type : {1,2,3}) {
                for (int ratioCapacityNumerator : {2,5,8}) {
                    int ratioCapacityDenominator = 9; // so that capacities are x tenth of the sum
                    string typeName;
                    switch (type) {
                        case 1:
                            typeName = "uncorr";
                            break;
                        case 2:
                            typeName = "weakcorr";
                            break;
                        case 3:
                            typeName = "strongcorr";
                            break;
                        default:
                            throw logic_error("unknown correlation type " + to_string(type));
                    }
                    int copies = 5;
                    for (int copyNumber = 0; copyNumber < copies; copyNumber++) {
                        // string foldername = "../../../instances/knapsack_instances_Pisinger/";
                        // string foldername = "../../../instances/NEP_knapsack_instances/";
                        string foldername = "../InstanceGenerator/generatedInstances/";
                        string instanceName = to_string(playerNumber) + "-" + to_string(itemNumber) + "-" + to_string(ratioCapacityNumerator) + "-" + typeName + "-" + to_string(copyNumber);
                        seed = generateKnapsackGameInstance(instanceName, foldername, seed, playerNumber, itemNumber, maxCoef, type, ratioCapacityNumerator, ratioCapacityDenominator);
                    }

                }
            }
        }
    }

    return 0;
}