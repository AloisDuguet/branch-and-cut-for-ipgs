//
// Created by Aloïs Duguet on 7/8/25.
//

#include "ExtremeRaysWithGurobi.h"

#include <Eigen/Sparse>

#include "BranchAndCutTools.h"

using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXd;

MatrixXd NashEquilibriumProblemManyEtasGurobi::buildExtremeRaysWithGurobi(shared_ptr<GRBModel> const& model) const {
    if (model->get(GRB_IntAttr_Status) != 2) {
        // not interested in LP basis if not an optimal solution
        throw logic_error("an intersection cut can't be derived "
                          "if the node problem was not solved optimally");
    }

    // declare useful objects to build a LinearBasis
    int const numDeclaredVariables = model->get(GRB_IntAttr_NumVars);
    int const numConstraints = model->get(GRB_IntAttr_NumConstrs);
    // the four following vectors allow to remember which column of
    // later matrices are associated with which coefficients
    // only indices from variables declared in gurobi model
    vector<indexx> basicVariableIndices = {};
    vector<indexx> nonbasicVariableIndices = {};
    // only indices from constraints declared in gurobi model
    vector<indexx> basicConstraintIndices = {};
    vector<indexx> nonbasicConstraintIndices = {};

    // get basic variables
    for (indexx variableIndex = 0; variableIndex < numDeclaredVariables; variableIndex++) {
        if (model->getVar(variableIndex).get(GRB_IntAttr_VBasis) == 0) // basic variable
            basicVariableIndices.push_back(variableIndex);
        else {
            nonbasicVariableIndices.push_back(variableIndex);
            // cout << "nonbasic declared variable of VBasis " << model->getVar(variableIndex).get(GRB_IntAttr_VBasis) << endl;
            throw runtime_error("nonbasic declared variable found");
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
            // cout << "non basic slack variable " << constraints[constraintIndex].get(GRB_IntAttr_CBasis) << endl;
            if (constraints[constraintIndex].get(GRB_IntAttr_CBasis) == -2)
                throw runtime_error("nonbasic slack variable at upper bound found");
        }
        if (constraints[constraintIndex].get(GRB_CharAttr_Sense) != '=')
            numInequalityConstraints++;
        else {
            // I am not sure on how to handle equality constraints
            // when building the LP basis so I let an exception here if it happens
            throw logic_error("ERROR(?): there is at least an equality "
                              "constraint at (gurobi) index " + to_string(constraintIndex));
        }
    }

    // number of variables in the LP basis
    int numBasicVariables = static_cast<int>(basicVariableIndices.size()) + static_cast<int>(basicConstraintIndices.size());

    // number vars + number inequality constraints among basic constraints (for slack variables) - number of basic (declared) variables
    int numNonbasicVariables = numDeclaredVariables + numInequalityConstraints - numBasicVariables;

    // declare now the rebased matrix
    Eigen::SparseMatrix<double> rebasedMatrixNonbasicVar(numBasicVariables,numNonbasicVariables);
    // elements with rebasedMatrix[perm[i],indexNonbasic] = Colj[i] for indexNonbasic in the list of nonbasic variables
    MatrixXd rebasedMatrixGurobi(numBasicVariables,numNonbasicVariables);

    // build extreme rays from gurobi functions (without external LU solve): rebuild RebasedMatrixNonbasicVariables.
    // 1 - using GRBBinvColj(model, j, x), get the permutation perm of constraints to get the identity matrix
    // to do this, if j corresponds to a basic var (be it slack or declared), then there is only one nonzero coef
    // in (say) index k. Then row j as given by GRBBinvColj should go to row k in rebasedMatrix (done in the next step)
    vector<int> permutation(numConstraints);
    // for basic variables declared (cf basicVariableIndices)
    for (indexx i = 0; i < basicVariableIndices.size(); i++) {
        indexx j = basicVariableIndices[i];
        GRBsvec* x = new GRBsvec();
        x->ind = new int [numDeclaredVariables+numConstraints];
        x->val = new double [numDeclaredVariables+numConstraints];
        // build extreme rays for intersection cut using Gurobi C model
        // with a change in gurobi_c++.h to make so that model->Cmodel is accessible (private changed to public line 523)
        int grb_exit = GRBBinvColj(model->Cmodel, j, x);
        removeCloseToZero(x);
        if (grb_exit != 0)
            throw runtime_error("GRBBinvColj failed with error code " + to_string(grb_exit));
        if (x->len != 1) {
            for (indexx index = 0; index < x->len; index++) cerr << x->val[index] << " ";
            cerr << endl;
            throw logic_error("GRBBinvColj for basic declared variable "
                + to_string(j) + " contains "
                + to_string(x->len) + " nonzero coefficients instead of 1");
        }
        if (!compareApproxEqual(x->val[0], 1.0))
            throw logic_error("GRBBinvColj for basic declared variable "
                + to_string(j) + " contains value "
                + to_string(x->val[0]) + " instead of 1");
        // permutation[i] = x->ind[0]; // we want the inverse of that
        permutation[x->ind[0]] = i;
        delete x->ind;
        delete x->val;
        delete x;
    }
    // same for basic slack variables (cf basicConstraintIndices)
    for (indexx i = 0; i < basicConstraintIndices.size(); i++) {
        indexx j = basicConstraintIndices[i] + numDeclaredVariables;
        GRBsvec* x = new GRBsvec();
        x->ind = new int [numDeclaredVariables+numConstraints];
        x->val = new double [numDeclaredVariables+numConstraints];
        int grb_exit = GRBBinvColj(model->Cmodel, j, x);
        removeCloseToZero(x);
        if (grb_exit != 0)
            throw runtime_error("GRBBinvColj failed with error code " + to_string(grb_exit));
        if (x->len != 1) {
            for (indexx index = 0; index < x->len; index++) cerr << x->val[index] << " ";
            cerr << endl;
            throw logic_error("GRBBinvColj for basic slack variable "
                + to_string(j) + " contains "
                + to_string(x->len) + " nonzero coefficients instead of 1");
        }
        if (!compareApproxEqual(x->val[0], 1.0))
            throw logic_error("GRBBinvColj for basic slack variable "
                + to_string(j) + " contains value "
                + to_string(x->val[0]) + " instead of 1");
        // permutation[i+basicVariableIndices.size()] = x->ind[0]; // we want the inverse of that
        permutation[x->ind[0]] = i+basicVariableIndices.size();
        delete x->ind;
        delete x->val;
        delete x;
    }
    // cout << "permutation: " << permutation << endl;

    // 2 - fulfill rebasedMatrix for each nonbasic variable (be it declared or slack) by permuting its nonzero
    for (indexx rowIndex = 0; rowIndex < numBasicVariables; rowIndex++)
        for (indexx colIndex = 0; colIndex < numNonbasicVariables; colIndex++)
            rebasedMatrixGurobi(rowIndex, colIndex) = 0;
    // nonbasic declared variables (nonbasicVariableIndices)
    for (indexx i = 0; i < nonbasicVariableIndices.size(); i++) {
        indexx nonbasicIndex = nonbasicVariableIndices[i];
        GRBsvec* x = new GRBsvec();
        x->ind = new int [numDeclaredVariables+numConstraints];
        x->val = new double [numDeclaredVariables+numConstraints];
        int grb_exit = GRBBinvColj(model->Cmodel, nonbasicIndex, x);
        if (grb_exit != 0)
            throw runtime_error("GRBBinvColj failed with error code " + to_string(grb_exit));
        for (indexx j = 0; j < x->len; j++) {
            // cout << "with declared nonbasic: rebased(" << permutation[x->ind[j]]
            // << "," << i << ") = " << x->val[j] << endl;
            rebasedMatrixGurobi(permutation[x->ind[j]],i) = x->val[j];
        }
        delete x->ind;
        delete x->val;
        delete x;
    }
    // same for nonbasic slack variables (nonbasicConstraintIndices)
    for (indexx i = 0; i < nonbasicConstraintIndices.size(); i++) {
        indexx nonbasicIndex = nonbasicConstraintIndices[i] + numDeclaredVariables;
        GRBsvec* x = new GRBsvec();
        x->ind = new int [numDeclaredVariables+numConstraints];
        x->val = new double [numDeclaredVariables+numConstraints];
        int grb_exit = GRBBinvColj(model->Cmodel, nonbasicIndex, x);
        if (grb_exit != 0)
            throw runtime_error("GRBBinvColj failed with error code " + to_string(grb_exit));
        for (indexx j = 0; j < x->len; j++) {
            // cout << "with slack nonbasic: rebased(" << permutation[x->ind[j]]
            // << "," << i+nonbasicVariableIndices.size()
            // << ") = " << x->val[j] << endl;
            rebasedMatrixGurobi(permutation[x->ind[j]],i+nonbasicVariableIndices.size()) = x->val[j];
        }
        delete x->ind;
        delete x->val;
        delete x;
    }

    // computation of extreme rays
    // extremeRays is the matrix containing the extreme rays.
    // row i represents extreme ray associated with nonbasic variable nonbasicVariableIndices[i].
    // columns are ordered first with explicitely declared variables,
    // then slack variables in the order of the constraints they are associated to
    MatrixXd extremeRays(numNonbasicVariables,numDeclaredVariables+numBasicVariables);
    // copy all columns of -rebasedMatrixNonbasicVar to the right columns of extremeRays
    // TODO generic: to support bounds on variables, in case of 'nonbasic at upper bound',
    // change the sign of the whole extreme ray (part with a_ij and coef 1 for the nonbasic variable)
    for (indexx rowIndex = 0; rowIndex < numNonbasicVariables; rowIndex++) {
        for (indexx colIndex = 0; colIndex < basicVariableIndices.size(); colIndex++) {
            extremeRays(rowIndex,basicVariableIndices[colIndex])
                = -rebasedMatrixGurobi(colIndex,rowIndex);
        }
        for (indexx colIndex = 0; colIndex < basicConstraintIndices.size(); colIndex++) {
            extremeRays(rowIndex,numDeclaredVariables + basicConstraintIndices[colIndex])
                = -rebasedMatrixGurobi(static_cast<int>(basicVariableIndices.size()) + colIndex,rowIndex);
        }
    }
    // add coefficients 1 corresponding to the indices of nonbasic variables and 0 to remaining indices
    int numNonbasicDeclaredVariables = nonbasicVariableIndices.size();
    for (indexx rayIndex = 0; rayIndex < numNonbasicVariables; rayIndex++) {
        if (rayIndex < numNonbasicDeclaredVariables) {
            // initialize column
            for (indexx rowIndex = 0; rowIndex < numNonbasicVariables; rowIndex++)
                extremeRays(rowIndex,nonbasicVariableIndices[rayIndex]) = 0;
            // add coefficient 1
            extremeRays(rayIndex,nonbasicVariableIndices[rayIndex]) = 1;
        }
        else {
            // initialize column
            for (indexx rowIndex = 0; rowIndex < numNonbasicVariables; rowIndex++)
                extremeRays(rowIndex,numDeclaredVariables +
                    nonbasicConstraintIndices[rayIndex-numNonbasicDeclaredVariables]) = 0;
            // add coefficient 1
            extremeRays(rayIndex,numDeclaredVariables +
                nonbasicConstraintIndices[rayIndex-numNonbasicDeclaredVariables]) = 1;
        }
    }

    // put 0 instead of all values of magnitude less than tolerance in extremeRays in the hope of limiting the numerical issues
    double tolerance = 1e-13;
    for (indexx rowIndex = 0; rowIndex < numNonbasicVariables; rowIndex++) {
        for (indexx colIndex = 0; colIndex < numDeclaredVariables+numBasicVariables; colIndex++) {
            if (isApproxZero(extremeRays(rowIndex,colIndex), tolerance))
                extremeRays(rowIndex,colIndex) = 0;
        }
    }

    return extremeRays;
}
