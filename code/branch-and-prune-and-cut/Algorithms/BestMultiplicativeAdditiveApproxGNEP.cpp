//
// Created by alois-duguet on 12/3/25.
//

#include "BestMultiplicativeAdditiveApproxGNEP.h"

#include "BranchAndCutMultiplicativeGNEP.h"

using namespace std;

BestMultiplicativeAdditiveApproxGNEP::BestMultiplicativeAdditiveApproxGNEP(
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
        BestMultiplicativeApproxGNEP(
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
            resultFilename),
        BCMultiplicativeAdditiveApproxGNEP(
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
        BranchAndCutMultiplicativeGNEP(multiplicativeApprox,
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
    auto startTime = startChrono();

    m_tagProblemPlusInitialApproximationValue =
        "minMultFixedAddApproxNE-(" + to_string(m_NEP->getApproximationPlayer(0))
        + ";" + to_string(m_NEP->getAdditiveApproximationPlayer(0))
        + ")";

    m_toleranceMinApproximation = 0.01;
    m_NETolerance = 1e-5;

    // if multiplicative approximation is not the same for each player, throw an error
    double temp = m_NEP->getMultiplicativeApproximationPlayer(0);
    for (indexx player = 1; player < m_NEP->getNumberOfPlayer(); player++) {
        if (temp != m_NEP->getMultiplicativeApproximationPlayer(player))
            throw logic_error("multiplicative approximation should be the same for each player");
    }
    // if additive approximation is not the same for each player, throw an error
    temp = m_NEP->getAdditiveApproximationPlayer(0);
    for (indexx player = 1; player < m_NEP->getNumberOfPlayer(); player++) {
        if (temp != m_NEP->getAdditiveApproximationPlayer(player))
            throw logic_error("additive approximation should be the same for each player");
    }

    m_timeSpent += elapsedChrono(startTime);
}

void BestMultiplicativeAdditiveApproxGNEP::printParameters(std::ostream &os) const {
    BranchAndCutMultiplicativeGNEP::printParameters(os);
    os << "min multiplicative approximation proved included in ["
       << m_minApproximationPossible << "," << m_bestApproximationFound << "]" << std::endl;
    os << "with fixed additive approximation " << m_NEP->getAdditiveApproximationPlayer(0) << endl;
    os << "currently looking for a " << to_string(getApproximation()) << "-NE" << endl;
}

BestMultiplicativeAdditiveBranchOutput* BestMultiplicativeAdditiveApproxGNEP::solve() {
    // initialize timer for this method
    auto startTime = startChrono();
    initialize();
    writeRootNode();

    // print the approximate NE looked for
    m_NEP->printEquilibriaSearched(cout);

    try {
        whileLoopSolve(startTime);
        return returnHandler(startTime);
    } catch (std::exception const& e) {
        return returnHandler(startTime, e.what());
    }
}

bool BestMultiplicativeAdditiveApproxGNEP::NEHandler(std::vector<double> const &solution) {
    // a getApproximation()-NE has been found
    // but a potentially better delta has been found: a maxRegret-NE has been found
    double maxRegret = computeNEWithMinMultiplicativeAndFixedAdditive(solution, 0);
    m_approximationValueOfLambdaConstraintsInQueueState = maxRegret;

    logNEFound(maxRegret);

    m_nodeSelector->incrementSolutionList(m_NEP,
                                          solution,
                                          computeSocialWelfare(m_node->getSolution()),
                                          m_NEP->differencesToBestResponseValues(solution),
                                          m_NETolerance);

    return checkAndChangeApproximation(maxRegret);
}

void BestMultiplicativeAdditiveApproxGNEP::logNEFound(double const maxRegret) const {
    if (m_verbosity >= 1) {
        cout << "----> "
             << getApproximation()
             << "-NE found with max multiplicative approximation "
             << maxRegret
             << " in Node "
             << m_node->getNodeNumber()
             << endl;
        cout << "number of cuts added to the model at the moment: "
             << m_totalNumberOfCuts - m_currentNumberOfCutsDropped << endl;
    }
}


void BestMultiplicativeAdditiveApproxGNEP::logNoNEFound() const {
    if (m_verbosity >= 1) {
        cout << "----> tree search finished with no (" << m_minApproximationPossible
             << "," << m_NEP->getAdditiveApproximationPlayer(0) << ")"
             << "-NE found in Node " << m_node->getNodeNumber() << endl;
        cout << "number of cuts added to the model at the moment: "
             << m_totalNumberOfCuts - m_currentNumberOfCutsDropped << endl;
        cout << "best multiplicative approx for delta is in ]" << m_minApproximationPossible
             << "," << m_bestApproximationFound << "]" << endl;
    }
}

BestMultiplicativeAdditiveBranchOutput *BestMultiplicativeAdditiveApproxGNEP::returnHandler(
        chrono::time_point<chrono::system_clock> startTime,
        string errorMsg) {
    string optimizationStatus = buildOptimizationStatusString(startTime, errorMsg);
    auto output = new BestMultiplicativeAdditiveBranchOutput(optimizationStatus,
                                  m_nodeSelector->getSolutionList(),
                                  m_nodeSelector->getExploredNodeNumber(),
                                  m_totalNumberOfCuts,
                                  m_timeSpent,
                                  m_timeCounters["totalCutDerivationTime"],
                                  m_node,
                                  m_antiCyclingMeasuresTaken,
                                  m_exactNEPossible,
                                  m_minApproximationPossible,
                                  m_bestApproximationFound,
                                  m_NEP->getAdditiveApproximationPlayer(0));

    // print results
    cout << *output << endl;

    // print one-line result in terminal
    oneLineResultHandler(*output);

    return output;
}

void BestMultiplicativeAdditiveApproxGNEP::oneLineResultHandler(BestMultiplicativeAdditiveBranchOutput const &output) const {
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

void BestMultiplicativeAdditiveApproxGNEP::writeOneLineResult(
        BestMultiplicativeAdditiveBranchOutput const &output, ostream &os) const {
    // separator
    string const sep1 = ",";

    // start of the line
    os << "result,";

    // type of problem solved
    os << m_tagProblemPlusInitialApproximationValue << sep1;

    writeEndOfOneLineResult(output, os);
}