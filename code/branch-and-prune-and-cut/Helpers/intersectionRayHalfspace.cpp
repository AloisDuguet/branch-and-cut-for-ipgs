//
// Created by alois-duguet on 10/31/25.
//

#include "intersectionRayHalfspace.h"

#include <vector>
#include <memory>
#include <numeric>
#include <Eigen/Sparse>

#include "BranchAndCutTools.h"

using namespace std;

// compute the positive scalar alpha solution to supremum for alpha >= 0 of
// point+alpha*ray is in halfspace
// the way it works is by computing the following:
// compute the scalar alpha such that the constraint halfspace (ax <= b)
// evaluated at point+alpha*ray (the ray) has no slack
// ie, a(point+alpha*ray) == b
// special case: if alpha < 0, it means alpha == infinity because
// for alpha == 0 the linear constraint is satisfied => the constraint
// is valid for all nonnegative alpha
double intersectionRayHalfspace(
        vector<double> const& point,
        Eigen::RowVectorXd const& ray,
        Constraint const& halfspace,
        double tolerance,
        int verbosity) {
    MySparseVector ax = halfspace.getAx();
    double numerator = halfspace.getB() - ax * point;
    double denominator = 0;
    for (indexx j = 0; j < ax.size(); j++) {
        denominator += ray(ax.getIndex(j)) * ax.getValue(j);
    }

    if (verbosity >= 5) {
        cout << "method numerator: " << numerator << " / denominator: "
             << denominator << endl;
    }

    if (verbosity >= 5) {
        if (denominator != 0) {
            if (isApproxZero(numerator, 1e-4)) {
                cout << "numerator close to zero: " << numerator
                     << " / denominator: " << denominator << endl;
            }
            if (numerator/denominator > 0
                and isApproxZero(numerator/denominator, 1e-4)) {
                for (indexx j = 0; j < ax.size(); j++) {
                    cout << "ray[" << ax.getIndex(j) << "] = " << ray(ax.getIndex(j))
                    << " and ax[] = " << ax.getValue(j) << endl;
                }
                cout << "fraction close to zero - method numerator: " << numerator << " / denominator: "
                     << denominator << endl;
            }
        }
    }

    if (isApproxZero(denominator,tolerance)) {
        return MY_INF;
    }
    if (numerator / denominator < 0.0) {
        // if the fraction is negative, it means that it is always valid for nonnegative alpha values
        // => alpha_i candidate = infinity
        return MY_INF;
    }
    return numerator / denominator;
}

vector<double> getVector(Eigen::RowVectorXd const& v) {
    vector<double> res{};
    for (indexx i = 0; i < v.size(); i++)
        res.push_back(v(i));
    return res;
}

// intersection of a ray and the curve of a convex quadratic constraint == 0
// by assumption (convexity of the quadratic constraint + ray for alpha == 0 is negative):
// there always exists a (unique) solution
// computation of this solution with the determinant of the quadratic function induced
double intersectionRayQuadraticCurve(
        vector<double> const &x,
        Eigen::RowVectorXd const &ray,
        Constraint const &constraint,
        double tolerance,
        int verbosity) {
    double b = constraint.getB();
    MySparseVector ax = constraint.getAx();
    MySparseMatrix Q = constraint.getXQx();
    vector<double> r = getVector(ray);

    // compute quadratic function in alpha induced by constraint == 0 (A*alpha^2+B*alpha+C == 0)
    // p := x+alpha*r
    // ap + pQp^T == b implies
    // A := rQr^T
    // B := a*r + rQx^T + xQr^T
    // C := ax + xQx - b
    double A,B,C = 0;
    for (indexx j = 0; j < Q.size(); j++) {
        A += ray(Q.getRowIndex(j)) * Q.getValue(j) * ray(Q.getColIndex(j));
        B += ray(Q.getRowIndex(j)) * Q.getValue(j) * x[Q.getColIndex(j)];
        B += x[Q.getRowIndex(j)] * Q.getValue(j) * ray(Q.getColIndex(j));
        C += x[Q.getRowIndex(j)] * Q.getValue(j) * x[Q.getColIndex(j)];
    }
    for (indexx j = 0; j < ax.size(); j++) {
        B += ax.getValue(j) * ray(ax.getIndex(j));
    }

    C += ax*x - b;

    // compute determinant
    double determinant = B*B - 4*A*C;
    if (isApproxZero(determinant))
        determinant = 0;
    if (determinant < 0)
        throw logic_error("determinant is negative when by assumption it should be positive: " + to_string(determinant));

    if (compareApproxEqual(A, 0, 1e-12)) {
        // it is a linear and not quadratic function
        double temp = intersectionRayHalfspace(x, ray, constraint, tolerance, verbosity);
        return temp;
    }

    // select positive root (determinant is positive by assumption)
    double alpha = (-B + sqrt(determinant))/(2*A);
    if (alpha <= 0)
        throw logic_error("this root should be positive");

    // check that the constraint is satisfied to equality with alpha
    vector<double> X = {};
    for (indexx j = 0; j < x.size(); j++)
        X.push_back(x[j] + alpha*ray(j));
    double lhs = constraint.evalLhs(X);
    if (!compareApproxEqual(lhs, constraint.getB())) {
        alpha = intersectionRayHalfspace(x, ray, constraint, tolerance, verbosity);
        return alpha;
    }

    return alpha;
}
