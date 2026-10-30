//
// Created by Aloïs Duguet on 7/8/25.
//

#ifndef EXTREMERAYSWITHGUROBI_H
#define EXTREMERAYSWITHGUROBI_H

#include <vector>
#include <memory>
#include <Eigen/LU>

#include "BranchAndCutTools.h"

// build extreme rays for intersection cut using Gurobi C model
// (with a change in gurobi_c++.h to make so that a gurobi C function is accessible)
[[nodiscard]] MatrixXd NashEquilibriumProblemManyEtasGurobi::buildExtremeRaysWithGurobi(std::shared_ptr<GRBModel> const& model) const;

#endif //EXTREMERAYSWITHGUROBI_H
