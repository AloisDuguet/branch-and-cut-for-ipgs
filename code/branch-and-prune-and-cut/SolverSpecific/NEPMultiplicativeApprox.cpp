//
// Created by alois-duguet on 9/19/25.
//

#include "NEPMultiplicativeApprox.h"

#include "CutInformation.h"
#include "GameParserAndBuilder.h"

using namespace std;

NEPMultiplicativeApprox::NEPMultiplicativeApprox(
    vector<double> const& factor,
    vector<shared_ptr<OptimizationProblem>> const& bestResponseProblems,
    string const& instanceName) :
        NEPManyEtasGurobi(bestResponseProblems) {
    m_factor = factor;
    if (instanceName != "") // only when instanceObject is necessary is instanceName not empty
        m_instanceObject = parseImplementationGameGNEP(instanceName);
    else
        m_instanceObject = {};
    // if (m_isGNEP)
    //     throw logic_error("GNEP not supported. Only NEP supported for the moment.");

    // initialize positiveResponses with false.
    // it will be set properly later on by the algorithm
    m_positiveResponses = false;
}

double NEPMultiplicativeApprox::getApproximationPlayer(indexx player) const {
    return m_factor[player];
}

void NEPMultiplicativeApprox::setApproximationPlayer(double approximation, indexx player) {
    m_factor[player] = approximation;
}

void NEPMultiplicativeApprox::print(ostream& os) const {
    printEquilibriaSearched(os);
    NEPManyEtasGurobi::print();
}

void NEPMultiplicativeApprox::printApproximation(ostream &os) const {
    os << "(";
    for (auto it = m_factor.begin(); it != m_factor.end()-1; ++it) {
        os << *it << ",";
    }
    os << m_factor[getNumberOfPlayer()-1] << ")";
}

void NEPMultiplicativeApprox::printEquilibriaSearched(ostream &os) const {
    os << "Looking for a multiplicative ";
    printApproximation(os);
    os << "-NE" << endl;
}

indexx NEPMultiplicativeApprox::getLambdaIndex() const {
    // lambda is just after all eta variables
    return getEtaIndex(getNumberOfPlayer()-1) + 1;
}

indexx NEPMultiplicativeApprox::getXiIndex(indexx player) const {
    // xi is just after lambda
    return getLambdaIndex() + 1 + player;
}

shared_ptr<OptimizationProblem> NEPMultiplicativeApprox::buildOptimizationProblem() {
    // 2 things to do:
    // 1 - 'concatenate' all constraints and integrality constraints from best responses
    // 2 - add specific parts due to additional variables and the objective function

    // compute number of variables coming from players' optimization problems
    int numberOfVariables = getTotalNumberOfVariablesOfPlayers();

    // concatenate constraints of the best responses
    vector<Constraint> constraints = m_bestResponseProblems[0]->getConstraints();
    for (indexx i = 1; i < m_numberOfPlayers; i++) {
        for (Constraint const& con : m_bestResponseProblems[i]->getConstraints()) {
            bool isConAlreadyAdded = false;
            for (Constraint const& conAlreadyAdded : constraints) {
                if (con == conAlreadyAdded) {
                    isConAlreadyAdded = true;
                    break;
                }
            }
            if (!isConAlreadyAdded)
                constraints.push_back(con);
        }
    }

    // add eta_i for each player with bounds
    for (indexx player = 0; player < m_numberOfPlayers; player++) {
        numberOfVariables++;

        // eta_i >= LB
        constraints.push_back(
            {{{-1.0}, {numberOfVariables-1}},
                {},
                -m_etaLowerBounds[player]});

        // eta_i <= UB
        constraints.push_back(
            {{{1.0}, {numberOfVariables-1}},
                {},
                m_etaUpperBounds[player]});
    }

    // add variable lambda
    numberOfVariables++;

    // objective function is variable lambda
    MySparseVector c = {{1},{numberOfVariables-1}};
    MySparseMatrix Q = {};

    // add constraints involving lambda and the regrets of players
    // IMPORTANT: keep those constraints as the last ones so that
    // adaptLambdaConstraints(true) works
    if (!m_isGNEP) {
        // it is not a GNEP, classic formulation of lambda constraints
        for (indexx player = 0; player < m_numberOfPlayers; player++) {
            constraints.push_back(buildLambdaConstraint(player));
        }
    }
    else {
        // it is a GNEP:
        // 1) adding epigraph variables xi_i for pi_i(x) with lower bounds and
        // 2) formulation of lambda constraints with epigraph variables xi as last constraints

        // 1
        for (indexx player = 0; player < m_numberOfPlayers; player++) {
            numberOfVariables++;

            // xi_i >= LB on pi_i(x)
            constraints.push_back(
                {{{-1.0}, {numberOfVariables-1}},
                    {},
                    -m_etaLowerBounds[player]});
        }

        // 2
        for (indexx player = 0; player < m_numberOfPlayers; player++) {
            constraints.push_back(buildLambdaConstraint(player));
        }
    }
    // define the definitive number of constraints of the root node
    // to ensure that no others are added after the lambda constraints
    // in this function
    int maxNumberOfConstraints = static_cast<int>(constraints.size());

    // concatenate integrality constraints
    vector<indexx> integralityConstraints =
        m_bestResponseProblems[0]->getIntegralityConstraints();
    for (indexx i = 1; i < m_numberOfPlayers; i++) {
        vector<indexx> newVec = m_bestResponseProblems[i]->getIntegralityConstraints();
        integralityConstraints.insert(
            integralityConstraints.end(),
            newVec.begin(),
            newVec.end());
    }

    // definition of OptimizationProblem problem
    shared_ptr<OptimizationProblem> problem =
        make_shared<OptimizationProblem>(numberOfVariables,
                                         c,
                                         Q,
                                         constraints,
                                         integralityConstraints);

    // check that lambda constraints are the last
    // so that adaptLambdaConstraints(true) works, cf reuseWithoutCuts
    if (maxNumberOfConstraints < static_cast<int>(constraints.size())) {
        throw logic_error("It seems that lambda constraints are not the last");
    }

    return problem;
}

Constraint NEPMultiplicativeApprox::buildLambdaConstraint(indexx player) const {
    double approximationValue = m_factor[player];

    vector<double> linearValues = {};
    vector<indexx> linearIndices = {};
    vector<double> quadValues = {};
    vector<indexx> quadRowIndices = {};
    vector<indexx> quadColIndices = {};

    if (!m_isGNEP) {
        // 1/C_i * pi_i(x)
        auto cObj = m_bestResponseProblems[player]->getC();
        for (indexx i = 0; i < cObj.size(); i++) {
            if (m_positiveResponses) // positive best responses -> 1/C * pi_i(x)
                linearValues.push_back(cObj.getValue(i)/approximationValue);
            else // negative best responses -> C * pi_i(x)
                linearValues.push_back(cObj.getValue(i)*approximationValue);
            linearIndices.push_back(cObj.getIndex(i));
        }
        auto QObj = m_bestResponseProblems[player]->getQ();
        for (indexx i = 0; i < QObj.size(); i++) {
            if (m_positiveResponses) // positive best responses -> 1/C * pi_i(x)
                quadValues.push_back(QObj.getValue(i)/approximationValue);
            else // negative best responses -> C * pi_i(x)
                quadValues.push_back(QObj.getValue(i)*approximationValue);
            quadRowIndices.push_back(QObj.getColIndex(i));
            quadColIndices.push_back(QObj.getRowIndex(i));
        }
    }
    else {
        // 1/C_i * xi_i
        if (m_positiveResponses) // positive best responses -> 1/C * xi_i
            linearValues.push_back(1/approximationValue);
        else // negative best responses -> C * xi_i
            linearValues.push_back(approximationValue);
        linearIndices.push_back(getXiIndex(player));
    }

    // - eta_i
    linearValues.push_back(-1);
    // linearIndices.push_back(numberOfVariables-m_numberOfPlayers-1+player);
    linearIndices.push_back(getEtaIndex(player));

    // - lambda
    linearValues.push_back(-1);
    linearIndices.push_back(getLambdaIndex());

    MySparseVector linTerms = {linearValues,linearIndices};
    MySparseMatrix quadTerms = {quadValues,quadRowIndices,quadColIndices};
    return {linTerms,quadTerms,0.0};
}

vector<double> NEPMultiplicativeApprox::computeRegrets(vector<double> const &point) const {
    vector<double> regrets = {};
    for (indexx player = 0; player < m_numberOfPlayers; player++) {
        auto bestResponseValue = m_bestResponses[player].objective;
        auto responseValue = evalResponseValue(player, point);
        if (isApproxZero(bestResponseValue)) {
            if (compareApproxLess(responseValue,0)) // same as best response value
                regrets.push_back(1);
            else // response value > 0
                regrets.push_back(MY_INF);
        } else if (bestResponseValue > 0) {
            // response value also > 0
            regrets.push_back(evalResponseValue(player, point) / m_bestResponses[player].objective);
        }
        else {
            // best response value < 0
            if (responseValue < 0) {
                /*regrets.push_back(1/((evalResponseValue(player, point) - m_bestResponses[player].objective)
                                    / m_bestResponses[player].objective + 1));*/
                regrets.push_back(m_bestResponses[player].objective / evalResponseValue(player, point));
            }
            else // response value >= 0
                regrets.push_back(MY_INF);
        }
    }
    return regrets;
}

double NEPMultiplicativeApprox::computeMaxRegretNE(vector<double> const &point,
                                             int verbosity) const {
    auto regrets = computeRegrets(point);
    if (verbosity >= 2)
        cout << *ranges::max_element(regrets.begin(), regrets.end()) << endl;
    return *ranges::max_element(regrets.begin(), regrets.end());
}

void NEPMultiplicativeApprox::printRegret(ostream &os, vector<double> const &point) const {
    auto regrets = computeRegrets(point);
    os << "(";
    for (auto it = regrets.begin(); it != regrets.end()-1; ++it) {
        os << *it << ",";
    }
    os << regrets[getNumberOfPlayer()-1] << ")";
}

double NEPMultiplicativeApprox::getThresholdPruning() const {
    return 0.0;
}

bool NEPMultiplicativeApprox::checkPlayerNECondition(
        vector<double> const &point,
        indexx player,
        double tolerance) const {
    auto bestResponseValue = m_bestResponses[player].objective;
    auto responseValue = evalResponseValue(player, point);
    if (bestResponseValue >= 0) {
        // definition of NE uses m_factor[player] >= 1
        // because the best response is nonnegative
        if (compareApproxLess(responseValue,
                          m_factor[player]*bestResponseValue,
                          tolerance)) {
            return true;
                          }
    }
    else {
        // definition of NE uses m_factor[player] <= 1
        // because the best response is negative
        if (compareApproxLess(responseValue,
                          1/m_factor[player]*bestResponseValue,
                          tolerance)) {
            return true;
                          }
    }
    return false;
}

bool NEPMultiplicativeApprox::checkNE(
        vector<double> const &point,
        double tolerance,
        int verbosity) const {
    for (indexx player = 0; player < m_numberOfPlayers; player++) {
        if (!checkPlayerNECondition(point, player, tolerance)) {
            if (verbosity >= 3)
                cout << "player " << player << " does not satisfy NE condition" << endl;
            return false;
        }
    }
    return true;
}

vector<double> NEPMultiplicativeApprox::extractNashEquilibrium(
        vector<double> nashEquilibrium,
        double tolerance) {
    if (m_isGNEP) {
        for (indexx player = 0; player < m_numberOfPlayers; player++) {
            // remove xi_player value
            nashEquilibrium.pop_back();
        }
    }
    // remove lambda value
    nashEquilibrium.pop_back();
    for (indexx player = 0; player < m_numberOfPlayers; player++) {
        // remove eta_player value
        nashEquilibrium.pop_back();
    }

    if (!checkNE(nashEquilibrium, tolerance, 0)) {
        // check if nashEquilibrium really is an NE
        stringstream ss;
        ss << "adding a vector as an NE but it is not an NE: " << nashEquilibrium << endl;
        throw logic_error(ss.str());
    }
    return nashEquilibrium;
}


vector<Cut> NEPMultiplicativeApprox::deriveEquilibriumCutForEachPlayer(
        shared_ptr<GRBModel> const &model,
        vector<double> const &point,
        double tolerance,
        int verbosity) const {
    vector<Cut> cuts = {};
    vector<int> const numberOfVariablesPerPlayer = getNumberOfVariablesPerPlayer();
    indexx startingVariableIndex = 0;
    // trying to add eta equilibrium cuts
    if (verbosity >= 3) {
        auto regrets = computeRegrets(point);
        cout << "regrets: " << regrets << endl;
    }
    for (indexx player = 0; player < m_numberOfPlayers; player++) {
        if (!testBestEtaPlayerPossible(player, point, tolerance, verbosity)) {
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

bool NEPMultiplicativeApprox::testBestXiPossible(
        indexx player,
        vector<double> const &point,
        double const &tolerance,
        int verbosity) const {
    // evaluate pi_i(x^*)
    double responseValue = evalResponseValue(player, point);
    // comparison between xi_i and pi_i(x^*)
    double xiValue = point[getXiIndex(player)];
    return compareApproxLess(responseValue, xiValue, tolerance);
}

vector<Constraint> NEPMultiplicativeApprox::getNEFreeSetXiConcave(
        indexx player) const {
    // only one constraint: xi_i <= pi_i(x), with pi_i concave in x and linear in x_-i
    // the constraint has to be of the form ax + x Qobj x <= b
    // pi_i can be written: pi_i(x) = cx + xQx
    // linear terms: xi_i - cx
    // quadratic terms: -xQx
    // constant terms (in the rhs): 0

    vector<Constraint> NEFreeSet;
    MySparseVector c = m_bestResponseProblems[player]->getC();
    MySparseMatrix Qobj = m_bestResponseProblems[player]->getQ();
    MySparseVector ax;
    MySparseMatrix Q;
    double b = 0;

    // linear term
    indexx variableXiIndex = getXiIndex(player);
    ax.addCoefficient(1, variableXiIndex);
    for (indexx i = 0; i < c.size(); i++) {
        ax.addCoefficient(-c.getValue(i), c.getIndex(i));
    }

    // quadratic term
    for (indexx i = 0; i < Qobj.size(); i++) {
        Q.addCoefficient(-Qobj.getValue(i), Qobj.getRowIndex(i), Qobj.getColIndex(i));
    }

    NEFreeSet.push_back({ax,Q,b});
    return NEFreeSet;
}

vector<Constraint> NEPMultiplicativeApprox::getNEFreeSetXiConvex(
        indexx player,
        vector<double> const &point) const {
    // only one constraint: xi_i <= pi_i(xstar) + grad_pi_i(xstar)(x-xstar),
    // with pi_i convex in x and linear in x_-i
    // the constraint has to be of the form ax + x Qobj x <= b
    // pi_i can be written: pi_i(x) = cx + xQx
    // grad_pi_i(x): c + xQ + x Qtranspose
    // linear terms: xi_i - cx - xstar Q x - x Q xstar
    // quadratic terms: 0
    // constant terms (in the rhs): - xstar Q xstar

    vector<Constraint> NEFreeSet;
    MySparseVector c = m_bestResponseProblems[player]->getC();
    MySparseMatrix Qobj = m_bestResponseProblems[player]->getQ();
    MySparseVector ax;
    double b = 0;

    // linear term
    indexx variableXiIndex = getXiIndex(player);
    ax.addCoefficient(1, variableXiIndex);
    for (indexx i = 0; i < c.size(); i++) {
        ax.addCoefficient(-c.getValue(i), c.getIndex(i));
    }
    for (indexx i = 0; i < Qobj.size(); i++) {
        ax.addCoefficient(-Qobj.getValue(i)*point[Qobj.getRowIndex(i)], Qobj.getColIndex(i));
        ax.addCoefficient(-Qobj.getValue(i)*point[Qobj.getColIndex(i)], Qobj.getRowIndex(i));
    }

    // constant term
    for (indexx i = 0; i < Qobj.size(); i++) {
        b += -point[Qobj.getRowIndex(i)] * Qobj.getValue(i) * point[Qobj.getColIndex(i)];
    }

    NEFreeSet.push_back({ax,{},b});
    return NEFreeSet;
}

std::vector<Constraint> NEPMultiplicativeApprox::getNEFreeSetXiLipschitz(
        indexx player,
        vector<double> const &point) {
    // two constraints:
    // 1) ||x-xstar||_2 < (pi_i(xstar) - xi_istar) / (1 + L_i)
    // 2) xi_i < xi_istar + (pi_i(xstar) - xi_istar) / (1 + L_i)
    // where L_i is the lipschitz constant of the cost of player i

    // compute local lipschitz constants
    indexx xiPlayerIndex = getXiIndex(player);
    double numerator = evalResponseValue(player, point) - point[xiPlayerIndex];
    m_lipschitzConstants[player] = computeLocalLipschitzConstantImplementationGame(
        player,
        m_instanceObject,
        m_bestResponseProblems,
        point,
        numerator);

    vector<Constraint> NEFreeSet;
    double denominator = 1 + m_lipschitzConstants[player];

    // 1) everything is squared so that the square root disappears and
    // it can be written in a quadratic form:
    // ||x-xstar||_2^2 < (pi_i(xstar) - xi_istar)^2 / (1 + L_i)^2
    // written in the form ax + xQx <= b:
    // a = -2 xstar
    // Q = Id
    // b = (pi_i(xstar) - xi_istar)^2 / (1 + L_i)^2 - ||xstar||^2
    MySparseVector a;
    MySparseMatrix Q;
    double normSquaredXstar = 0;
    for (indexx k = 0; k < getTotalNumberOfVariablesOfPlayers(); k++) {
        a.addCoefficient(-2*point[k], k);
        Q.addCoefficient(1, k, k);
        normSquaredXstar += point[k]*point[k];
    }
    double b = numerator*numerator / (denominator*denominator) - normSquaredXstar;
    NEFreeSet.push_back({a,Q,b});

    // 2) no need to square everything
    // written in the form ax + xQx <= b:
    // a = indicator of xi_i
    // Q = 0
    // b = xi_istar + (pi_i(xstar) - xi_istar) / (1 + L_i)
    MySparseVector a2 = {{1}, {xiPlayerIndex}};
    double b2 = point[xiPlayerIndex] + numerator / denominator;
    NEFreeSet.push_back({a2,{},b2});
    return NEFreeSet;
}

vector<Cut> NEPMultiplicativeApprox::deriveICForEachPlayer(
        shared_ptr<GRBModel> const &nodeModel,
        vector<double> const &point,
        string const& whichCutString,
        double tolerance,
        int verbosity) {
    vector<Cut> cuts = {};
    int intersectionCutAlreadyComputed = 0;
    auto const setupIC = initializeSetupIC(nodeModel, verbosity);
    MatrixXd extremeRays = buildExtremeRays(nodeModel, setupIC, 0);
    vector<Constraint> NEFreeSet;
    for (indexx player = 0; player < m_numberOfPlayers; player++) {
        bool cutForPlayer = false;
        auto timerIC = startChrono();
        // first test if a cut for eta or for xi has to be derived
        // cuts for xi have priority, so check that first
        if (!testBestXiPossible(player, point, tolerance)) {
            cutForPlayer = true;
            // derive NE-free set depending on whichCutString
            if (whichCutString == "intersectionCutsXiConcave") {
                // cout << "concave xi cut for player " << player << endl;
                NEFreeSet = getNEFreeSetXiConcave(player);
            }
            if (whichCutString == "intersectionCutsXiConvex") {
                // cout << "convex xi cut for player " << player << endl;
                NEFreeSet = getNEFreeSetXiConvex(player, point);
            }
            if (whichCutString == "intersectionCutsLipschitz") {
                cout << "lipschitz xi cut for player " << player << endl;
                NEFreeSet = getNEFreeSetXiLipschitz(player, point);
            }
            if (verbosity >= 3) {
                for (Constraint const& constraint : NEFreeSet)
                    cout << constraint << endl;
            }
        }
        else if (!testBestEtaPlayerPossible(player, point, tolerance)) {
            cutForPlayer = true;
            // derive eta-cut
            NEFreeSet = getNEFreeSetEta(player, setupIC.numDeclaredVariables);
        }
        else
            if (verbosity >= 2)
                cout << "no cut for player " << player << endl;

        if (cutForPlayer) {
            Cut cut = deriveICForOnePlayer(NEFreeSet,
                                           player,
                                           point,
                                           nodeModel,
                                           extremeRays,
                                           setupIC);
            if (verbosity >= 3) {
                cout << "intersection cut for player " << player;
                cout << " (" << nodeModel->get(GRB_IntAttr_NumConstrs) << " constraints, ";
                cout << "IC computed in " << elapsedChrono(timerIC) << " seconds)" << endl;
                cout << "cut:     " << cut << endl;
            }
            cuts.push_back(cut);
            intersectionCutAlreadyComputed++;
            checkICSatisfaction(cut, player, point, nodeModel, extremeRays, setupIC, NEFreeSet);
        }
    }
    return cuts;
}


CutInformation NEPMultiplicativeApprox::deriveCut(
        vector<double> const &point,
        string const &whichCutString,
        double const &tolerance,
        shared_ptr<GRBEnv> const &globalEnv,
        shared_ptr<GRBModel> const &nodeModel,
        shared_ptr<OptimizationProblem> const &nodeProblem,
        int verbosity) {
    if (whichCutString == "eqCuts") {
        return {deriveEquilibriumCutForEachPlayer(nodeModel, point, tolerance, verbosity),{}};
    }
    if (whichCutString == "intersectionCutsXiConcave"
        or whichCutString == "intersectionCutsXiConvex"
        or whichCutString == "intersectionCutsLipschitz") {
        return {deriveICForEachPlayer(nodeModel, point, whichCutString, tolerance, verbosity),{}};
    }

    // after all declared cut types
    throw runtime_error("Wrong input whichCutString: string '"
                            + whichCutString + "' describing cut has not "
                            "been coded in NEPMultiplicativeApprox::deriveCut");
}

void NEPMultiplicativeApprox::setPositiveResponses(bool b) {
    m_positiveResponses = b;
}

void NEPMultiplicativeApprox::setLipschitzConstants(
        string const &gameTypeString,
        string const &whichCutString,
        string const &instanceName) {
    if (gameTypeString == "implementationGame" and whichCutString == "intersectionCutsLipschitz")
    m_lipschitzConstants =
        computeAllLipschitzConstantImplementationGame(instanceName, m_bestResponseProblems);
    else {
        if (whichCutString == "intersectionCutsLipschitz") {
            throw logic_error("for now the computation of lipschitz constants of the costs "
                "of player is only implemented for gameTypeString == implementationGame");
        }
    }
}
