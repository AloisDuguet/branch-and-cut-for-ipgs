//
// Created by alois-duguet on 10/6/25.
//

#include "BestMultiplicativeApproxGNEP.h"

#include <algorithm>
#include <cmath>

#include "NodeGurobi.h"
#include "CommonlyUsedFunctions.h"

using namespace std;

BestMultiplicativeApproxGNEP::BestMultiplicativeApproxGNEP(
        vector<double> & factor,
        std::shared_ptr<NashEquilibriumProblem> const& NEPInit,
        std::string const& gameTypeString,
        bool globalCutSwitch,
        std::string const& modelTypeString,
        std::string const& whichCutString,
        std::string const& branchingRuleString,
        std::string const& algorithmVariant,
        std::string const& instanceName,
        bool enumerateNE,
        double timeLimit,
        int verbosity,
        std::string const& resultFilename) :
            BranchAndCutMultiplicativeGNEP(
                factor,
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
        "minMultiplicativeNE-" + to_string(BestMultiplicativeApproxGNEP::getApproximation());

    m_minApproximationPossible = 1;
    m_originalMinApproximationPossible = m_minApproximationPossible;
    m_toleranceMinApproximation = 0.01;
    m_exactNEPossible = true;
    m_bestApproximationFound = MY_INF;
    m_queueState = make_unique<DFS>();
    m_approximationValueOfLambdaConstraintsInQueueState = BestMultiplicativeApproxGNEP::getApproximation();
    m_currentNumberOfCutsDropped = 0;
    m_timeSpent += elapsedChrono(startTime);
    m_globalEnv->set(GRB_DoubleParam_SoftMemLimit, 3);

    m_timeSpent += elapsedChrono(startTime);
}

void BestMultiplicativeApproxGNEP::printParameters(std::ostream &os) const {
    BranchAndCutMultiplicativeGNEP::printParameters(os);
    os << "min approximation proved included in [" << m_minApproximationPossible
       << "," << m_bestApproximationFound << "]" << std::endl;
    os << "currently looking for a " << to_string(getApproximation()) << "-NE" << endl;
}

void BestMultiplicativeApproxGNEP::setApproximation(double approximationValue) const {
    for (indexx player = 0; player < m_NEP->getNumberOfPlayer(); player++)
        m_NEP->setApproximationPlayer(approximationValue, player);
}

double BestMultiplicativeApproxGNEP::getApproximation() const {
    // the approximation value is the same for all players
    return m_NEP->getApproximationPlayer(0);
}

BestMultiplicativeBranchOutput* BestMultiplicativeApproxGNEP::solve() {
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

void BestMultiplicativeApproxGNEP::whileLoopSolve(chrono::time_point<chrono::system_clock> startTime) {
    while ((m_nodeSelector->nodeListSize() > 0 or m_reuseNode) and checkTimeLimit(startTime)) {
        getNewNode();
        printNodeInformation(elapsedChrono(startTime));
        solveNode(computeSolverTimeLimit(startTime));
        checkTime(startTime);

        if (!m_node->isSolved()) {
            unsolvedNodeHandler(startTime);
        } else if (!compareApproxLess(m_node->getSolution().objective,
                m_NEP->getThresholdPruning(), m_pruningZeroObjTolerance)) {
            // prune the node because objective > delta
            if (m_verbosity >= 2)
                cout << "pruning: node obj > approximation value" << endl;
        }
        else {
            if (m_node->isIntegerSolution(m_integralityTolerance)) {
                auto solution = computeBestResponseHandler(computeSolverTimeLimit(startTime));
                checkBestResponseValueSigns();
                checkTime(startTime);
                detectCycling(solution);
                if (checkNE(solution)) {
                    if (NEHandler(solution))
                        break;
                } else {
                    cutOrBranch(solution);
                }
            } else {
                // usual branching case
                branchingHandler();
            }
            updateLastNodeSolution();
        }
        if (m_nodeSelector->nodeListSize() == 0 and !m_reuseNode)
            noNEHandler();
    }
}

bool BestMultiplicativeApproxGNEP::checkNE(std::vector<double> const &solution) const {
    return m_NEP->checkNE(solution, m_NETolerance, m_verbosity);
}

double BestMultiplicativeApproxGNEP::computeMinApproximationParameter(
        vector<double> const &solution) const {
    double const originalApprox = getApproximation();
    double maxApprox = originalApprox;
    double minApprox = m_minApproximationPossible;
    double currentApprox = (maxApprox + minApprox) / 2;
    double const binarySearchTolerance = 1e-4*(maxApprox-minApprox);

    // before binary search, try with minApprox because it cannot be reached in while loop
    setApproximation(minApprox);
    if (m_NEP->checkNE(solution, 1e-8, m_verbosity)) {
        setApproximation(originalApprox);
        return minApprox;
    }

    // binary search in [minApprox,maxApprox]
    while (maxApprox - minApprox > binarySearchTolerance) {
        setApproximation(currentApprox);
        // tolerance for NE = 1e-8 (close to but not zero) because
        // we are looking for the best value while knowing it is an NE
        // if we used m_NETolerance it would systematically find a slightly smaller value than what is real,
        // with 1e-8 it is less impacting and still removes the problem that can occur with a tolerance of 0
        if (m_NEP->checkNE(solution, 1e-8, m_verbosity)) {
            maxApprox = currentApprox;
        } else {
            minApprox = currentApprox;
        }
        currentApprox = (maxApprox + minApprox) / 2;
    }

    // put the original multiplicative approximation because this method is not responsible for changing it
    setApproximation(originalApprox);

    return maxApprox;
}

double BestMultiplicativeApproxGNEP::computeNEWithMinMultiplicativeAndFixedAdditive(
        vector<double> const &solution) const {
    // binary search up to tolerance for minimum value of factor such that
    // solution is a (factor,additiveApprox)-NE

    if (m_verbosity >= 3)
        cout << "computing minimum multiplicative approximation value of the NE, "
                "given the additive approximation value" << endl;

    return computeMinApproximationParameter(solution);
}

bool BestMultiplicativeApproxGNEP::NEHandler(std::vector<double> const &solution) {
    // a getApproximation()-NE has been found
    // but a potentially better delta has been found: a maxRegret-NE has been found
    double maxRegret = computeNEWithMinMultiplicativeAndFixedAdditive(solution);
    m_approximationValueOfLambdaConstraintsInQueueState = maxRegret;

    logNEFound(maxRegret);

    // TODO: 'regrets' for 4th argument of incrementSolutionList
    // can be computed player by player in a similar way as
    // with method computeNEWithMinMultiplicativeAndFixedAdditive
    m_nodeSelector->incrementSolutionList(m_NEP,
                                          solution,
                                          computeSocialWelfare(m_node->getSolution()),
                                          m_NEP->differencesToBestResponseValues(solution),
                                          m_NETolerance);

    return checkAndChangeApproximation(maxRegret);
}

void BestMultiplicativeApproxGNEP::logNEFound(double maxRegret) const {
    if (m_verbosity >= 1) {
        cout << "----> "
             << getApproximation()
             << "-NE found with max regret "
             << maxRegret
             << " in Node "
             << m_node->getNodeNumber()
             << endl;
        cout << "number of cuts added to the model at the moment: "
             << m_totalNumberOfCuts - m_currentNumberOfCutsDropped << endl;
    }
}

bool BestMultiplicativeApproxGNEP::checkAndChangeApproximation(double const maxRegret) {
    m_NEFound = true;
    m_bestApproximationFound = maxRegret;
    // stopping when range approximationValue < tolerance:
    // in case bestApproximationFound - minApproximationPossible == tolerance, use m_exactNEPossible:
    // if an exact NE is still possible, continue. Else, stop
    if (m_bestApproximationFound - m_minApproximationPossible <= m_toleranceMinApproximation and
        (!m_exactNEPossible or compareApproxEqual(maxRegret, m_originalMinApproximationPossible))) {
        if (m_verbosity >= 1) {
            cout << "stopping because best approximate NE possible is found (up to "
                 << m_toleranceMinApproximation << ")" << endl << endl;
        }
        return true;
        // the else case is treated later on as a special case to try to find an exact NE
    } else {
        auto const startSaveQueue = startChrono();
        // currentApproximation is not the best approximation that one can hope.
        // reduce its value, save queue state, set reuseNode to true to not close the current node
        //TODO: set new approximationValue to arithmetic or geometric mean?
        setApproximation((m_bestApproximationFound + m_minApproximationPossible)/2);
        // special case: try 1-NE if delta is sufficiently close to 1
        if (m_bestApproximationFound - m_minApproximationPossible <= m_toleranceMinApproximation and
            m_exactNEPossible) {
            setApproximation(m_originalMinApproximationPossible);
        }

        if (m_algorithmVariant == "reuseTreeSearch" or m_algorithmVariant == "reuseWithoutCuts") {

            // temporarily add current node so that it is copied with the remaining unexplored nodes
            m_nodeSelector->incrementNodeList(m_node->clone());

            if (m_algorithmVariant == "reuseTreeSearch") {
                // adapt constraints of the root node involving lambda with the new approximationValue
                adaptTreeSearchToNewApproximation(false);
            } else if (m_algorithmVariant == "reuseWithoutCuts") {
                adaptTreeSearchToNewApproximation(true);
                // empty cuts of m_node separately because it is only a copy that is in the node list
                m_node->emptyCuts();
            }

            m_queueState = m_nodeSelector->clone();
            m_nodeSelector->removeLastNode();

            // finally update model of m_node, since cuts from which it was built have changed
            m_node->buildModel();

            // reuse current node with adapted lambda constraints
            m_reuseNode = true;

            if (m_verbosity >= 1) {
                double timeSaveQueue = elapsedChrono(startSaveQueue);
                cout << "time to save queue (" << m_queueState->nodeListSize() << " nodes)"
                        ", adapt lambda constraints and rebuild model: " << timeSaveQueue
                     << " seconds" << endl;
                cout << format("{:.{}g}", m_bestApproximationFound, getPrecisionFloatFormat(m_bestApproximationFound)) << "-NE found, trying to find a ";
                m_NEP->printApproximation(cout);
                cout << "-NE now" << endl;
            }
        } else if (m_algorithmVariant == "basicAlgorithm") {
            if (m_verbosity >= 1) {
                cout << "resetting queue to root node" << endl;
                cout << format("{:.{}g}", m_bestApproximationFound, getPrecisionFloatFormat(m_bestApproximationFound)) << "-NE found, trying to find a ";
                m_NEP->printApproximation(cout);
                cout << "-NE now" << endl;
            }
            // rebuild root node and nodeSelector
            m_node = resetBranchingWithSolutionList();
        } else {
            throw logic_error("algorithmVariant " + m_algorithmVariant + " not recognised");
        }

        return false;
    }
}

void BestMultiplicativeApproxGNEP::noNEHandler() {
    m_exactNEPossible = false;

    // update min approximation value feasible
    m_minApproximationPossible = getApproximation();

    logNoNEFound();

    // update approximation value used
    if (m_bestApproximationFound != MY_INF)
        setApproximation((m_minApproximationPossible + m_bestApproximationFound)/2);
    else
        setApproximation(m_minApproximationPossible*10);

    // check if precision on best delta-NE is sufficient
    if (m_bestApproximationFound - m_minApproximationPossible <= m_toleranceMinApproximation) {
        if (m_verbosity >= 1)
            cout << "stopping because best approximation has been found up to tolerance "
             << m_toleranceMinApproximation << endl;
    } else {
        cout << "trying to find a ";
        m_NEP->printApproximation(cout);
        cout << "-NE" << endl;

        if (m_algorithmVariant == "reuseTreeSearch" or m_algorithmVariant == "reuseWithoutCuts") {
            // recover queue state from best delta-NE found
            if (m_bestApproximationFound != MY_INF) {
                recoverQueueState();

                // give m_node a node of the queue, which can not be empty here
                m_node = m_nodeSelector->getlastNode();

                // adapt constraints of the root node involving lambda with the new approximationValue
                adaptTreeSearchToNewApproximation(false);

                // save again queue state in case of failing to find a better delta-NE
                m_queueState = m_nodeSelector->clone();

                // no need to rebuild model of current node since it was proven infeasible
            }
            else {
                cout << "rebuilding root node" << endl;
                // no approximate NE found up to now, do as for basicAlgorithm
                m_node = resetBranchingWithSolutionList();
            }
        } else if (m_algorithmVariant == "basicAlgorithm") {
            // rebuild root node and nodeSelector
            m_node = resetBranchingWithSolutionList();
        } else {
            throw logic_error("algorithmVariant " + m_algorithmVariant + " not recognised");
        }
    }
}

void BestMultiplicativeApproxGNEP::logNoNEFound() const {
    if (m_verbosity >= 1) {
        cout << "----> tree search finished with no " << m_minApproximationPossible
             << "-NE found in Node " << m_node->getNodeNumber() << endl;
        cout << "number of cuts added to the model at the moment: "
             << m_totalNumberOfCuts - m_currentNumberOfCutsDropped << endl;
        cout << "best delta-NE for delta is in ]" << m_minApproximationPossible
             << "," << m_bestApproximationFound << "]" << endl;
    }
}

shared_ptr<Node> BestMultiplicativeApproxGNEP::resetBranchingWithSolutionList() {
    m_rootNode->setSubproblem(m_NEP->buildOptimizationProblem());
    // no need to copy the empty solution list but it is not an issue
    m_nodeSelector = shared_ptr<NodeSelector>(
        m_nodeSelector->createNewWithSolutionList());
    return m_nodeSelector->buildChildOfRootNode(m_rootNode);
}

void BestMultiplicativeApproxGNEP::recoverQueueState() {
    m_nodeSelector = std::move(m_queueState);
}

indexx BestMultiplicativeApproxGNEP::findFirstLambdaConstraint(vector<Constraint> const& constraints, indexx const lambdaIndexInProblem) {
    indexx posLambdaConstraint = 0;
    while (posLambdaConstraint < constraints.size()) {
        bool isLambdaUsed = false;
        auto v = constraints[posLambdaConstraint].getAx();
        for (indexx k = 0;k < v.size(); k++)
            if (v.getIndex(k) == lambdaIndexInProblem)
                isLambdaUsed = true;
        if (isLambdaUsed)
            break;
        posLambdaConstraint++;
    }
    return posLambdaConstraint;
}

void BestMultiplicativeApproxGNEP::updateLambdaConstraints(vector<Constraint> & constraints, indexx const posLambdaConstraint) {
    for (indexx player = 0; player < m_NEP->getNumberOfPlayer(); player++)
        constraints[posLambdaConstraint+player] = m_NEP->buildLambdaConstraint(player);
}

void BestMultiplicativeApproxGNEP::adaptNodeToNewApproximation(shared_ptr<Node> const& node, indexx lambdaIndexInProblem, bool removeCuts) {
    auto constraints = node->getRootNode()->getSubproblem()->getConstraints();
    indexx posLambdaConstraint = findFirstLambdaConstraint(constraints, lambdaIndexInProblem);

    updateLambdaConstraints(constraints, posLambdaConstraint);

    if (removeCuts) {
        // global cuts are added after lambda constraints in the root node
        while (constraints.size() > posLambdaConstraint + m_NEP->getNumberOfPlayer())
            constraints.pop_back();
    }
    node->getSubproblem()->setConstraints(constraints);
}

void BestMultiplicativeApproxGNEP::adaptTreeSearchToNewApproximation(bool const removeCuts) {
    // find position of first constraint with lambda coef, all the others follow
    indexx lambdaIndexInProblem = m_NEP->getTotalNumberOfVariablesOfPlayers()+m_NEP->getNumberOfPlayer();

    // update lambda constraints of root node + remove global cuts in subproblem of root node
    auto newRootNode = m_node->getRootNode();
    adaptNodeToNewApproximation(newRootNode, lambdaIndexInProblem, removeCuts);

    if (!m_globalCutSwitch and removeCuts) {
        // local cuts
        // reset attribute m_cuts of all nodes and hidden nodes of the tree search
        // no need to remove cuts from the subproblem attribute of those nodes
        // because they are not reused (except for the root node)
        m_nodeSelector->emptyAllCuts();
    }

}

void BestMultiplicativeApproxGNEP::unsolvedNodeHandler(chrono::time_point<chrono::system_clock> startTime) {
    if (m_node->getSolution().status == 4 || m_node->getSolution().status == 3) {
        if (m_verbosity >= 2)
            cout << "pruning: node infeasible" << endl;
    }
    else if (m_node->getSolution().status == 9) {
        // time limit reached
        // the node has not been explored so it is kept open
        m_reuseNode = true;
    }
    else if (m_node->getSolution().status == 17) {
        // soft memory limit reached in subsolver
        throw runtime_error("out of memory during subproblem after " +
            to_string(elapsedChrono(startTime) + m_timeSpent) + "s");
        /*if (m_verbosity >= 1) {
            cout << "branching on Node " << m_node->getNodeNumber()
            << " because subsolver out of memory" << endl;
        }
        branchingHandler();*/
    }
    else if (m_node->getSolution().status == 10003) {
        // Inf value in model
        // can be produced by current approximation parameter reaching max of double values
        throw runtime_error("subproblem errored because of infinite value in the model");
    }
    else if (m_node->getSolution().status != 2
             and m_node->getSolution().status != 4
             and m_node->getSolution().status != 3)
        throw logic_error("stopping the algorithm because of "
                          "abnormal optimization status code " +
                          to_string(m_node->getSolution().status));
}

BestMultiplicativeBranchOutput *BestMultiplicativeApproxGNEP::returnHandler(
        chrono::time_point<chrono::system_clock> startTime,
        string errorMsg) {
    string optimizationStatus = buildOptimizationStatusString(startTime, errorMsg);
    auto output = new BestMultiplicativeBranchOutput(optimizationStatus,
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
                                  m_bestApproximationFound);

    // print results
    cout << *output << endl;

    // print one-line result in terminal
    oneLineResultHandler(*output);

    return output;
}

void BestMultiplicativeApproxGNEP::oneLineResultHandler(BestMultiplicativeBranchOutput const &output) const {
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

void BestMultiplicativeApproxGNEP::writeOneLineResult(
        BestMultiplicativeBranchOutput const &output,
        ostream &os) const {
    // separator
    string const sep1 = ",";

    // start of the line
    os << "result,";

    // problem solved: min delta-NE
    os << m_tagProblemPlusInitialApproximationValue << sep1;

    writeEndOfOneLineResult(output, os);
}

std::string BestMultiplicativeApproxGNEP::buildOptimizationStatusString(
        chrono::time_point<chrono::system_clock> startTime,
        string errorMsg) {
    // build NEPOutput
    m_timeSpent += elapsedChrono(startTime);
    string optimizationStatus = "UNKNOWN";
    if (errorMsg == "no error") {
        if (m_timeSpent > m_timeLimit - m_bufferTime) {
            optimizationStatus = "TIME_LIMIT_REACHED";
        } else {
            if (m_nodeSelector->getSolutionList().empty())
                optimizationStatus = "NO_SOLUTION_FOUND";
            else
                optimizationStatus = "SOLUTION_FOUND";
        }
    } else {
        // case of error caught
        cerr << errorMsg << endl;
        // remove all ',' because of csv usage
        for (char & c : errorMsg) {
            if (c == ',')
                c = ';';
        }
        optimizationStatus = "ERROR: " + errorMsg;
        // setting time spent to time limit so that analysis of the results
        // does not confuse an instance that errored with a properly solved instance
        m_timeSpent = m_timeLimit;
    }

    return optimizationStatus;
}

void BestMultiplicativeApproxGNEP::checkFeasibilityAndEquilibrium(std::vector<double> const &point) const {
    bool neFeasible = m_node->getSubproblem()->checkPointFeasibility(point);
    if (neFeasible) {
        cout << "point feasible for this node" << endl;
        if (bool neTest = checkNE(point))
            cout << "exact NE has been checked as an approximate NE in this node" << endl;
        else
            cout << "exact NE does not satisfy the approximate NE def in this node" << endl;
    } else {
        cout << "point not feasible for this node" << endl;
    }
}

void BestMultiplicativeApproxGNEP::handleCycling(std::vector<double> const &solution) {
    // if an NE was found last node solve, we don't need to check for cycling
    // because the NE condition cuts changed and thus the cuts derived too
    if (m_nodeSelector->getSolutionList().empty())
        BranchAndCutForGNEP::handleCycling(solution);
    else {
        auto lastNE = m_nodeSelector->getSolutionList().back().NE;
        // produce solution trimmed from non-strategy profile variables (proxy variables and lambda variable)
        vector<double> strategyProfile = {solution.begin(), solution.begin()+m_NEP->getTotalNumberOfVariablesOfPlayers()};

        if (!compareApproxEqual(strategyProfile, lastNE))
            BranchAndCutForGNEP::handleCycling(solution);
    }
}
