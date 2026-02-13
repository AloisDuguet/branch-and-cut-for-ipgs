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

double intersectionRayQuadraticCurve(
    std::vector<double> const& point,
    Eigen::RowVectorXd const& direction,
    Constraint const& constraint,
    double tolerance = 1e-10,
    int verbosity = 0);

#endif //INTERSECTIONRAYHALFSPACE_H
