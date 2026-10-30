//
// Created by Aloïs Duguet on 11/28/24.
//

#ifndef CONSTRAINT_H
#define CONSTRAINT_H

#include <iostream>

#include "MySparseMatrix.h"
#include "MySparseVector.h"

class Constraint {
public:
    Constraint();

    Constraint(MySparseVector const& v, MySparseMatrix const& M, double rhs);

    void print(std::ostream& os = std::cout, bool endLine = false) const;

    [[nodiscard]] MySparseVector getAx() const;

    [[nodiscard]] MySparseMatrix getXQx() const;

    [[nodiscard]] double getB() const;

    void increaseB(double value);

    // return true if cut1 == cut2
    [[nodiscard]] bool areCutsEqual(Constraint const& cut) const;

    // computes the euclidean norm of coefficients between cut1 and cut2
    [[nodiscard]] double distanceMeasure(Constraint const& cut) const;

    // return true if point does not satisfy constraint *this
    [[nodiscard]] bool checkConstraintWithPoint(std::vector<double> const& point,
                                                int verbosity,
                                                double const& tolerance = 5e-6) const;

    // return false if point is not feasible for constraints
    bool checkPointFeasibility(std::vector<double> const &point,
                               std::vector<Constraint> const &constraints,
                               bool verboseIfFeasible) const;

    // evaluate lhs of constraint
    [[nodiscard]] double evalLhs(std::vector<double> const& point) const;

    // check that a constraint is an branching constraint with integer bound
    [[nodiscard]] bool isIntegerBranchingConstraint() const;

    // check if a constraint gives an upper bound
    [[nodiscard]] bool isUpperBoundProof(
        std::vector<bool> const& areVariablesNonnegativ) const;

    // return a list of constraints being upper bounds written as branching constraints
    // from a constraint proving upper bounds
    [[nodiscard]] std::vector<Constraint> extractUpperBoundConstraints() const;

protected:
    // constraint ax + xQx <= b
    // Example: Constraint(SparseVector({1.0},{1}),{},1) for x1 <= 1

    // linear coefficients of lhs as nonzero coefficients
    MySparseVector m_ax;

    // quadratic coefficients of lhs as nonzero coefficients
    MySparseMatrix m_xQx;

    // rhs
    double m_b;
};

typedef Constraint Cut;

std::ostream& operator<<(std::ostream& os, Constraint const& constraint);
bool operator==(Constraint const& a, Constraint const& b);

#endif //CONSTRAINT_H
