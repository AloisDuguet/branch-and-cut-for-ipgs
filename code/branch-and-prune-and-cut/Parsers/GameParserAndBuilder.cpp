//
// Created by Aloïs Duguet on 2/20/25.
//

#include "GameParserAndBuilder.h"

#include <complex>
#include <fstream>

#include "TestInstances.h"
#include "BranchAndCutTools.h"

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

vector<double> computeAllLipschitzConstantImplementationGame(
        string const& filename,
        vector<shared_ptr<OptimizationProblem>> const& bestResponseProblems) {
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

    // vector with the lipschitz constants for each player
    vector<double> lipschitzVector;

    for (indexx player = 0; player < numberOfPlayers+1; player++) {
        lipschitzVector.push_back(computeLipschitzConstantImplementationGame(
            player,
            bestResponseProblems[player],
            numberOfEdges,
            numberOfPlayers,
            U,
            capacities,
            pMax,
            u));
    }

    return lipschitzVector;
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
    }
    while(numerator / (1+lipschitzConstant) > ballSize);

    return lipschitzConstant;
}

double getMaxTermLipschitzImplementationGame(double const uValue, double pmin, double pmax) {
    pmin = max(0.0,pmin);
    return max(abs(pmin - uValue), abs(pmax - uValue));
}

double computeLipschitzConstantImplementationGame(
        indexx const player,
        shared_ptr<OptimizationProblem> const& playerProblem,
        int numberOfEdges,
        int numberOfRealPlayers,
        vector<vector<double>> const& U,
        vector<vector<double>> const& capacities,
        vector<double> const& pMax,
        vector<double> const& u,
        string const& mode,
        vector<double> const& point,
        double ballSize) {

    if (mode != "global" and mode != "local")
        throw logic_error("mode must be 'global' or 'local'");

    double lipschitzConstant = 0;
    double pmax;
    // formula of the real players is different from the one of the last player (central authority)
    if (player != numberOfRealPlayers) {
        // pi_i(x,p) = (p-U_i)x_i
        // formula derived from the gradient of pi_i
        // L_i = max_(x,p) sqrt(sum_{e in E} (p_e-U_{i,e})^2 + x_{i,e}^2)
        for (indexx edge = 0; edge < numberOfEdges; edge++) {
            indexx variableIndex = player*(numberOfEdges+1) + edge;
            // (p_e-U_{i,e})^2, p_e in [0,pMax_e]
            if (mode == "global")
                pmax = pMax[edge];
            else if (mode == "local") {
                indexx startingIndexCentralAuthority = numberOfRealPlayers*(numberOfEdges+1);
                pmax = point[startingIndexCentralAuthority+edge]+ballSize;
                pmax = min(pmax, pMax[edge]);
            }
            double maxTerm = getMaxTermLipschitzImplementationGame(U[edge][player], 0, pmax);
            // double maxTerm = max(abs(-U[edge][player]),
            // abs(pMax[edge] - U[edge][player]));
            lipschitzConstant += maxTerm*maxTerm;
            // x_{i,e}^2, x_{i,e} in [0,capacities_{i,e}]
            lipschitzConstant += capacities[edge][player]*capacities[edge][player];
        }
    } else {
        // player is central authority
        // pi_N(x,p) = (u-l(x))p
        // where l(x) = (l_1(x),...,l_numberOfEdges(x))
        // and l_e(x) = sum_i x_{i,e}
        // formula derived from the gradient of pi_N
        // L_i = sqrt(sum_e (N*pMax_e^2 + max(abs(u_e), abs(u_e-l_e^max(x)))^2))
        for (indexx edge = 0; edge < numberOfEdges; edge++) {
            // N*pMax_e^2
            if (mode == "global")
                pmax = pMax[edge];
            else if (mode == "local") {
                indexx startingIndexCentralAuthority = numberOfRealPlayers*(numberOfEdges+1);
                pmax = point[startingIndexCentralAuthority+edge]+ballSize;
                pmax = min(pmax, pMax[edge]);
            }
            lipschitzConstant += numberOfRealPlayers*pmax*pmax;
            // each player has the same capacity on an edge
            double maxEdgeLoad = capacities[edge][0]; // l_e(x) <= capacity_edge
            for (indexx p = 0; p < numberOfRealPlayers; p++) {
                if (maxEdgeLoad != capacities[edge][p]) {
                    throw logic_error("capacities are not the same for each player, "
                        "lipschitz constant of central authority not computed properly");
                }
            }
            // max(abs(u_e), abs(u_e-l_e^max(x)))^2
            double maxTerm = max(abs(u[edge]), abs(u[edge] - maxEdgeLoad));
            lipschitzConstant += maxTerm*maxTerm;
        }
    }
    lipschitzConstant = sqrt(lipschitzConstant);

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

    double flowObjectiveFactor = 100;

    cout << "network:" << endl;
    for (indexx i = 0; i < network.size()/2; i++) {
        cout << "edge " << i << ": ";
        if (network.getValue(2*i) == 1)
            cout << network.getRowIndex(2*i) << " " << network.getRowIndex(2*i+1) << endl;
        else
            cout << network.getRowIndex(2*i+1) << " " << network.getRowIndex(2*i) << endl;
    }

    // using instance file of an implementation game,
    // but using U as loadCosts a_ie, i=player and e=edge

    vector<int> numberOfVariablesPerPlayer = {};
    for (int player = 0; player < numberOfPlayers; player++) {
        // variables x_player,e representing flow for player on edge e
        numberOfVariablesPerPlayer.push_back(numberOfEdges);
    }

    // initialize vector of optimizationProblem and other things
    vector<shared_ptr<OptimizationProblem>> bestResponseProblems = {};

    // build OptimizationProblem for each player
    for (indexx player = 0; player < numberOfPlayers; player++) {
        vector<double> cValues = {};
        vector<indexx> cIndices = {};
        indexx sourcePlayer = sourceSinks[player][0];
        indexx sinkPlayer = sourceSinks[player][1];
        for (indexx nnzIndex = 0; nnzIndex < network.size(); nnzIndex++) {
            // for each edge, check if one of its ends is the source
            // checking only for row index because the format of network is
            // (value:value, row:vertex, col:edge)
            // thus the end points of an edge appear in two different triplets
            if (sourcePlayer == network.getRowIndex(nnzIndex)) {
                // if it is exiting the source, count -1
                // (positive flow to the sink with minimization of the costs and thus negative flow counted in objective)
                // if it is entering the source, count 1
                // (negative flow to the sink with minimization of the costs and thus positive flow counted in objective)
                if (network.getValue(nnzIndex) == 1)
                    cValues.push_back(-1*flowObjectiveFactor);
                if (network.getValue(nnzIndex) == -1)
                    cValues.push_back(1*flowObjectiveFactor);
                cIndices.push_back(numberOfEdges*player+network.getColIndex(nnzIndex));
            }
        }
        MySparseVector c = {cValues, cIndices};

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
                    double value = max(loadCosts[edge][player],1.0);
                    if (loadCosts[edge][player] < 1)
                        cout << "warning: load costs set to 1 for loadCosts of player "
                             << player << " and edge " << edge << endl;
                    if (value < 1)
                        throw logic_error("load costs need to be >= 1 so that all best-response values are nonnegative");
                    QValues.push_back(value); // might be a coefficient too big to make an interesting game
                    QRowIndices.push_back(numberOfEdges*p1+edge);
                    QColIndices.push_back(numberOfEdges*p2+edge);
                }
            }
        }
        MySparseMatrix Q = {QValues, QRowIndices, QColIndices};

        // build constraints
        vector<Constraint> constraints = {};
        // positivity of flows
        for (indexx edge = 0; edge < numberOfEdges; edge++) {
            // -x_i,e <= 0 (x_i,e >= 0)
            constraints.push_back({{{-1},{numberOfEdges*player+edge}},
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
            vectorIndices[node].push_back(numberOfEdges*player+edge);
        }
        // generating constraints
        for (indexx node = 0; node < numberOfNodes; node++) {
            // flow polyhedron is the same for non-source/sink nodes
            // and constraints associated with source and sink are unnecessary
            if (node != sourcePlayer and node != sinkPlayer) {
                MySparseVector sparseVector = {vectorValues[node],vectorIndices[node]};
                constraints.push_back({sparseVector,{},0});
                constraints.push_back({-sparseVector, {}, 0});
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
                indices.push_back(numberOfEdges*p+edge);
            }
            MySparseVector sparseVector = {values, indices};
            constraints.push_back({sparseVector,{},b});
        }

        // build OptimizationProblem after integrality variable indices
        vector<indexx> integralityIndices = {};
        for (indexx varIndex = numberOfEdges*player; varIndex < numberOfEdges*(player+1); varIndex++)
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
