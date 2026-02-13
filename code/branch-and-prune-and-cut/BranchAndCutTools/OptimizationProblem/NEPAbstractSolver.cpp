//
// Created by Aloïs Duguet on 12/6/24.
//

#include "NEPAbstractSolver.h"

#include "CutInformation.h"

using namespace std;

void NEPAbstractSolver::initialize() {
    cout << "NEPAbstractSolver::initialize() is a virtual function"
         << endl;
}

vector<SolveOutput> NEPAbstractSolver::getBestResponses() const {
    return m_bestResponses;
}

SolveOutput NEPAbstractSolver::getBestResponse(indexx player) const {
    return m_bestResponses[player];
}

vector<double> NEPAbstractSolver::getBestResponseWithOnlyPlayerVariables(
        indexx player) const {
    indexx startingVariableIndex = 0;
    for (indexx i = 0; i < player; i++)
        startingVariableIndex += getNumberOfVariablesOfPlayer(i);
    indexx endingVariableIndex =
        startingVariableIndex + getNumberOfVariablesOfPlayer(player);

    auto fullBestResponse = m_bestResponses[player].solution;
    vector<double> realBestResponse(fullBestResponse.begin()+startingVariableIndex,
        fullBestResponse.begin()+endingVariableIndex);
    return realBestResponse;
}


std::shared_ptr<OptimizationProblem> NEPAbstractSolver::buildOptimizationProblem() {
    return nullptr;
}

double NEPAbstractSolver::evalResponseValue(indexx player,
                                            vector<double> const& point) const {
    cout << "NashEquilibriumProblemAbstractSolver::evalResponseValue "
            "is an abstract method" << endl;
    return -1.0;
}

bool NEPAbstractSolver::checkNE(std::vector<double> const &point, double tolerance, int verbosity) const {
    cout << "NashEquilibriumProblemAbstractSolver::checkNE "
            "is an abstract method" << endl;
    return false;
}

// uses computeBestResponse for each player and return a list of best responses
void NEPAbstractSolver::computeAllBestResponses(
        std::vector<double> const& point,
        shared_ptr<GRBEnv> globalEnv,
        double solverTimeLimit,
        int verbosity) {}

vector<double> NEPAbstractSolver::extractNashEquilibrium(
        vector<double> nashEquilibrium,
        double tolerance) {
    return nashEquilibrium;
}

Cut NEPAbstractSolver::buildDisaggregatedEquilibriumCut(
        indexx player,
        std::vector<double> const& point) const {
    return {};
}

Cut NEPAbstractSolver::addAggregatedEquilibriumCut(std::vector<double> const& point) const {
    return {};
}

double NEPAbstractSolver::evalNikaidoIsoda(std::vector<double> const& point,
                                           double objectiveValue,
                                           bool isSolutionInteger) const {
    return 0.0;
}

double NEPAbstractSolver::computeMaxRegretNE(std::vector<double> const& point,
                                             int verbosity) const {
    cout << "NEPAbstractSolver::checkDeltaNE is a virtual function, it does nothing" << endl;
    return 0.0;
}

vector<double> NEPAbstractSolver::differencesToBestResponseValues(
        vector<double> const& point) const {
    return {};
}

bool NEPAbstractSolver::testBestEtaPossible(std::vector<double> const& point,
                                                               double const& tolerance,
                                                               int verbosity) const {
    return false;
}

void NEPAbstractSolver::computeEtasBounds(double mipGap) {
    cout << "NEPAbstractSolver::computeEtasBounds "
            "is a virtual function" << endl;
}

SolveOutput NEPAbstractSolver::computeEtaBound(indexx player,
                                               bool isUpperBound, double mipGap) const {
    return {};
}

std::vector<double> NEPAbstractSolver::getEtaUpperBounds() const {
    cout << "NEPAbstractSolver::getEtaUpperBounds "
            "is a virtual function, it does nothing" << endl;
    return {};
}

std::vector<double> NEPAbstractSolver::getEtaLowerBounds() const {
    cout << "NEPAbstractSolver::getEtaUpperBounds "
            "is a virtual function, it does nothing" << endl;
    return {};
}

CutInformation NEPAbstractSolver::deriveCut(vector<double> const &point,
                                            string const &whichCutString,
                                            double const &tolerance,
                                            shared_ptr<GRBEnv> const &globalEnv,
                                            shared_ptr<GRBModel> const &model,
                                            shared_ptr<OptimizationProblem> const &nodeProblem,
                                            int verbosity) {
    return {};
}

indexx NEPAbstractSolver::getEtaIndex(indexx player) const {
    cout << "NEPAbstractSolver::getEtaIndex() is a virtual function, it does nothing" << endl;
    return -1;
}

double NEPAbstractSolver::getApproximation() const {
    cout << "NEPAbstractSolver::getApproximation() is a virtual function, it does nothing" << endl;
    return 0.0;
}

double NEPAbstractSolver::getApproximationPlayer(indexx player) const {
    cout << "NEPAbstractSolver::getApproximationPlayer(...) is a virtual function, it does nothing" << endl;
    return 0.0;
}

void NEPAbstractSolver::setApproximation(double delta) {
    cout << "NEPAbstractSolver::setApproximation() is a virtual function, it does nothing" << endl;
}

void NEPAbstractSolver::setApproximationPlayer(double delta, indexx player) {
    cout << "NEPAbstractSolver::setApproximationPlayer() is a virtual function, it does nothing" << endl;
}

void NEPAbstractSolver::printApproximation(ostream &os) const {
    os << "NEPAbstractSolver::printApproximation is a virtual function" << endl;
}

void NEPAbstractSolver::printEquilibriaSearched(std::ostream &os) const {
    os << "NEPAbstractSolver::printEquilibriaSearched is a virtual function" << endl;
}

void NEPAbstractSolver::printRegret(ostream &os, vector<double> const& point) const {
    os << "NEPAbstractSolver::printRegret is a virtual function" << endl;
}

std::vector<double> NEPAbstractSolver::computeRegrets(std::vector<double> const &point) const {
    cout << "NEPAbstractSolver::computeRegrets is a virtual function" << endl;
    return {};
}

double NEPAbstractSolver::getThresholdPruning() const {
    return 0.0;
}

void NEPAbstractSolver::setPositiveResponses(bool b) {
    cout << "NEPAbstractSolver::setPositiveResponses is a virtual function" << endl;
}

Constraint NEPAbstractSolver::buildLambdaConstraint(indexx player) const {
    cout << "NEPAbstractSolver::buildLambdaConstraint is a virtual function" << endl;
    return {};
}

void NEPAbstractSolver::setLipschitzConstants(
        string const &gameTypeString,
        string const &whichCutString,
        string const &instanceName) {
    cout << "NEPAbstractSolver::setLipschitzConstants is a virtual function" << endl;
}

double NEPAbstractSolver::getMultiplicativeApproximationPlayer(indexx player) const {
    cout << "NEPAbstractSolver::getMultiplicativeApproximationPlayer is a virtual function" << endl;
    return -1;
}

double NEPAbstractSolver::getAdditiveApproximationPlayer(indexx player) const {
    cout << "NEPAbstractSolver::getAdditiveApproximationPlayer is a virtual function" << endl;
    return -1;
}





