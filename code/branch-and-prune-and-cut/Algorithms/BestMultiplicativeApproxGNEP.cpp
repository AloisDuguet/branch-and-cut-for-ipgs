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
        "minMultiplicativeNE-" + to_string(m_NEP->getApproximationPlayer(0));

    m_NETolerance = 1e-5;
    m_minApproximationPossible = 1;
    m_toleranceMinApproximation = 0.1;
    m_exactNEPossible = true;
    m_bestApproximationFound = MY_INF;
    m_queueState = make_unique<DFS>();
    m_approximationValueOfLambdaConstraintsInQueueState = getApproximation();
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
                    // add a cut and iterate on the same Node
                    addCuts(solution);
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

double BestMultiplicativeApproxGNEP::computeNEWithMinMultiplicativeAndFixedAdditive(
        vector<double> const &solution,
        int verbosity) const {
    // binary search up to tolerance for minimum value of factor such that
    // solution is a (factor,additiveApprox)-NE

    double originalMultApprox = m_NEP->getApproximationPlayer(0);
    double maxMultApprox = originalMultApprox;
    double minMultApprox = m_minApproximationPossible;
    double currentMultApprox = (maxMultApprox + minMultApprox) / 2;
    double binarySearchTolerance = 1e-4*(maxMultApprox-minMultApprox);

    // before binary search, try with minMultApprox because it cannot be reached in while loop
    setApproximation(minMultApprox);
    if (m_NEP->checkNE(solution, 1e-8, m_verbosity)) {
        setApproximation(originalMultApprox);
        return minMultApprox;
    }

    // binary search in [minMultApprox,maxMultApprox]
    while (maxMultApprox - minMultApprox > binarySearchTolerance) {
        setApproximation(currentMultApprox);
        // tolerance for NE = 1e-8 (close to but not zero) because
        // we are looking for the best value while knowing it is an NE
        // if we used m_NETolerance it would systematically find a slightly smaller value than what is real,
        // with 1e-8 it is less impacting and still removes the problem that can occur with a tolerance of 0
        if (m_NEP->checkNE(solution, 1e-8, m_verbosity)) {
            maxMultApprox = currentMultApprox;
        } else {
            minMultApprox = currentMultApprox;
        }
        currentMultApprox = (maxMultApprox + minMultApprox) / 2;
    }

    // put the original multiplicative approximation because this method is not responsible for changing it
    setApproximation(originalMultApprox);

    return maxMultApprox;
}

bool BestMultiplicativeApproxGNEP::NEHandler(std::vector<double> const &solution) {
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

bool BestMultiplicativeApproxGNEP::checkAndChangeApproximation(double maxRegret) {
    m_NEFound = true;
    m_bestApproximationFound = maxRegret;
    // stopping when range approximationValue < tolerance:
    // in case bestApproximationFound - minApproximationPossible == tolerance, use m_exactNEPossible:
    // if an exact NE is still possible, continue. Else, stop
    if (m_bestApproximationFound - m_minApproximationPossible <= m_toleranceMinApproximation and
        (!m_exactNEPossible or compareApproxEqual(maxRegret, 1))) {
        if (m_verbosity >= 1) {
            cout << "stopping because best delta-NE possible is found (up to "
                 << m_toleranceMinApproximation << ")" << endl << endl;
        }
        return true;
        // the else case is treated later on as a special case to try to find an exact NE
    } else {
        auto startSaveQueue = startChrono();
        // currentApproximation is not the best delta that one can hope.
        // reduce its value, save queue state, set reuseNode to true to not close the current node
        setApproximation((m_bestApproximationFound + m_minApproximationPossible)/2);
        // special case: try 1-NE if delta is sufficiently close to 1
        if (m_bestApproximationFound - m_minApproximationPossible <= m_toleranceMinApproximation and
            m_exactNEPossible) {
            setApproximation(1);
        }

        if (m_algorithmVariant == "reuseTreeSearch" or m_algorithmVariant == "reuseWithoutCuts") {

            if (m_algorithmVariant == "reuseTreeSearch") {
                // adapt constraints of the root node involving lambda with the new approximationValue
                adaptLambdaConstraints(false);
            } else if (m_algorithmVariant == "reuseWithoutCuts") {
                if (m_globalCutSwitch == 1) {
                    // all cuts are in the root node subproblem
                    // reset root node without changing nodeSelector
                    // change lambda constraints and remove cuts
                    adaptLambdaConstraints(true);
                    // m_rootNode->setSubproblem(m_NEP->buildOptimizationProblem());
                } else {
                    throw runtime_error("reuseWithoutCuts for local cuts not implemented");
                    // implementation: go through all nodes and hidden nodes of the tree and reset m_cuts
                }
            }

            // temporarily add current node so that it is copied with the remaining unexplored nodes
            m_nodeSelector->incrementNodeList(m_node->clone());
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
                cout << m_bestApproximationFound << "-NE found, trying to find a "
                     << getApproximation() << "-NE now" << endl;
            }
        } else if (m_algorithmVariant == "basicAlgorithm") {
            if (m_verbosity >= 1) {
                cout << "resetting queue to root node" << endl;
                cout << m_bestApproximationFound << "-NE found, trying to find a "
                     << getApproximation() << "-NE now" << endl;
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

    // update approximation value used
    if (m_bestApproximationFound != MY_INF)
        setApproximation((m_minApproximationPossible + m_bestApproximationFound)/2);
    else
        setApproximation(m_minApproximationPossible*10);

    logNoNEFound();

    // check if precision on best delta-NE is sufficient
    if (m_bestApproximationFound - m_minApproximationPossible <= m_toleranceMinApproximation) {
        if (m_verbosity >= 1)
            cout << "stopping because best approximation has been found up to tolerance "
             << m_toleranceMinApproximation << endl;
    } else {
        cout << "trying to find a " << getApproximation() << "-NE" << endl;

        if (m_algorithmVariant == "reuseTreeSearch" or m_algorithmVariant == "reuseWithoutCuts") {
            // recover queue state from best delta-NE found
            if (m_bestApproximationFound != MY_INF) {
                recoverQueueState();

                // give m_node a node of the queue, which can not be empty here
                m_node = m_nodeSelector->getlastNode();

                // adapt constraints of the root node involving lambda with the new approximationValue
                adaptLambdaConstraints(false);

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

void BestMultiplicativeApproxGNEP::adaptLambdaConstraints(bool removeCuts) const {
    // find position of first constraint with lambda coef, all the others follow
    indexx posLambdaConstraint = 0;
    auto newRootNode = m_node->getRootNode();
    // indexx lambdaIndexInProblem = newRootNode->getSubproblem()->getNumberOfVariables()-1;
    indexx lambdaIndexInProblem = m_NEP->getTotalNumberOfVariablesOfPlayers()+m_NEP->getNumberOfPlayer();
    auto constraints = newRootNode->getRootNode()->getSubproblem()->getConstraints();
    while (posLambdaConstraint < constraints.size()) {
        bool isLambdaUsed = false;
        auto v = constraints[posLambdaConstraint].getAx();
        for (indexx k = 0;k < v.size(); k++) {
            if (v.getIndex(k) == lambdaIndexInProblem) {
                isLambdaUsed = true;
            }
        }
        if (isLambdaUsed) {
            break;
        }
        else
            posLambdaConstraint++;
    }

    // rebuild them with the new approximation value
    for (indexx player = 0; player < m_NEP->getNumberOfPlayer(); player++) {
        constraints[posLambdaConstraint+player] = m_NEP->buildLambdaConstraint(player);
    }

    if (removeCuts) {
        // global cuts are added after lambda constraints in the root node
        while (constraints.size() > posLambdaConstraint + m_NEP->getNumberOfPlayer())
            constraints.pop_back();
    }
    newRootNode->getSubproblem()->setConstraints(constraints);
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
    else if (m_node->getSolution().status != 2
             and m_node->getSolution().status != 4
             and m_node->getSolution().status != 3) {
        throw logic_error("stopping the algorithm because of "
                          "abnormal optimization status code " +
                          to_string(m_node->getSolution().status));
             }
}

BestMultiplicativeBranchOutput *BestMultiplicativeApproxGNEP::returnHandler(
        chrono::time_point<chrono::system_clock> startTime,
        string errorMsg) {
    string optimizationStatus = buildOptimizationStatusString(startTime, errorMsg);
    auto output = new BestMultiplicativeBranchOutput(optimizationStatus,
                                  m_nodeSelector->getSolutionList(),
                                  m_nodeSelector->getExploredNodeNumber(),
                                  m_totalNumberOfCuts,
                                  m_timeSpent,
                                  m_timeCounters["totalCutDerivationTime"],
                                  m_node,
                                  m_antiCyclingMeasuresTaken,
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