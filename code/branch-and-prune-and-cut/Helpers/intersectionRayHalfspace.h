//
// Created by alois-duguet on 10/31/25.
//

#ifndef INTERSECTIONRAYHALFSPACE_H
#define INTERSECTIONRAYHALFSPACE_H

#include <Eigen/Sparse>

#include "Constraint.h"

double intersectionRayHalfspace(
    std::vector<double> const& point,
    Eigen::RowVectorXd const& direction,
    Constraint const& halfspace,
    double tolerance = 1e-10,
    int verbosity = 0);

std::vector<double> getVector(Eigen::RowVectorXd const& v);

double linearEquationFromQuadraticConstraint(double linearTerm,
                                             double constantTerm,
                                             double tolerance);
double intersectionRayLipschitzNEFreeSetImplementationGame(
    std::vector<double> const &x,
    Eigen::RowVectorXd const &ray,
    double lipschitzConstant,
    double evalObjective,
    indexx xiIndex,
    int verbosity);

double intersectionRayQuadraticCurve(
    std::vector<double> const& point,
    Eigen::RowVectorXd const& direction,
    Constraint const& constraint,
    double tolerance = 1e-10,
    int verbosity = 0);

#endif //INTERSECTIONRAYHALFSPACE_H
