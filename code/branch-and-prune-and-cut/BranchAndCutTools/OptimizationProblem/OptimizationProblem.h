//
// Created by Aloïs Duguet on 10/22/24.
//

#ifndef OPTIMIZATIONPROBLEM_H
#define OPTIMIZATIONPROBLEM_H

#include <vector>
#include <iostream>

#include "CommonlyUsedFunctions.h"
#include "MySparseVector.h"
#include "MySparseMatrix.h"
#include "Constraint.h"

class OptimizationProblem {
public:
    // constructor for children nodes
    explicit OptimizationProblem(int numberOfVariables);

    // build m_model with vectors and matrices in arguments
    OptimizationProblem(
        int numberOfVariables,
        MySparseVector const& c,
        MySparseMatrix const& Q,
        std::vector<Constraint> const& constraints,
        std::vector<indexx> const& integralityConstraints);

    [[nodiscard]] int getNumberOfVariables() const;

    void print(std::ostream& os = std::cout) const;

    MySparseVector getC();

    MySparseMatrix getQ();

    std::vector<Constraint> getConstraints();

    std::vector<indexx> getIntegralityConstraints();

    void setC(MySparseVector const& c);

    void setQ(MySparseMatrix const& Q);

    void setConstraints(std::vector<Constraint> const& c);

    void setIntegralityConstraints(std::vector<indexx> const& integralityConstraints);

    void AddConstraint(Constraint const &constraint);

    // return bool with feasibility of point
    [[nodiscard]] bool checkPointFeasibility(std::vector<double> const& point) const;

    // return objective value of point
    [[nodiscard]] double evalObjective(std::vector<double> const& point) const;

    // return a shared_ptr to a copy of this
    [[nodiscard]] std::shared_ptr<OptimizationProblem> clone() const;

    // increase rhs of the last numberOfCuts constraints with rhsIncrease
    void updateRhsCuts(double rhsIncrease, int numberOfCuts);

    // remove nonlinear cuts that are constraints at the end of the constraint list
    int removeNonlinearCutsAsConstraints(int numberOfCuts);

    // print all constraints of the model together with the evaluation of the lhs at point
    void printConstraintsAndLhsEval(std::vector<double> const& point) const;

protected:
    // count the number of variables of the model
    int m_numberOfVariables;

    // objective cx + x^TQx
    MySparseVector m_c;

    // matrix for quadratic terms in the objective
    MySparseMatrix m_Q;

    // Ax <= b represented as list of Constraint objects
    std::vector<Constraint> m_constraints;

    // contains the indices of integer variables
    std::vector<indexx> m_integralityConstraints;
};

std::ostream& operator<<(std::ostream& os, OptimizationProblem const& problem);

#endif //OPTIMIZATIONPROBLEM_H
