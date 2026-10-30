//
// Created by alois-duguet on 7/8/26.
//

#include "MibSForGNEP.h"

using namespace std;

MibSForGNEP::MibSForGNEP(
    shared_ptr<NashEquilibriumProblem> const &NEPInit,
    string const &gameTypeString,
    bool globalCutSwitch,
    string const &modelTypeString,
    string const &whichCutString,
    string const &branchingRuleString,
    string const &algorithmVariant,
    string const &instanceName,
    bool enumerateNE,
    double timeLimit,
    int verbosity,
    string const &resultFilename) : BranchAndCutForGNEP(
        NEPInit,
        gameTypeString,
        globalCutSwitch,
        modelTypeString,
        whichCutString,
        branchingRuleString,
        algorithmVariant,
        instanceName,
        enumerateNE,
        timeLimit,
        verbosity,
        resultFilename) {
    m_algorithmVariant = "MibS";
    m_whichCutString = "MibSCuts";
    m_globalCutSwitch = false;
    m_branchingRuleString = "MibSDefaultBranching";
    m_feasibilityTol = 1e-4; // did not find the default tolerance of MibS

    // prepare path to file where the MibS instance will be written
    string const folderInstanceFiles = "../../../instances/";
    m_mibsInstanceFilename = folderInstanceFiles + "MibS/MibSInstanceFiles/"
        + m_gameTypeString + "-"
        + m_instanceName.substr(m_instanceName.find_last_of('/') + 1);
    m_mibsInstanceFilename = m_mibsInstanceFilename.substr(0,m_mibsInstanceFilename.size()-4);
    string const folderSolutionFiles = "../../../results/";
    m_mibsSolutionFilename = folderSolutionFiles + "MibS/MibSSolutionFiles/"
        + m_gameTypeString + "-"
        + m_instanceName.substr(m_instanceName.find_last_of('/') + 1);
    m_mibsSolutionFilename = m_mibsSolutionFilename.substr(0,m_mibsSolutionFilename.size()-4) + ".sol";
    m_mibsLogFilename = folderSolutionFiles + "MibS/MibSSolutionFiles/"
        + m_gameTypeString + "-"
        + m_instanceName.substr(m_instanceName.find_last_of('/') + 1);
    m_mibsLogFilename = m_mibsLogFilename.substr(0,m_mibsLogFilename.size()-4) + ".log";
}

void MibSForGNEP::initialize() {
    m_rootNode->setSubproblem(m_NEP->buildOptimizationProblemMibS());
    m_node = m_rootNode;

    checkValidArguments();

    m_globalEnv->set(GRB_DoubleParam_FeasibilityTol, m_feasibilityTol);
}

void MibSForGNEP::checkValidArguments() const {
    if (m_modelTypeString != "manyEtasGurobi")
        throw logic_error("not implemented for a model type different from manyEtasGurobi");
    if (m_gameTypeString != "GNEP-fullInteger"
        and m_gameTypeString != "NEP-fullInteger-linearized-in-GNEP") {
        if (m_gameTypeString == "NEP-fullInteger")
            throw logic_error("game type NEP-fullInteger is fine in theory but the quadratic term "
                              "of the objective should be linearized to be handled by MibS: use "
                              "NEP-fullInteger-linearized-in-GNEP for an equivalent game where "
                              "those terms are linearized");
        else {
            throw logic_error("MibS is only able to handle bilevel problems with:\n"
                              "- linking variables that are bounded integer\n"
                              "- linear upper-level and lower-level objectives\n"
                              "- linear constraints\n"
                              "check if the game type " + m_gameTypeString +
                              " satisfies everything above");
        }
    }
    for (const auto& constraint : m_node->getSubproblem()->getConstraints()) {
        if (constraint.getXQx().size() != 0)
            throw logic_error("a quadratic term in a constraint is defined; MibS does not handle that");
    }
    if (m_node->getSubproblem()->getQ().size() != 0)
        throw logic_error("a quadratic term in the objective is defined; MibS does not handle that");
}

NEPBranchOutput *MibSForGNEP::solve() {
    auto startTime = startChrono();
    initialize();
    cout << "bilevel problem without lower-level objective: " << endl;
    cout << *m_node->getSubproblem() << endl;

    // generate .mps and .aux files
    m_node->writeMibSFiles(m_globalEnv,
                           computeSolverTimeLimit(startTime),
                           m_verbosity,
                           false,
                           m_mibsInstanceFilename);

    // call MibS
    // string const writeSolArg = " -MibS_writeSolnFile " + m_mibsSolutionFilename;
    double const timeLimitMibS = computeSolverTimeLimit(startTime);
    string const argTimeLimit = " -Alps_timeLimit " + to_string(timeLimitMibS);
    string const argSubsolver = " -MibS_feasCheckSolver CPLEX"; // Cbc, SYMPHONY and CPLEX should work
    string const args = argTimeLimit + argSubsolver;

    string s = "mibs -instance " + m_mibsInstanceFilename + ".mps" + args;
    s = s + " > " + m_mibsLogFilename;
    cout << "running command:" << endl << s << endl;
    system(s.c_str());

    // extract solution and build NEPBranchOutput*
    auto output = returnHandler(startTime);
    return output;
}

NEPBranchOutput* MibSForGNEP::returnHandler(
        chrono::time_point<chrono::system_clock> const startTime,
        string errorMsg) {
    // compute time remaining before setting up time spent to its proper value
    double remainingTime = getRemainingTime(startTime) + m_bufferTime;
    // time is set now and not at the end of the function because what is computed
    // now is only information on the NE found if one was found, it could be
    // computed later but it would be cumbersome
    m_timeSpent += elapsedChrono(startTime);
    string optimizationStatus = "UNKNOWN";
    MibSSolutionInfo solutionInfo = {};
    if (errorMsg == "no error") {
        if (m_timeSpent > m_timeLimit - m_bufferTime) {
            optimizationStatus = "TIME_LIMIT_REACHED";
        } else {
            solutionInfo = parseMibSSolutionFile();
            optimizationStatus = solutionInfo.optimizationStatus;
        }
    }
    else {
        // case of error caught
        cerr << errorMsg << endl;
        // remove all ',' because of csv usage
        for (indexx i = 0; i < errorMsg.size(); i++) {
            if (errorMsg[i] == ',')
                errorMsg[i] = ';';
        }
        optimizationStatus = "ERROR: " + errorMsg;
        // setting time spent to time limit so that analysis of the results
        // does not confuse an instance that errored with a properly solved instance
        cout << "time spent when error is returned: " << m_timeSpent << endl;
        m_timeSpent = m_timeLimit;
    }

    double socialWelfare = numeric_limits<double>::max();
    vector<double> bestResponseValues = {};
    vector<double> differenceToBestResponseValues = {};
    for (indexx player = 0; player < m_NEP->getNumberOfPlayer(); player++) {
        bestResponseValues.push_back(numeric_limits<double>::max());
        differenceToBestResponseValues.push_back(numeric_limits<double>::max());
    }
    if (optimizationStatus == "SOLUTION_FOUND") {
        m_node->setSolved();
        socialWelfare = computeSocialWelfare({
            2,
                solutionInfo.solution,
            solutionInfo.cost});
        m_NEP->computeAllBestResponses(
            solutionInfo.solution,
            m_globalEnv,
            remainingTime,
            0);
        for (indexx player = 0; player < m_NEP->getNumberOfPlayer(); player++)
            bestResponseValues[player] = m_NEP->getBestResponse(player).objective;
        differenceToBestResponseValues =
            m_NEP->differencesToBestResponseValues(solutionInfo.solution);
    }
    auto solution = NEInfo(
        solutionInfo.solution,
        socialWelfare,
        bestResponseValues,
        differenceToBestResponseValues);
    auto output = new NEPBranchOutput(optimizationStatus,
                                          {solution},
                                          solutionInfo.numberOfNodes,
                                          solutionInfo.numberOfCuts,
                                          m_timeSpent,
                                          solutionInfo.timeOfCuts,
                                          m_node,
                                          m_antiCyclingMeasuresTaken,
                                          m_NEBypassedIntegerFeasibility);
    // write log in file instanceName
    // print results
    cout << *output << endl;

    oneLineResultHandler(*output);

    return output;
}

MibSSolutionInfo MibSForGNEP::parseMibSSolutionFile() const {
    ifstream file0(m_mibsLogFilename);
    if (file0.fail())
        throw runtime_error("this file could not be opened:" + m_mibsLogFilename);
    string line = "UNK";

    // write file for logs
    while (getline(file0, line))
        cout << line << endl;
    file0.close();

    ifstream file(m_mibsLogFilename);
    string temp;
    vector<string> targets = {
        "Number of nodes",
        "Called MIBS cut generator",
        "Cost = "};
    string optimizationStatus = "UNKNOWN";
    int numberOfNodes = -1;
    int numberOfCuts = -1;
    double timeOfCuts = -1;
    double cost = -1;

    // marker for optimisation finished because of time limit: "Reached time limit"
    // marker for optimization finished because an optimal solution was found:
    // "Search completed" and next line "Best solution found" I believe

    while (getline(file, line)) {
        if (line.find("Reached time limit") != string::npos) {
            optimizationStatus = "TIME_LIMIT_REACHED";
            break;
        }
        if (line.find("Best solution found") != string::npos) {
            optimizationStatus = "POTENTIAL_SOLUTION_FOUND";
            break;
        }
    }
    if (optimizationStatus == "UNKNOWN")
        throw runtime_error("optimization status of MibS call not found;"
            " check if it is a case of NO_SOLUTION_FOUND");

    vector<double> solution;
    if (optimizationStatus == "POTENTIAL_SOLUTION_FOUND") {
        line = findFirstLineWith(file, targets[0]);
        // two different lines in log:
        // "Number of nodes processed:" or "Number of nodes fully processed:"
        string alternativeTarget = "Number of nodes fully processed:";
        if (line.find(alternativeTarget) != string::npos)
            temp = line.substr(line.find(alternativeTarget)+32);
        else
            temp = line.substr(line.find(targets[0])+26);
        replaceAll(temp, " ", "");
        try {
            numberOfNodes = stoi(temp); // here
        } catch (std::exception const&  e) {
            cerr << "exception: " << e.what() << endl;
            cerr << "for the number of nodes in stoi(temp) " << temp << " in line " << line << endl;
            numberOfNodes = -1;
        }

        line = findFirstLineWith(file, targets[1]);
        temp = line.substr(line.find("generated ")+10);
        temp = temp.substr(0, temp.find(" cuts"));
        numberOfCuts = stoi(temp);
        temp = line.substr(line.find("CPU time")+9);
        temp = temp.substr(0, temp.find(" seconds"));
        timeOfCuts = stod(temp);
        while (getline(file, line)) {
            if (line.find("cut generator") != string::npos) {
                temp = line.substr(line.find("generated ")+10);
                temp = temp.substr(0, temp.find(" cuts"));
                numberOfCuts += stoi(temp);
                temp = line.substr(line.find("CPU time")+9);
                temp = temp.substr(0, temp.find(" seconds"));
                timeOfCuts += stod(temp);
            }
            else
                break;
        }

        line = findFirstLineWith(file, targets[2]);
        temp = line.substr(line.find("Cost = ")+7);
        cost = stod(temp);
        // depending on the value of the upper-level objective, the solution found
        // is an NE or not
        if (isApproxZero(cost))
            optimizationStatus = "SOLUTION_FOUND";
        else {
            optimizationStatus = "NO_SOLUTION_FOUND";
            cout << "no solution found because cost of upper-level objective is "
                 << cost << endl;
        }

        // find solution: first line with "x" after cost is the first nonzero variable
        // all upper-level variables x follow
        // if no x follows, it means all variables are zero at the optimum
        int numberOfBilevelVariables = m_node->getSubproblem()->getNumberOfVariables();
        int numberOfUpperLevelVariables = numberOfBilevelVariables/2;
        // prepare solution as full zeros and adapt it afterwards
        for (int i = 0; i < numberOfUpperLevelVariables; i++)
            solution.push_back(0);
        bool solutionParsed = false;
        try {
            line = findFirstLineWith(file, "x");
        }
        catch (std::exception const& e) {
            cout << "probable catch of an instance with optimal solution all zeros" << endl;
            cout << "with error message: " << e.what() << endl;
            solutionParsed = true;
        }
        if (!solutionParsed) {
            // continuing to fill the solution from the first line with an "x" found
            string strIndex = line.substr(line.find('x')+1, line.find(" = ")-1);
            indexx i = stoi(strIndex);
            string strValue = line.substr(line.find(" = ")+3, string::npos);
            double value = stod(strValue);
            solution[i] = value;
            while(getline(file, line)) {
                if (line.find('x') != string::npos) {
                    strIndex = line.substr(line.find('x')+1, line.find(" = ")-1);
                    i = stoi(strIndex);
                    strValue = line.substr(line.find(" = ")+3, string::npos);
                    value = stod(strValue);
                    solution[i] = value;
                }
                else
                    break;
            }
        }
    }
    return {optimizationStatus, cost, solution, numberOfNodes, numberOfCuts, timeOfCuts};
}

