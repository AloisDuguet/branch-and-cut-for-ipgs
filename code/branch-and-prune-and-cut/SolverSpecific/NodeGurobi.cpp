//
// Created by Aloïs Duguet on 10/28/24.
//

#include "NodeGurobi.h"

#include <vector>
#include <chrono>
#include "gurobi_c++.h"
#include <cmath>

#include "BranchAndCutTools.h"

using namespace std;

NodeGurobi::NodeGurobi() {
}

NodeGurobi::NodeGurobi(shared_ptr<NodeGurobi> ptrnode, Constraint *constraint) {
    m_nodeNumber = -1;
    m_fatherNode = ptrnode;
    m_branch = *constraint;
    m_cuts = {};
    m_subproblem = make_shared<OptimizationProblem>(ptrnode->m_subproblem->getNumberOfVariables());
    m_isSolved = false;
    m_solution = {};
}

NodeGurobi::NodeGurobi(shared_ptr<NodeGurobi> ptrnode) {
    m_nodeNumber = -1;
    m_fatherNode = ptrnode;
    m_branch = {};
    m_cuts = {};
    m_subproblem = make_shared<OptimizationProblem>(ptrnode->m_subproblem->getNumberOfVariables());
    m_isSolved = false;
    m_solution = {};
}

NodeGurobi* NodeGurobi::buildChildOfRootNode() {
    // return type is not shared_ptr<NodeGurobi> because it is not related to shared_ptr<Node>
    // whereas we want to use the property that a NodeGurobi* can be cast as a Node*
    // shared_ptr<NodeGurobi> ptrnode = make_shared<NodeGurobi>(*this);
    return new NodeGurobi();
}

NodeGurobi* NodeGurobi::buildChildWithBranch(Constraint *constraint) {
    // return type is not shared_ptr<NodeGurobi> because it is not related to shared_ptr<Node>
    // whereas we want to use the property that a NodeGurobi* can be cast as a Node*
    // shared_ptr<NodeGurobi> ptrnode = make_shared<NodeGurobi>(*this);
    // return new NodeGurobi(shared_ptr<NodeGurobi>(this), constraint);
    return new NodeGurobi();
}

std::shared_ptr<Node> NodeGurobi::clone() {
    auto node = make_shared<NodeGurobi>(*this);
    node->setSubproblem(m_subproblem->clone());
    return node;
}

void NodeGurobi::printIIS(std::shared_ptr<GRBModel> const& model, vector<double> const& point) {
    cout << "IIS:" << endl;
    model->computeIIS();
    int numConstrs = model->get(GRB_IntAttr_NumConstrs);
    GRBConstr* constrs = model->getConstrs();

    for (int i = 0; i < numConstrs; i++) {
        if (constrs[i].get(GRB_IntAttr_IISConstr)) {
            // Get constraint name
            string cname = constrs[i].get(GRB_StringAttr_ConstrName);

            // Get the sense of the constraint (<=, =, >=)
            char sense = constrs[i].get(GRB_CharAttr_Sense);
            string senseStr;
            if (sense == GRB_LESS_EQUAL) senseStr = "<=";
            else if (sense == GRB_EQUAL) senseStr = "=";
            else if (sense == GRB_GREATER_EQUAL) senseStr = ">=";
            else senseStr = "?";

            // Get the RHS
            double rhs = constrs[i].get(GRB_DoubleAttr_RHS);

            // Build the LHS expression manually
            // Unfortunately, the C++ API doesn't directly expose the full expression,
            // so you need to loop over the model's constraints and use getCoeff.

            // This gets the LHS expression as a linear expression
            GRBLinExpr lhs = model->getRow(constrs[i]);

            // Build string representation of LHS
            ostringstream lhsExpr;
            for (int j = 0; j < lhs.size(); ++j) {
                double coeff = lhs.getCoeff(j);
                string varName = lhs.getVar(j).get(GRB_StringAttr_VarName);
                if (j > 0 && coeff >= 0) lhsExpr << " + ";
                else if (coeff < 0) lhsExpr << " - ";
                lhsExpr << fabs(coeff) << "*" << varName;
            }

            // Print the constraint expression
            cout << cname << ": " << lhsExpr.str() << " " << senseStr << " " << rhs << endl;

            // evaluate lhs with point
            double lhsValue = 0;
            for (int j = 0; j < lhs.size(); ++j) {
                double coeff = lhs.getCoeff(j);
                string varIndex = lhs.getVar(j).get(GRB_StringAttr_VarName);
                varIndex.erase(0,1);
                indexx varRealIndex = stoi(varIndex);
                lhsValue += coeff*point[varRealIndex];
                if (coeff != 0 and point[varRealIndex] != 0)
                    cout << varRealIndex << ": " << coeff << "*" << point[varRealIndex] << endl;
                else
                    cout << varRealIndex << ": " << coeff << "*" << point[varRealIndex] << endl;
            }
            cout << "expected solution: " << point << endl;
            cout << "lhsValue: " << lhsValue << endl;
        }
    }

    delete[] constrs;
}


shared_ptr<GRBModel> NodeGurobi::solveModel(
        shared_ptr<GRBEnv> const& globalEnv,
        double solverTimeLimit,
        int verbosity,
        bool activateNumericFocus,
        double feasibilityTol) {
    // solve a QP with gurobi corresponding to the current node subproblem of the branching algorithm
    // and populates m_isSolved and m_solution

    try {
        if (verbosity >= 4) {
            globalEnv->set(GRB_IntParam_LogToConsole, 1);
            globalEnv->set(GRB_IntParam_OutputFlag, 1);
        }
        shared_ptr<GRBModel> model = make_shared<GRBModel>(*globalEnv);
        model->set(GRB_DoubleParam_TimeLimit, solverTimeLimit);

        // in case of numerical issues, the following options could help:
        if (activateNumericFocus)
            model->set(GRB_IntParam_NumericFocus, 3);

        int numberOfVariables = m_subproblem->getNumberOfVariables();
        vector<GRBVar> variables(numberOfVariables);

        // create variables
        MySparseVector c = m_subproblem->getC();
        MySparseMatrix Q = m_subproblem->getQ();
        vector<Constraint> constraints = m_subproblem->getConstraints();
        for (indexx i = 0; i < numberOfVariables; i++) { // variables called "x" + to_string(i)
            variables[i] = model->addVar(-GRB_INFINITY, GRB_INFINITY, 0, GRB_CONTINUOUS, "x" + to_string(i));
        }

        // create constraints
        double count_nonlinear_cuts = 0;
        for (indexx i = 0; i < constraints.size(); i++) {
            GRBLinExpr linExpr = 0;
            Constraint const& con = constraints[i];
            for (indexx j = 0; j < con.getAx().size(); j++) {
                linExpr += con.getAx().getValue(j) * variables.at(con.getAx().getIndex(j));
            }
            if (con.getXQx().size() == 0)
                model->addConstr(linExpr <= con.getB(), "c" + to_string(i));
            else {
                count_nonlinear_cuts++;
                GRBQuadExpr quadExpr = 0;
                for (indexx j = 0; j < con.getXQx().size(); j++) {
                    quadExpr += con.getXQx().getValue(j) * variables.at(con.getXQx().getRowIndex(j)) * variables.at(con.getXQx().getColIndex(j));
                }
                model->addQConstr(linExpr+quadExpr <= con.getB(), "c" + to_string(i) + "Quad");
                if (verbosity >= 4)
                    cout << endl << con;
            }
        }

        // set objective
        GRBLinExpr linExpr = 0;
        for (indexx i = 0; i < c.size(); i++) {
            linExpr += c.getValue(i) * variables[c.getIndex(i)];
        }
        GRBQuadExpr quadExpr = 0;
        for (indexx i = 0; i < Q.size(); i++) {
            quadExpr += Q.getValue(i) * variables[Q.getRowIndex(i)] * variables[Q.getColIndex(i)];
        }
        model->setObjective(linExpr+quadExpr, GRB_MINIMIZE);

        if (verbosity >= 4 and count_nonlinear_cuts >= 1) {
            try {
                auto reduced_model = model->presolve();
                reduced_model.write("presolved_model.lp");
                model->write("model.lp");
                cout << "presolved model written to file presolved_model.lp" << endl;
            } catch (GRBException const& e) {
                cout << "could not write presolve because of gurobi error code " << e.getErrorCode() << endl;
            }
        }

        model->write("nodeModel.lp");

        // optimize model
        model->optimize();

        // Status checking
        int status = model->get(GRB_IntAttr_Status);

        if (status == GRB_OPTIMAL) {
            // getting solution quality information
            if (verbosity >= 4) {
                model->write("model.mps");
                printSolutionQuality(model);
            }

            // print computation time:
            if (verbosity >= 3)
                cout << "node computed in " << model->get(GRB_DoubleAttr_Runtime) << " seconds" << endl;
            // query values of the variable at the optimal solution found
            double objectiveValue = model->get(GRB_DoubleAttr_ObjVal);
            // cout << "Objective value: " << objectiveValue << endl;
            vector<double> solution(numberOfVariables);
            for (indexx i = 0; i < numberOfVariables; i++) {
                solution[i] = variables[i].get(GRB_DoubleAttr_X);
            }

            // do some integer rounding BEFORE working with those values
            // solution = integralityRounding(solution);
            // objectiveValue = integralityRounding(objectiveValue);
            // modify Node attributes
            m_isSolved = true;
            m_solution = {status, solution, objectiveValue};
            printSolveOutput(m_solution, verbosity);
        }
        else {
            if (status == 9)
                cout << "time limit reached during node resolution" << endl;
            else if (status == 12)
                cout << "status of optimization is 12: 'Optimization was terminated due to unrecoverable numerical difficulties.'" << endl;
            else if (status == 13)
                cout << "status of optimization is 13: 'Unable to satisfy optimality tolerances; a sub-optimal solution is available.'" << endl;
            else if (status != 4 and status != 3)
                cout << "status of optimization is neither OPTIMAL nor INFEASIBLE (nor UNBOUNDED) but " << status << endl;
            else if (status == 3) {
                if (verbosity >= 3)
                    cout << "node problem: status of optimization is INFEASIBLE" << endl;
                if (verbosity >= 4) {
                    // printIIS(model);
                }
            }
            else if (status == 4) {
                if (verbosity >= 4) {
                    // printIIS(model);
                }
            }
            m_solution = {status, {}, 1e100};
        }
        if (verbosity >= 4) {
            globalEnv->set(GRB_IntParam_LogToConsole, 0);
            globalEnv->set(GRB_IntParam_OutputFlag, 0);
        }
        return model;
    } catch(GRBException const& e) {
        cout << "Error code = " << e.getErrorCode() << endl;
        cout << e.getMessage() << endl;
    } catch(std::exception const& e) {
        cerr << "Error code = " << e.what() << endl;
        throw runtime_error("Exception during optimization");
    }
    return make_shared<GRBModel>(*globalEnv);
}

shared_ptr<GRBModel> NodeGurobi::checkPointFeasibility(
        vector<double> const& point,
        shared_ptr<GRBEnv> const& globalEnv,
        double solverTimeLimit,
        int verbosity,
        double feasibilityTol) {
    // solve a QP with gurobi to check if point is feasible for the node problem

    try {
        shared_ptr<GRBModel> model = make_shared<GRBModel>(*globalEnv);
        /*if (solverTimeLimit < 0) // the time can be negative because it is computed before being checked
            solverTimeLimit = 0; // the goal is just to simulate a time limit
            // return {9, {}, 1e100}; // status == 9 for time limit*/
        model->set(GRB_DoubleParam_TimeLimit, solverTimeLimit);
        model->set(GRB_DoubleParam_FeasibilityTol, feasibilityTol); // originally 1e-6 but it created cycling quite often

        // in case of numerical issues, the following options could help:
        // model->set(GRB_DoubleParam_MarkowitzTol, 0.1);
        // model->set(GRB_IntParam_NumericFocus, 3);

        int numberOfVariables = m_subproblem->getNumberOfVariables();
        vector<GRBVar> variables(numberOfVariables);

        // create variables
        MySparseVector c = m_subproblem->getC();
        MySparseMatrix Q = m_subproblem->getQ();
        vector<Constraint> constraints = m_subproblem->getConstraints();
        for (indexx i = 0; i < numberOfVariables-1; i++) {
            // variables called "x" + to_string(i)
            variables[i] = model->addVar(point[i], point[i], 0, GRB_CONTINUOUS, "x" + to_string(i));
        }
        // specific bounds for lambda the objective value
        variables[numberOfVariables-1] = model->addVar(-GRB_INFINITY, GRB_INFINITY, 0, GRB_CONTINUOUS, "x_lambda");

        // create constraints
        for (indexx i = 0; i < constraints.size(); i++) {
            GRBLinExpr linExpr = 0;
            Constraint const& con = constraints[i];
            for (indexx j = 0; j < con.getAx().size(); j++) {
                linExpr += con.getAx().getValue(j) * variables.at(con.getAx().getIndex(j));
            }
            if (con.getXQx().size() == 0)
                model->addConstr(linExpr <= con.getB(), "c" + to_string(i));
            else {
                GRBQuadExpr quadExpr = 0;
                for (indexx j = 0; j < con.getXQx().size(); j++) {
                    quadExpr += con.getXQx().getValue(j) * variables.at(con.getXQx().getRowIndex(j)) * variables.at(con.getXQx().getColIndex(j));
                }
                model->addQConstr(linExpr+quadExpr <= con.getB(), "c" + to_string(i));
            }
        }

        // set objective
        GRBLinExpr linExpr = 0;
        for (indexx i = 0; i < c.size(); i++) {
            linExpr += c.getValue(i) * variables[c.getIndex(i)];
        }
        GRBQuadExpr quadExpr = 0;
        for (indexx i = 0; i < Q.size(); i++) {
            quadExpr += Q.getValue(i) * variables[Q.getRowIndex(i)] * variables[Q.getColIndex(i)];
        }
        model->setObjective(linExpr+quadExpr, GRB_MINIMIZE);

        // model->write("model.mps");

        // optimize model
        model->optimize();

        // Status checking
        int status = model->get(GRB_IntAttr_Status);

        if (status == GRB_OPTIMAL) {
            cout << "point feasible for gurobi model of value " << model->get(GRB_DoubleAttr_ObjVal) << endl;
        }
        else {
            cout << "point not feasible for gurobi model" << endl;
            if (status == 3) {
                if (verbosity >= 3)
                    cout << "node problem: status of optimization is INFEASIBLE" << endl;
                if (verbosity >= 4) {
                    printIIS(model, point);
                }
            }
            else if (status == 4) {
                if (verbosity >= 4) {
                    printIIS(model, point);
                }
            }
            m_solution = {status, {}, 1e100};
        }
        return model;
    } catch(GRBException const& e) {
        cout << "Error code = " << e.getErrorCode() << endl;
        cout << e.getMessage() << endl;
    } catch(std::exception const& e) {
        cerr << "Error code = " << e.what() << endl;
        throw runtime_error("Exception during optimization");
    }
    return make_shared<GRBModel>(*globalEnv);
}

void printSolutionQuality(shared_ptr<GRBModel> const& model) {
    model->update();

    int numConstrs = model->get(GRB_IntAttr_NumConstrs);
    GRBConstr* constrs = model->getConstrs();
    double maxSlack = 0.0;
    std::string worstConstr;

    for (int i = 0; i < numConstrs; ++i) {
        double slack = constrs[i].get(GRB_DoubleAttr_Slack);
        if (std::fabs(slack) > std::fabs(maxSlack)) {
            maxSlack = slack;
            worstConstr = constrs[i].get(GRB_StringAttr_ConstrName);
        }
    }
    std::cout << "Max constraint slack: " << maxSlack
              << " in constraint: " << worstConstr << "\n";

    int numVars = model->get(GRB_IntAttr_NumVars);
    GRBVar* vars = model->getVars();
    double maxIntViol = 0.0;
    std::string worstVar;

    for (int i = 0; i < numVars; ++i) {
        double val = vars[i].get(GRB_DoubleAttr_X);
        if (vars[i].get(GRB_CharAttr_VType) != GRB_CONTINUOUS) {
            double viol = std::fabs(val - std::round(val));
            if (viol > maxIntViol) {
                maxIntViol = viol;
                worstVar = vars[i].get(GRB_StringAttr_VarName);
            }
        }
    }
    std::cout << "Max integrality violation: " << maxIntViol
              << " in variable: " << worstVar << "\n";
    delete[] vars;
    delete[] constrs;
}