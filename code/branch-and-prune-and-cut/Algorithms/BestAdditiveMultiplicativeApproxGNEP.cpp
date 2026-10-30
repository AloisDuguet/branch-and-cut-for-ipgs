//
// Created by ChatGPT adaptation.
//

#include "BestAdditiveMultiplicativeApproxGNEP.h"

using namespace std;

BestAdditiveMultiplicativeApproxGNEP::BestAdditiveMultiplicativeApproxGNEP(
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

    auto startTime = startChrono();

    m_tagProblemPlusInitialApproximationValue =
        "minAddFixedMultApproxNE-(" +
        to_string(m_NEP->getMultiplicativeApproximationPlayer(0)) + ";" +
        to_string(m_NEP->getAdditiveApproximationPlayer(0)) + ")";

    // minimizing additive parameter
    m_minApproximationPossible = 0;
    m_originalMinApproximationPossible = m_minApproximationPossible;

    m_timeSpent += elapsedChrono(startTime);
}

double BestAdditiveMultiplicativeApproxGNEP::getApproximation() const {
    // the approximation value is the same for all players
    return m_NEP->getAdditiveApproximationPlayer(0);
}

void BestAdditiveMultiplicativeApproxGNEP::setApproximation(double approximationValue) const {
    for (indexx player = 0; player < m_NEP->getNumberOfPlayer(); player++)
        m_NEP->setAdditiveApproximationPlayer(approximationValue, player);
}

void BestAdditiveMultiplicativeApproxGNEP::printParameters(ostream &os) const {
    BranchAndCutMultiplicativeGNEP::printParameters(os);
    os << "min additive approximation proved included in ["
       << m_minApproximationPossible << "," << m_bestApproximationFound << "]" << endl
       << "with fixed multiplicative approximation "
       << m_NEP->getMultiplicativeApproximationPlayer(0) << endl
       << "currently looking for an additive approx "
       << to_string(getApproximation()) << endl;
}

double BestAdditiveMultiplicativeApproxGNEP::computeNEWithMinAdditiveAndFixedMultiplicative(
        vector<double> const &solution) const {
    if (m_verbosity >= 3)
        cout << "computing minimum additive approximation value of the NE, "
                "given the fixed multiplicative approximation value" << endl;

    return computeMinApproximationParameter(solution);
}

bool BestAdditiveMultiplicativeApproxGNEP::NEHandler(vector<double> const &solution) {
    double maxAdditive =
        computeNEWithMinAdditiveAndFixedMultiplicative(solution);

    m_approximationValueOfLambdaConstraintsInQueueState = maxAdditive;

    logNEFound(maxAdditive);

    m_nodeSelector->incrementSolutionList(m_NEP,
                                          solution,
                                          computeSocialWelfare(m_node->getSolution()),
                                          m_NEP->differencesToBestResponseValues(solution),
                                          m_NETolerance);

    return checkAndChangeApproximation(maxAdditive);
}

void BestAdditiveMultiplicativeApproxGNEP::logNEFound(double const maxAdditive) const {
    if (m_verbosity >= 1) {
        cout << "----> "
             << "approximate NE found with max additive approximation "
             << maxAdditive
             << " in Node "
             << m_node->getNodeNumber()
             << endl;
        cout << "number of cuts added to the model at the moment: "
             << m_totalNumberOfCuts - m_currentNumberOfCutsDropped << endl;
    }
}

void BestAdditiveMultiplicativeApproxGNEP::logNoNEFound() const {
    if (m_verbosity >= 1) {
        cout << "----> tree search finished with no ("
             << m_NEP->getAdditiveApproximationPlayer(0)
             << "," << m_minApproximationPossible << ")"
             << "-NE found in Node " << m_node->getNodeNumber() << endl;
    }
}

BestAdditiveMultiplicativeBranchOutput* BestAdditiveMultiplicativeApproxGNEP::returnHandler(
        chrono::time_point<chrono::system_clock> startTime,
        string errorMsg) {

    string optimizationStatus = buildOptimizationStatusString(startTime, errorMsg);

    auto output = new BestAdditiveMultiplicativeBranchOutput(
                                  optimizationStatus,
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
                                  m_NEP->getMultiplicativeApproximationPlayer(0));

    cout << *output << endl;

    oneLineResultHandler(*output);

    return output;
}

void BestAdditiveMultiplicativeApproxGNEP::oneLineResultHandler(
    BestAdditiveMultiplicativeBranchOutput const &output) const {

    writeOneLineResult(output, cout);

    if (m_resultFilename != "noWriteResult") {
        ofstream file;
        file.open(m_resultFilename, ofstream::app);
        writeOneLineResult(output, file);
        file.close();
    }
}

void BestAdditiveMultiplicativeApproxGNEP::writeOneLineResult(
        BestAdditiveMultiplicativeBranchOutput const &output,
        ostream &os) const {

    string const sep1 = ",";

    os << "result,";
    os << m_tagProblemPlusInitialApproximationValue << sep1;

    writeEndOfOneLineResult(output, os);
}