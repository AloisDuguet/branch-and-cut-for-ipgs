//
// Created by alois-duguet on 10/31/25.
//

#include "intersectionRayHalfspace.h"

#include <vector>
#include <memory>
#include <numeric>
#include <Eigen/Sparse>

#include "BranchAndCutTools.h"
#include "NEPMultiplicativeApprox.h"

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
    // works only if halfspace is a linear constraint
    if (halfspace.getXQx().size() > 0)
        throw logic_error("intersectionRayHalfspace works only with linear constraints");

    MySparseVector ax = halfspace.getAx();
    double numerator = halfspace.getB() - ax * point;
    double denominator = 0;
    for (indexx j = 0; j < ax.size(); j++) {
        denominator += ray(ax.getIndex(j)) * ax.getValue(j);
    }

    if (verbosity >= 5) {
        cout << "method numerator: " << numerator << " / denominator: "
             << denominator << endl;
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

double linearEquationFromQuadraticConstraint(double const linearTerm,
                                             double const constantTerm,
                                             double const tolerance) {
    // three cases:
    // 1) no linear term -> solution = inf
    // 2) negative solution -constantTerm/linearTerm -> solution = inf because we look for nonnegative solutions
    // 3) nonnegative solution -constantTerm/linearTerm -> keep this solution
    if (isApproxZero(linearTerm, tolerance))
        return MY_INF;
    else if (-constantTerm / linearTerm < 0)
        return MY_INF;
    else
        return - constantTerm / linearTerm;
}

double intersectionRayLipschitzNEFreeSetImplementationGame(
    vector<double> const &x,
    Eigen::RowVectorXd const &ray,
    double lipschitzConstant,
    double evalObjective,
    indexx xiIndex,
    int verbosity) {

    double normRayX = 0;
    for (indexx i = 0; i < x.size(); i++) {
        normRayX += ray[i]*ray[i];
    }
    normRayX = sqrt(normRayX);
    double alpha = (evalObjective - x[xiIndex]) / (lipschitzConstant*normRayX + ray[xiIndex]);

    // check that there is equality
    double lhs = alpha*normRayX;
    double rhs = (evalObjective - (x[xiIndex] + alpha*ray[xiIndex])) / lipschitzConstant;
    if (!compareApproxEqual(lhs, rhs))
        throw logic_error("intersection badly computed: " + to_string(lhs) + " != " + to_string(rhs));

    return alpha;
}

// intersection of a ray and the curve of a convex quadratic constraint == 0
// by assumption (convexity of the quadratic constraint + ray for alpha == 0 is negative):
// there always exists a (unique) solution
// computation of this solution with the discriminant of the quadratic function
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
    double A = 0, B = 0, C = 0;
    for (indexx j = 0; j < Q.size(); j++) {
        A += ray(Q.getRowIndex(j)) * Q.getValue(j) * ray(Q.getColIndex(j));
        B += ray(Q.getRowIndex(j)) * Q.getValue(j) * x[Q.getColIndex(j)];
        B += x[Q.getRowIndex(j)] * Q.getValue(j) * ray(Q.getColIndex(j));
        C += x[Q.getRowIndex(j)] * Q.getValue(j) * x[Q.getColIndex(j)];
    }
    for (indexx j = 0; j < ax.size(); j++) {
        B += ax.getValue(j) * ray(ax.getIndex(j));
    }

    C += ax * x - b;

    // handling numerical issues on A
    // 1) A could be negative, then it is not a convex quadratic function anymore
    if (A < 0) {
        if (isApproxZero(A, tolerance)) {
            // if it is close to 0, do as if A == 0
            return linearEquationFromQuadraticConstraint(B, C, tolerance);
        } else {
            // if not, throw an error due to numerical issues
            throw runtime_error("numerical issue in intersection of ray and "
                                "quadratic curve: the curve is not convex, the "
                                "quadratic coefficient is " + to_string(A));
        }
    }
    // 2) A could be nonnegative but close to zero, then the resolution of the second
    // degree polynomial is very sensitive to numerical error on A
    if ( A >= 0 and compareApproxEqual(A, 0, tolerance)) {
        // do as if it were a linear function and not a quadratic one
        return linearEquationFromQuadraticConstraint(B, C, tolerance);
    }

    // compute discriminant
    double discriminant = B*B - 4*A*C;
    if (isApproxZero(discriminant))
        discriminant = 0;
    if (discriminant < 0) {
        cerr << "error in intersection of ray with quadratic curve" << endl;
        cerr << "computing negative discriminant of quadratic polynomial A*alpha^2+B*alpha+C == 0 with" << endl;
        cerr << "A = " << A << endl;
        cerr << "B = " << B << endl;
        cerr << "C = " << C << endl;
        cerr << "if A is too close to 0, the numerical error may have flipped its sign, hence the error" << endl;
        cerr << "adjusting the threshold for A might be the thing to do" << endl;
        throw logic_error("discriminant is negative when by assumption it should be positive: " + to_string(discriminant));
    }

    // select positive root (discriminant is positive by assumption)
    double alpha = (-B + sqrt(discriminant))/(2*A);
    if (alpha <= 0) {
        cout << "(warning) in intersection of ray with quadratic curve" << endl;
        cout << "computing negative root (" << alpha;
        cout << ") of quadratic polynomial A*alpha^2+B*alpha+C == 0 with" << endl;
        cout << "A = " << A << endl;
        cout << "B = " << B << endl;
        cout << "C = " << C << endl;
        cout << "if A is too close to 0, the numerical error may have flipped its sign, hence the error" << endl;
        cout << "adjusting the threshold for A might be the thing to do" << endl;
        cout << "it could also be due to the tolerance of deriving eta-cut being too small" << endl;
        if (isApproxZero(alpha)) {
            alpha = linearEquationFromQuadraticConstraint(B, C, tolerance);
            return alpha;
        } else {
            throw logic_error("this root should be positive: " + to_string(alpha));
        }
    }

    // check that the constraint is satisfied to equality with alpha
    vector<double> X = {};
    for (indexx j = 0; j < x.size(); j++)
        X.push_back(x[j] + alpha*ray(j));
    double lhs = constraint.evalLhs(X);
    double relativeTolerance = tolerance;
    if (abs(constraint.getB()) > 1)
        relativeTolerance *= abs(constraint.getB());
    if (!compareApproxEqual(lhs, constraint.getB(), relativeTolerance)) {
        cerr << "warning: lhs and rhs do not correspond in intersection of ray and quadratic curve:" << endl;
        cerr << "lhs = " << format("{:.10g}", lhs) << " and rhs = "
             << format("{:.10g}", constraint.getB()) << endl;
        cerr << "discriminant = " << discriminant << ", alpha = " << alpha << endl;
        // A . (x+ray*alpha) = lhs
        cerr << "ray = " << r << endl;
        cerr << "A = " << A << endl;
        cerr << "B = " << B << endl;
        cerr << "C = " << C << endl;
        // throw logic_error("It should not happen, investigate this case.");
    }

    return alpha;
}
