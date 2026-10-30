//
// Created by Aloïs Duguet on 2/20/25.
//

#include "GameParserAndBuilder.h"

#include <complex>
#include <fstream>

#include "TestInstances.h"
#include "BranchAndCutTools.h"
#include "LipschitzComputation.h"

using namespace std;

NEPKnapsackInstance parseNEPKnapsack(string const& filename) {
    // parser for instance file with an NEP knapsack instance from Dragotto23

    ifstream file(filename);
    if (file.fail())
        throw runtime_error("this file could not be opened: " + filename);
    string line;
    string temp;

    // parse first line to get number of players and items
    getline(file, line);
    istringstream iss(line);
    getline(iss,temp,' ');
    int numberOfPlayers = stoi(temp);
    getline(iss,temp,' ');
    int numberOfItems = stoi(temp);

    // declaration of attributes of NEPKnapsackInstance:
    vector<int> capacities;
    vector<vector<int>> profits;
    vector<vector<int>> weights;
    vector<vector<vector<int>>> interactionCoefficients;
    vector<int> itemsAvailable;

    // first initialize each value with 0
    for (int player = 0; player < numberOfPlayers; player++) {
        capacities.push_back(0);
        profits.push_back({});
        weights.push_back({});
        interactionCoefficients.push_back({});
        for (int item = 0; item < numberOfItems; item++) {
            profits[player].push_back(0);
            weights[player].push_back(0);
        }
        for (int player2 = 0; player2 < numberOfPlayers; player2++) {
            interactionCoefficients[player].push_back({});
            if (player != player2) {
                for (int item = 0; item < numberOfItems; item++)
                    interactionCoefficients[player][player2].push_back(0);
            }
        }
    }
    for (indexx item = 0; item < numberOfItems; item++)
        itemsAvailable.push_back(0);

    // parse second line for capacities
    getline(file, line);
    vector<string> strCapacities = splitString(line);
    for (int player = 0; player < numberOfPlayers; player++)
        capacities[player] = stoi(strCapacities[player]);

    // parse each other line in a for loop
    for (int item = 0; item < numberOfItems; item++) {
        getline(file, line);
        // line gives those infos for three players and item 0:
        // item_number p^1_0	w^1_0	p^2_0	w^2_0	p^3_0	w^3_0	C^1_20	C^1_30	C^2_10	C^2_30	C^3_10	C^3_20
        // with p^1_0 the profit of player 1 with item 0
        //      w^1_0 the weight of player 1 with item 0
        //      C^1_20 the interaction coefficient of player 1 w.r.t. player 2 and item 0
        vector<string> parsedLine = splitString(line);

        // first info (item_number) is already known with i
        int posInfo = 1;

        // get profits and weights
        for (int player = 0; player < numberOfPlayers; player++) {
            profits[player][item] = stoi(parsedLine[posInfo++]);
            weights[player][item] = stoi(parsedLine[posInfo++]);
        }
        // get interaction coefficients
        for (int player = 0; player < numberOfPlayers; player++) {
            for (int player2 = 0; player2 < numberOfPlayers; player2++) {
                if (player2 != player)
                    interactionCoefficients[player][player2][item] = stoi(parsedLine[posInfo++]);
            }
        }
        // get number of copies of the item available
        if (parsedLine.size() == posInfo+1) {
            // case in which the number of items available among
            // all the players is defined, contrarily to Dragotto's instances
            itemsAvailable[item] = stoi(parsedLine[posInfo++]);
        }
    }

    NEPKnapsackInstance instance =
        {numberOfPlayers,
        numberOfItems,
        capacities,
        profits,
        weights,
        interactionCoefficients,
        itemsAvailable};

    return instance;
}

shared_ptr<NashEquilibriumProblem> createNEPKnapsackInstance(
        string const& filename,
        string const& optionIntegrality) {
    cout << "building model of NEP knapsack game" << endl;

    // parse filename
    NEPKnapsackInstance const& instance = parseNEPKnapsack(filename);

    int numberOfPlayers = instance.numberOfPlayers;
    int numberOfItems = instance.numberOfItems;

    vector<int> numberOfVariablesPerPlayer = {};
    for (int player = 0; player < numberOfPlayers; player++)
        numberOfVariablesPerPlayer.push_back(numberOfItems);

    // initialize vector of optimizationProblem and other things
    vector<shared_ptr<OptimizationProblem>> bestResponseProblems = {};
    int countNumberOfVariables = 0;

    // for each player, build OptimizationProblem
    for (int player = 0; player < numberOfPlayers; player++) {
        // build c, multiply all coefficients by -1 because
        // it is a maximization problem but we will minimize it
        vector<double> cValues = {};
        vector<indexx> cIndices = {};
        for (int item = 0; item < numberOfItems; item++) {
            // minus sign for maximization
            cValues.push_back(-instance.profits[player][item]);
            cIndices.push_back(countNumberOfVariables++);
        }
        MySparseVector c = {cValues, cIndices};

        // build Q
        vector<double> QValues = {};
        vector<indexx> QRowIndices = {};
        vector<indexx> QColIndices = {};
        for (int player2 = 0; player2 < numberOfPlayers; player2++) {
            if (player2 != player) {
                for (int item = 0; item < numberOfItems; item++) {
                    // minus sign for maximization
                    QValues.push_back(-instance.interactionCoefficients[player][player2][item]);
                    QRowIndices.push_back(numberOfItems*player+item);
                    QColIndices.push_back(numberOfItems*player2+item);
                }
            }
        }
        MySparseMatrix Q = {QValues, QRowIndices, QColIndices};

        // build constraints
        vector<Constraint> constraints = {};
        // constraints for bounds of binary vectors
        for (int item = 0; item < numberOfItems; item++) {
            constraints.push_back({{{1.0},
                                            {numberOfItems*player+item}},
                                            {},
                                            1.0});
            constraints.push_back({{{-1.0},
                                            {numberOfItems*player+item}},
                                            {},
                                            0.0});
        }
        // capacity constraint and integralityConstraints
        vector<double> weightValues = {};
        vector<indexx> capacityIndices = {};
        for (int item = 0; item < numberOfItems; item++) {
            weightValues.push_back(instance.weights[player][item]);
            capacityIndices.push_back(numberOfItems*player+item);
        }
        constraints.push_back({{weightValues, capacityIndices},
                                        {},
                                        static_cast<double>(instance.capacities[player])});

        // build OptimizationProblem, reuse capacityIndices as integrality variable indices
        vector<indexx> integralityIndices = {};
        if (optionIntegrality == "fullInteger") {
            // all variables are integer
            integralityIndices = capacityIndices;
        }
        else if (optionIntegrality == "halfInteger") {
            // all even variable indices are integer, so approximately half of them
            for (indexx i = 0; i < capacityIndices.size(); i += 2)
                integralityIndices.push_back(capacityIndices[i]);
        }
        bestResponseProblems.push_back(make_shared<OptimizationProblem>(
            numberOfVariablesPerPlayer[player],
            c,
            Q,
            constraints,
            integralityIndices));
    }

    // instantiate NEP
    shared_ptr<NashEquilibriumProblem> NEP =
        make_shared<NashEquilibriumProblem>(bestResponseProblems);

    return NEP;
}

shared_ptr<NashEquilibriumProblem> createNEPKnapsackInstanceWithQuadraticTermsReformulated(
        string const& filename) {
    cout << "building model of NEP knapsack game with binary quadratic"
            " terms reformulated, thus producing a GNEP" << endl;

    // parse filename
    NEPKnapsackInstance const& instance = parseNEPKnapsack(filename);

    int numberOfPlayers = instance.numberOfPlayers;
    int numberOfItems = instance.numberOfItems;

    vector<int> numberOfVariablesPerPlayer = {};
    int additionalVariablesPerPlayer = numberOfItems*(numberOfPlayers-1);
    int variablesPerPlayer = numberOfItems*numberOfPlayers;
    for (int player = 0; player < numberOfPlayers; player++) {
        numberOfVariablesPerPlayer.push_back(numberOfItems+additionalVariablesPerPlayer);
    }

    // initialize vector of optimizationProblem and other things
    vector<shared_ptr<OptimizationProblem>> bestResponseProblems = {};
    int countNumberOfVariables = 0;

    // for each player, build OptimizationProblem
    for (int player = 0; player < numberOfPlayers; player++) {
        // build reformulation of quadratic terms Q into additional
        // continuous variables, linear objective terms and
        // linear constraints mixing variables
        // of two players (thus it creates a GNEP)

        // build c, multiply all coefficients by -1 because
        // it is a maximization problem but we will minimize it
        vector<double> cValues = {};
        vector<indexx> cIndices = {};
        for (int item = 0; item < numberOfItems; item++) {
            // minus sign for maximization
            cValues.push_back(-instance.profits[player][item]);
            cIndices.push_back(countNumberOfVariables++);
        }
        // adding linear terms coming from the reformulation
        for (int player2 = 0; player2 < numberOfPlayers; player2++) {
            if (player2 != player) {
                for (int item = 0; item < numberOfItems; item++) {
                    // minus sign for maximization
                    cValues.push_back(-instance.interactionCoefficients[player][player2][item]);
                    cIndices.push_back(countNumberOfVariables++);
                }
            }
        }
        MySparseVector c = {cValues, cIndices};

        // build constraints
        vector<Constraint> constraints = {};
        // constraints for bounds of binary vectors
        for (int item = 0; item < numberOfItems; item++) {
            constraints.push_back({{{1.0},
                                            {variablesPerPlayer*player+item}},
                                            {},
                                            1.0});
            constraints.push_back({{{-1.0},
                                            {variablesPerPlayer*player+item}},
                                            {},
                                            0.0});
        }
        // capacity constraint and integralityConstraints
        vector<double> weightValues = {};
        vector<indexx> capacityIndices = {};
        for (int item = 0; item < numberOfItems; item++) {
            weightValues.push_back(instance.weights[player][item]);
            capacityIndices.push_back(variablesPerPlayer*player+item);
        }
        // four constraints for the reformulation of each quadratic term (by variable z for the clarity)
        // z <= x
        // z <= y
        // z >= 0
        // z >= x + y - 1
        int countAdditionalVariable = 0;
        for (int player2 = 0; player2 < numberOfPlayers; player2++) {
            if (player2 != player) {
                for (int item = 0; item < numberOfItems; item++) {
                    indexx zVariable = variablesPerPlayer*player+numberOfItems+countAdditionalVariable;
                    indexx xVariable = variablesPerPlayer*player+item;
                    indexx yVariable = variablesPerPlayer*player2+item;
                    constraints.push_back({{
                            {1,-1},
                            {zVariable, xVariable},
                        }, {}, 0});
                    constraints.push_back({{
                            {1,-1},
                            {zVariable,yVariable}
                        },{}, 0});
                    constraints.push_back({{
                            {-1},
                            {zVariable}
                        }, {}, 0});
                    constraints.push_back({{
                            {-1,1,1},
                            {zVariable, xVariable, yVariable}
                        }, {}, 1});
                    countAdditionalVariable++;
                }
            }
        }
        constraints.push_back({{weightValues, capacityIndices},
                                        {},
                                        static_cast<double>(instance.capacities[player])});

        // build OptimizationProblem, reuse capacityIndices as integrality variable indices
        bestResponseProblems.push_back(make_shared<OptimizationProblem>(
            numberOfVariablesPerPlayer[player],
            c,
            MySparseMatrix({},{},{}),
            constraints,
            capacityIndices));
    }

    // instantiate NEP
    shared_ptr<NashEquilibriumProblem> NEP =
        make_shared<NashEquilibriumProblem>(bestResponseProblems);

    return NEP;
}

shared_ptr<NashEquilibriumProblem> createGNEPKnapsackInstance(
    string const& filename,
    string const& optionIntegrality) {
    cout << "building model of GNEP knapsack game" << endl;

    // parse filename
    NEPKnapsackInstance const& instance = parseNEPKnapsack(filename);

    int numberOfPlayers = instance.numberOfPlayers;
    int numberOfItems = instance.numberOfItems;

    vector<int> numberOfVariablesPerPlayer = {};
    for (int player = 0; player < numberOfPlayers; player++)
        numberOfVariablesPerPlayer.push_back(numberOfItems);

    // initialize vector of optimizationProblem and other things
    vector<shared_ptr<OptimizationProblem>> bestResponseProblems = {};
    int countNumberOfVariables = 0;

    // for each player, build OptimizationProblem
    for (int player = 0; player < numberOfPlayers; player++) {
        // build c, multiply all coefficients by -1 because
        // it is a maximization problem but we will minimize it
        vector<double> cValues = {};
        vector<int> cIndices = {};
        for (int item = 0; item < numberOfItems; item++) {
            // minus sign for maximization
            cValues.push_back(-instance.profits[player][item]);
            cIndices.push_back(countNumberOfVariables++);
        }
        MySparseVector c = {cValues, cIndices};

        // build Q
        MySparseMatrix Q = {{}, {}, {}};

        // build constraints
        vector<Constraint> constraints = {};
        // constraints for bounds of binary vectors
        for (int item = 0; item < numberOfItems; item++) {
            constraints.push_back({{{1.0},
                                            {numberOfItems*player+item}},
                                            {},
                                            1.0});
            constraints.push_back({{{-1.0},
                                            {numberOfItems*player+item}},
                                            {},
                                            0.0});
        }
        // constraints forcing an item to be taken by maximum its number of copies
        for (int item = 0; item < numberOfItems; item++) {
            vector<double> ones = {};
            vector<indexx> sameItemVariables = {};
            for (indexx j = 0; j < numberOfPlayers; j++) {
                ones.push_back(1.0);
                sameItemVariables.push_back(numberOfItems*j+item);
            }
            MySparseVector v = {ones, sameItemVariables};
            constraints.push_back({v,
                                       {},
                                       static_cast<double>(instance.itemsAvailable[item])});
        }
        // capacity constraint
        vector<double> weightValues = {};
        vector<int> capacityIndices = {};
        for (int item = 0; item < numberOfItems; item++) {
            weightValues.push_back(instance.weights[player][item]);
            capacityIndices.push_back(numberOfItems*player+item);
        }
        constraints.push_back({{weightValues, capacityIndices},
                               {},
                               static_cast<double>(instance.capacities[player])});

        // build OptimizationProblem, reuse capacityIndices
        // as integrality variable indices
        vector<indexx> integralityIndices = {};
        if (optionIntegrality == "fullInteger") {
            // all variables are integer
            integralityIndices = capacityIndices;
        }
        else if (optionIntegrality == "halfInteger") {
            // all even variable indices are integer, so approximately half of them
            for (indexx i = 0; i < capacityIndices.size(); i += 2)
                integralityIndices.push_back(capacityIndices[i]);
        }
        bestResponseProblems.push_back(
            make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[player],
                                              c,
                                              Q,
                                              constraints,
                                              integralityIndices));
    }

    // instantiate GNEP
    shared_ptr<NashEquilibriumProblem> GNEP =
        make_shared<NashEquilibriumProblem>(bestResponseProblems);

    return GNEP;
}

GNEPImplementationGameInstance parseImplementationGameGNEP(string const& filename) {
    ifstream file(filename);

    if (file.fail())
        throw runtime_error("this file could not be opened: " + filename);

    string line;
    string temp;

    // parse filename to get number of players
    int startPos = filename.find_last_of("/");
    string instanceName = filename.substr(startPos+1);
    int endPos = instanceName.find("_", 2);
    int numberOfPlayers = stoi(instanceName.substr(2,endPos-2));

    // get number of nodes and edges of network
    getline(file, line);
    istringstream iss(line);
    getline(iss,temp,' '); // "size"
    getline(iss,temp,' '); // number of nodes
    int numberOfNodes = stoi(temp);
    getline(iss,temp,' '); // number of edges
    int numberOfEdges = stoi(temp);
    getline(iss,temp,' '); // "nnz"
    getline(iss,temp,' '); // number of nonzeros in network
    int nnzNetwork = stoi(temp);
    getline(iss,temp,' '); // "value"

    // declare and fill in indices lists and values of the sparse representation of network
    vector<indexx> rowIndexList = {};
    for (indexx nnzIndex = 0; nnzIndex < nnzNetwork; nnzIndex++) {
        getline(iss,temp,' ');
        rowIndexList.push_back(stoi(temp));
    }
    vector<indexx> colIndexList = {};
    for (indexx nnzIndex = 0; nnzIndex < nnzNetwork; nnzIndex++) {
        getline(iss,temp,' ');
        colIndexList.push_back(stoi(temp));
    }
    vector<double> values = {};
        for (indexx nnzIndex = 0; nnzIndex < nnzNetwork; nnzIndex++) {
            getline(iss,temp,' ');
            values.push_back(stod(temp));
        }
    // fill in a nonsparse representation of network
    MySparseMatrix network = {};
    for (indexx nnzIndex = 0; nnzIndex < nnzNetwork; nnzIndex++) {
        // -1 because matlab starts indices at 1
        network.addCoefficient(values[nnzIndex],
                               rowIndexList[nnzIndex]-1,
                               colIndexList[nnzIndex]-1);
    }

    getline(file, line);
    vector<vector<double>> capacities = parseFlattenedMatrix(line);
    getline(file, line);
    // remove " 1" in positions 4:5 to have a matrix2D instead of a matrix3D
    line = line.substr(0,4) + line.substr(6,string::npos);
    vector<vector<double>> sourceSinks = parseFlattenedMatrix(line, true);
    // -1 to all elements of sourceSinks because matlab starts indices at 1
    for (indexx rowIndex = 0; rowIndex < sourceSinks.size(); rowIndex++) {
        for (indexx colIndex = 0; colIndex < sourceSinks[rowIndex].size(); colIndex++) {
            sourceSinks[rowIndex][colIndex] -= 1;
        }
    }
    getline(file, line);
    vector<vector<double>> U = parseFlattenedMatrix(line);
    getline(file, line);
    vector<double> demands = parseFlattenedVector(line);
    getline(file, line);
    vector<double> pMax = parseFlattenedVector(line);
    getline(file, line);
    vector<double> u = parseFlattenedVector(line);

    return GNEPImplementationGameInstance(
        {numberOfPlayers,
            numberOfNodes,
            numberOfEdges,
            network,
            capacities,
            sourceSinks,
            U,
            demands,
            pMax,
            u});
}

Schwarze23Instance parseSchwarze23DiscreteNEPInstance(string const& filename) {
    ifstream file(filename);

    if (file.fail())
        throw runtime_error("this file could not be opened: " + filename);

    string line;

    // parse filename to get convexity, number of players, number of variables per player and instance index
    int startPos = static_cast<int>(filename.find_last_of("/"));
    string instanceName = filename.substr(startPos+1);

    bool convexity = false;
    int numberOfPlayers = 0;
    int numberOfVariablesPerPlayer = 0;

    // two different formats: with one or two '_'
    // only one '_':
    if (count(filename.begin(), filename.end(), '_') == 1) {
        if (instanceName.substr(0,2) == "rg") {
            convexity = true;
            numberOfPlayers = stoi(instanceName.substr(2,1));
            numberOfVariablesPerPlayer = stoi(instanceName.substr(3,1));
        } else {
            convexity = false;
            numberOfPlayers = stoi(instanceName.substr(3,1));
            numberOfVariablesPerPlayer = stoi(instanceName.substr(4,1));
        }
    } else if (count(filename.begin(), filename.end(), '_') == 2) {
        int firstUnderscorePosition = static_cast<int>(instanceName.find_first_of("_"));
        int secondUnderscorePosition = static_cast<int>(instanceName.find_last_of("_"));
        int firstNumberPosition = 0;
        if (instanceName.substr(0,2) == "rg") {
            firstNumberPosition = 2;
            convexity = true;
        } else {
            firstNumberPosition = 3;
            convexity = false;
        }
        numberOfPlayers = stoi(instanceName.substr(firstNumberPosition,
                                                      firstUnderscorePosition-startPos));
        numberOfVariablesPerPlayer = stoi(instanceName.substr(firstUnderscorePosition+1,
                                                                 secondUnderscorePosition-firstUnderscorePosition-1));
    } else {
        throw logic_error("unrecognised filename format: "+filename);
    }


    // prepare containers
    vector<vector<vector<double>>> constraintMatrices;
    vector<vector<double>> constraintB;
    vector<vector<vector<double>>> bounds;
    vector<vector<vector<double>>> objectiveC;
    vector<vector<vector<double>>> objectiveQ;
    vector<vector<double>> objectiveB;
    vector<vector<bool>> costsConvexity;

    // parse optimization problems
    for (indexx player = 0; player < numberOfPlayers; player++) {
        // forget about first line which is a comment
        getline(file, line);
        getline(file, line);
        constraintMatrices.push_back(parseFlattenedMatrix(line));
        getline(file, line);
        constraintB.push_back(parseFlattenedVector(line));
        getline(file, line);
        bounds.push_back(parseFlattenedMatrix(line));
        getline(file, line);
        objectiveC.push_back(parseFlattenedMatrix(line));
        getline(file, line);
        objectiveQ.push_back(parseFlattenedMatrix(line));
        getline(file, line);
        objectiveB.push_back(parseFlattenedVector(line));
        getline(file, line);
        auto tempVector = parseFlattenedVector(line);
        for (indexx p = 0; p < numberOfPlayers; p++) {
            costsConvexity.push_back({});
            if (tempVector[p] == 1)
                costsConvexity[player].push_back(true);
            else
                costsConvexity[player].push_back(false);
        }
    }

    return Schwarze23Instance(
        {numberOfPlayers,
        numberOfVariablesPerPlayer,
        costsConvexity,
        constraintMatrices,
        constraintB,
        bounds,
        objectiveC,
        objectiveQ,
        objectiveB});
}

shared_ptr<NashEquilibriumProblem> createSchwarze23DiscreteNEPInstance(
        string const& filename) {
    auto [numberOfPlayers,
    nVarPerPlayer,
    costsConvexity,
    constraintMatrices,
    constraintB,
    bounds,
    objectiveC,
    objectiveQ,
    objectiveB] = parseSchwarze23DiscreteNEPInstance(filename);

    vector<int> numberOfVariablesPerPlayer = {};
    for (int player = 0; player < numberOfPlayers; player++) {
        numberOfVariablesPerPlayer.push_back(nVarPerPlayer);
    }

    // initialize vector of optimizationProblem and other things
    vector<shared_ptr<OptimizationProblem>> bestResponseProblems = {};

    for (indexx player = 0; player < numberOfPlayers; player++) {
        // build c from objectiveB
        vector<double> cValues = {};
        vector<indexx> cIndices = {};
        for (indexx variableIndex = 0; variableIndex < nVarPerPlayer; variableIndex++) {
            if (objectiveB[player][variableIndex] != 0) {
                cValues.push_back(objectiveB[player][variableIndex]);
                cIndices.push_back(player*nVarPerPlayer+variableIndex);
            }
        }
        MySparseVector c = {cValues, cIndices};

        // build Q from objectiveC and objectiveQ
        vector<double> QValues = {};
        vector<indexx> QRowIndices = {};
        vector<indexx> QColIndices = {};
        // contributions from objectiveC (bilinear terms: player var times other player var)
        for (indexx rowIndex = 0; rowIndex < nVarPerPlayer; rowIndex++) {
            // loop on players with a smaller index than current player
            for (indexx p = 0; p < player; p++) {
                for (indexx colIndex = 0; colIndex < nVarPerPlayer; colIndex++) {
                    auto realColIndex = p*nVarPerPlayer+colIndex;
                    auto val = objectiveC[player][rowIndex][realColIndex];
                    if (val != 0) {
                        QValues.push_back(val);
                        QRowIndices.push_back(player*nVarPerPlayer+rowIndex);
                        QColIndices.push_back(realColIndex);
                    }
                }
            }
            // then loop on later players
            for (indexx p = player+1; p < numberOfPlayers; p++) {
                for (indexx colIndex = 0; colIndex < nVarPerPlayer; colIndex++) {
                    auto realColIndex = p*nVarPerPlayer+colIndex;
                    // remove number of variables of player when reading objectiveC:
                    auto val = objectiveC[player][rowIndex][realColIndex-nVarPerPlayer];
                    if (val != 0) {
                        QValues.push_back(val);
                        QRowIndices.push_back(player*nVarPerPlayer+rowIndex);
                        QColIndices.push_back(realColIndex);
                    }
                }
            }
        }
        // contributions from objectiveQ (quadratic terms for the player)
        for (indexx rowIndex = 0; rowIndex < nVarPerPlayer; rowIndex++) {
            for (indexx colIndex = 0; colIndex < nVarPerPlayer; colIndex++) {
                if (objectiveQ[player][rowIndex][colIndex] != 0) {
                    QValues.push_back(0.5*objectiveQ[player][rowIndex][colIndex]);
                    QRowIndices.push_back(player*nVarPerPlayer+rowIndex);
                    QColIndices.push_back(player*nVarPerPlayer+colIndex);
                }
            }
        }
        MySparseMatrix Q = {QValues, QRowIndices, QColIndices};

        // build constraints from constraintMatrices, constraintB and bounds
        vector<Constraint> constraints = {};
        // first from constraintMatrices and constraintB
        int numberConstraints = static_cast<int>(constraintMatrices[player].size());
        for (indexx constraintIndex = 0; constraintIndex < numberConstraints; constraintIndex++) {
            vector<double> constraintValues = {};
            vector<indexx> constraintIndices = {};
            int bonusPlayerIndex = player*nVarPerPlayer;
            for (indexx variableIndex = 0; variableIndex < nVarPerPlayer; variableIndex++) {
                auto val = constraintMatrices[player][constraintIndex][variableIndex];
                if (val != 0) {
                    constraintValues.push_back(val);
                    constraintIndices.push_back(variableIndex+bonusPlayerIndex);
                }
            }
            constraints.push_back(
                {{constraintValues,constraintIndices},
                {},
                constraintB[player][constraintIndex]});
        }
        // second from bounds
        for (indexx variableIndex = 0; variableIndex < nVarPerPlayer; variableIndex++) {
            auto realVariableIndex = player*nVarPerPlayer+variableIndex;
            constraints.push_back({{{-1},{realVariableIndex}},{},-bounds[player][variableIndex][0]});
            constraints.push_back({{{1},{realVariableIndex}},{},bounds[player][variableIndex][1]});
        }

        // integrality variable indices
        vector<indexx> integralityIndices = {};
        for (indexx varIndex = player*nVarPerPlayer; varIndex < (player+1)*nVarPerPlayer; varIndex++)
            integralityIndices.push_back(varIndex);

        // build OptimizationProblem
        bestResponseProblems.push_back(
            make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[player],
                                               c,
                                               Q,
                                               constraints,
                                               integralityIndices));
    }

    // build NEP
    shared_ptr<NashEquilibriumProblem> NEP = make_shared<NashEquilibriumProblem>(bestResponseProblems);

    return NEP;
}

double computeLocalLipschitzConstantImplementationGame(
        indexx player,
        GNEPImplementationGameInstance const& gameInstance,
        vector<shared_ptr<OptimizationProblem>> const& bestResponseProblems,
        vector<double> const& point,
        double const numerator) {
    auto [numberOfPlayers,
        numberOfNodes,
        numberOfEdges,
        network,
        capacities,
        sourceSinks,
        U,
        demands,
        pMax,
        u] = gameInstance;

    double lipschitzConstant;
    double ballSize = 0.5;
    do {
        ballSize *= 2;
        lipschitzConstant =
            computeLipschitzConstantImplementationGame(
                player,
                bestResponseProblems[player],
                numberOfEdges,
                numberOfPlayers,
                U,
                capacities,
                pMax,
                u,
                "local",
                point,
                ballSize);
        // cout << "lipschitz: " << lipschitzConstant << " with ballSize: " << ballSize << endl;
    }
    while (numerator / (1+lipschitzConstant) > ballSize);

    return lipschitzConstant;
}

shared_ptr<NashEquilibriumProblem> createGNEPImplementationGameInstance(string const& filename) {
    auto [numberOfPlayers,
    numberOfNodes,
    numberOfEdges,
    network,
    capacities,
    sourceSinks,
    U,
    demands,
    pMax,
    u] = parseImplementationGameGNEP(filename);


    vector<int> numberOfVariablesPerPlayer = {};
    for (int player = 0; player < numberOfPlayers; player++) {
        // for ordinary players (x_player,e and z_player)
        numberOfVariablesPerPlayer.push_back(numberOfEdges+1);
    }
    // for the central authority (p_e)
    numberOfVariablesPerPlayer.push_back(numberOfEdges);

    // initialize vector of optimizationProblem and other things
    vector<shared_ptr<OptimizationProblem>> bestResponseProblems = {};

    // +1 due to binary variable z_i
    int startVariableCentralAuthority = numberOfPlayers*(numberOfEdges+1);

    // for each player, build OptimizationProblem
    for (indexx player = 0; player < numberOfPlayers; player++) {
        vector<double> cValues = {};
        vector<indexx> cIndices = {};
        for (indexx edge = 0; edge < numberOfEdges; edge++) {
            cValues.push_back(-U[edge][player]);
            cIndices.push_back((numberOfEdges+1)*player+edge);
        }
        MySparseVector c = {cValues, cIndices};

        // build Q
        vector<double> QValues = {};
        vector<indexx> QRowIndices = {};
        vector<indexx> QColIndices = {};
        for (int edge = 0; edge < numberOfEdges; edge++) {
            QValues.push_back(1);
            QRowIndices.push_back((numberOfEdges+1)*player+edge);
            QColIndices.push_back(startVariableCentralAuthority+edge);
        }
        MySparseMatrix Q = {QValues, QRowIndices, QColIndices};

        // build constraints
        vector<Constraint> constraints = {};
        // positivity of flows
        for (indexx edge = 0; edge < numberOfEdges; edge++) {
            // -x_i,e <= 0 (x_i,e >= 0)
            constraints.push_back({{{-1},{(numberOfEdges+1)*player+edge}},
                                       {},
                                       0});
        }

        // flow polyhedron
        // declare all parts of the flow polyhedron constraints,
        // then update them with nnz of network and sourceSinks
        vector<vector<double>> vectorValues = {};
        vector<vector<indexx>> vectorIndices = {};
        vector<double> vectorB = {};
        for (indexx node = 0; node < numberOfNodes; node++) {
            // declare all parts of the flow polyhedron constraints
            vectorValues.push_back({});
            vectorIndices.push_back({});
            vectorB.push_back(0);
        }
        for (indexx nnzNetwork = 0; nnzNetwork < network.size(); nnzNetwork++) {
            // update with nnz
            double value = network.getValue(nnzNetwork);
            indexx node = network.getRowIndex(nnzNetwork);
            indexx edge = network.getColIndex(nnzNetwork);
            vectorValues[node].push_back(value);
            vectorIndices[node].push_back((numberOfEdges+1)*player+edge);
        }
        // adding demands of the source and sink
        vectorB[sourceSinks[player][0]] = demands[player];
        vectorB[sourceSinks[player][1]] = -demands[player];
        // adding contribution in z_i
        vectorValues[sourceSinks[player][0]].push_back(demands[player]);
        vectorIndices[sourceSinks[player][0]].push_back((numberOfEdges+1)*player+numberOfEdges);
        vectorValues[sourceSinks[player][1]].push_back(-demands[player]);
        vectorIndices[sourceSinks[player][1]].push_back((numberOfEdges+1)*player+numberOfEdges);
        // generating constraints
        for (indexx node = 0; node < numberOfNodes; node++) {
            MySparseVector sparseVector = {vectorValues[node],vectorIndices[node]};
            constraints.push_back({sparseVector,{},vectorB[node]});
        }

        // capacity per edge
        for (indexx edge = 0; edge < numberOfEdges; edge++) {
            vector<double> values = {};
            vector<indexx> indices = {};
            // we take column of index 'player' but all are the same
            double b = capacities[edge][player];
            for (indexx p = 0; p < numberOfPlayers; p++) {
                values.push_back(1);
                indices.push_back((numberOfEdges+1)*p+edge);
            }
            MySparseVector sparseVector = {values, indices};
            constraints.push_back({sparseVector,{},b});
        }

        // {0} is a feasible strategy (thanks to binary variable z_i forcing x_i to zero if z_i == 0)
        // z_i >= 0
        constraints.push_back({{{-1},{(numberOfEdges+1)*player+numberOfEdges}},{},0});

        // z_i <= 1
        constraints.push_back({{{1},{(numberOfEdges+1)*player+numberOfEdges}},{},1});

        for (indexx edge = 0; edge < numberOfEdges; edge++) {
            double M = capacities[edge][player]; // big-M value
            MySparseVector sparseVector =
                {{1,M},{(numberOfEdges+1)*player+edge,(numberOfEdges+1)*player+numberOfEdges}};
            constraints.push_back({sparseVector,{},M});
        }

        // build OptimizationProblem after integrality variable indices
        vector<indexx> integralityIndices = {};
        for (indexx varIndex = (numberOfEdges+1)*player; varIndex < (numberOfEdges+1)*(player+1); varIndex++)
            integralityIndices.push_back(varIndex);
        bestResponseProblems.push_back(
            make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[player],
                                               c,
                                               Q,
                                               constraints,
                                               integralityIndices));
    }

    // build optimizationProblem of central authority
    vector<double> cValues = {};
    vector<indexx> cIndices = {};
    for (int edge = 0; edge < numberOfEdges; edge++) {
        cValues.push_back(u[edge]);
        cIndices.push_back(startVariableCentralAuthority+edge);
    }
    MySparseVector c = {cValues, cIndices};

    // build Q
    vector<double> QValues = {};
    vector<indexx> QRowIndices = {};
    vector<indexx> QColIndices = {};
    for (indexx edge = 0; edge < numberOfEdges; edge++) {
        for (indexx p = 0; p < numberOfPlayers; p++) {
            QValues.push_back(-1);
            QRowIndices.push_back((numberOfEdges+1)*p+edge);
            QColIndices.push_back(startVariableCentralAuthority+edge);
        }
    }
    MySparseMatrix Q = {QValues, QRowIndices, QColIndices};

    // build constraints
    vector<Constraint> constraints = {};
    for (indexx edge = 0; edge < numberOfEdges; edge++)
        constraints.push_back(
            {{{-1},{startVariableCentralAuthority+edge}},{},0});
    for (indexx edge = 0; edge < numberOfEdges; edge++)
        constraints.push_back(
            {{{1},{startVariableCentralAuthority+edge}},{},pMax[edge]});

    // build integer indices
    // there are no integer indices necessary because the linear program of this player is integral
    // indeed, the only constraints are integer upper and lower bound constraints, so the matrix is TU
    vector<indexx> integerIndices = {};

    bestResponseProblems.push_back(
        make_shared<OptimizationProblem>(numberOfEdges,
                                          c,
                                          Q,
                                          constraints,
                                          vector<indexx>(integerIndices)));

    // instantiate GNEP
    shared_ptr<NashEquilibriumProblem> GNEP = make_shared<NashEquilibriumProblem>(bestResponseProblems);

    return GNEP;
}

shared_ptr<NashEquilibriumProblem> createNEPImplementationGameInstance(
        string const& filename) {
        auto [numberOfPlayers,
        numberOfNodes,
        numberOfEdges,
        network,
        capacities,
        sourceSinks,
        U,
        demands,
        pMax,
        u] = parseImplementationGameGNEP(filename);

    vector<int> numberOfVariablesPerPlayer = {};
    for (int player = 0; player < numberOfPlayers; player++) {
        // for ordinary players (x_player,e and z_player)
        numberOfVariablesPerPlayer.push_back(numberOfEdges+1);
    }
    // for the central authority (p_e)
    numberOfVariablesPerPlayer.push_back(numberOfEdges);

    // initialize vector of optimizationProblem and other things
    vector<shared_ptr<OptimizationProblem>> bestResponseProblems = {};

    // +1 due to binary variable z_i
    int startVariableCentralAuthority = numberOfPlayers*(numberOfEdges+1);

    // for each player, build OptimizationProblem
    for (indexx player = 0; player < numberOfPlayers; player++) {
        vector<double> cValues = {};
        vector<indexx> cIndices = {};
        for (indexx edge = 0; edge < numberOfEdges; edge++) {
            cValues.push_back(-U[edge][player]);
            cIndices.push_back((numberOfEdges+1)*player+edge);
        }
        MySparseVector c = {cValues, cIndices};

        // build Q
        vector<double> QValues = {};
        vector<indexx> QRowIndices = {};
        vector<indexx> QColIndices = {};
        for (int edge = 0; edge < numberOfEdges; edge++) {
            QValues.push_back(1);
            QRowIndices.push_back((numberOfEdges+1)*player+edge);
            QColIndices.push_back(startVariableCentralAuthority+edge);
        }
        MySparseMatrix Q = {QValues, QRowIndices, QColIndices};

        // build constraints
        vector<Constraint> constraints = {};
        // positivity of flows
        for (indexx edge = 0; edge < numberOfEdges; edge++) {
            // -x_i,e <= 0 (x_i,e >= 0)
            constraints.push_back({{{-1},{(numberOfEdges+1)*player+edge}},{},0});
        }

        // flow polyhedron
        // declare all parts of the flow polyhedron constraints,
        // then update them with nnz of network and sourceSinks
        vector<vector<double>> vectorValues = {};
        vector<vector<indexx>> vectorIndices = {};
        vector<double> vectorB = {};
        for (indexx node = 0; node < numberOfNodes; node++) {
            // declare all parts of the flow polyhedron constraints
            vectorValues.push_back({});
            vectorIndices.push_back({});
            vectorB.push_back(0);
        }
        for (indexx nnzNetwork = 0; nnzNetwork < network.size(); nnzNetwork++) {
            // update with nnz
            double value = network.getValue(nnzNetwork);
            indexx node = network.getRowIndex(nnzNetwork);
            indexx edge = network.getColIndex(nnzNetwork);
            vectorValues[node].push_back(value);
            vectorIndices[node].push_back((numberOfEdges+1)*player+edge);
        }
        // adding demands of the source and sink
        vectorB[sourceSinks[player][0]] = demands[player];
        vectorB[sourceSinks[player][1]] = -demands[player];
        // adding contribution in z_i
        vectorValues[sourceSinks[player][0]].push_back(demands[player]);
        vectorIndices[sourceSinks[player][0]].push_back((numberOfEdges+1)*player+numberOfEdges);
        vectorValues[sourceSinks[player][1]].push_back(-demands[player]);
        vectorIndices[sourceSinks[player][1]].push_back((numberOfEdges+1)*player+numberOfEdges);
        // generating constraints
        for (indexx node = 0; node < numberOfNodes; node++) {
            MySparseVector sparseVector = {vectorValues[node],vectorIndices[node]};
            constraints.push_back({sparseVector,{},vectorB[node]});
        }

        // capacity per edge
        for (indexx edge = 0; edge < numberOfEdges; edge++) {
            vector<double> values = {};
            vector<indexx> indices = {};
            // we take column of index 'player' but all are the same
            double b = capacities[edge][player];
            // NEP implementation game: no other player variables in the capacity constraints
            values.push_back(1);
            indices.push_back((numberOfEdges+1)*player+edge);
            MySparseVector sparseVector = {values, indices};
            constraints.push_back({sparseVector,{},b});
        }

        // {0} is a feasible strategy (thanks to binary variable z_i forcing x_i to zero if z_i == 0)
        // z_i >= 0
        constraints.push_back({{{-1},{(numberOfEdges+1)*player+numberOfEdges}},{},0});

        // z_i <= 1
        constraints.push_back({{{1},{(numberOfEdges+1)*player+numberOfEdges}},{},1});

        for (indexx edge = 0; edge < numberOfEdges; edge++) {
            double M = capacities[edge][player]; // big-M value
            MySparseVector sparseVector =
                {{1,M},{(numberOfEdges+1)*player+edge,(numberOfEdges+1)*player+numberOfEdges}};
            constraints.push_back({sparseVector,{},M});
        }

        // build OptimizationProblem after integrality variable indices
        vector<indexx> integralityIndices = {};
        for (indexx varIndex = (numberOfEdges+1)*player;
             varIndex < (numberOfEdges+1)*(player+1);
             varIndex++)
            integralityIndices.push_back(varIndex);
        bestResponseProblems.push_back(
            make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[player],
                                               c,
                                               Q,
                                               constraints,
                                               integralityIndices));
    }

    // build optimizationProblem of central authority
    vector<double> cValues = {};
    vector<indexx> cIndices = {};
    for (int edge = 0; edge < numberOfEdges; edge++) {
        cValues.push_back(u[edge]);
        cIndices.push_back(startVariableCentralAuthority+edge);
    }
    MySparseVector c = {cValues, cIndices};

    // build Q
    vector<double> QValues = {};
    vector<indexx> QRowIndices = {};
    vector<indexx> QColIndices = {};
    for (indexx edge = 0; edge < numberOfEdges; edge++) {
        for (indexx p = 0; p < numberOfPlayers; p++) {
            QValues.push_back(-1);
            QRowIndices.push_back((numberOfEdges+1)*p+edge);
            QColIndices.push_back(startVariableCentralAuthority+edge);
        }
    }
    MySparseMatrix Q = {QValues, QRowIndices, QColIndices};

    // build constraints
    vector<Constraint> constraints = {};
    for (indexx edge = 0; edge < numberOfEdges; edge++)
        constraints.push_back(
            {{{-1},{startVariableCentralAuthority+edge}},{},0});
    for (indexx edge = 0; edge < numberOfEdges; edge++)
        constraints.push_back(
            {{{1},{startVariableCentralAuthority+edge}},{},pMax[edge]});

    // build integer indices
    // there are no integer indices necessary because the linear program of this player is integral
    // indeed, the only constraints are integer upper and lower bound constraints, so the matrix is TU
    vector<indexx> integerIndices = {};

    bestResponseProblems.push_back(
        make_shared<OptimizationProblem>(numberOfEdges,
                                          c,
                                          Q,
                                          constraints,
                                          vector<indexx>(integerIndices)));

    // instantiate GNEP
    shared_ptr<NashEquilibriumProblem> GNEP =
        make_shared<NashEquilibriumProblem>(bestResponseProblems);

    // add fake constant term to each optimization problem so that all response values are nonnegative
    vector<double> constantCoefficients;
    // for 'real' players
    for (indexx player = 0; player < numberOfPlayers; player++) {
        double lowerBound = 0;
        for (indexx edge = 0; edge < numberOfEdges; edge++) {
            lowerBound -= U[player][edge]*capacities[edge][player];
        }
        // new lower bound is 1
        constantCoefficients.push_back(-lowerBound+1);
    }
    // for the central authority
    double lowerBound = 0;
    for (indexx edge = 0; edge < numberOfEdges; edge++) {
        double sumCapacities = 0;
        for (indexx player = 0; player < numberOfPlayers; player++)
            sumCapacities += U[player][edge];
        lowerBound += (u[edge] - sumCapacities)*pMax[edge];
    }
    constantCoefficients.push_back(-lowerBound+1);
    GNEP = addFakeConstantTerm(GNEP, constantCoefficients);

    return GNEP;
}

shared_ptr<NashEquilibriumProblem> createMaxFlowMinLoadCostInstance(string const& filename) {
    auto [numberOfPlayers,
    numberOfNodes,
    numberOfEdges,
    network,
    capacities,
    sourceSinks,
    loadCosts,
    olddemands,
    oldpMax, // useless here
    oldu] = parseImplementationGameGNEP(filename);

    // replaces arcs with edges in network
    for (indexx nnzIndex = 0; nnzIndex < 2*numberOfEdges; nnzIndex++) {
        double value = network.getValue(nnzIndex);
        indexx node = network.getRowIndex(nnzIndex);
        indexx edge = network.getColIndex(nnzIndex);
        network.addCoefficient(-value, node, edge+numberOfEdges);
    }
    numberOfEdges *= 2;
    // capacities and loadCosts should also be extended
    for (indexx edge = 0; edge < numberOfEdges/2; edge++) {
        capacities.push_back(capacities[edge]);
        loadCosts.push_back(loadCosts[edge]);
    }

    // to show network in a human-readable format:
    cout << "network after arcs changed to edges:" << endl;
    for (indexx i = 0; i < network.size()/2; i++) {
        cout << "edge " << i << ": ";
        if (network.getValue(2*i) == 1)
            cout << network.getRowIndex(2*i) << " " << network.getRowIndex(2*i+1) << endl;
        else
            cout << network.getRowIndex(2*i+1) << " " << network.getRowIndex(2*i) << endl;
    }

    // using instance file of an implementation game,
    // but using U as loadCosts a_ie, i=player and e=edge

    // number of variables: flow variables + constant objective term variable
    // variables x_player,e representing flow for player on edge e
    // variable removing the max cost from the objective so that the objective
    // is nonpositive (constant objective term variable)
    int numberOfVariablesOfPlayer = numberOfEdges+1;
    vector<int> numberOfVariablesPerPlayer = {};
    for (int player = 0; player < numberOfPlayers; player++) {
        numberOfVariablesPerPlayer.push_back(numberOfVariablesOfPlayer);
    }

    // initialize vector of optimizationProblem and other things
    vector<shared_ptr<OptimizationProblem>> bestResponseProblems = {};

    // build OptimizationProblem for each player
    for (indexx player = 0; player < numberOfPlayers; player++) {
        vector<double> cValues = {};
        vector<indexx> cIndices = {};
        indexx sourcePlayer = sourceSinks[player][0];
        indexx sinkPlayer = sourceSinks[player][1];

        // compute upper bound on costs (positive term of the costs)
        double maxCostPlayer = 0;
        for (indexx edge = 0; edge < numberOfEdges; edge++) {
            double capacity = capacities[edge][player];
            maxCostPlayer += loadCosts[edge][player]*capacity*capacity;
            // check if capacity is the same for each player, else maxCostPlayer is computed wrongly
            for (indexx p = 0; p < numberOfPlayers; p++)
                if (capacity != capacities[edge][p])
                    throw logic_error("capacity of an edge should be the same for each player "
                                      "so that maxCostPlayer is properly computed");
        }
        cout << "upper bound of cost of "
             << player << " is " << maxCostPlayer << endl;
        if (maxCostPlayer != floor(maxCostPlayer))
            throw logic_error("max cost of player should be an integer because intersection cuts need integer coefficients");
        // index of this variable is the last of player
        indexx indexConstantObjectiveTerm = numberOfVariablesOfPlayer*(player+1)-1;

        // build c: empty because no gains
        MySparseVector c = {{}, {}};

        // build Q: sum on all edges of cost of player,edge times square of load costs l_edge(x)
        // l_edge(x) = sum_player x_player,edge
        // => l_edge(x)^2 = sum_p1 sum_p2 x_p1,edge * x_p2,edge
        vector<double> QValues = {};
        vector<indexx> QRowIndices = {};
        vector<indexx> QColIndices = {};
        for (indexx edge = 0; edge < numberOfEdges; edge++) {
            for (indexx p1 = 0; p1 < numberOfPlayers; p1++) {
                for (indexx p2 = 0; p2 < numberOfPlayers; p2++) {
                    // cost_player,edge * x_p1,edge * x_p2,edge
                    QValues.push_back(loadCosts[edge][player]);
                    QRowIndices.push_back(numberOfVariablesOfPlayer*p1+edge);
                    QColIndices.push_back(numberOfVariablesOfPlayer*p2+edge);
                }
            }
        }
        MySparseMatrix Q = {QValues, QRowIndices, QColIndices};

        // build constraints
        vector<Constraint> constraints = {};
        // nonnegativity of flows
        for (indexx edge = 0; edge < numberOfEdges; edge++) {
            // -x_i,e <= 0 (x_i,e >= 0)
            constraints.push_back({{{-1},{numberOfVariablesOfPlayer*player+edge}},
                                       {},
                                       0});
        }

        // flow polyhedron
        // declare all parts of the flow polyhedron constraints,
        // then update them with nnz of network and sourceSinks
        vector<vector<double>> vectorValues = {};
        vector<vector<indexx>> vectorIndices = {};
        vector<double> vectorB = {};
        for (indexx node = 0; node < numberOfNodes; node++) {
            // declare all parts of the flow polyhedron constraints
            vectorValues.push_back({});
            vectorIndices.push_back({});
        }
        for (indexx nnzNetwork = 0; nnzNetwork < network.size(); nnzNetwork++) {
            // update with nnz
            double value = network.getValue(nnzNetwork);
            indexx node = network.getRowIndex(nnzNetwork);
            indexx edge = network.getColIndex(nnzNetwork);
            vectorValues[node].push_back(value);
            vectorIndices[node].push_back(numberOfVariablesOfPlayer*player+edge);
        }
        // generating constraints
        for (indexx node = 0; node < numberOfNodes; node++) {
            // flow polyhedron is the same for non-source/sink nodes
            // and constraints associated with source and sink are unnecessary
            if (node != sourcePlayer and node != sinkPlayer) {
                if (!vectorValues[node].empty()) {
                    MySparseVector sparseVector = {vectorValues[node],vectorIndices[node]};
                    constraints.push_back({sparseVector,{},0});
                    constraints.push_back({-sparseVector, {}, 0});
                } else {
                    cout << "node " << node << " has no edges" << endl;
                }
            }
            else if (node == sourcePlayer) {
                // positive flow for source (to get a nonpositive response value)
                MySparseVector sparseVector = {vectorValues[node],vectorIndices[node]};
                constraints.push_back({-sparseVector, {}, -1});
            }
        }

        // capacity per edge
        for (indexx edge = 0; edge < numberOfEdges; edge++) {
            vector<double> values = {};
            vector<indexx> indices = {};
            // we take column of index 'player' but all are the same
            double b = capacities[edge][player];
            for (indexx p = 0; p < numberOfPlayers; p++) {
                values.push_back(1);
                indices.push_back(numberOfVariablesOfPlayer*p+edge);
            }
            MySparseVector sparseVector = {values, indices};
            constraints.push_back({sparseVector,{},b});
        }

        // fix value of y_i to max cost
        constraints.push_back({{{1},{indexConstantObjectiveTerm}}, {}, maxCostPlayer});
        constraints.push_back({{{-1},{indexConstantObjectiveTerm}}, {}, -maxCostPlayer});

        // build OptimizationProblem after integrality variable indices
        vector<indexx> integralityIndices = {};
        for (indexx varIndex = numberOfVariablesOfPlayer*player; varIndex < numberOfVariablesOfPlayer*(player+1)-1; varIndex++)
            integralityIndices.push_back(varIndex);
        bestResponseProblems.push_back(
            make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[player],
                                               c,
                                               Q,
                                               constraints,
                                               integralityIndices));
    }

    // instantiate GNEP
    shared_ptr<NashEquilibriumProblem> GNEP = make_shared<NashEquilibriumProblem>(bestResponseProblems);

    return GNEP;
}

shared_ptr<NashEquilibriumProblem> createGNEPQuadraticKnapsackInstance(string const& filename) {
    auto GNEP = createGNEPKnapsackInstance(filename, "fullInteger");
    cout << filename << endl;

    unsigned int seed = 1234;
    // changing the seed with the numbers inside filename
    int multiplier = 31;
    for (char c : filename) {
        seed += multiplier*c;
        multiplier++;
    }

    // extend GNEP knapsack instance with a random definite positive term
    for (indexx player = 0; player < GNEP->getNumberOfPlayer(); player++) {
        int size = GNEP->getTotalNumberOfVariablesOfPlayers();
        auto rangeLowerTriangularDiagonal = Bounds(1,3);
        auto rangeLowerPart = Bounds(1,2);
        // average proportion of values of the definite positive term that are nonzero
        double sparsity = 0.1;
        auto spd = symmetricPositiveDefiniteGeneration(
            size,
            rangeLowerTriangularDiagonal,
            rangeLowerPart,
            sparsity,
            seed);
        // change the seed for the other players to get other values
        seed++;

        // produce a MySparseMatrix from the vector<vector<double>>
        vector<double> values;
        vector<indexx> rowIndices;
        vector<indexx> colIndices;
        for (indexx i = 0; i < size; i++) {
            for (indexx j = 0; j < size; j++) {
                if (double val = spd[i][j]; val != 0) {
                    values.push_back(val);
                    rowIndices.push_back(i);
                    colIndices.push_back(j);
                }
            }
        }
        auto C_player = MySparseMatrix(values, rowIndices, colIndices);
        GNEP->getBestResponseProblems()[player]->setQ(C_player);
    }

    // change linear objective terms so that the objective function of any player is always nonpositive
    // it is useful to apply the approx NE def (costs should be of the same sign all the time)
    double maxSumRowLinearTerms = 0;
    for (indexx player = 0; player < GNEP->getNumberOfPlayer(); player++) {
        // compute maximum sum of coefs among all rows
        double sumCoefs = 0;
        indexx currentRow = -1;
        auto Q = GNEP->getBestResponseProblems()[player]->getQ();
        for (indexx cptIndex = 0; cptIndex < Q.size(); cptIndex++) {
            double coef = Q.getValue(cptIndex);
            indexx rowIndex = Q.getRowIndex(cptIndex);
            if (currentRow != rowIndex) {
                if (sumCoefs > maxSumRowLinearTerms)
                    maxSumRowLinearTerms = sumCoefs;
                currentRow = rowIndex;
                sumCoefs = coef;
            } else {
                sumCoefs += coef;
            }
        }
    }
    cout << "max sum row linear terms: " << maxSumRowLinearTerms << endl;
    for (indexx player = 0; player < GNEP->getNumberOfPlayer(); player++) {
        auto oldC = GNEP->getBestResponseProblems()[player]->getC();
        MySparseVector c;
        for (indexx termIndex = 0; termIndex < oldC.size(); termIndex++) {
            auto value = oldC.getValue(termIndex);
            auto variableIndex = oldC.getIndex(termIndex);
            c.addCoefficient(value-maxSumRowLinearTerms, variableIndex);
        }
        GNEP->getBestResponseProblems()[player]->setC(c);
    }

    return GNEP;
}

MySparseVector incrementIndices(MySparseVector const& v, indexx thresholdIndex) {
    auto indices = v.getIndices();
    for (int & indice : indices) {
        if (indice >= thresholdIndex)
            indice++;
    }
    return {v.getValues(), indices};
}

MySparseMatrix incrementIndices(MySparseMatrix const& m, indexx thresholdIndex) {
    auto rowIndices = m.getRowIndices();
    auto colIndices = m.getColIndices();
    for (indexx i = 0; i < rowIndices.size(); i++) {
        if (rowIndices[i] >= thresholdIndex)
            rowIndices[i]++;
        if (colIndices[i] >= thresholdIndex)
            colIndices[i]++;
    }
    return {m.getValues(), rowIndices, colIndices};
}

shared_ptr<NashEquilibriumProblem> addFakeConstantTerm(
        shared_ptr<NashEquilibriumProblem> GNEP,
        vector<double> const& values) {
    cout << "adding fake constant terms to have response values of the same sign"
    " and thus same inequality to check for the approximate-NE definition" << endl;
    cout << "constant values: " << values << endl;
    if (values.size() != GNEP->getNumberOfPlayer())
        throw logic_error("there is not the right number of constant term. It should be the number of players");

    // hypothesis: variables of player i have higher indices
    // than player i-1 and lower than i+1

    // throughout the for loop, lastIndexCurrentPlayer is the index of the fake variable for each player
    indexx lastIndexCurrentPlayer = -1;
    for (indexx player = 0; player < GNEP->getNumberOfPlayer(); player++) {
        // update lastIndexCurrentPlayer for the next player
        lastIndexCurrentPlayer += GNEP->getNumberOfVariablesOfPlayer(player)+1;
        // cout << "last index of player " << player << " should be " << lastIndexCurrentPlayer << endl;

        // increase indices of variables with greater indices by 1
        for (indexx p = 0; p < GNEP->getNumberOfPlayer(); p++) {
            // c, Q, each constraint
            auto c = incrementIndices(GNEP->getBestResponseProblems()[p]->getC(), lastIndexCurrentPlayer);
            GNEP->getBestResponseProblems()[p]->setC(c);
            auto Q = incrementIndices(GNEP->getBestResponseProblems()[p]->getQ(), lastIndexCurrentPlayer);
            GNEP->getBestResponseProblems()[p]->setQ(Q);
            auto constraints = GNEP->getBestResponseProblems()[p]->getConstraints();
            for (indexx constraintIndex = 0; constraintIndex < constraints.size(); constraintIndex++) {
                auto constraintVector = incrementIndices(constraints[constraintIndex].getAx(), lastIndexCurrentPlayer);
                auto constraintMatrix = incrementIndices(constraints[constraintIndex].getXQx(), lastIndexCurrentPlayer);
                constraints[constraintIndex] = Constraint(constraintVector, constraintMatrix, constraints[constraintIndex].getB());
            }
            GNEP->getBestResponseProblems()[p]->setConstraints(constraints);
        }
        // update number of variables of the problem
        GNEP->getBestResponseProblems()[player]->incrementNumberOfVariables();
        // add linear term with fake integer variable with bounds [1,1]
        // cout << "check new term in linear costs, the number of terms should increase by 1:" << endl;
        // cout << GNEP->getBestResponseProblems()[player]->getC().size() << endl;
        int oldVarNumber = GNEP->getBestResponseProblems()[player]->getC().size();
        auto newC = GNEP->getBestResponseProblems()[player]->getC();
        newC.addCoefficient(values[player], lastIndexCurrentPlayer);
        GNEP->getBestResponseProblems()[player]->setC(newC);
        // cout << GNEP->getBestResponseProblems()[player]->getC().size() << endl;
        int newVarNumber = GNEP->getBestResponseProblems()[player]->getC().size();
        if (newVarNumber - oldVarNumber != 1)
            throw logic_error("the number of linear terms should increase by 1 but does not");
        GNEP->getBestResponseProblems()[player]->addConstraint(
            {{{1},{lastIndexCurrentPlayer}},{},1});
        GNEP->getBestResponseProblems()[player]->addConstraint(
            {{{-1},{lastIndexCurrentPlayer}},{},-1});
    }
    return GNEP;
}

randomGNEP parseRandomGNEPInstance(string const& filename) {
    // parser for instance file built from the method randomGNEPInstance

    // it has only linear terms.
    // objective function only has negative coefs (because it comes from a max problem)
    // there is the same number of variables and constraints for each player
    // each rhs is positive, and each lhs coefficient is positive so that
    // the null strategy profile is feasible

    ifstream file(filename);
    if (file.fail())
        throw runtime_error("this file could not be opened: " + filename);
    string line;
    string temp;

    // parse first line to get number of players, constraints and variables
    getline(file, line);
    istringstream iss(line);
    getline(iss,temp,' ');
    int numberOfPlayers = stoi(temp);
    getline(iss,temp,' ');
    int numberOfVariables = stoi(temp);
    getline(iss,temp,' ');
    int numberOfConstraints = stoi(temp);

    int totalNumberOfVariables = numberOfPlayers*numberOfVariables;

    // declaration of containers
    vector<randomPlayerProblem> problems;

    // first initialize each value with 0
    for (int player = 0; player < numberOfPlayers; player++) {
        // next line for objective coefficients
        getline(file, line);
        istringstream issobj(line);
        vector<double> objectiveValues;
        vector<int> variableIndices;
        for (indexx indexVariable = 0; indexVariable < totalNumberOfVariables; indexVariable++) {
            getline(issobj, temp, ' ');
            objectiveValues.push_back(static_cast<double>(stoi(temp)));
            variableIndices.push_back(indexVariable);
        }
        MySparseVector objective = {objectiveValues, variableIndices};
        vector<Constraint> constraints;
        for (indexx indexConstraint = 0; indexConstraint < numberOfConstraints; indexConstraint++) {
            getline(file, line);
            istringstream isscons(line);
            vector<double> constraintValues;
            for (indexx indexVariable = 0; indexVariable < totalNumberOfVariables; indexVariable++) {
                getline(isscons, temp, ' ');
                constraintValues.push_back(static_cast<double>(stoi(temp)));
            }
            getline(isscons, temp, ' ');
            MySparseVector lhs = {constraintValues, variableIndices};
            Constraint constraint = {lhs, {}, static_cast<double>(stoi(temp))};
            constraints.push_back(constraint);
        }

        // form randomPlayerProblem
        randomPlayerProblem problem = {
            player,
            numberOfVariables*player,
            numberOfVariables,
            numberOfConstraints,
            objective,
            constraints};
        problems.push_back(problem);
    }
    randomGNEP randomGame = {numberOfPlayers, numberOfConstraints, numberOfVariables, problems};
    return randomGame;
}

shared_ptr<NashEquilibriumProblem> createGNEPRandomInstance(
        string const& filename) {
    cout << "building model of GNEP random game" << endl;

    // parse filename
    randomGNEP instance = parseRandomGNEPInstance(filename);

    int numberOfPlayers = instance.numberOfPlayers;
    int numberOfVariables = instance.numberOfVariables;

    vector<int> numberOfVariablesPerPlayer = {};
    for (int player = 0; player < numberOfPlayers; player++)
        numberOfVariablesPerPlayer.push_back(numberOfVariables);

    // initialize vector of optimizationProblem and other things
    vector<shared_ptr<OptimizationProblem>> bestResponseProblems = {};
    int countNumberOfVariables = 0;

    // for each player, build OptimizationProblem
    for (int player = 0; player < numberOfPlayers; player++) {
        randomPlayerProblem problem = instance.problems[player];
        // build c
        MySparseVector c = problem.objective;

        // build Q
        MySparseMatrix Q = {};

        // build constraints
        vector<Constraint> constraints = problem.constraints;
        // constraints for bounds of binary vectors
        for (indexx variableIndex = problem.firstVariable;
            variableIndex < problem.firstVariable+problem.numberOfVariables;
            variableIndex++) {
            // 0 <= variable <= 10
            constraints.push_back({{{1.0},
                                            {variableIndex}},
                                            {},
                                            10});
            constraints.push_back({{{-1.0},
                                            {variableIndex}},
                                            {},
                                            0.0});
        }

        // build OptimizationProblem, reuse capacityIndices as integrality variable indices
        vector<indexx> integralityIndices = {};
        for (indexx variableIndex = problem.firstVariable;
            variableIndex < problem.firstVariable+problem.numberOfVariables;
            variableIndex++) {
            integralityIndices.push_back(variableIndex);
        }

        bestResponseProblems.push_back(make_shared<OptimizationProblem>(
            numberOfVariablesPerPlayer[player],
            c,
            Q,
            constraints,
            integralityIndices));
    }

    // instantiate NEP
    shared_ptr<NashEquilibriumProblem> NEP =
        make_shared<NashEquilibriumProblem>(bestResponseProblems);

    return NEP;
}

shared_ptr<NashEquilibriumProblem> createGNEPRandomQuadraticInstance(
        string const& filename) {
    // same as createGNEPRandomInstance except for the objective of each players' problem:
    // the terms depending on other players' variables are removed, and
    // the remaining term c_i x_i is changed to c_i x_i^2 -> concave quadratic because c_i < 0
    cout << "building model of GNEP random quadratic (concave) game" << endl;

    // parse filename
    randomGNEP instance = parseRandomGNEPInstance(filename);

    int numberOfPlayers = instance.numberOfPlayers;
    int numberOfVariables = instance.numberOfVariables;

    vector<int> numberOfVariablesPerPlayer = {};
    for (int player = 0; player < numberOfPlayers; player++)
        numberOfVariablesPerPlayer.push_back(numberOfVariables);

    // initialize vector of optimizationProblem and other things
    vector<shared_ptr<OptimizationProblem>> bestResponseProblems = {};
    int countNumberOfVariables = 0;

    // for each player, build OptimizationProblem
    for (int player = 0; player < numberOfPlayers; player++) {
        randomPlayerProblem problem = instance.problems[player];
        // build c
        MySparseVector c = {};

        // build Q
        vector<double> values = {};
        vector<indexx> variableIndices = {};
        // remove other players' terms from values
        for (indexx ind = 0; ind < problem.objective.size(); ind++) {
            auto indexVariable = problem.objective.getIndex(ind);
            auto value = problem.objective.getValue(ind);
            if (indexVariable >= problem.firstVariable
                and indexVariable < problem.firstVariable+problem.numberOfVariables) {
                values.push_back(value);
                variableIndices.push_back(ind);
            }
        }
        MySparseMatrix Q = {values, variableIndices, variableIndices};

        // build constraints
        vector<Constraint> constraints = problem.constraints;
        // constraints for bounds of binary vectors
        for (indexx variableIndex = problem.firstVariable;
            variableIndex < problem.firstVariable+problem.numberOfVariables;
            variableIndex++) {
            // 0 <= variable <= 10
            constraints.push_back({{{1.0},
                                            {variableIndex}},
                                            {},
                                            10});
            constraints.push_back({{{-1.0},
                                            {variableIndex}},
                                            {},
                                            0.0});
        }

        // build OptimizationProblem, reuse capacityIndices as integrality variable indices
        vector<indexx> integralityIndices = {};
        for (indexx variableIndex = problem.firstVariable;
            variableIndex < problem.firstVariable+problem.numberOfVariables;
            variableIndex++) {
            integralityIndices.push_back(variableIndex);
        }

        bestResponseProblems.push_back(make_shared<OptimizationProblem>(
            numberOfVariablesPerPlayer[player],
            c,
            Q,
            constraints,
            integralityIndices));
    }

    // instantiate NEP
    shared_ptr<NashEquilibriumProblem> NEP =
        make_shared<NashEquilibriumProblem>(bestResponseProblems);

    return NEP;
}