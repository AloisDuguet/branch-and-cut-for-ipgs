//
// Created by alois-duguet on 11/14/25.
//

#include "NEPMultAddApprox.h"

using namespace std;

NEPMultAddApprox::NEPMultAddApprox(
    vector<double> const &factor,
    vector<double> const &additiveApprox,
    vector<shared_ptr<OptimizationProblem>> const &bestResponseProblems,
    string const& instanceName) :
        NEPMultiplicativeApprox(factor, bestResponseProblems, instanceName) {
    m_additiveApprox = additiveApprox;
}

void NEPMultAddApprox::print(ostream &os) const {
    printEquilibriaSearched(os);
    NEPManyEtasGurobi::print();
}

void NEPMultAddApprox::printApproximation(ostream &os) const {
    os << "(";
    for (auto it = m_factor.begin(); it != m_factor.end()-1; ++it) {
        os << *it << ",";
    }
    os << m_factor[getNumberOfPlayer()-1] << ";";
    for (auto it = m_additiveApprox.begin(); it != m_additiveApprox.end()-1; ++it) {
        os << *it << ",";
    }
    os << m_additiveApprox[getNumberOfPlayer()-1] << ")";
}

double NEPMultAddApprox::getAdditiveApproximationPlayer(indexx player) const {
    return m_additiveApprox[player];
}

double NEPMultAddApprox::getMultiplicativeApproximationPlayer(indexx player) const {
    return NEPMultiplicativeApprox::getApproximationPlayer(player);
}

void NEPMultAddApprox::printEquilibriaSearched(ostream& os) const {
    os << "Looking for a multiplicative-additive ";
    printApproximation(os);
    os << "-NE" << endl;
}

Constraint NEPMultAddApprox::buildLambdaConstraint(indexx player) const {
    // NEP case
    // 1/C_i * pi_i(x) - eta_i - lambda <= add_i/C_i if positive best responses
    // C_i * pi_i(x) - eta_i - lambda <= C_i*add_i if negative best responses

    // GNEP case
    // 1/C_i * xi_i - eta_i - lambda <= add_i/C_i if positive best responses
    // C_i * xi_i - eta_i - lambda <= C_i*add_i if negative best responses

    double additiveApprox = m_additiveApprox[player]; // add_i
    double multiplicativeApprox = m_factor[player]; // C_i

    vector<double> linearValues = {};
    vector<indexx> linearIndices = {};
    vector<double> quadValues = {};
    vector<indexx> quadRowIndices = {};
    vector<indexx> quadColIndices = {};
    double rhs = 0;

    if (!m_isGNEP) {
        // NEP case
        // linear terms
        auto cObj = m_bestResponseProblems[player]->getC();
        for (indexx i = 0; i < cObj.size(); i++) {
            if (m_positiveResponses) {
                // positive best responses -> 1/C_i * pi_i(x)
                linearValues.push_back(cObj.getValue(i)/multiplicativeApprox);
            }
            else {
                // negative best responses -> C_i * pi_i(x)
                linearValues.push_back(cObj.getValue(i)*multiplicativeApprox);
            }
            linearIndices.push_back(cObj.getIndex(i));
        }
        // quadratic terms
        auto QObj = m_bestResponseProblems[player]->getQ();
        for (indexx i = 0; i < QObj.size(); i++) {
            if (m_positiveResponses) // positive best responses -> 1/C_i * pi_i(x)
                quadValues.push_back(QObj.getValue(i)/multiplicativeApprox);
            else // negative best responses -> C_i * pi_i(x)
                quadValues.push_back(QObj.getValue(i)*multiplicativeApprox);
            quadRowIndices.push_back(QObj.getColIndex(i));
            quadColIndices.push_back(QObj.getRowIndex(i));
        }
    }
    else {
        // GNEP case
        if (m_positiveResponses) {
            // positive best responses -> 1/C_i * xi_i
            linearValues.push_back(1/multiplicativeApprox);
        }
        else // negative best responses -> C_i * xi_i
            linearValues.push_back(multiplicativeApprox);
        linearIndices.push_back(getXiIndex(player));
    }

    // constant term
    if (m_positiveResponses) // add_i/C_i
        rhs += additiveApprox/multiplicativeApprox;
    else // C_i*add_i
        rhs += multiplicativeApprox*additiveApprox;

    // - eta_i
    linearValues.push_back(-1);
    linearIndices.push_back(getEtaIndex(player));

    // - lambda
    linearValues.push_back(-1);
    linearIndices.push_back(getLambdaIndex());

    MySparseVector linTerms = {linearValues,linearIndices};
    MySparseMatrix quadTerms = {quadValues,quadRowIndices,quadColIndices};
    return {linTerms,quadTerms,rhs};
}

std::vector<double> NEPMultAddApprox::computeRegrets(
        vector<double> const &point) const {
    throw logic_error("It is not clear how to define the regrets with "
        "multiplicative-additive approximate NEs");
    return {};
}

bool NEPMultAddApprox::checkPlayerNECondition(
        vector<double> const &point,
        indexx player,
        double tolerance) const {
    auto bestResponseValue = m_bestResponses[player].objective;
    auto responseValue = evalResponseValue(player, point);
    if (bestResponseValue >= 0) {
        // definition of NE uses m_factor[player] >= 1
        // because the best response is nonnegative
        if (compareApproxLess(responseValue,
                          m_factor[player]*bestResponseValue+m_additiveApprox[player],
                          tolerance)) {
            return true;
        }
    }
    else {
        // definition of NE uses m_factor[player] <= 1
        // because the best response is negative
        if (compareApproxLess(responseValue,
                          1/m_factor[player]*bestResponseValue+m_additiveApprox[player],
                          tolerance)) {
            return true;
        }
    }
    return false;
}

std::vector<Cut> NEPMultAddApprox::deriveEquilibriumCutForEachPlayer(
        shared_ptr<GRBModel> const &model,
        vector<double> const &point,
        double tolerance,
        int verbosity) const {
    vector<Cut> cuts = {};
    vector<int> const numberOfVariablesPerPlayer = getNumberOfVariablesPerPlayer();
    indexx startingVariableIndex = 0;
    // trying to add eta equilibrium cuts
    for (indexx player = 0; player < m_numberOfPlayers; player++) {
        if (!testBestEtaPlayerPossible(player, point, tolerance)) {
            cuts.push_back(buildDisaggregatedEquilibriumCut(player, point));
            if (verbosity >= 3) {
                cout << "adding equilibrium cut for player "
                << player << ": " << cuts[cuts.size()-1] << endl;
            }
            else if (verbosity >= 2)
                cout << "adding equilibrium cut for player "
                     << player << endl;
        }
        else if (verbosity >= 2)
            cout << "no equilibrium cut for player " << player << endl;
        startingVariableIndex += numberOfVariablesPerPlayer[player];
    }
    return cuts;
}
