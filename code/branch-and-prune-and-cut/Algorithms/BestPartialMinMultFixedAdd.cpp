//
// Created by alois-duguet on 3/27/26.
//

#include "BestPartialMinMultFixedAdd.h"
#include "CommonlyUsedFunctions.h"

using namespace std;

BestPartialMinMultFixedAdd::BestPartialMinMultFixedAdd(
    vector<bool> const& multToMinimize,
    vector<double> &multiplicativeApprox,
    vector<double> &additiveApprox,
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
    string const &resultFilename) :
        BestMultiplicativeAdditiveApproxGNEP(
            multiplicativeApprox,
            additiveApprox,
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
            resultFilename),
        BranchAndCutMultiplicativeGNEP(
            multiplicativeApprox,
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
    auto const startTime = startChrono();

    m_multToMinimize = multToMinimize;

    m_tagProblemPlusInitialApproximationValue =
        "partialMinMultFixedAddApproxNE-(" + to_string(multiplicativeApprox)
        + ";" + to_string(additiveApprox)
        + ";" + to_string(m_multToMinimize) + ")";

    m_toleranceMinApproximation = 0.01;

    // build a list of players for which the multiplicative approx is to be minimized
    for (indexx player = 0; player < m_NEP->getNumberOfPlayer(); player++)
        if (m_multToMinimize[player])
            m_listWithMultApproxToMinimize.push_back(player);

    // increase memory limit in case of NEP implementation game because too much instances stopped because of it
    if (gameTypeString == "NEPImplementationGame")
        m_globalEnv->set(GRB_DoubleParam_SoftMemLimit, 16);

    // set m_maxCutsPerNode to a non infinite value for lipschitz GNEP implementation games
    // because without it is stuck deriving cuts to the same node
    // if(gameTypeString == "implementationGame") {
    //     m_maxCutsPerNode = 5*m_NEP->getNumberOfPlayer();
    //     cout << "setting maxCutsPerNode to " << m_maxCutsPerNode << endl;
    // }

    cout << "m_NETolerance: " << m_NETolerance << endl;
    cout << "m_pruningZeroObjTolerance: " << m_pruningZeroObjTolerance << endl;
    cout << "m_toleranceMinApproximation: " << m_toleranceMinApproximation << endl;

    m_timeSpent += elapsedChrono(startTime);
}

void BestPartialMinMultFixedAdd::initialize() {
    BranchAndCutMultiplicativeGNEP::initialize();
    checkValidArguments();
}

void BestPartialMinMultFixedAdd::checkValidArguments() const {
    if (m_listWithMultApproxToMinimize.empty())
        throw logic_error("The multiplicative approximation is not minimized for any player."
            "What is the point of using this derived class? "
            "Please use class BCMultiplicativeAdditiveApproxGNEP instead.");

    // if multiplicative approximation is not the same for each player
    // for which the multiplicative approximation is minimized, throw an error
    double temp = m_NEP->getMultiplicativeApproximationPlayer(m_listWithMultApproxToMinimize[0]);
    for (indexx player = 1; player < m_listWithMultApproxToMinimize.size(); player++)
        if (temp != m_NEP->getMultiplicativeApproximationPlayer(m_listWithMultApproxToMinimize[player]))
            throw logic_error("multiplicative approximation should be the same for each player");

    // if additive approximation is not the same for each player, throw an error
    temp = m_NEP->getAdditiveApproximationPlayer(0);
    for (indexx player = 1; player < m_NEP->getNumberOfPlayer(); player++) {
        // exception case for gameTypeString == "NEPImplementationGame" where the last player
        // should have 0 so that the implementation property is true
        if (player != m_NEP->getNumberOfPlayer()-1 or m_gameTypeString != "NEPImplementationGame") {
            if (temp != m_NEP->getAdditiveApproximationPlayer(player))
                throw logic_error("additive approximation should be the same for each player");
        } else {
            if (m_NEP->getAdditiveApproximationPlayer(player) != 0)
                throw logic_error("NEPImplementationGame should have an additive approximation of 0"
                    " for the last player to satisfy the implementation property");
        }
    }

    // check length of m_multToMinimize
    if (m_multToMinimize.size() != m_NEP->getNumberOfPlayer())
        throw logic_error("multToMinimize is of size " +
            to_string(m_multToMinimize.size()) + " instead of " +
            to_string(m_NEP->getNumberOfPlayer()));
}

void BestPartialMinMultFixedAdd::printParameters(ostream &os) const {
    BranchAndCutMultiplicativeGNEP::printParameters(os);
    os << "partial min multiplicative approximation proved included in ["
       << m_minApproximationPossible << "," << m_bestApproximationFound << "]"
       << " for players " << m_listWithMultApproxToMinimize << endl
       << "with fixed additive approximation "
       << m_NEP->getAdditiveApproximation() << endl
       << "currently looking for a multiplicative approx "
       << to_string(getApproximation()) << endl;
}

void BestPartialMinMultFixedAdd::setApproximation(double const approximationValue) const {
    for (indexx const player : m_listWithMultApproxToMinimize)
        m_NEP->setApproximationPlayer(approximationValue, player);
}

double BestPartialMinMultFixedAdd::getApproximation() const {
    // the current multiplicative approximation value is the same for all players for which we minimize it
    return m_NEP->getApproximationPlayer(m_listWithMultApproxToMinimize[0]);
}

string BestPartialMinMultFixedAdd::getCurrentApproxString() const {
    string s = "(";
    for (indexx player = 0; player < m_NEP->getNumberOfPlayer(); player++) {
        if (m_multToMinimize[player])
            s += to_string(m_minApproximationPossible);
        else
            s += to_string(m_NEP->getApproximationPlayer(player));
        if (player != m_NEP->getNumberOfPlayer() - 1)
            s += " ";
    }
    s += ";";
    for (indexx player = 0; player < m_NEP->getNumberOfPlayer()-1; player++)
        s += to_string(m_NEP->getAdditiveApproximationPlayer(player)) + " ";
    s += ")";
    return s;
}

void BestPartialMinMultFixedAdd::logNoNEFound() const {
    if (m_verbosity >= 1) {
        cout << "----> tree search finished with no ";
        m_NEP->printApproximation(cout);
        cout << "-NE found in Node " << m_node->getNodeNumber() << endl;
        cout << "number of cuts added to the model at the moment: "
             << m_totalNumberOfCuts - m_currentNumberOfCutsDropped << endl;
        cout << "best multiplicative approx for delta is in ]" << format("{:.{}g}", m_minApproximationPossible, getPrecisionFloatFormat(m_minApproximationPossible))
             << "," << format("{:.{}g}", m_bestApproximationFound, getPrecisionFloatFormat(m_bestApproximationFound)) << "]" << endl;
    }
}

void BestPartialMinMultFixedAdd::logNEFound(double const maxRegret) const {
    auto temp = m_listWithMultApproxToMinimize;
    if (m_verbosity >= 1) {
        cout << "----> ";
        m_NEP->printApproximation(cout);
        cout << "-NE found with max multiplicative approximation "
             << maxRegret << " for players "
             << to_string(temp)
             << " in Node "
             << m_node->getNodeNumber()
             << endl;
        cout << "number of cuts added to the model at the moment: "
             << m_totalNumberOfCuts - m_currentNumberOfCutsDropped << endl;
    }
}

BestPartialMinMultFixedAddBranchOutput* BestPartialMinMultFixedAdd::returnHandler(
        chrono::time_point<chrono::system_clock> startTime,
        string errorMsg) {
    string optimizationStatus = buildOptimizationStatusString(startTime, errorMsg);
    auto output = new BestPartialMinMultFixedAddBranchOutput(optimizationStatus,
                                  m_nodeSelector->getSolutionList(),
                                  NodeSelector::getExploredNodeNumber(),
                                  m_totalNumberOfCuts,
                                  m_timeSpent,
                                  m_timeCounters["totalCutDerivationTime"],
                                  m_node,
                                  m_antiCyclingMeasuresTaken,
                                  m_NEBypassedIntegerFeasibility,
                                  m_exactNEPossible,
                                  m_minApproximationPossible,
                                  m_bestApproximationFound,
                                  m_NEP->getAdditiveApproximationPlayer(0),
                                  m_multToMinimize);

    // print results
    cout << *output << endl;

    // print one-line result in terminal
    oneLineResultHandler(*output);

    return output;
}

void BestPartialMinMultFixedAdd::writeOneLineResult(
        BestPartialMinMultFixedAddBranchOutput const &output,
        ostream &os) const {
    // separator
    string const sep1 = ",";

    // start of the line
    os << "result,";

    // problem solved: min delta-NE
    os << m_tagProblemPlusInitialApproximationValue << sep1;

    writeEndOfOneLineResult(output, os);
}

void BestPartialMinMultFixedAdd::oneLineResultHandler(
        BestPartialMinMultFixedAddBranchOutput const &output) const {
    // print one-line result in terminal
    writeOneLineResult(output, cout);

    // write one-line result in m_resultFilename
    if (m_resultFilename != "noWriteResult") {
        cout << "writing one-line output in " << m_resultFilename << endl;
        ofstream file;
        file.open(m_resultFilename, ofstream::app);
        writeOneLineResult(output, file);
        file.close();
    }
}