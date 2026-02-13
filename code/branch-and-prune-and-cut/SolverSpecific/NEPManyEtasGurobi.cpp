//
// Created by Aloïs Duguet on 12/3/24.
//

#include "NEPManyEtasGurobi.h"

#include <vector>
#include <memory>
#include <format>
#include <numeric>
#include <algorithm>
#include <chrono>
#include <Eigen/Sparse>

#include "BranchAndCutTools.h"
#include "CutInformation.h"
#include "intersectionRayHalfspace.h"

using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXd;

NEPManyEtasGurobi::NEPManyEtasGurobi(
        vector<shared_ptr<OptimizationProblem>> const& bestResponseProblems) {
    m_numberOfPlayers = static_cast<int>(bestResponseProblems.size());
    m_bestResponseProblems = bestResponseProblems;
    m_isGNEP = detectGNEP();
    m_nashEquilibriumList = {};
    m_bestResponses = {};
    for (int i = 0; i < m_numberOfPlayers; i++)
        m_bestResponses.push_back({});
    for (indexx player = 0; player < m_numberOfPlayers; player++) {
        m_etaLowerBounds.push_back(0);
        m_etaUpperBounds.push_back(0);
    }
}

void NEPManyEtasGurobi::initialize() {
    throw logic_error("NEPManyEtasGurobi::initialize() does not do anything");
}

void NEPManyEtasGurobi::print(ostream& os) const {
    NashEquilibriumProblem::print(os);
    os << "initial eta lower bounds: " << m_etaLowerBounds << endl;
    os << "initial eta upper bounds: " << m_etaUpperBounds << endl;
    if (m_nashEquilibriumList.size() > 1) {
        os << m_nashEquilibriumList.size() << " equilibria found right now:" << endl;
        for (auto const& nashEquilibrium : m_nashEquilibriumList)
            os << nashEquilibrium << endl;
    } else if (m_nashEquilibriumList.size() == 1) {
        os << "1 equilibrium found right now:" << endl;
        os << m_nashEquilibriumList[0] << endl;
    } else
        os << "no equilibrium found right now" << endl;
}

vector<vector<double>> NEPManyEtasGurobi::getNashEquilibriumList() {
    return m_nashEquilibriumList;
}

indexx NEPManyEtasGurobi::getEtaIndex(indexx player) const {
    indexx etaIndex = 0;
    // add number of variables of each player
    for (indexx p = 0; p < getNumberOfPlayer(); p++)
        etaIndex += m_bestResponseProblems[p]->getNumberOfVariables();
    // add 1 for each eta variable before
    for (indexx p = 0; p < player; p++)
        etaIndex += 1;
    return etaIndex;
}

double NEPManyEtasGurobi::evalResponseValue(indexx player,
                                            vector<double> const& point) const {
    double val = 0.0;

    vector<int> numberOfVariablesPerPlayer = getNumberOfVariablesPerPlayer();

    // rebuild cost function evaluation
    MySparseVector c = m_bestResponseProblems[player]->getC();
    MySparseMatrix Q = m_bestResponseProblems[player]->getQ();
    // simple loop for c
    for (indexx k = 0; k < c.size(); k++)
        val += c.getValue(k) * point[c.getIndex(k)];

    // simple loop for Q
    for (indexx k = 0; k < Q.size(); k++) {
        indexx i = Q.getRowIndex(k);
        indexx j = Q.getColIndex(k);
        val += Q.getValue(k) * point[i] * point[j];
    }

    return val;
}

bool NEPManyEtasGurobi::checkNE(
        vector<double> const& point,
        double tolerance,
        int verbosity) const {
    for (indexx player = 0; player < m_numberOfPlayers; player++) {
        if (!compareApproxLess(evalResponseValue(player, point),
                               m_bestResponses[player].objective,
                               tolerance)) {
            if (verbosity >= 3) {
                cout << "response value of player " << player << " is "
                     << format("{:.12f}", evalResponseValue(player, point)) << endl;
                cout << "BR value is                   "
                     << format("{:.12f}",m_bestResponses[player].objective) << endl;
            }
            return false;
        }
    }
    return true;
}

void NEPManyEtasGurobi::computeBestResponse(indexx player,
                                            vector<double> const& point,
                                            shared_ptr<GRBEnv> const& globalEnv,
                                            double solverTimeLimit,
                                            int verbosity) {
    if (verbosity >= 3)
        cout << "BR of player " << player << ": ";

    // erasing last best response from m_bestResponses[player]
    m_bestResponses[player] = {-1, {}, GRB_INFINITY};

    try {
        if (verbosity >= 5) {
            cout << "compute best response" << endl;
            globalEnv->set(GRB_IntParam_LogToConsole, 1);
            globalEnv->set(GRB_IntParam_OutputFlag, 1);
        }

        vector<int> numberOfVariablesPerPlayer = getNumberOfVariablesPerPlayer();
        int numberOfVariables = accumulate(numberOfVariablesPerPlayer.begin(),
                                           numberOfVariablesPerPlayer.end(),
                                           0);
        vector<GRBVar> variables(numberOfVariables);

        GRBModel model = GRBModel(*globalEnv);
        model.set(GRB_DoubleParam_TimeLimit, solverTimeLimit);
        model.set(GRB_DoubleParam_MIPGap, 0);

        // create variables (valid for NEP and GNEP)
        MySparseVector c = m_bestResponseProblems[player]->getC();
        MySparseMatrix Q = m_bestResponseProblems[player]->getQ();
        vector<Constraint> constraints = m_bestResponseProblems[player]->getConstraints();
        vector<indexx> integerVariables = m_bestResponseProblems[player]->getIntegralityConstraints();
        indexx startingVariableIndex = 0;

        // put startingVariableIndex to its proper value
        for (indexx i = 0; i < player; i++)
            startingVariableIndex += getNumberOfVariablesOfPlayer(i);

        indexx endingVariableIndex = startingVariableIndex + getNumberOfVariablesOfPlayer(player);

        for (indexx i = 0; i < numberOfVariables; i++) {
            // variables called "x" + to_string(i)
            // test if i index of player:
            if (i >= startingVariableIndex && i < endingVariableIndex) {
                // declaring the player's variables
                if (ranges::find(integerVariables.begin(), integerVariables.end(), i)
                    != integerVariables.end())
                    variables[i] = model.addVar(-GRB_INFINITY,
                                                GRB_INFINITY,
                                                0,
                                                GRB_INTEGER,
                                                "x" + to_string(i));
                else // continuous variable
                    variables[i] = model.addVar(-GRB_INFINITY,
                                                GRB_INFINITY,
                                                0,
                                                GRB_CONTINUOUS,
                                                "x" + to_string(i));
            } else {
                // setting the values of other players' variables
                // to the value indicated by point
                variables[i] = model.addVar(point[i],
                                            point[i],
                                            0,
                                            GRB_CONTINUOUS,
                                            "x" + to_string(i));
            }
        }

        // create constraints
        for (indexx i = 0; i < constraints.size(); i++) {
            GRBLinExpr linExpr = 0;
            Constraint const& con = constraints[i];
            for (indexx j = 0; j < con.getAx().size(); j++) {
                linExpr += con.getAx().getValue(j) * variables[con.getAx().getIndex(j)];
            }
            if (con.getXQx().size() == 0)
                model.addConstr(linExpr <= con.getB(), "c" + to_string(i));
            else {
                GRBQuadExpr quadExpr = 0;
                for (indexx j = 0; j < con.getXQx().size(); j++) {
                    quadExpr += con.getXQx().getValue(j) * variables.at(con.getXQx().getRowIndex(j)) * variables.at(con.getXQx().getColIndex(j));
                }
                model.addQConstr(linExpr+quadExpr <= con.getB(), "c" + to_string(i) + "Quad");
                throw runtime_error("quadratic constraints in computeBestResponse for the first time, "
                    "check carefully if it works with the rest of the code");
            }
        }

        // set objective
        GRBLinExpr linExpr = 0;
        for (indexx i = 0; i < c.size(); i++) {
            linExpr += c.getValue(i) * variables[c.getIndex(i)];
        }
        GRBQuadExpr quadExpr = 0;
        for (indexx i = 0; i < Q.size(); i++) {
            quadExpr += Q.getValue(i)
                        * variables[Q.getRowIndex(i)]
                        * variables[Q.getColIndex(i)];
        }

        model.setObjective(linExpr+quadExpr, GRB_MINIMIZE);

        // optimize model
        model.optimize();

        // Status checking
        int status = model.get(GRB_IntAttr_Status);

        if (status == GRB_OPTIMAL) {
            // query values of the variable at the optimal solution found
            double objectiveValue = model.get(GRB_DoubleAttr_ObjVal);
            vector<double> solution(numberOfVariables);
            for (indexx i = 0; i < numberOfVariables; i++) {
                solution[i] = variables[i].get(GRB_DoubleAttr_X);
            }
            // do some integer rounding BEFORE working with those values
            // solution = integralityRounding(solution);
            // objectiveValue = integralityRounding(objectiveValue);

            // modify Node attributes
            m_bestResponses[player] = {status, solution, objectiveValue};

            if (verbosity >= 3)
                cout << model.get(GRB_DoubleAttr_Runtime) << " seconds: ";
            // verbosity == 3 for printing solution
            // verbosity == 2 for printing only status and obj value
            printSolveOutput(m_bestResponses[player], verbosity);
        }
        else if (status == GRB_TIME_LIMIT) {
            m_bestResponses[player] = {status, {}, GRB_INFINITY};
        }
        else {
            cout << "status of optimization for best response of player "
                 << player << " is not OPTIMAL but "
                 << status << " so what shall we do ?" << endl;
        }
    } catch(GRBException e) {
        cout << "Error code = " << e.getErrorCode() << endl;
        cout << e.getMessage() << endl;
    } catch(...) {
        throw runtime_error("Exception during optimization");
    }
}

void NEPManyEtasGurobi::computeAllBestResponses(vector<double> const& point,
                                                shared_ptr<GRBEnv> globalEnv,
                                                double solverTimeLimit,
                                                int verbosity) {
    for (indexx i = 0; i < m_numberOfPlayers; i++) {
        auto start = startChrono();
        computeBestResponse(i, point, globalEnv, solverTimeLimit, verbosity);
        solverTimeLimit = max(solverTimeLimit - elapsedChrono(start), 0.0);
    }
}

shared_ptr<OptimizationProblem> NEPManyEtasGurobi::buildOptimizationProblem() {
    // 2 steps:
    // 1 - 'concatenate' all best responses into one
    // 2 - add specific parts coming from the Nikaido-Isoda function and the modeling

    // 1 - concatenation of opt problems
    int numberOfVariables = 0;
    for (indexx i = 0; i < m_numberOfPlayers; i++) {
        numberOfVariables += getNumberOfVariablesOfPlayer(i);
    }
    MySparseVector c = m_bestResponseProblems[0]->getC();
    for (indexx i = 1; i < m_numberOfPlayers; i++) {
        c = c + m_bestResponseProblems[i]->getC();
    }
    MySparseMatrix Q = m_bestResponseProblems[0]->getQ();
    for (indexx i = 1; i < m_numberOfPlayers; i++) {
        Q = Q + m_bestResponseProblems[i]->getQ();
    }
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
    // knowing the integrality constraints is necessary to build child nodes.
    // Those constraints are relaxed in the node problem
    // when building the solver-specific model
    vector<indexx> integralityConstraints =
        m_bestResponseProblems[0]->getIntegralityConstraints();
    for (indexx i = 1; i < m_numberOfPlayers; i++) {
        vector<indexx> newVec = m_bestResponseProblems[i]->getIntegralityConstraints();
        integralityConstraints.insert(
            integralityConstraints.end(),
            newVec.begin(),
            newVec.end());
    }

    // 2 - special ingredients : eta_i for each player
    // add eta_i for each player with bounds
    for (indexx player = 0; player < m_numberOfPlayers; player++) {
        numberOfVariables++;
        c.addCoefficient(-1.0, numberOfVariables-1);

        // eta_i >= LB
        constraints.push_back({{{-1.0}, {numberOfVariables-1}}, {}, -m_etaLowerBounds[player]});

        // eta_i <= UB
        constraints.push_back({{{1.0}, {numberOfVariables-1}}, {}, m_etaUpperBounds[player]});
    }

    // definition of OptimizationProblem problem
    shared_ptr<OptimizationProblem> problem =
        make_shared<OptimizationProblem>(numberOfVariables,
                                         c,
                                         Q,
                                         constraints,
                                         integralityConstraints);

    return problem;
}

Cut NEPManyEtasGurobi::buildDisaggregatedEquilibriumCut(indexx player,
                                                        vector<double> const& point) const {
    // eta_i - \pi_i(y*_i,x_-i) <= 0

    // initializing separately all attributes of a Cut
    MySparseVector ax = MySparseVector();
    MySparseMatrix xQx = MySparseMatrix();
    double b = 0;

    vector<int> numberOfVariablesPerPlayer = getNumberOfVariablesPerPlayer();
    indexx startingVariableIndex = 0;
    indexx endingVariableIndex = 0;

    for (indexx i = 0; i < player; i++)
        startingVariableIndex += numberOfVariablesPerPlayer[i];
    endingVariableIndex = startingVariableIndex + numberOfVariablesPerPlayer[player];

    // rebuild cost function evaluation
    // between startingVariableIndex and endingVariableIndex it is y*_i, elsewhere it is x_-i
    vector<double> bestResponseVector = m_bestResponses[player].solution;
    MySparseVector c = m_bestResponseProblems[player]->getC();
    MySparseMatrix Q = m_bestResponseProblems[player]->getQ();
    // simple index check for c
    for (indexx k = 0; k < c.size(); k++) {
        // coefficients of c are not sorted, so we need to check
        if (c.getIndex(k) >= startingVariableIndex && c.getIndex(k) < endingVariableIndex) {
            // coef of the form c y*_i, so it is constant
            // so it goes to the rhs with a plus
            b += c.getValue(k) * bestResponseVector[c.getIndex(k)];
        } else {
            // adding a coef of form c x_-i
            // may not happen in practice because it is a constant term when minimizing in x_i
            ax.addCoefficient(-c.getValue(k), c.getIndex(k));
        }
    }

    // double index check for Q
    for (indexx k = 0; k < Q.size(); k++) {
        indexx i = Q.getRowIndex(k);
        indexx j = Q.getColIndex(k);
        if (i >= startingVariableIndex && i < endingVariableIndex) {
            if (j>= startingVariableIndex && j < endingVariableIndex) {
                // y*_p Qij y*_p -> constant term
                b -= bestResponseVector[i] * (-Q.getValue(k)) * bestResponseVector[j];
            } else {
                // y*_p Qij x_-p -> linear term
                ax.addCoefficient(bestResponseVector[i] * (-Q.getValue(k)), j);
            }
        } else {
            if (j >= startingVariableIndex && j < endingVariableIndex) {
                // x_-p Qij y*_p -> linear term
                ax.addCoefficient(-Q.getValue(k) * bestResponseVector[j], i);
            } else {
                // x_-p Qij x_-p -> quadratic term
                xQx.addCoefficient(-Q.getValue(k), i, j);
            }
        }
    }

    // adding eta_i value
    ax.addCoefficient(1.0, getEtaIndex(player));

    // instantiate a Cut object with its individual components
    Cut cut = Cut(ax, xQx, b);
    return cut;
}

double NEPManyEtasGurobi::evalNikaidoIsoda(vector<double> const& point,
                                           double objectiveValue,
                                           bool isSolutionInteger) const  {
    // Psi(x*,y*) = objectiveValue + sum_player (eta_player - bestResponseValue)

    // recover the sum of costs from the node objective value
    double sumOfBestResponseValue = 0.0;
    double sumOfEtas = 0;
    for (indexx player = 0; player < m_numberOfPlayers; player++) {
        sumOfBestResponseValue += m_bestResponses[player].objective;
        sumOfEtas += point[getEtaIndex(player)];
    }
    double value = objectiveValue + sumOfEtas - sumOfBestResponseValue;

    // check that value > -tolerance
    if (!compareApproxMore(value,0) and isSolutionInteger) {
        // only valid if the solution satisfies integrality requirements
        throw logic_error("evaluation of V hat function < 0: " + to_string(value));
    }
    return value;
}

vector<double> NEPManyEtasGurobi::differencesToBestResponseValues(
        vector<double> const& point) const {
    vector<double> differences = {};
    for (indexx player = 0; player < m_numberOfPlayers; player++) {
        differences.push_back(evalResponseValue(player, point)
                              - m_bestResponses[player].objective);
    }
    return differences;
}

bool NEPManyEtasGurobi::testBestEtaPlayerPossible(indexx player,
                                                  vector<double> const& point,
                                                  double const& tolerance,
                                                  int verbosity) const {
    double eta_player = point[getEtaIndex(player)];
    double const& bestResponseValue = m_bestResponses[player].objective;
    // possible cause of numerical issues: tolerance for the comparison below
    if (verbosity == 3)
        cout << "the proxy eta for player " << player << " has a difference of "
             << format("{:.6f}", eta_player-bestResponseValue) << endl;
    if (verbosity >= 4)
        cout << "is eta_" << player << " at the right value ? eta_"
             << player << " = " << format("{:.6f}", eta_player)
             << " and BR value = " << format("{:.6f}", bestResponseValue)
             << " which means a difference of "
             << format("{:.6f}", eta_player-bestResponseValue) << endl;
    // numerical issues with 1e-6. The solution did not satisfy the new cut by 1.2e-6,
    // but gurobi still found the same solution at the next iteration

    // increasing tolerance value because of numerical errors
    return compareApproxLess(eta_player, bestResponseValue, tolerance);
}

bool NEPManyEtasGurobi::testBestEtaPossible(vector<double> const& point,
                                            double const& tolerance,
                                            int verbosity) const {
    for (indexx player = 0; player < m_numberOfPlayers; player++)
        if (!testBestEtaPlayerPossible(player, point, tolerance, verbosity)) {
            // tolerance == 0 because this pruning step is valid only when node obj > 0,
            // so it has already been pruned if it should be pruned here
            return false;
        }
    return true;
}

SolveOutput NEPManyEtasGurobi::computeEtaBound(indexx player, bool isUpperBound, double mipGap) const {
    try {
        // Create an environment
        GRBEnv env = GRBEnv(true);
        env.set(GRB_IntParam_OutputFlag, 0);
        env.set("LogFile", "");
        env.start();

        vector<int> numberOfVariablesPerPlayer = getNumberOfVariablesPerPlayer();
        int numberOfVariables = accumulate(numberOfVariablesPerPlayer.begin(),
                                           numberOfVariablesPerPlayer.end(),
                                           0);
        vector<GRBVar> variables(numberOfVariables);
        double normalMipGap = env.get(GRB_DoubleParam_MIPGap);
        env.set(GRB_DoubleParam_MIPGap, mipGap);
        GRBModel model = GRBModel(env);

        // objective will be updated with all players best response models
        GRBLinExpr linExprObj = 0;
        GRBQuadExpr quadExprObj = 0;

        // create variables
        for (indexx i = 0; i < numberOfVariables; i++)
            variables[i] = model.addVar(-GRB_INFINITY,
                                          GRB_INFINITY,
                                          0,
                                          GRB_CONTINUOUS,
                                          "x" + to_string(i));

        // model objective and constraints
        MySparseVector c = m_bestResponseProblems[player]->getC();
        MySparseMatrix Q = m_bestResponseProblems[player]->getQ();

        // create constraints of all players
        for (indexx p = 0; p < m_numberOfPlayers; p++) {
            vector<Constraint> constraints = m_bestResponseProblems[p]->getConstraints();
            for (indexx i = 0; i < constraints.size(); i++) {
                // constraint for x variables
                GRBLinExpr linExpr = 0;
                Constraint const& con = constraints[i];
                for (indexx j = 0; j < con.getAx().size(); j++) {
                    linExpr += con.getAx().getValue(j) * variables[con.getAx().getIndex(j)];
                }
                if (con.getXQx().size() == 0)
                    model.addConstr(linExpr <= con.getB(), "c" + to_string(i));
                else {
                    GRBQuadExpr quadExpr = 0;
                    for (indexx j = 0; j < con.getXQx().size(); j++) {
                        quadExpr += con.getXQx().getValue(j) * variables.at(con.getXQx().getRowIndex(j)) * variables.at(con.getXQx().getColIndex(j));
                    }
                    model.addQConstr(linExpr+quadExpr <= con.getB(), "c" + to_string(i) + "Quad");
                    throw runtime_error("quadratic constraints in computeEtaBound for the first time, "
                        "check carefully if it works with the rest of the code");
                }
            }
        }

        // set objective
        for (indexx i = 0; i < c.size(); i++) {
            indexx index = c.getIndex(i);
            linExprObj += c.getValue(i) * variables[index];
        }
        for (indexx i = 0; i < Q.size(); i++) {
            indexx rowIndex = Q.getRowIndex(i);
            indexx colIndex = Q.getColIndex(i);
            quadExprObj += Q.getValue(i) * variables[rowIndex] * variables[colIndex];
        }
        // set objective after all player's best response models have been treated
        if (isUpperBound)
            model.setObjective(linExprObj+quadExprObj, GRB_MAXIMIZE);
        else
            model.setObjective(linExprObj+quadExprObj, GRB_MINIMIZE);

        // optimize model
        model.optimize();
        // put normal MIPGap value again
        env.set(GRB_DoubleParam_MIPGap, normalMipGap);

        // Status checking
        int status = model.get(GRB_IntAttr_Status);

        if (status == GRB_OPTIMAL) {
            // query values of the variable at the optimal solution found
            double objectiveValue = model.get(GRB_DoubleAttr_ObjVal);
            vector<double> solution(numberOfVariables);
            for (indexx i = 0; i < numberOfVariables; i++) {
                solution[i] = variables[i].get(GRB_DoubleAttr_X);
            }

            // do some integer rounding BEFORE working with those values
            // solution = integralityRounding(solution);
            // objectiveValue = integralityRounding(objectiveValue);

            // modify Node attributes
            SolveOutput infoSolve = {status, solution, model.get(GRB_DoubleAttr_ObjBound)};

            return infoSolve;
        }
        else {
            if (status != 4 and status != 3)
                cout << "status of optimization is neither OPTIMAL "
                        "nor INFEASIBLE (nor UNBOUNDED) but "
                     << status << endl;
            else if (status == 3)
                cout << "best response problem: status of "
                        "optimization is INFEASIBLE" << endl;
            else
                cout << "best response problem: status of optimization "
                        "is INFEASIBLE (or UNBOUNDED)" << endl;
            SolveOutput infoSolve = {status,
                                     {},
                                     -GRB_INFINITY*(static_cast<int>(isUpperBound)*2-1)};
            return infoSolve;
        }
    } catch(GRBException e) {
        cout << "Error code = " << e.getErrorCode() << endl;
        cout << e.getMessage() << endl;
    } catch(...) {
        cout << "Exception during optimization" << endl;
    }
    return {-1, {}, -GRB_INFINITY*(static_cast<int>(isUpperBound)*2-1)};
}

void NEPManyEtasGurobi::computeEtasBounds(double mipGap) {
    auto startTime = startChrono();
    for (indexx player = 0; player < m_numberOfPlayers; player++) {
        double UB,LB;
        SolveOutput infoUBSolve = computeEtaBound(player, true, mipGap);
        if (infoUBSolve.status == GRB_OPTIMAL) {
            UB = infoUBSolve.objective;
        }
        else
            UB = etaUB;
        SolveOutput infoLBSolve = computeEtaBound(player, false, mipGap);
        if (infoLBSolve.status == GRB_OPTIMAL) {
            LB = infoLBSolve.objective;
        }
        else
            LB = etaLB;
        if (!m_isGNEP) {
            m_etaLowerBounds[player] = LB;
            m_etaUpperBounds[player] = UB;
        } else {
            // integer data necessary for GNEP because of assumption for intersection cuts
            m_etaLowerBounds[player] = floor(LB);
            m_etaUpperBounds[player] = ceil(UB);
        }
        cout << "eta_" << player << " bounded by ["
             << m_etaLowerBounds[player] << ", " << m_etaUpperBounds[player] << "]" << endl;

    }
    cout << "computation time of eta bounds with MIP gap " << mipGap
         << " is " << elapsedChrono(startTime) << " seconds" << endl;
}

vector<double> NEPManyEtasGurobi::extractNashEquilibrium(
        vector<double> nashEquilibrium,
        double tolerance) {
    for (indexx player = 0; player < m_numberOfPlayers; player++) {
        // remove eta_player value
        nashEquilibrium.pop_back();
    }
    // check if nashEquilibrium really is an NE
    if (!checkNE(nashEquilibrium, tolerance, 0)) {
        stringstream ss;
        ss << "adding a vector as an NE but it is not an NE: " << nashEquilibrium << endl;
        throw logic_error(ss.str());
    }
    return nashEquilibrium;
}

std::vector<Cut> NEPManyEtasGurobi::deriveEquilibriumCutForEachPlayer(
        shared_ptr<GRBModel> const &model,
        vector<double> const &point,
        double tolerance,
        int verbosity) const {
    vector<Cut> cuts = {};
    vector<int> const numberOfVariablesPerPlayer = getNumberOfVariablesPerPlayer();
    indexx startingVariableIndex = 0;
    for (indexx player = 0; player < m_numberOfPlayers; player++) {
        if (!testBestEtaPlayerPossible(player, point, tolerance)) {
            cuts.push_back(buildDisaggregatedEquilibriumCut(player, point));
            if (verbosity >= 3)
                cout << "adding equilibrium cut for player "
                     << player << ": " << cuts[cuts.size()-1] << endl;
            else if (verbosity >= 2)
                cout << "adding equilibrium cut for player "
                     << player << endl;
        }
        else if (verbosity >= 2)
            cout << "no cut for player " << player << endl;
        startingVariableIndex += numberOfVariablesPerPlayer[player];
    }
    return cuts;
}

CutInformation NEPManyEtasGurobi::deriveICForEachPlayer(
    shared_ptr<GRBModel> const &nodeModel,
    vector<double> const &point,
    double tolerance,
    int verbosity) const {
    vector<Cut> cuts = {};
    int intersectionCutAlreadyComputed = 0;
    MatrixXd extremeRays;
    unordered_map<string, double> timeCounters =
        {{"rayBuildingTime", 0.0},
         {"NEFreeSetBuildingTime", 0.0},
         {"FinalStepICBuildingTime", 0.0}};
    auto setupIC = initializeSetupIC(nodeModel, verbosity);
    for (indexx player = 0; player < m_numberOfPlayers; player++) {
        if (!testBestEtaPlayerPossible(player, point, tolerance)) {
            auto timerIC = startChrono();
            if (intersectionCutAlreadyComputed == 0) {
                auto timerBuildExtremeRays = startChrono();
                extremeRays = buildExtremeRays(nodeModel, setupIC, 0);
                timeCounters["rayBuildingTime"] += elapsedChrono(timerBuildExtremeRays);
                if (verbosity >= 3) {
                    cout << "extreme rays computed in " << elapsedChrono(timerBuildExtremeRays) << " seconds" << endl;
                }
            }
            auto timerBuildNEFreeSet = startChrono();
            auto NEFreeSet = getNEFreeSetEta(player, setupIC.numDeclaredVariables);
            timeCounters["NEFreeSetBuildingTime"] += elapsedChrono(timerBuildNEFreeSet);
            if (verbosity >= 3)
                cout << "NE-free set computed in " << elapsedChrono(timerBuildNEFreeSet) << " seconds" << endl;
            auto timerFinalStepIC = startChrono();
            Cut cut = deriveICForOnePlayer(NEFreeSet,
                                           player,
                                           point,
                                           nodeModel,
                                           extremeRays,
                                           setupIC,
                                           verbosity);
            timeCounters["FinalStepICBuildingTime"] += elapsedChrono(timerFinalStepIC);

            if (verbosity >= 4) {
                // check if big coefficients in the cut
                for (double value : cut.getAx().getValues()) {
                    if (value > 1e6) {
                        cout << cut << endl;
                        cout << "high value of cut: " << value << endl;
                        // auto temp = buildExtremeRays(nodeModel, setupIC, verbosity);
                        break;
                    }
                }
            }

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
        else
            if (verbosity >= 2)
                cout << "no cut for player " << player << endl;
    }
    return {cuts,timeCounters};
}

std::vector<Cut> NEPManyEtasGurobi::deriveNoGoodCut(
        vector<double> const &point,
        int verbosity) const {
    // valid only when all variables are binary
    // add only one cut
    vector<double> values = {};
    vector<indexx> indices = {};
    double b = -1;
    int nVariables = getTotalNumberOfVariablesOfPlayers();
    for (indexx index = 0; index < nVariables;index++) {
        double xi = point[index];
        indices.push_back(index);
        if (compareApproxEqual(xi,0)) {
            values.push_back(-1);
        } else if (compareApproxEqual(xi,1)) {
            values.push_back(1);
            b += 1;
        } else {
            cout << point << endl;
            //TODO: implement a proper test that only binary variables are present
            // because the following one does not always work
            throw logic_error("deriving a no-good cut for a non-binary vector "
                              "because of coefficient "
                              + to_string(point[index]) + " at index " + to_string(index));
        }
    }
    Cut cut({MySparseVector(values,indices),{},b});
    if (verbosity >= 3)
        cout << cut << endl;
    return {cut};
}

CutInformation NEPManyEtasGurobi::deriveCut(vector<double> const &point,
                                            string const &whichCutString,
                                            double const &tolerance,
                                            shared_ptr<GRBEnv> const &globalEnv,
                                            shared_ptr<GRBModel> const &nodeModel,
                                            shared_ptr<OptimizationProblem> const &nodeProblem,
                                            int verbosity) {
    if (whichCutString == "eqCuts") {
        return {deriveEquilibriumCutForEachPlayer(nodeModel, point, tolerance, verbosity),{}};
    }
    if (whichCutString == "intersectionCuts") {
        return deriveICForEachPlayer(nodeModel, point, tolerance, verbosity);
    }
    if (whichCutString == "noGoodCuts") {
        return {deriveNoGoodCut(point, verbosity),{}};
    }

    // after all declared cut types
    throw runtime_error("Wrong input whichCutString: string '"
                            + whichCutString + "' describing cut has not "
                            "been coded in NashEquilibriumProblemManyEtasGurobi::deriveCut");
}

SetupIC NEPManyEtasGurobi::initializeSetupIC(
        shared_ptr<GRBModel> const& model,
        int const verbosity) {
    // declare useful objects to build an LP basis
    int const numDeclaredVariables = model->get(GRB_IntAttr_NumVars);
    int const numConstraints = model->get(GRB_IntAttr_NumConstrs);
    // the four following vectors allow to remember which column of
    // later matrices are associated with which coefficients
    // the two following vectors contain only indices from variables declared in gurobi model
    vector<indexx> basicVariableIndices = {};
    vector<indexx> nonbasicVariableIndices = {};
    // the two following vectors contain only indices from constraints declared in gurobi model
    vector<indexx> basicConstraintIndices = {};
    vector<indexx> nonbasicConstraintIndices = {};

    // get basic variables
    for (indexx variableIndex = 0; variableIndex < numDeclaredVariables; variableIndex++) {
        if (model->getVar(variableIndex).get(GRB_IntAttr_VBasis) == 0) // basic variable
            basicVariableIndices.push_back(variableIndex);
        else {
            nonbasicVariableIndices.push_back(variableIndex);
            if (verbosity >= 1)
                cout << "nonbasic declared variable found" << endl;
            // throw runtime_error("nonbasic declared variable found");
            // in general it is to be expected but the implementation might be only working if
            // all declared variables are basic variables
        }
    }

    // get basic constraints
    int numInequalityConstraints = 0;
    auto constraints = model->getConstrs();
    for (indexx constraintIndex = 0; constraintIndex < numConstraints; constraintIndex++) {
        if (constraints[constraintIndex].get(GRB_IntAttr_CBasis) == 0)
            basicConstraintIndices.push_back(constraintIndex);
        else {
            nonbasicConstraintIndices.push_back(constraintIndex);
            if (constraints[constraintIndex].get(GRB_IntAttr_CBasis) == -2)
                throw runtime_error("nonbasic slack variable at upper bound found");
        }
        if (constraints[constraintIndex].get(GRB_CharAttr_Sense) != '=') {
            numInequalityConstraints++;
            if (constraints[constraintIndex].get(GRB_CharAttr_Sense) == '>') {
                throw logic_error("check if IC implementation works with >= constraints");
            }
        }
        else {
            // only '<=' constraints are handled, so raises an error if an equality is detected
            throw logic_error("ERROR(?): there is at least an equality "
                              "constraint at (gurobi) index " + to_string(constraintIndex));
        }
    }

    // number of variables in the LP basis
    int numBasicVariables = static_cast<int>(basicVariableIndices.size()) + static_cast<int>(basicConstraintIndices.size());

    // number variables + number inequality constraints among basic constraints
    // (for slack variables) - number of basic (declared) variables
    int numNonbasicVariables = numDeclaredVariables + numInequalityConstraints - numBasicVariables;

    // check if numNonbasicVariables == numDeclaredVariables
    if (numNonbasicVariables != numDeclaredVariables) {
        throw logic_error("implementation might not work because the number of "
                "nonbasic variables is not the number of declared variables");
    }

    SetupIC setupIC(
        basicVariableIndices,
        nonbasicVariableIndices,
        basicConstraintIndices,
        nonbasicConstraintIndices,
        numBasicVariables,
        numNonbasicVariables,
        numDeclaredVariables,
        numConstraints,
        constraints);
    return setupIC;
}

MatrixXd NEPManyEtasGurobi::buildExtremeRays(shared_ptr<GRBModel> const& model,
                                             SetupIC const& setupIC,
                                             int verbosity) {
    if (model->get(GRB_IntAttr_Status) != 2) {
        // not interested in LP basis if not an optimal solution
        throw logic_error("an intersection cut can't be derived "
                          "if the node problem was not solved optimally");
    }

    // compute extreme rays of the corner polyhedron of the node problem described in model
    // pointed at the optimal solution
    // steps:
    // 1) compute coefficients of the LP basis as triplets (rowIndex, colIndex, value) for sparsity
    // 2) declare the sparse matrix
    // 3) perform LU decomposition on the LP basis and apply the inverse of the LP basis to the nonbasic columns
    // 4) build the extreme rays
    // 5) remove coefficients too small for numerical issues (maybe not needed)

    // 1) compute coefficients of the LP basis as triplets (rowIndex, colIndex, value) for sparsity

    // declare now the rebased matrix
    Eigen::SparseMatrix<double> rebasedMatrixNonbasicVar(setupIC.numBasicVariables,setupIC.numNonbasicVariables);

    // build sparse matrices
    vector<Eigen::Triplet<double>> tripletsMatrixBasicVar;
    vector<Eigen::Triplet<double>> tripletsMatrixNonbasicVar;

    // fulfill matrices with columns of basic and nonbasic variables
    int colIndexBasicMatrix = 0;
    int colIndexNonbasicMatrix = 0;
    // for declared variables
    for (indexx variableIndex = 0; variableIndex < setupIC.numDeclaredVariables; variableIndex++) {
        if (model->getVar(variableIndex).get(GRB_IntAttr_VBasis) == 0) {
            // basic variables; add full column to matrix
            for (indexx rowIndex = 0; rowIndex < setupIC.numConstraints; rowIndex++) {
                if (model->getCoeff(setupIC.constraints[rowIndex], model->getVar(variableIndex)) != 0)
                    tripletsMatrixBasicVar.emplace_back(rowIndex,
                                                        colIndexBasicMatrix,
                                                        model->getCoeff(setupIC.constraints[rowIndex],
                                                        model->getVar(variableIndex)));
            }
            colIndexBasicMatrix++;
        } else {
            // nonbasic (explicitely declared) variable
            for (indexx rowIndex = 0; rowIndex < setupIC.numConstraints; rowIndex++) {
                if (model->getCoeff(setupIC.constraints[rowIndex], model->getVar(variableIndex)) != 0)
                    tripletsMatrixNonbasicVar.emplace_back(rowIndex,
                                                           colIndexNonbasicMatrix,
                                                           model->getCoeff(setupIC.constraints[rowIndex],
                                                           model->getVar(variableIndex)));
            }
            colIndexNonbasicMatrix++;
        }
    }
    // for slack variables associated to constraints, declared by gurobi
    for (indexx constraintIndex = 0; constraintIndex < setupIC.numConstraints; constraintIndex++) {
        if (setupIC.constraints[constraintIndex].get(GRB_IntAttr_CBasis) == 0) {
            // add coef 1 to corresponding row for the slack variable
            for (int rowIndex = 0; rowIndex < setupIC.numConstraints; rowIndex++) {
                if (rowIndex == constraintIndex) {
                    tripletsMatrixBasicVar.emplace_back(rowIndex,colIndexBasicMatrix, 1);
                }
            }
            colIndexBasicMatrix++;
        } else { // nonbasic slack variable
            for (int rowIndex = 0; rowIndex < setupIC.numConstraints; rowIndex++) {
                if (rowIndex == constraintIndex) {
                    tripletsMatrixNonbasicVar.emplace_back(rowIndex,colIndexNonbasicMatrix, 1);
                }
            }
            colIndexNonbasicMatrix++;
        }
    }

    // 2) declare the sparse matrix
    // access elements with sparseMatrix.coeff(rowIndex,colIndex)
    Eigen::SparseMatrix<double> sparseMatrixBasicVar(setupIC.numBasicVariables,setupIC.numBasicVariables);
    sparseMatrixBasicVar.setFromTriplets(tripletsMatrixBasicVar.begin(), tripletsMatrixBasicVar.end());
    Eigen::SparseMatrix<double> sparseMatrixNonbasicVar(setupIC.numBasicVariables,setupIC.numNonbasicVariables);
    sparseMatrixNonbasicVar.setFromTriplets(tripletsMatrixNonbasicVar.begin(), tripletsMatrixNonbasicVar.end());

    // 3) perform LU decomposition on the LP basis and apply the inverse of the LP basis to the nonbasic columns
    // initialize LU decomposition with sparseMatrixBasicVar
    Eigen::SparseLU<Eigen::SparseMatrix<double> > lu(sparseMatrixBasicVar);
    // apply the inverse of the LP basis to the nonbasic columns
    rebasedMatrixNonbasicVar = lu.solve(sparseMatrixNonbasicVar);
    // technical stuff of Eigen
    rebasedMatrixNonbasicVar = rebasedMatrixNonbasicVar.pruned();
    rebasedMatrixNonbasicVar.makeCompressed();

    // 4) build the extreme rays
    // extremeRays is the matrix containing the extreme rays.
    // row i represents the extreme ray associated with the nonbasic variable nonbasicVariableIndices[i].
    // columns are ordered first with explicitely declared variables,
    // then slack variables in the order of the constraints they are associated to
    MatrixXd extremeRays(setupIC.numNonbasicVariables,setupIC.numDeclaredVariables+setupIC.numBasicVariables);
    // currently, all constraints are <= constraints, thus there is one slack variable per constraint
    // so numBasicVariables == numDeclaredVariables+numConstraints
    // and numNonbasicVariables == numDeclaredVariables

    // copy all columns of -rebasedMatrixNonbasicVar to the proper columns of extremeRays
    // TODO generic: to support bounds on variables, in case of 'nonbasic at upper bound',
    // TODO generic: change the sign of the whole extreme ray (part with a_ij and coef 1 for the nonbasic variable)
    for (indexx rowIndex = 0; rowIndex < setupIC.numNonbasicVariables; rowIndex++) {
        for (indexx colIndex = 0; colIndex < setupIC.basicVariableIndices.size(); colIndex++) {
            extremeRays(rowIndex,setupIC.basicVariableIndices[colIndex])
                = -rebasedMatrixNonbasicVar.coeff(colIndex,rowIndex);
        }
        for (indexx colIndex = 0; colIndex < setupIC.basicConstraintIndices.size(); colIndex++) {
            extremeRays(rowIndex,setupIC.numDeclaredVariables + setupIC.basicConstraintIndices[colIndex])
                = -rebasedMatrixNonbasicVar.coeff(static_cast<int>(setupIC.basicVariableIndices.size()) + colIndex,rowIndex);
        }
    }
    // add coefficients 1 corresponding to the indices of
    // nonbasic variables and 0 to remaining indices
    int numNonbasicDeclaredVariables = static_cast<int>(setupIC.nonbasicVariableIndices.size());
    for (indexx rayIndex = 0; rayIndex < setupIC.numNonbasicVariables; rayIndex++) {
        if (rayIndex < numNonbasicDeclaredVariables) {
            // initialize column
            for (indexx rowIndex = 0; rowIndex < setupIC.numNonbasicVariables; rowIndex++)
                extremeRays(rowIndex,setupIC.nonbasicVariableIndices[rayIndex]) = 0;
            // add coefficient 1
            extremeRays(rayIndex,setupIC.nonbasicVariableIndices[rayIndex]) = 1;
        }
        else {
            // initialize column
            for (indexx rowIndex = 0; rowIndex < setupIC.numNonbasicVariables; rowIndex++)
                extremeRays(rowIndex,setupIC.numDeclaredVariables +
                    setupIC.nonbasicConstraintIndices[rayIndex-numNonbasicDeclaredVariables]) = 0;
            // add coefficient 1
            extremeRays(rayIndex,setupIC.numDeclaredVariables +
                setupIC.nonbasicConstraintIndices[rayIndex-numNonbasicDeclaredVariables]) = 1;
        }
    }

    // 5) remove coefficients too small for numerical issues (maybe not needed)
    // put 0 instead of all values of magnitude less than tolerance
    // in extremeRays in the hope of limiting the numerical issues
    double tolerance = 1e-11;
    for (indexx rowIndex = 0; rowIndex < setupIC.numNonbasicVariables; rowIndex++) {
        for (indexx colIndex = 0; colIndex < setupIC.numDeclaredVariables+setupIC.numBasicVariables; colIndex++) {
            if (isApproxZero(extremeRays(rowIndex,colIndex), tolerance))
                extremeRays(rowIndex,colIndex) = 0;
        }
    }

    // verbosity
    if (verbosity >= 4) {
        cout << "triplets of basic matrix:" << endl;
        for (auto triplet : tripletsMatrixBasicVar) {
            cout << triplet.row() << " " << triplet.col() << " " << triplet.value() << endl;
        }
        cout << "triplets of nonbasic matrix:" << endl;
        for (auto triplet : tripletsMatrixNonbasicVar) {
            cout << triplet.row() << " " << triplet.col() << " " << triplet.value() << endl;
        }
    }

    return extremeRays;
}

Constraint NEPManyEtasGurobi::buildPlayerConstraintOfNEFreeSet(
        Constraint const& constraint,
        vector<double> const& y_i_star,
        indexx startingPlayerVariableIndex,
        indexx endingPlayerVariableIndex) {

    // create constraint a (y_i_star,x_-i) + (y_i_star,x_-i) Q (y_i_star,x_-i) <= b + 1
    // from constraint ax + xQx <= b in player's optimization problem
    // constant terms (rhs): b + 1 - a_i y_i_star - y_i_star Q_ii y_i_star
    // linear terms: a_-i x_-i + y_i_star Q_i,-i x_-i + x_-i Q_-i,i y_i_star
    // quadratic term, not supported: x_-i Q_-i,-i x_-i

    double b = constraint.getB();
    MySparseVector ax = constraint.getAx();
    MySparseMatrix Q = constraint.getXQx();


    // building arguments for intersectionRayHalfspace
    MySparseVector newAx;
    double newB = b + 1;
    // dealing with the linear term c of the constraint
    for (indexx j = 0; j < ax.size(); j++) {
        // check if it is a variable of this player
        if (ax.getIndex(j) >= startingPlayerVariableIndex and ax.getIndex(j) < endingPlayerVariableIndex)
            newB -= ax.getValue(j)*y_i_star[ax.getIndex(j)];
        // check if it is a variable of other players
        else if (ax.getIndex(j) < startingPlayerVariableIndex or ax.getIndex(j) >= endingPlayerVariableIndex)
            newAx.addCoefficient(ax.getValue(j),ax.getIndex(j));
    }

    // dealing with the quadratic term Q of the constraint
    // two cases: y_i_star Q x_-i and x_-i Q y_i_star treated in this order
    for (indexx j = 0; j < Q.size(); j++) {
        // row index of the player
        if (Q.getRowIndex(j) >= startingPlayerVariableIndex and Q.getRowIndex(j) < endingPlayerVariableIndex) {
            // column index of the other players
            if (Q.getColIndex(j) < startingPlayerVariableIndex or Q.getColIndex(j) >= endingPlayerVariableIndex) {
                newAx.addCoefficient(y_i_star[Q.getRowIndex(j)]*Q.getValue(j),Q.getColIndex(j));
            }
            else {
                // constant term y_i_star Q y_i_star
                newB -= y_i_star[Q.getRowIndex(j)]*Q.getValue(j)*y_i_star[Q.getColIndex(j)];
            }
        }
        // row index of the other players
        if (Q.getRowIndex(j) < startingPlayerVariableIndex or Q.getRowIndex(j) >= endingPlayerVariableIndex) {
            // column index of the player
            if (Q.getColIndex(j) >= startingPlayerVariableIndex and Q.getColIndex(j) < endingPlayerVariableIndex) {
                newAx.addCoefficient(y_i_star[Q.getColIndex(j)]*Q.getValue(j),Q.getRowIndex(j));
            }
            else {
                // quadratic term x_-i Q x_-i
                if (!compareApproxEqual(Q.getValue(j),0))
                    throw logic_error("quadratic term in a player constraint of a free set, check if it is normal");
            }
        }
    }
    Constraint newConstraint(newAx,{},newB);
    return newConstraint;
}

Constraint NEPManyEtasGurobi::buildEquilibriumConstraintOfNEFreeSet(
        int player,
        vector<double> const& y_i_star,
        indexx startingPlayerVariableIndex,
        indexx endingPlayerVariableIndex,
        indexx endingAllPlayerIndex) const {
    indexx etaIndex = getEtaIndex(player);
    MySparseVector c = m_bestResponseProblems[player]->getC();
    MySparseMatrix Q = m_bestResponseProblems[player]->getQ();

    // create linear constraint eta_i >= pi_i(y_i_star,x_-i)
    // in the form pi_i(y_i_star,x_-i) - eta_i <= 0
    // with pi_i the cost function of player i
    // pi_i(y_i_star,x_-i) = c (y_i_star,x_-i) + (y_i_star,x_-i) Q (y_i_star,x_-i)
    // constant terms with - sign because b is the rhs: -c_i y_i_star - y_i_star Q_ii y_i_star
    // linear terms: c_-i x_-i + y_i_star Q_i,-i x_-i + x_-i Q_-i,i y_i_star
    // quadratic term, not supported: x_-i Q_-i,-i x_-i

    MySparseVector ax;
    MySparseMatrix xQx;
    double b = 0;
    // dealing with the linear term c of the cost function
    for (indexx j = 0; j < c.size(); j++) {
        // check if it is a variable of this player
        if (c.getIndex(j) >= startingPlayerVariableIndex and c.getIndex(j) < endingPlayerVariableIndex)
            b -= c.getValue(j)*y_i_star[c.getIndex(j)];
        // check if it is a variable of other players
        else if (c.getIndex(j) < startingPlayerVariableIndex or c.getIndex(j) >= endingPlayerVariableIndex)
            ax.addCoefficient(c.getValue(j),c.getIndex(j));
    }
    // eta_i linear coefficient
    ax.addCoefficient(-1, etaIndex);

    // dealing with the quadratic term Q of the cost function
    // four cases:
    // y_i_star Q y_i_star (constant),
    // y_i_star Q x_-i (linear),
    // x_-i Q y_i_star (linear) and
    // x_-i Q x_-i (quadratic)
    for (indexx j = 0; j < Q.size(); j++) {
        // row index of the player
        if (Q.getRowIndex(j) >= startingPlayerVariableIndex and Q.getRowIndex(j) < endingPlayerVariableIndex) {
            // column index of the other players
            if (Q.getColIndex(j) < startingPlayerVariableIndex or Q.getColIndex(j) >= endingPlayerVariableIndex) {
                ax.addCoefficient(y_i_star[Q.getRowIndex(j)]*Q.getValue(j),Q.getColIndex(j));
                /*if (!compareApproxEqual(0,y_i_star[Q.getRowIndex(j)]*Q.getValue(j))) {
                    cout << "not an error but it was not tested with GNEP implementation games for exact NE" << endl;
                    cout << "nonzero linear coef coming from Q: ";
                    cout << y_i_star[Q.getRowIndex(j)]*Q.getValue(j);
                    cout << endl;
                    throw logic_error("check output above");
                }*/
            }
            else {
                // constant term y_i_star Q y_i_star
                b -= y_i_star[Q.getRowIndex(j)]*Q.getValue(j)*y_i_star[Q.getColIndex(j)];
            }
        }
        // row index of the other players
        if (Q.getRowIndex(j) < startingPlayerVariableIndex or Q.getRowIndex(j) >= endingPlayerVariableIndex) {
            // column index of the player
            if (Q.getColIndex(j) >= startingPlayerVariableIndex and Q.getColIndex(j) < endingPlayerVariableIndex) {
                ax.addCoefficient(y_i_star[Q.getColIndex(j)]*Q.getValue(j),Q.getRowIndex(j));
                /*if (!compareApproxEqual(0,y_i_star[Q.getColIndex(j)]*Q.getValue(j))) {
                    cout << "not an error but it was not tested with GNEP implementation games for exact NE" << endl;
                    cout << "nonzero linear coef coming from Q: ";
                    cout << y_i_star[Q.getColIndex(j)]*Q.getValue(j);
                    cout << endl;
                    throw logic_error("check output above");
                }*/
            }
            else {
                // quadratic term x_-i Q x_-i
                if (!compareApproxEqual(Q.getValue(j),0)) {
                    xQx.addCoefficient(Q.getValue(j), Q.getRowIndex(j), Q.getColIndex(j));
                    // throw logic_error("quadratic term in an equilibrium constraint of a free set, check if it is normal");
                }
            }
        }
    }
    Constraint constraint(ax,xQx,b);
    return constraint;
}

double NEPManyEtasGurobi::oldComputeIntersectionRayEquilibriumConstraint(
        int player,
        indexx i,
        vector<double> const& y_i_star,
        vector<double> const& solution,
        MatrixXd extremeRays,
        indexx startingPlayerVariableIndex,
        indexx endingPlayerVariableIndex,
        indexx endingAllPlayerIndex,
        double toleranceDenominator,
        int verbosity) const {

    // compute coefficient for constraint eta_i >= pi_i(y_i_star,x_-i)

    // all coefficients have a sign difference with the new method (cf buildEquilibriumConstraintOfNEFreeSet)
    // but the final result is the same because -b/-a = b/a

    indexx etaIndex = endingAllPlayerIndex+player;
    MySparseVector c = m_bestResponseProblems[player]->getC();
    MySparseMatrix Q = m_bestResponseProblems[player]->getQ();
    double alpha_i = -1;

    double c_iy_i_star = 0;
    for (indexx j = 0; j < c.size(); j++) {
        // check if it is a variable of this player
        if (c.getIndex(j) >= startingPlayerVariableIndex and c.getIndex(j) < endingPlayerVariableIndex)
            c_iy_i_star += c.getValue(j)*y_i_star[c.getIndex(j)];
    }
    double c_mix_mi_star = 0;
    for (indexx j = 0; j < c.size(); j++) {
        // check if it is a variable of other players
        if (c.getIndex(j) < startingPlayerVariableIndex or c.getIndex(j) >= endingPlayerVariableIndex)
            c_mix_mi_star += c.getValue(j)*solution[c.getIndex(j)];
    }

    // forgotten term in the preprint coming from the quadratic term of the cost
    // matters only in the implementation game but not in the GNEP knapsack
    double Q_miy_i_star = 0;
    for (indexx j = 0; j < Q.size(); j++) {
        // row variable of player i
        if (Q.getRowIndex(j) >= startingPlayerVariableIndex and Q.getRowIndex(j) < endingPlayerVariableIndex) {
            // column variable of other player -> y_i* Q x_-i
            if (Q.getColIndex(j) < startingPlayerVariableIndex or Q.getColIndex(j) >= endingPlayerVariableIndex) {
                Q_miy_i_star += y_i_star[Q.getRowIndex(j)]*Q.getValue(j)*solution[Q.getColIndex(j)];
            }
        }
        // row variable of other player
        if (Q.getRowIndex(j) < startingPlayerVariableIndex or Q.getRowIndex(j) >= endingPlayerVariableIndex) {
            // column variable of player i -> x_-i Q y_i*
            if (Q.getColIndex(j) >= startingPlayerVariableIndex and Q.getColIndex(j) < endingPlayerVariableIndex) {
                Q_miy_i_star += solution[Q.getRowIndex(j)]*Q.getValue(j)*y_i_star[Q.getColIndex(j)];
            }
        }
    }
    if (!compareApproxEqual(Q_miy_i_star, 0) and verbosity >= 4) {
        cout << "Q_miy_i_star = " << Q_miy_i_star << endl;
    }

    double numerator = c_iy_i_star + c_mix_mi_star + Q_miy_i_star - solution[etaIndex];
    // double numerator = c_iy_i_star + c_mix_mi_star - solution[etaIndex]; // old (and false) way

    double c_mir_xmi = 0;
    for (indexx j = 0; j < c.size(); j++) {
        // check if it is a variable of other players
        if (c.getIndex(j) < startingPlayerVariableIndex or c.getIndex(j) >= endingPlayerVariableIndex)
            c_mir_xmi += c.getValue(j)*extremeRays(i,c.getIndex(j));
    }
    double Q_mir_xmi = 0;
    for (indexx j = 0; j < Q.size(); j++) {
        // row variable of player i
        if (Q.getRowIndex(j) >= startingPlayerVariableIndex and Q.getRowIndex(j) < endingPlayerVariableIndex) {
            // column variable of other player -> y_i* Q r_-i
            if (Q.getColIndex(j) < startingPlayerVariableIndex or Q.getColIndex(j) >= endingPlayerVariableIndex) {
                Q_mir_xmi += y_i_star[Q.getRowIndex(j)]*Q.getValue(j)*extremeRays(i, Q.getColIndex(j));
            }
        }
        // row variable of other player
        if (Q.getRowIndex(j) < startingPlayerVariableIndex or Q.getRowIndex(j) >= endingPlayerVariableIndex) {
            // column variable of player i -> r_-i Q y_i*
            if (Q.getColIndex(j) >= startingPlayerVariableIndex and Q.getColIndex(j) < endingPlayerVariableIndex) {
                Q_mir_xmi += extremeRays(i, Q.getRowIndex(j))*Q.getValue(j)*y_i_star[Q.getColIndex(j)];
            }
        }
    }
    if (!compareApproxEqual(Q_mir_xmi, 0) and verbosity >= 4) {
        cout << "Q_mir_xmi: " << Q_mir_xmi << endl;

    }

    double denominator = extremeRays(i,etaIndex) - c_mir_xmi - Q_mir_xmi;
    // double denominator = extremeRays(i,etaIndex) - c_mir_xmi; // old (and false) way

    if (verbosity >= 4) {
        cout << "old method numerator: " << numerator << " / denominator: "
             << denominator << " / alpha_i: " << alpha_i << endl;
    }

    if (isApproxZero(denominator,toleranceDenominator)) {
        // do we need to put a tolerance? coefficients are fractional
        alpha_i = -1; // code for infinite value
    }
    else if (numerator / denominator > 0.0) {
        // if the fraction is negative, it means that it is always valid for nonnegative alpha values
        // => alpha_i candidate = infinity
        double limitValue = numerator / denominator;
        // alpha_i starts with this value
        // (instead of a min between itself (infinity undefined) and limitValue)
        // because it is the min among all constraints of S_i^+ of limit values
        alpha_i = limitValue;
    }
    return alpha_i;
}

double NEPManyEtasGurobi::oldComputeIntersectionRayPlayerConstraint(
        indexx i,
        Constraint const& constraint,
        vector<double> const& y_i_star,
        vector<double> const& solution,
        MatrixXd extremeRays,
        indexx startingPlayerVariableIndex,
        indexx endingPlayerVariableIndex,
        double toleranceDenominator,
        int verbosity) {

    double b = constraint.getB();
    MySparseVector ax = constraint.getAx();
    MySparseMatrix Q = constraint.getXQx();

    double A_mix_mi = 0;
    for (indexx j = 0; j < ax.size(); j++) {
        // check if it is a variable of other players
        if (ax.getIndex(j) < startingPlayerVariableIndex or ax.getIndex(j) >= endingPlayerVariableIndex)
            A_mix_mi += ax.getValue(j)*solution[ax.getIndex(j)];
    }
    double A_iy_i_star = 0;
    for (indexx j = 0; j < ax.size(); j++) {
        // check if it is a variable of this player
        if (ax.getIndex(j) >= startingPlayerVariableIndex and ax.getIndex(j) < endingPlayerVariableIndex)
            A_iy_i_star += ax.getValue(j)*y_i_star[ax.getIndex(j)];
    }
    double numerator = b - A_mix_mi - A_iy_i_star + 1;

    double A_mir_xmi = 0;
    for (indexx j = 0; j < ax.size(); j++) {
        // check if it is a variable of other players
        if (ax.getIndex(j) < startingPlayerVariableIndex or ax.getIndex(j) >= endingPlayerVariableIndex)
            A_mir_xmi += ax.getValue(j)*extremeRays(i,ax.getIndex(j));
    }
    double denominator = A_mir_xmi;

    if (verbosity >= 4) {
        cout << "old method numerator: " << numerator << " / denominator: " << denominator << endl;
    }

    if (!isApproxZero(denominator,toleranceDenominator)) {
        // if the fraction is negative, it means that it is always valid for nonnegative alpha values
        // => alpha_i candidate = infinity
        if (numerator / denominator > 0.0) {
            return numerator / denominator;
        }
    }
    return -1;
}

std::vector<Constraint> NEPManyEtasGurobi::getNEFreeSetEta(
        indexx player,
        indexx numDeclaredVariables) const {
    // best response vector
    vector<double> y_i_star = m_bestResponses[player].solution;

    // indices of variables
    indexx startingPlayerVariableIndex = getStartingPlayerVariableIndex(player);
    indexx endingPlayerVariableIndex = startingPlayerVariableIndex
                                       + m_bestResponseProblems[player]->getNumberOfVariables();
    indexx endingAllPlayerIndex = numDeclaredVariables-getNumberOfPlayer();

    vector<Constraint> newConstraints;

    // 1) build the equilibrium inequality eta_i - pi_i(y_i^*,x_{-i}) >= 0
    newConstraints.push_back(buildEquilibriumConstraintOfNEFreeSet(
        player,
        y_i_star,
        startingPlayerVariableIndex,
        endingPlayerVariableIndex,
        endingAllPlayerIndex));

    // 2: b - A_{-i}x_{-i} - A_iy_i^* >= -1 for all rows of A
    // (constraints of the best response problem of player i)
    // build remaining constraints of the NE-free set from the player's optimization problem
    for (Constraint const& constraint : m_bestResponseProblems[player]->getConstraints()) {
        newConstraints.push_back(buildPlayerConstraintOfNEFreeSet(
            constraint,
            y_i_star,
            startingPlayerVariableIndex,
            endingPlayerVariableIndex));
    }
    return newConstraints;
}

vector<double> NEPManyEtasGurobi::computeAlphaForIC(
    MatrixXd const& extremeRays,
    indexx player,
    vector<double> const& solution,
    int numNonbasicVariables,
    vector<Constraint> const &NEFreeSet,
    int verbosity) {
    // compute alpha coefs as sup (x^star,nu^star) + alpha_i*r^j \in S^+_i the NE-free set
    vector<double> alpha = {};

    for (indexx i = 0; i < numNonbasicVariables; i++) {
        // i is just a counter among all nonbasic variables giving the row of extremeRays
        // most probably small coefficients appeared because of numerical issues
        // compute for each constraint of the NE-free set the maximum value of alpha_i
        vector<double> alpha_iCandidates = {};

        // direction is a vector corresponding to the ray i of extremeRays
        Eigen::RowVectorXd direction = extremeRays.row(i);
        for (Constraint const& constraint : NEFreeSet) {
            double const toleranceDenominator = 1e-10;
            if (constraint.getXQx().size() == 0) {
                // linear constraint
                alpha_iCandidates.push_back(
                    intersectionRayHalfspace(solution,
                                               direction,
                                               constraint,
                                               toleranceDenominator,
                                               verbosity));
            }
            else {
                // quadratic constraint
                alpha_iCandidates.push_back(intersectionRayQuadraticCurve(
                                                solution,
                                                direction,
                                                constraint,
                                                toleranceDenominator,
                                                verbosity));
            }
        }
        // we still cannot replace -1 (meaning infinity) by infinity, so the next step should include that

        // replacement of old method for computing alpha_i by new method:
        double newAlpha_i = *ranges::min_element(alpha_iCandidates.begin(), alpha_iCandidates.end());
        if (newAlpha_i == MY_INF) {
            // minimum is infinity, with code -1
            newAlpha_i = -1;
        }
        alpha.push_back(newAlpha_i);


        if (verbosity >= 5) {
            cout << "alpha validated: " << newAlpha_i << endl;
        }
    }
    return alpha;
}

Cut NEPManyEtasGurobi::computeICFromAlpha(
        SetupIC const& setupIC,
        vector<double> const& alpha,
        shared_ptr<GRBModel> const& model) {
    // compute intersection cut from the vector alpha by replacing the nonbasic
    // variables x_j by basic expressions in the formula for intersection cuts:
    // sum_{j in nonbasic variable indices} x_j/alpha_j >= 1
    // ( equivalent to sum_j -x_j/alpha_j <= -1).
    // the coefficients of denseVectorValues and rhs are computed
    // for the form sum_j x_j/alpha_j >= 1
    vector<double> denseVectorValues = {};
    double rhs = 1; // rhs of the intersection cut
    // initialize denseVectorValues
    for (indexx i = 0; i < setupIC.numDeclaredVariables; i++)
        denseVectorValues.push_back(0);
    // fulfill denseVectorValues
    for (indexx i = 0; i < setupIC.numNonbasicVariables; i++) {
        // adding contribution of nonbasic variable x_j using
        // constraints in equality form (with slack variables)
        // TODO generic: if bounds are not 0, IC comes from a more general formula:
        // sum_'nonbasic at LB' (x_j - x_j^L)/alpha_j + sum_'nonbasic at UB' (x_j^U - x_j)/alpha_j
        if (alpha[i] != -1) {
            if (i < setupIC.nonbasicVariableIndices.size()) {
                // x_j is a declared variable, adding x_j / alpha_j is sufficient.
                indexx j = setupIC.nonbasicVariableIndices[i];
                denseVectorValues[j] += 1/alpha[i];
            }
            else {
                // x_j is a slack variable. Replace it via the constraint that
                // introduced x_j as a slack variable. Do not forget the constant term.
                // for now, works only for <= inequalities (and thus nonnegative slack variable)
                // TODO generic: for >= inequalities, change the sign (to check by hand first)
                indexx j = setupIC.nonbasicConstraintIndices[i-setupIC.nonbasicVariableIndices.size()];
                for (indexx variableIndex = 0; variableIndex < setupIC.numDeclaredVariables; variableIndex++)
                    denseVectorValues[variableIndex] +=
                        -model->getCoeff(setupIC.constraints[j], model->getVar(variableIndex)) / alpha[i];
                rhs += -setupIC.constraints[j].get(GRB_DoubleAttr_RHS) / alpha[i];
            }
        }
    }

    // we build step by step a sparse vector (pairs of (nonzero coefficient,index) with the linear part
    MySparseVector ICAx;
    for (indexx j = 0; j < setupIC.numDeclaredVariables; j++) {
        // denseVectorValues has coefs for the form sum_j x_j/alpha_j >= 1
        // (greater than), but we want less than so we change the sign
        ICAx.addCoefficient(-denseVectorValues[j], j);
    }

    // build Cut
    auto cut = Cut(ICAx,{},-rhs);
    return cut;
}

Cut NEPManyEtasGurobi::deriveICForOnePlayer(vector<Constraint> const &NEFreeSet,
                                            indexx player,
                                            vector<double> const &solution,
                                            shared_ptr<GRBModel> const& model,
                                            MatrixXd const& extremeRays,
                                            SetupIC const& setupIC,
                                            int verbosity) {
    // we can only derive an intersection cut if it is an optimal solution
    if (model->get(GRB_IntAttr_Status) != 2)
        throw logic_error("an intersection cut can't be derived if the node problem was not solved optimally");

    // compute vector alpha with the intersections between the NE-free set and the extreme rays
    auto timerAlpha = startChrono();
    auto alpha = computeAlphaForIC(
        extremeRays,
        player,
        solution,
        setupIC.numNonbasicVariables,
        NEFreeSet,
        verbosity);
    if (verbosity >= 3) {
        cout << "alpha computed in " << elapsedChrono(timerAlpha) << " seconds" << endl;
    }

    // compute IC from alpha and other things
    auto timerFinalPartIC = startChrono();
    Cut cut = computeICFromAlpha(setupIC, alpha, model);

    if (verbosity >= 3) {
        cout << "compute IC after alpha computed in " << elapsedChrono(timerFinalPartIC) << " seconds" << endl;
    }
    return cut;
}

void NEPManyEtasGurobi::checkICSatisfaction(
    Constraint const &cut,
    indexx player,
    vector<double> const &point,
    shared_ptr<GRBModel> const &nodeModel,
    MatrixXd const &extremeRays,
    SetupIC const &setupIC,
    vector<Constraint> const &NEFreeSet) {
    // check that cut cuts off point by a margin 'close to' 1
    // in theory it is 1, but there are sometimes numerical issues
    MySparseVector const ax = cut.getAx();
    MySparseMatrix const xQx = cut.getXQx();
    double lhsEval = 0;
    for (indexx i = 0; i < ax.size(); i++)
        lhsEval += ax.getValue(i)*point[ax.getIndex(i)];
    for (indexx i = 0; i < xQx.size(); i++)
        lhsEval += xQx.getValue(i)*point[xQx.getRowIndex(i)]*point[xQx.getColIndex(i)];
    if (lhsEval - cut.getB() < 0.5) {
        // case margin < 1, 0.82 happened
        /* cout << extremeRays << endl;
        Cut cutError = deriveICForOnePlayer(NEFreeSet,
                                            player,
                                            point,
                                            nodeModel,
                                            extremeRays,
                                            setupIC,
                                            4);
        cout << "invalid cut: " << cut << endl;
        throw logic_error("intersection cut not satisfied by a margin "
                          "of 1 as in theory, but by a strictly inferior margin: "
                          + to_string(lhsEval) + " <= " + to_string(cut.getB())
                          + " with a margin of "
                          + to_string(lhsEval - cut.getB()));*/

        // the cut will be checked again in the algorithm class
        // so here, just prints a message saying the invalidity was detected
        cout << "Warning: intersection cut not satisfied by a margin "
                  "of 1 as in theory, but by a strictly inferior margin: "
                  + to_string(lhsEval) + " <= " + to_string(cut.getB())
                  + " with a margin of "
                  + to_string(lhsEval - cut.getB()) << endl;

    }
}

std::vector<double> NEPManyEtasGurobi::getEtaUpperBounds() const {
    return m_etaUpperBounds;
}

std::vector<double> NEPManyEtasGurobi::getEtaLowerBounds() const {
    return m_etaLowerBounds;
}

std::ostream &operator<<(std::ostream &os, NEPManyEtasGurobi const& NEP) {
    NEP.print(os);
    return os;
}
