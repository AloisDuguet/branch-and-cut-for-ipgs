//
// Created by alois-duguet on 9/19/25.
//

#include "BranchAndCutMultiplicativeGNEP.h"

#include <vector>
#include <memory>
#include <chrono>

#include "NashEquilibriumProblem.h"
#include "BranchAndCutTools.h"
#include "NEPMultiplicativeApprox.h"

using namespace std;

BranchAndCutMultiplicativeGNEP::BranchAndCutMultiplicativeGNEP(
    vector<double> & factor,
    shared_ptr<NashEquilibriumProblem> const& NEPInit,
    string const& gameTypeString,
    bool globalCutSwitch,
    string const& modelTypeString,
    string const& whichCutString,
    string const& branchingRuleString,
    string const& algorithmVariant,
    string const& instanceName,
    bool enumerateNE,
    double timeLimit,
    int verbosity,
    string const& resultFilename) :
        BranchAndCutForGNEP(
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
    // give proper size to factor in case it is defined as a vector of 1 element
    // indeed, it is then a shortcut for same approx value for each player
    cout << "factor before extension: " << factor << endl;
    if (factor.size() == 1 and NEPInit->getNumberOfPlayer() > 1) {
        for (indexx player = 1; player < NEPInit->getNumberOfPlayer(); player++)
            factor.push_back(factor[0]);
    }

    if (modelTypeString == "manyEtasGurobi") {
        if (gameTypeString != "implementationGame")
            m_NEP = make_shared<NEPMultiplicativeApprox>(
                factor,
                NEPInit->getBestResponseProblems());
        else
            m_NEP = make_shared<NEPMultiplicativeApprox>(
                factor,
                NEPInit->getBestResponseProblems(),
                instanceName);
    } else
        throw runtime_error("Wrong argument: NEP type '"
            + modelTypeString + "' not recognized.");

    // put feasibility tolerance of subproblem to the standard
    m_feasibilityTol = 1e-6;

    m_timeSpent += elapsedChrono(startTime);
}

void BranchAndCutMultiplicativeGNEP::initialize() {
    BranchAndCutForGNEP::initialize();

    setPositiveResponse();

    // compute and save lipschitz constants of costs of player
    // if the NE-free set using lipschitz constant is used
    if (m_gameTypeString == "implementationGame" and
        m_whichCutString == "intersectionCutsLipschitz") {
        m_NEP->setLipschitzConstants(m_gameTypeString, m_whichCutString, m_instanceName);
    }
}

void BranchAndCutMultiplicativeGNEP::setPositiveResponse() {
    // set if responses are positive or negative depending on gameTypeString
    if (m_gameTypeString == "NEPImplementationGame"
        or m_gameTypeString == "NEP-fullInteger"
        or m_gameTypeString == "NEP-mixedInteger"
        or m_gameTypeString == "GNEP-fullInteger"
        or m_gameTypeString == "implementationGame")
        m_NEP->setPositiveResponses(false);
    else if (m_gameTypeString == "GNEP-maxFlowGame") {
        // add gameTypeString with positive best responses to the test above
        m_NEP->setPositiveResponses(true);
    }
    else {
        throw logic_error("gameTypeString not linked to positive or negative best responses");
    }
}


void BranchAndCutMultiplicativeGNEP::checkValidArguments() const {
    // not used in the constructor because it is already used in the base constructor
    BranchAndCutForGNEP::checkValidArguments();
    if (m_gameTypeString != "implementationGame" and m_whichCutString == "intersectionCutsLipschitz")
        throw logic_error("for now the computation of lipschitz constants of the costs "
                "of player is only implemented for gameTypeString == implementationGame");
    if (m_whichCutString == "intersectionCutsXiConvex" and m_gameTypeString != "GNEP-fullInteger" and m_gameTypeString != "GNEP-maxFlowGame")
        throw logic_error("intersectionCutsXiConvex is valid only for GNEP-fullInteger instances;"
            " not for " + m_gameTypeString);
    if (m_whichCutString == "intersectionCutsXiConcave" and m_gameTypeString != "GNEP-fullInteger")
        throw logic_error("intersectionCutsXiConcave is valid only for GNEP-fullInteger instances;"
            " not for " + m_gameTypeString);
    // if (m_NEP->getIsGNEP())
    //     throw logic_error("GNEP not implemented for multiplicative approximate Nash equilibria.");
    // if (m_whichCutString == "intersectionCuts")
    //     throw logic_error("intersection cuts have not been proved to work with multiplicative approximate Nash equilibria.");
}

void BranchAndCutMultiplicativeGNEP::printParameters(ostream &os) const {
    BranchAndCutForGNEP::printParameters(os);
    os << endl;
    m_NEP->printEquilibriaSearched(cout);
}

void BranchAndCutMultiplicativeGNEP::solveNode(double solverTimeLimit) {
    // solve node problem
    if (m_verbosity >= 2)
        cout << solverTimeLimit << "s remaining. " ;
    if (solverTimeLimit < 0) {
        // can happen because the check happens before the computation of
        // solverTimeLimit. This will trigger a time limit from the solver,
        // and thus will be handled automatically
        solverTimeLimit = 0;
    }
    // using high numeric focus for the solver
    m_nodeModel = m_node->solveModel(m_globalEnv,
                                     solverTimeLimit,
                                     m_verbosity,
                                     true,
                                     m_feasibilityTol);
}

NEPBranchOutput* BranchAndCutMultiplicativeGNEP::solve() {
    // initialize timer for this method
    auto startTime = startChrono();
    initialize();
    writeRootNode();

    // print the approximate NE looked for
    m_NEP->printEquilibriaSearched(cout);

    try {
        while ((m_nodeSelector->nodeListSize() > 0 or m_reuseNode) and checkTimeLimit(startTime)) {
            getNewNode();
            printNodeInformation(elapsedChrono(startTime));

            solveNode(computeSolverTimeLimit(startTime));
            checkTime(startTime);

            if (!m_node->isSolved()) {
                // handles unsolved nodes
                unsolvedNodeHandler();
            } else if (!compareApproxLess(m_node->getSolution().objective,
                    m_NEP->getThresholdPruning(),
                    m_pruningZeroObjTolerance)) {
                // prune the node because objective > delta
                if (m_verbosity >= 2)
                    cout << "pruning: node obj > threshold" << endl;
            }
            else {
                if (m_node->isIntegerSolution(m_integralityTolerance)) {
                    auto solution = computeBestResponseHandler(computeSolverTimeLimit(startTime));
                    checkBestResponseValueSigns();
                    checkTime(startTime);
                    detectCycling(solution);
                    if (m_NEP->checkNE(solution, m_NETolerance, m_verbosity)) {
                        if (NEHandler(solution))
                            break;
                    } else {
                        // add a cut and iterate on the same Node
                        addCuts(solution);
                    }
                } else {
                    // usual branching case
                    branchingHandler();
                }
                updateLastNodeSolution();
            }
        }

        auto output = returnHandler(startTime);
        return output;

    } catch (exception const& e) {
        auto output = returnHandler(startTime, e.what());
        return output;
    }
}

void BranchAndCutMultiplicativeGNEP::checkBestResponseValueSigns() const {

    if (m_verbosity >= 3) {
        for (indexx player = 0; player < m_NEP->getNumberOfPlayer(); player++) {
            cout << "cost of player " << player << ": "
                 << m_NEP->evalResponseValue(player, m_node->getSolution().solution) << endl;
        }
    }
}

bool BranchAndCutMultiplicativeGNEP::NEHandler(vector<double> const &solution) {
    // tolerance not the default one because when converging to an NE,
    // the node can be pruned by eta values too close to BR values. That makes pruning
    // happen before the case where an NE is found

    if (m_verbosity >= 1) {
        m_NEP->printApproximation(cout);
        cout << "-NE found with regrets ";
        m_NEP->printRegret(cout, solution);
        cout << endl;
    }
    m_nodeSelector->incrementSolutionList(m_NEP,
                                          solution,
                                          computeSocialWelfare(m_node->getSolution()),
                                          m_NEP->computeRegrets(solution),
                                          m_NETolerance);
    m_NEFound = true;
    if (m_verbosity >= 1)
        cout << "stopping at first NE found" << endl << endl;
    return true;
}

bool BranchAndCutMultiplicativeGNEP::detectCycling(vector<double> const& solution) {
    if (m_numberOfCutsOfCurrentNode > 0) {
        // m_numberOfCutsOfCurrentNode > 0 means that the solution from
        // the last iteration is from the same node as the current one
        double dist = norm2(solution, m_lastIterationSolution);
        // warning that there might be numerical issues with the cuts:
        if (dist < 1e-3) {
            cout << "solution and last solution quite close in norm 2: "
                 << format("{:.7f}", dist) << endl;
            for (int i = 0; i < m_lastIterationSolution.size(); i++)
                if (m_lastIterationSolution[i]-solution[i] != 0)
                    cout << "index " << i << " are "
                         << format("{:.7f}", m_lastIterationSolution[i])
                         << " and " << format("{:.7f}", solution[i]) << endl;
        }
        if (m_lastIterationSolution == solution)
            // cycling because the solution of the last iteration has not been cut off
                return true;
    } else
        m_rhsBonusForRestrictedCut = m_rhsBaseBonusForRestrictedCut;
    return false;
}

void BranchAndCutMultiplicativeGNEP::writeOneLineResult(
        NEPBranchOutput const &output,
        ostream &os) const {
    // separator
    string const sep1 = ",";

    // start of the line
    os << "result,";

    // problem solved: multiplicative approx-NE
    os << "multiplicativeNE-";
    m_NEP->printApproximation(os);
    os << sep1;

    writeEndOfOneLineResult(output, os);
}

void BranchAndCutMultiplicativeGNEP::logCutNotAdded() const {
    if (m_verbosity == 1) {
        cout << "cut not added because it does not cut off the solution" << endl;
    }
    else if (m_verbosity >= 2) {
        cout << "approximation factor: " << m_NEP->getApproximationPlayer(0) << endl;
        cout << "cut not added because it does not cut off the solution" << endl;
    }
}

void BranchAndCutMultiplicativeGNEP::logNoCutAdded() const {
    cout << "approximation factor: " << m_NEP->getApproximationPlayer(0) << endl;
    cout << "the first lambda constraint should be:" << endl;
    cout << m_NEP->buildLambdaConstraint(0) << endl;
}

