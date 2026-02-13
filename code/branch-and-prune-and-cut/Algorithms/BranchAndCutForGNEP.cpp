//
// Created by Aloïs Duguet on 11/12/24.
//

#include "BranchAndCutForGNEP.h"

#include <cstring>
#include <format>
#include <chrono>
#include <fstream>

#include "NEPManyEtasGurobi.h"
#include "NodeGurobi.h"
#include "BranchAndCutTools.h"
#include "CutInformation.h"

using namespace std;

BranchAndCutForGNEP::BranchAndCutForGNEP(
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
    string const& resultFilename) {

    // start time to build the model
    auto startTime = startChrono();

    // declare model type
    if (modelTypeString == "manyEtasGurobi") {
        m_NEP = make_shared<NEPManyEtasGurobi>(NEPInit->getBestResponseProblems());
    } else
        throw runtime_error("Wrong argument: NEP type '"
                            + modelTypeString + "' not recognized.");
    cout << *m_NEP << endl;
    m_gameTypeString = gameTypeString;

    // declare attributes related to branching
    m_globalCutSwitch = globalCutSwitch;
    m_modelTypeString.assign(modelTypeString);
    m_branchingRuleString.assign(branchingRuleString);
    m_whichCutString.assign(whichCutString);
    m_algorithmVariant.assign(algorithmVariant);
    m_nodeSelector = make_shared<DFS>(DFS());

    if (branchingRuleString == "firstFractional")
        m_branchingRule = make_shared<BranchingRuleFirstFractional>();
    else if (branchingRuleString == "mostFractional")
        m_branchingRule = make_shared<BranchingRuleMostFractional>();
    else if (branchingRuleString == "random")
        m_branchingRule = make_shared<BranchingRuleRandom>();
    else
        throw runtime_error("Wrong argument: branchingRuleString '"
                            + branchingRuleString + "' not recognized.");

    // build rootNode and node instance
    m_rootNode = make_shared<NodeGurobi>();
    m_node = shared_ptr<NodeGurobi>();

    // ----------------------------------------------------------------
    // tolerances

    // tolerance on hat_V(x) the sum of differences between
    // best response values and response values
    m_NETolerance = 1e-3;

    // tolerance for integrality constraints
    m_integralityTolerance = 1e-5;

    // it makes sense to have the same value as NETolerance because
    // the node problem is meant to mimic the hat V function used in the pruning step
    m_pruningZeroObjTolerance = m_NETolerance;

    // slightly above m_NETolerance, fixed experimentally
    m_deriveCutTolerance = 1e-6;

    m_feasibilityTol = 1e-9;

    // initialize parameters with infos on the progress of the algorithm
    m_instanceName = instanceName;
    // filename in which to enter the one-line result:
    m_resultFilename = resultFilename;

    // declare gurobi environment and models for the whole algorithm
    m_globalEnv = make_shared<GRBEnv>(true);
    m_globalEnv->set(GRB_DoubleParam_IntFeasTol, m_integralityTolerance);
    m_globalEnv->set(GRB_IntParam_OutputFlag, 0);
    m_globalEnv->set("LogFile", "");
    m_globalEnv->set("ThreadLimit", "1");
    m_globalEnv->start();

    // ----------------------------------------------------------------
    // cut counters
    m_totalNumberOfCuts = 0;
    m_numberOfCutsOfCurrentNode = 0;
    m_maxNonIntegerCutsPerNode = 10;

    // total time to derive some things related to intersection cuts
    m_timeCounters = {
        {"totalCutDerivationTime", 0.0},
        {"totalBestResponseTime", 0.0},
        {"totalNodeSolveTime", 0.0}};

    // changed to true if an optimal solution is found.
    m_NEFound = false;

    // reuse node of last iteration if bool reuseNode is true
    m_reuseNode = false;

    m_enumerateNE = enumerateNE;

    // lastIterationSolution contains the solution of last node problem
    m_lastIterationSolution = {};
    m_timeLimit = timeLimit;

    // when time limit is reached
    // during node resolution, all further operations are executed in less than
    // the buffer time to not go over the time limit
    m_bufferTime = 0.1;
    if (m_timeLimit < m_bufferTime)
        throw runtime_error("time limit is lower than buffer time ("
                            + to_string(m_timeLimit)
                            + ") the algorithm will stop immediately");

    // verbosity level
    // 0 for only initialization and termination information
    // 1 for adding a print every 100 cuts and 1000 nodes
    // 2 for adding few node information like node number, if cuts were added in the node, pruning situations, optimal value of problems
    // 3 for more node information like solution of problems and cuts derived
    // 4 for some debug information
    // 5 for some more specific debug information
    m_verbosity = verbosity;

    // anticycling attributes
    m_antiCyclingMeasuresTaken = false;
    m_rhsBaseBonusForRestrictedCut = 1e-6;
    m_rhsBonusForRestrictedCut = 1e-6;
    m_factorIncreaseForRestrictedCut = 2;

    // IMPORTANT: keep at the end of the function
    // initialize m_timeSpent with time spent in this function
    m_timeSpent = elapsedChrono(startTime);
}

void BranchAndCutForGNEP::initialize() {
    // initialization of etas bounds with proper bounds
    if (m_modelTypeString == "manyEtasGurobi") {
        if (m_gameTypeString == "GNEP-maxFlowGame") {
            // hard to solve because of many nonconvex quadratic terms in the objective function
            m_NEP->computeEtasBounds(0.05);
        }
        else
            m_NEP->computeEtasBounds(0.0001);
    }

    m_rootNode->setSubproblem(m_NEP->buildOptimizationProblem());
    m_node = m_nodeSelector->buildChildOfRootNode(m_rootNode);

    checkValidArguments();

    m_globalEnv->set(GRB_DoubleParam_FeasibilityTol, m_feasibilityTol);
}

void BranchAndCutForGNEP::checkValidArguments() const {
    // unique arguments invalid
    if (m_gameTypeString == "GNEP-mixedInteger")
        throw logic_error("Argument gameTypeString == GNEP-mixedInteger "
                          "has no finite proof termination.\n");
    if (m_enumerateNE == true)
        throw logic_error("No theoretical results on how to compute all NEs "
                          "with this algorithm.");
    // pairs of arguments invalid
    if (m_whichCutString == "eqCuts" and m_modelTypeString == "oneEtaGurobi")
        throw logic_error("Arguments (whichCutString, modelTypeString) == (eqCuts, oneEtaGurobi) "
                          "is not possible.\neqCuts are for manyEtasGurobi "
                          "while aggEqCuts are for oneEtaGurobi");
    if (m_whichCutString == "eqCuts" and
        (m_gameTypeString == "GNEP-fullInteger" or m_gameTypeString == "GNEP-mixedInteger"))
        throw logic_error("Arguments (whichCutString, modelType) == (eqCuts, GNEP-fullInteger or GNEP-mixedInteger) "
                          "is not possible.\neqCuts are for NEP while "
                          "intersectionCuts are for GNEP");
    if (m_whichCutString == "intersectionCuts" and m_globalCutSwitch == true)
        throw logic_error("Arguments (whichCutString, globalCutSwitch) == (intersectionCuts, true) "
                          "is not possible.\nintersectionCuts are not globally valid "
                          "because they use 'local' equilibrium constraints'");
    if (m_whichCutString == "intersectionCuts" and
        (m_gameTypeString == "GNEP-mixedInteger" or m_gameTypeString == "NEP-mixedInteger"))
        throw logic_error("Arguments (whichCutString, modelType) == (intersectionCuts, GNEP-mixedInteger or NEP-mixedInteger) "
                          "is not possible.\nintersectionCuts need an optimal node solution that is integer");
    if (m_whichCutString == "intersectionCuts" and m_algorithmVariant == "cuttingBeforeBranching")
        throw logic_error("Arguments (whichCutString, algorithmVariant) == (intersectionCuts, cuttingBeforeBranching) "
                          "is not possible.\nintersectionCuts need an optimal node solution that is integer.");
    if (m_whichCutString == "intersectionCuts" and m_modelTypeString == "oneEtaModel")
        throw logic_error("Arguments (whichCutString, modelTypeString) == (intersectionCuts, oneEtaModel) "
                          "is not possible.\nintersectionCuts as described in the preprint are tailored"
                          " to manyEtasModel");
    if (m_whichCutString == "intersectionCuts" and !m_rootNode->getSubproblem()->getQ().size() == 0)
        throw logic_error("Arguments (whichCutString, rootNodeProblem) == (intersectionCuts, "
                          "quadratic terms in objective) "
                          "is not possible.\nIntersection cuts need linear objective");
    if (m_whichCutString == "noGoodCuts" and m_algorithmVariant == "cuttingBeforeBranching")
        throw logic_error("Arguments (whichCutString, algorithmVariant) == (noGoodCuts, cuttingBeforeBranching) "
                          "is not possible.\nnoGoodCuts need an optimal node solution that is binary.");
    if (m_whichCutString == "aggEqCuts" and m_modelTypeString == "manyEtasGurobi")
        throw logic_error("Arguments (whichCutString, modelTypeString) == (aggEqCuts, manyEtasGurobi) "
                          "is not possible.\neqCuts are for manyEtasGurobi "
                          "while aggEqCuts are for oneEtaGurobi");
    if (m_whichCutString == "aggEqCuts" and
        (m_gameTypeString == "GNEP-fullInteger" or m_gameTypeString == "GNEP-mixedInteger"))
        throw logic_error("Arguments (whichCutString, modelType) == (aggEqCuts, GNEP-fullInteger or GNEP-mixedInteger) "
                          "is not possible.\naggEqCuts are for NEP while "
                          "intersectionCuts are for GNEP");
    if (m_whichCutString == "aggEqCuts" and m_globalCutSwitch == true)
        throw logic_error("Arguments (whichCutString, globalCutSwitch) == (aggEqCuts, true) "
                          "is not possible.\naggEqCuts are only locally valid cuts");
}

NEPBranchOutput* BranchAndCutForGNEP::solve() {
    // initialize timer for this method
    auto startTime = startChrono();
    initialize();
    writeRootNode();

    try {
        while ((m_nodeSelector->nodeListSize() > 0 or m_reuseNode)
                and checkTimeLimit(startTime)) {
            getNewNode();
            printNodeInformation(elapsedChrono(startTime));
            solveNode(computeSolverTimeLimit(startTime));
            checkTime(startTime);

            if (!m_node->isSolved()) {
                // handle unsolved nodes
                unsolvedNodeHandler();
            } else if (!compareApproxLess(m_node->getSolution().objective,
                                          0,
                                          m_pruningZeroObjTolerance)) {
                // prune the node because objective > 0
                if (m_verbosity >= 2)
                    cout << "pruning: node obj > 0" << endl;
            } else if (m_node->isSolved()) {
                if (m_node->isIntegerSolution(m_integralityTolerance)) {
                    auto solution = computeBestResponseHandler(computeSolverTimeLimit(startTime));
                    checkTime(startTime);
                    detectCycling(solution);
                    double valNikaidoIsoda =
                        m_NEP->evalNikaidoIsoda(solution, m_node->getSolution().objective);
                    if (m_verbosity >= 2)
                        cout << "Psi(x*,y*) == " << valNikaidoIsoda << endl;
                    if (isApproxZero(valNikaidoIsoda, m_NETolerance)) {
                        if (NEHandler(solution))
                            break;
                    } else {
                        // add a cut and iterate on the same Node
                        addCuts(solution);
                    }
                } else if (m_algorithmVariant == "cuttingBeforeBranching") {
                    auto solution =
                        computeBestResponseHandler(computeSolverTimeLimit(startTime));
                    checkTime(startTime);
                    detectCycling(solution);
                    if (!m_NEP->testBestEtaPossible(solution,
                                                    m_deriveCutTolerance,
                                                    m_verbosity) ) {
                        if (m_numberOfCutsOfCurrentNode < m_maxNonIntegerCutsPerNode) {
                            if (m_verbosity >= 2)
                                cout << "trying to add cuts without an integer solution"
                                     << endl;
                            addCuts(solution);
                        }
                        else {
                            // cuttingBeforeBranching forced branching case because there were 'many' cuts added
                            // with a non integer solution for the current node
                            if (m_verbosity >= 2)
                                cout << "branching because max non integer "
                                        "cuts per node has been reached" << endl;
                            branchingHandler();
                        }
                    } else {
                        // cuttingBeforeBranching branching case
                        branchingHandler();
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

    } catch (std::exception const& e) {
        auto output = returnHandler(startTime, e.what());
        return output;
    }

}

void BranchAndCutForGNEP::getNewNode() {
    if (!m_reuseNode) {
        // free now useless memory
        m_node->resetSubproblem(m_rootNode->getSubproblem());


        m_node = m_nodeSelector->nextNodeToExplore();

        // build model from scratch
        m_node->buildModel();
        m_numberOfCutsOfCurrentNode = 0;
    }
    else {
        m_reuseNode = false;
        m_node->setUnsolved();
    }
}

void BranchAndCutForGNEP::printNodeInformation(double timeSpent) const {
    if (m_verbosity == 1) {
        // not printing information at each new node
        if (m_numberOfCutsOfCurrentNode == 0) {
            int nodeNumber = m_node->getNodeNumber();
            if (nodeNumber % 1000 == 0) {
                cout << "Node " << nodeNumber
                << " after " << timeSpent << "s" << endl;
            }
        }
        else {
            if (m_totalNumberOfCuts % 100 == 0)
                cout << "cuts derived: " << m_totalNumberOfCuts << endl;
        }
    }
    else if (m_verbosity >= 2) {
        // printing information at each node
        cout << endl;
        cout << "Node " << m_node->getNodeNumber() << " ";
        cout << "with " << m_numberOfCutsOfCurrentNode << " cuts";
        if (m_node->getBranch().getAx().size() > 0) {
            // if a branch exists, print it
            cout << " and branch ";
            cout << m_node->getBranch();
        }
        cout << endl;
    }
}

void BranchAndCutForGNEP::solveNode(double solverTimeLimit) {
    // solve node problem
    if (m_verbosity >= 2)
        cout << solverTimeLimit << "s remaining. " ;
    if (solverTimeLimit < 0) {
        // can happen because the check happens before the computation of
        // solverTimeLimit. This will trigger a time limit from the solver,
        // and thus will be handled automatically
        solverTimeLimit = 0;
    }
    auto startNodeSolveTime = startChrono();
    m_nodeModel = m_node->solveModel(m_globalEnv,
                                     solverTimeLimit,
                                     m_verbosity,
                                     false,
                                     m_feasibilityTol);
    m_timeCounters["totalNodeSolveTime"] += elapsedChrono(startNodeSolveTime);
}

void BranchAndCutForGNEP::unsolvedNodeHandler() {
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
        throw runtime_error("subsolver out of memory");
    }
    else {
        throw logic_error("stopping the algorithm because of "
                          "abnormal optimization status code " +
                          to_string(m_node->getSolution().status));
    }
}

vector<double> BranchAndCutForGNEP::computeBestResponseHandler(
        double solverTimeLimit) {
    if (solverTimeLimit < 0) {
        // can happen because the check happens before
        // the computation of solverTimeLimit

        // this will trigger a time limit from the solver,
        // and thus will be handled automatically
        solverTimeLimit = 0;
    }
    vector<double> solution = m_node->getSolution().solution;
    auto startBestResponseComputationTime = startChrono();
    m_NEP->computeAllBestResponses(solution, m_globalEnv, solverTimeLimit, m_verbosity);
    m_timeCounters["totalBestResponseTime"] += elapsedChrono(startBestResponseComputationTime);

    // check that all best responses have optimal status
    for (auto const& bestResponse : m_NEP->getBestResponses()) {
        if (bestResponse.status == 9) {
            cout << "all best responses not computed "
                    "because of time limit reached" << endl;
            return {};
        }
    }
    return solution;
}

bool BranchAndCutForGNEP::NEHandler(vector<double> const& solution) {
    // tolerance not the default one because when converging to an NE,
    // the node can be pruned by eta values too close to BR values makes pruning
    // happen before the case where an NE is found

    if (m_verbosity >= 1)
        cout << "NE found" << endl;
    m_nodeSelector->incrementSolutionList(m_NEP,
                                          solution,
                                          computeSocialWelfare(m_node->getSolution()),
                                          m_NEP->differencesToBestResponseValues(solution),
                                          m_NETolerance);
    m_NEFound = true;
    if (m_verbosity >= 1) {
        cout << "stopping at first NE found" << endl << endl;
    }
    return true;
}

bool BranchAndCutForGNEP::detectCycling(std::vector<double> const& solution) {
    if (m_numberOfCutsOfCurrentNode > 0) {
        // m_numberOfCutsOfCurrentNode > 0 means that the solution from
        // the last iteration is from the same node as the current one
        double dist = norm2(solution, m_lastIterationSolution);
        // warning that there might be numerical issues with the cuts:
        if (dist < 1e-3) {
            cout << "solution and last solution quite close in norm 2: "
                 << format("{:.7f}", dist) << endl;
            double valNikaidoIsoda =
                        m_NEP->evalNikaidoIsoda(solution, m_node->getSolution().objective, m_node->isIntegerSolution(m_integralityTolerance));
            cout << "Psi(x*,y*) == " << valNikaidoIsoda << endl;
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

void BranchAndCutForGNEP::addCuts(vector<double> const& solution) {
    auto startCutDerivationTime = startChrono();
    CutInformation cutInformation = m_NEP->deriveCut(solution,
                                        m_whichCutString,
					                    m_deriveCutTolerance,
					                    m_globalEnv,
					                    m_nodeModel,
					                    m_rootNode->getSubproblem(),
					                    m_verbosity);
    m_timeCounters["totalCutDerivationTime"] += elapsedChrono(startCutDerivationTime);
    vector<Cut> cuts = cutInformation.getCuts();
    m_timeCounters += cutInformation.getTimeValues();
    handleCycling(cuts, solution);
    checkCutsValidity(cuts, solution);
}

void BranchAndCutForGNEP::handleCycling(vector<Cut> &cuts, vector<double> const &solution) {
    // restrict cuts by restrictRhsCut in case of cycling
    if (detectCycling(solution)) {
        m_antiCyclingMeasuresTaken = true;
        cout << "\nerror: cycling detected. The solution of the last "
                "iteration is the current solution, it has not been cut off."
             << endl;
        if (m_rhsBonusForRestrictedCut > 0) {
            // WARNING: the execution of this if-block does not
            // respect the correctness of the algorithm
            // check value of m_antiCyclingMeasuresTaken
            // at the end of the computation to know if
            // the result is to be trusted
            cout << "----> substracting a small positive value "
                 << m_rhsBonusForRestrictedCut
                 << " to each rhs of cuts. It may remove an NE."
                 << endl;
            for (auto & cut : cuts) {
                cout << "old cut: " << cut << endl;
                cut.increaseB(-m_rhsBonusForRestrictedCut);
                cout << "new cut: " << cut << endl;
            }
            // increasing rhsBonus in case it does not resolve the cycling
            m_rhsBonusForRestrictedCut *= m_factorIncreaseForRestrictedCut;
            if (m_verbosity >= 4)
                writeLogCycling(cuts,solution); // useful when debugging
        } else
            throw logic_error("m_rhsBonusForRestrictedCut == 0 means that no "
                              "trial to solve the cycling will be done, so we stop");
    }
}

void BranchAndCutForGNEP::checkCutsValidity(
    vector<Cut> const& cuts,
    vector<double> const &solution) {
    if (!cuts.empty()) {
        logCutToAdd(cuts);
        int countCutAdded = 0;
        for (Cut const& cut : cuts) {
            if (!cut.checkConstraintWithPoint(solution, m_verbosity)) {
                logCutNotAdded();
            } else {
                // the cut really cuts off the solution
                countCutAdded += 1;
                // add cut to list of cuts and to subproblem
                m_node->addCut(cut, m_globalCutSwitch);
            }
        }
        m_numberOfCutsOfCurrentNode += countCutAdded;
        m_totalNumberOfCuts += countCutAdded;
        if (countCutAdded == 0) {
            logNoCutAdded();
            throw logic_error("No cut added because they did not cut off the solution, "
                "probably due to numerical issues");
        }
        m_reuseNode = true;

        // special check for the instance of implementation games which do not find any NE
        // when there is one
        // if (m_gameTypeString == "implementationGames")
            // checkValidNENotCutExactGNEPImplementationGames(cuts);

    } else {
        auto solution = computeBestResponseHandler(60);
        bool testNE = m_NEP->checkNE(solution, m_NETolerance, 5);
        throw logic_error("Abnormal case: no cuts added, which means cycling "
                      "OR threshold for deriving a cut is too high and we "
                      "are close to an NE, check NI value");
    }

}

void BranchAndCutForGNEP::checkValidNENotCutExactGNEPImplementationGames(vector<Cut> const& cuts) {
    string folder_implementation_games = "../../../instances/implementation_games/";

    // WARNING: NE is a bad name as it contains also the values of the best responses

    // HPC 212
    string instanceName = folder_implementation_games + "I_2_20_mm_10_1.txt";
    vector<double> NE = {0,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,5,5,0,0,0,0,0,0,0,0,0,5,0,0,0,0,0,
            0,0,0,0,0,9,0,0,0,9,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,9,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-240,-378,0};
    checkPointNotCutSpecificInstance(instanceName, NE, cuts);
    NE = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,9,0,0,0,9,0,0,0,0,0,0,0,0,14,5,0,0,0,0,0,0,0,0,5,9,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-513,0,0};
    checkPointNotCutSpecificInstance(instanceName, NE, cuts);

    // HPC 123
    instanceName = folder_implementation_games + "I_2_10_mm_10_2.txt";
    NE = {0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,4,7,0,0,0,0,0,7,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-216,0};
    checkPointNotCutSpecificInstance(instanceName, NE, cuts);

    // HPC 134
    instanceName = folder_implementation_games + "I_2_10_mm_1_3.txt";
    NE = {0,0,0,0,0,1,0,1,0,1,0,0,1,1,0,0,1,0,1,0,1,0,0,0,0,9.79751,0,0,0,0,0,0,0,0,0,-23,-75.2025,0};
    checkPointNotCutSpecificInstance(instanceName, NE, cuts);

    // HPC 138
    instanceName = folder_implementation_games + "I_2_10_mm_1_7.txt";
    NE = {0,0,1,1,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,0,0,0,0,0,0,0,-3,-21,-7};
    checkPointNotCutSpecificInstance(instanceName, NE, cuts);
    NE = {0,0,1,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,0,0,0,0,0,0,0,0,-32,-7};
    checkPointNotCutSpecificInstance(instanceName, NE, cuts);
    NE = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,2,0,1,1,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,0,0,0,0,0,0,0,0,-43,-7};
    checkPointNotCutSpecificInstance(instanceName, NE, cuts);

    // HPC 149
    instanceName = folder_implementation_games + "I_2_10_ss_10_8.txt";
    NE = {0,6,0,0,0,6,0,0,0,0,0,0,5,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-54,-105,0};
    checkPointNotCutSpecificInstance(instanceName, NE, cuts);

    // HPC 152
    instanceName = folder_implementation_games + "I_2_10_ss_1_1.txt";
    NE = {0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-30,-21,0};
    checkPointNotCutSpecificInstance(instanceName, NE, cuts);

    // HPC 158
    instanceName = folder_implementation_games + "I_2_10_ss_1_7.txt";
    NE = {0,1,1,0,0,0,0,0,1,1,0,0,1,0,0,1,0,0,1,0,0,1,1,0,0,0,0,0,1,1,0,0,1,0,0,0,0,0,0,0,0,0,17,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,13,-40,-32,0};
    checkPointNotCutSpecificInstance(instanceName, NE, cuts);

    // HPC 159
    instanceName = folder_implementation_games + "I_2_10_ss_1_8.txt";
    NE = {0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,1,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-18,-45,0};
    checkPointNotCutSpecificInstance(instanceName, NE, cuts);

    // HPC 197
    instanceName = folder_implementation_games + "I_2_15_ss_1_6.txt";
    NE = {1,0,1,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,1,0,0,0,0,1,0,0,1,1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-24,-84,0};
    checkPointNotCutSpecificInstance(instanceName, NE, cuts);

    // HPC 198
    instanceName = folder_implementation_games + "I_2_15_ss_1_7.txt";
    NE = {1,0,1,0,0,0,1,0,0,0,1,0,0,1,1,1,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,1,0,0,0,1,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-68,-37,0};
    checkPointNotCutSpecificInstance(instanceName, NE, cuts);

    // HPC 248
    instanceName = folder_implementation_games + "I_2_20_mm_10_52.txt";
    NE = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1,1,0,0,1,0,0,0,0,1,1,1,0,0,1,0,1,0,0,1,1,0,0,1,0,1,0,0,0,1,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-193,0};
    checkPointNotCutSpecificInstance(instanceName, NE, cuts);

    // HPC 301
    instanceName = folder_implementation_games + "I_2_20_mm_1_10.txt";
    NE = {1,0,0,0,0,1,1,1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-66,-26,0};
    checkPointNotCutSpecificInstance(instanceName, NE, cuts);

    // HPC 315
    instanceName = folder_implementation_games + "I_2_20_ss_10_4.txt";
    NE = {0,0,0,0,9,0,0,9,0,0,0,0,0,0,0,0,0,0,0,9,9,0,0,0,0,0,5,0,0,5,0,0,0,0,0,0,0,0,0,0,0,5,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-549,-165,0};
    checkPointNotCutSpecificInstance(instanceName, NE, cuts);

    // HPC 327
    instanceName = folder_implementation_games + "I_2_20_ss_1_6.txt";
    NE = {0,0,1,1,0,1,0,0,0,1,0,0,2,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,19.5,0,0,0,0,0,0,-119,0,0};
    checkPointNotCutSpecificInstance(instanceName, NE, cuts);

    // HPC 333
    instanceName = folder_implementation_games + "I_4_10_mm_10_2.txt";
    NE = {1,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,5,2,0,7,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,2,0,0,0,2,0,0,0,0,2,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-32,-91,-50,-76,0};
    checkPointNotCutSpecificInstance(instanceName, NE, cuts);
    NE = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,2,0,0,0,0,0,0,0,0,2,0,0,0,2,0,0,0,0,4,0,2,0,0,0,0,0,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,39,0,0,0,0,0,-50,-76,0};
    checkPointNotCutSpecificInstance(instanceName, NE, cuts);

    // HPC 345
    instanceName = folder_implementation_games + "I_4_10_mm_1_4.txt";
    NE = {0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,1,0,0,1,1,0,0,0,0,0,0,0,0,1,0,0,0,1,1,1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-24,-40,-28,-32,0};
    checkPointNotCutSpecificInstance(instanceName, NE, cuts);

    // HPC 365
    instanceName = folder_implementation_games + "I_4_10_ss_1_4.txt";
    NE = {1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-32,-29,-39,-18,0};
    checkPointNotCutSpecificInstance(instanceName, NE, cuts);

    // HPC 211 (solved with mostFractional)
    instanceName = folder_implementation_games + "I_2_20_mm_10_19.txt";
    NE = {0,0,0,0,0,2,2,1,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,2,0,0,4,0,1,0,5,0,0,0,0,2,0,0,0,0,5,0,0,0,2,0,0,2,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-217,-144,0};
    checkPointNotCutSpecificInstance(instanceName, NE, cuts);
    NE = {0,0,0,0,0,4,2,1,0,0,0,0,0,2,2,0,0,2,0,0,0,0,0,4,0,0,0,0,2,2,0,0,4,0,1,0,5,0,0,0,0,0,0,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,13,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-281,-110,0};
    checkPointNotCutSpecificInstance(instanceName, NE, cuts);
    NE = {0,0,0,0,0,4,1,1,0,0,0,0,0,2,2,0,0,2,0,0,0,0,0,4,0,0,0,0,1,2,0,0,4,0,1,0,5,0,0,0,0,0,1,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,13.3333,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-247.333333,-126,0};
    checkPointNotCutSpecificInstance(instanceName, NE, cuts);
    NE = {0,0,0,0,0,3,2,1,0,0,0,0,0,2,1,0,0,1,0,0,0,0,0,4,0,0,0,0,1,2,0,0,4,0,1,0,5,0,0,0,0,1,0,0,0,0,5,0,0,0,1,0,0,1,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,14,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-248,-125,0};
    checkPointNotCutSpecificInstance(instanceName, NE, cuts);
    NE = {0,0,0,0,0,2,2,1,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,2,0,0,4,0,1,0,5,0,0,0,0,2,0,0,0,0,5,0,0,0,2,0,0,2,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,26.5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-217,-115,0};
    checkPointNotCutSpecificInstance(instanceName, NE, cuts);
    NE = {0,0,0,0,0,3,1,1,0,0,0,0,0,2,1,0,0,1,0,0,0,0,0,4,0,0,0,0,0,2,0,0,4,0,1,0,5,0,0,0,0,1,1,0,0,0,5,0,0,0,1,0,0,1,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,13,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-216,-142,0};
    checkPointNotCutSpecificInstance(instanceName, NE, cuts);
}

void BranchAndCutForGNEP::checkPointNotCutSpecificInstance(
        string const &instanceName,
        vector<double> const &point,
        vector<Cut> const &cuts) {
    if (m_instanceName == instanceName and m_verbosity >= 3) {
        // test with m_verbosity just helps to not go into that block if computation on the HPC
        cout << "entering check of a real NE for the special instance " << instanceName << endl;
        checkPointNotCut(cuts, point);
    }
}
void BranchAndCutForGNEP::checkPointNotCut(
        vector<Cut> const &cuts,
        vector<double> const &point) {
    // check if in this node NE is feasible. If not, as the cuts are local, we do not check if the cut cuts it
    if (m_node->checkPointSatisfyBranchingConstraints(point)) {
        double dist = norm2(point, m_node->getSolution().solution);
        cout << "distance between real NE and point to cut off: " << dist << endl;
        cout << "real NE:       " << point << endl;
        cout << "point cut off: " << m_node->getSolution().solution << endl;
        for (auto & cut : cuts) {
            if (cut.checkConstraintWithPoint(point, 4)) {
                cout << "cut: " << cut << endl;
                throw logic_error("the real NE is cut by the cut written above");
            }
            else
                cout << "real NE not cut off" << endl;
        }
    }
}

void BranchAndCutForGNEP::logCutToAdd(std::vector<Cut> const& cuts) const {
    if (m_verbosity >= 2) {
        cout << "trying to add " << cuts.size();
        if (m_globalCutSwitch)
            cout << " global";
        else
            cout << " local";
        if (cuts.size() == 1)
            cout << " cut";
        else
            cout << " cuts";
        cout << " to node:" << endl;
    }
}

void BranchAndCutForGNEP::logCutNotAdded() const {
    if (m_verbosity >= 1)
        cout << "cut not added because it does not cut off the solution"
             << endl;
}

void BranchAndCutForGNEP::logNoCutAdded() const {
    // nothing to say
}

void BranchAndCutForGNEP::branchingHandler() const {
    if (m_verbosity >= 2)
        cout << "branching" << endl;
    m_nodeSelector->buildChildNodes(m_branchingRule, m_node);
}

void BranchAndCutForGNEP::printParameters(ostream& os) const {
    os << "instance name: " << m_instanceName << endl;
    os << "type of game: ";
    if (m_NEP->getIsGNEP())
        os << "GNEP" << endl;
    else
        os << "NEP" << endl;
    os << "algorithm variant: " << m_algorithmVariant << endl;
    os << "model type: " << m_modelTypeString << endl;
    os << "cut type: " << m_whichCutString << " with ";
    if (m_globalCutSwitch)
        os << "global cuts" << endl;
    else
        os << "local cuts" << endl;
    os << "branching rule: " << m_branchingRuleString << endl;
    os << "enumeration of NE: ";
    if (m_enumerateNE)
        os << "true" << endl;
    else
        os << "false" << endl;

    // separation with other informations
    os << endl;

    os << "feasibility tolerance for constraints: "
       << format("{:.9f}", m_feasibilityTol) << endl;
}

std::string BranchAndCutForGNEP::buildFilenameWithOptions(bool withDateAndTime) const {
    // computing suffix of file name with options of the algorithm instance
    string sepOption = "_";
    string filename = m_instanceName.substr(
        0,
        static_cast<indexx>(m_instanceName.size())-4)
        +sepOption;
    // if there are "/" in filename, we keep only the part after the last "/":
    auto pos = static_cast<indexx>(filename.find_last_of('/'));
    if (pos != string::npos)
        filename = filename.substr(pos+1);
    if (m_NEP->getIsGNEP())
        filename += "GNEP"+sepOption;
    else
        filename += "NEP"+sepOption;
    if (withDateAndTime)
        filename += getCurrentTimeAndDate()+sepOption;
    filename += m_algorithmVariant+sepOption
                +m_modelTypeString+sepOption
                +m_whichCutString;
    filename += sepOption;
    if (m_globalCutSwitch)
        filename += "globalCuts";
    else
        filename += "localCuts";
    filename += sepOption+m_branchingRuleString+"feasibilityTol";
    filename += sepOption+format("{:.9f}", m_feasibilityTol);
    return filename;
}

void BranchAndCutForGNEP::writeRootNode() const {
    cout << "Root node problem: " << endl;
    cout << *m_rootNode->getSubproblem();
}

void BranchAndCutForGNEP::writeLog(NEPBranchOutput const& NEPOutput) const {
    ofstream outputFile;
    string filename = buildFilenameWithOptions();
    string logFilename = "../logFiles/"+filename+".txt";

    // delete previous content of the file
    outputFile.open(logFilename, ofstream::trunc);
    if (!outputFile.is_open())
        throw runtime_error("Log file of name "
                            + logFilename
                            + "could not be opened");

    printParameters(outputFile);
    outputFile << *m_NEP << endl;
    outputFile << "last subproblem solved:\n"
               << *NEPOutput.finalNode->getSubproblem()
               << endl;
    outputFile << NEPOutput << endl;
    outputFile.close();
}

void BranchAndCutForGNEP::writeLogCut(vector<Cut> const& cuts) const {
    string filename = buildFilenameWithOptions(false);
    string logFilename = "../logCutFiles/"+filename;
    ofstream outputFile;
    outputFile.open(logFilename, ofstream::app);
    outputFile << "(" << getCurrentTimeAndDate() << ") node "
               << m_node->getNodeNumber()
	           << " with " << m_numberOfCutsOfCurrentNode << " cuts: ";
    for (Cut const& cut : cuts)
        outputFile << "    " << cut << endl;
    outputFile.close();
}

void BranchAndCutForGNEP::writeLogCycling(vector<Cut> const& cuts,
                                          vector<double> const& solution) const {
    string filename = buildFilenameWithOptions(false);
    string logFilename = "../logCutFiles/"+filename;
    ofstream outputFile;
    outputFile.open(logFilename, ofstream::app);
    outputFile << "\nerror: cycling detected. The solution of the last iteration is "
                  "the current solution, it has not been cut off." << endl;
    outputFile << "adding small positive value " << m_rhsBonusForRestrictedCut
               << " to each rhs of cuts. Hopefully, it will remove the current solution. "
                  "However, it may also remove an NE." << endl;
    outputFile << "current solution: " << solution << endl;
    outputFile << "node " << m_node->getNodeNumber() << " with "
               << m_numberOfCutsOfCurrentNode << " artificially restricted cuts: ";
    for (Cut const& cut : cuts)
        outputFile << "    " << cut << endl;
    outputFile.close();
}

void BranchAndCutForGNEP::updateLastNodeSolution() {
    // it is a copy, not just creating a reference to the same object
    m_lastIterationSolution = m_node->getSolution().solution;
}

void BranchAndCutForGNEP::oneLineResultHandler(NEPBranchOutput const &output) const {
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

void BranchAndCutForGNEP::writeOneLineResult(NEPBranchOutput const& output,
                                             ostream& os) const {
    // start of the line
    os << "result,";

    writeEndOfOneLineResult(output, os);
}

void BranchAndCutForGNEP::writeEndOfOneLineResult(NEPBranchOutput const &output, std::ostream &os) const {
    // separator
    string const sep1 = ",";

    // print input
    os << m_instanceName << sep1;

    // automatic detection of NEP/GNEP by looking at the constraints
    if (m_NEP->getIsGNEP())
        os << "GNEP";
    else
        os << "NEP";

    os << sep1;
    os << m_gameTypeString << sep1;
    os << m_algorithmVariant << sep1;
    os << m_whichCutString << sep1;
    if (m_globalCutSwitch)
        os << "globalCuts";
    else
        os << "localCuts";
    os << sep1;
    if (m_enumerateNE)
        os << "true";
    else
        os << "false";
    os << sep1;
    os << m_modelTypeString << sep1;
    os << m_branchingRuleString << sep1;
    os << format("{:.9f}", m_feasibilityTol) << sep1;

    // print output
    output.oneLinePrint(os, sep1);

    // end print
    os << endl;
}


NEPBranchOutput* BranchAndCutForGNEP::returnHandler(
        chrono::time_point<chrono::system_clock> const startTime,
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
            else {
                optimizationStatus = "SOLUTION_FOUND";
                // print the number of cuts in the node in which the NE was found
                cout << "number of local cuts in the node (and parents) in which the NE was found: "
                     << m_node->getNumberOfCutsInNodeProblem() << endl;
                cout << "depth of node in which the NE was found: " << m_node->getDepth() << endl;
            }
        }
    } else {
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
        m_timeSpent = m_timeLimit;
    }

    // print some times
    cout << endl << m_timeCounters << endl;

    auto output = new NEPBranchOutput(optimizationStatus,
                                      m_nodeSelector->getSolutionList(),
                                      m_nodeSelector->getExploredNodeNumber(),
                                      m_totalNumberOfCuts,
                                      m_timeSpent,
                                      m_timeCounters["totalCutDerivationTime"],
                                      m_node,
                                      m_antiCyclingMeasuresTaken);
    // write log in file instanceName
    // print results
    cout << *output << endl;

    oneLineResultHandler(*output);

    return output;
}

double BranchAndCutForGNEP::computeSocialWelfare(SolveOutput const& output) const {
    if (!m_node->isSolved())
        throw logic_error("Trying to compute social welfare in "
                          "computeSocialWelfare with an unsolved node");

    double socialWelfare = 0;
    for (int player = 0; player < m_NEP->getNumberOfPlayer(); player++)
        socialWelfare += m_NEP->evalResponseValue(player, output.solution);
    return socialWelfare;
}

void BranchAndCutForGNEP::printSocialWelfare(SolveOutput const& output) const {
    if (m_node->isSolved()) {
        if (m_verbosity >= 2)
            cout << "social welfare: " << computeSocialWelfare(output) << endl;
    }
}

bool BranchAndCutForGNEP::checkTimeLimit(
        chrono::time_point<chrono::system_clock> const& startTime) const {
    auto currentTime = elapsedChrono(startTime) + m_timeSpent;
    if (currentTime > m_timeLimit - m_bufferTime) {
        if (m_verbosity >= 1)
            cout << endl << "time limit exceeded (up to buffer time of "
                 << m_bufferTime << " seconds), stopping the algorithm." << endl;
        // no time remaining, stopping the algorithm
        return false;
    }

    // time remaining, continuing
    return true;
}

double BranchAndCutForGNEP::computeSolverTimeLimit(
        chrono::time_point<chrono::high_resolution_clock> const& startTime) const {
    // m_timeSpent is the time spent in the initialization
    // of the class BranchAndCutForGNEP
    return (m_timeLimit - elapsedChrono(startTime) - m_timeSpent - m_bufferTime);
}

void BranchAndCutForGNEP::resetExploredNodeNumber() const {
    m_nodeSelector->resetExploredNodeNumber();
}
