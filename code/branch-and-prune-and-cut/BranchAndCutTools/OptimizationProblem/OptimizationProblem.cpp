//
// Created by Aloïs Duguet on 10/22/24.
//

#include "OptimizationProblem.h"

#include <vector>

#include "MySparseVector.h"
#include "MySparseMatrix.h"
#include "CommonlyUsedFunctions.h"
#include "Constraint.h"

using namespace std;

OptimizationProblem::OptimizationProblem(int const numberOfVariables) {
    m_numberOfVariables = numberOfVariables;
    m_c = {{}, {}};
    m_Q = {{}, {}, {}};
    m_constraints = {};
    m_integralityConstraints = {};
}

OptimizationProblem::OptimizationProblem(
            int numberOfVariables,
            MySparseVector const& c,
            MySparseMatrix const& Q,
            vector<Constraint> const& constraints,
            vector<indexx> const& integralityConstraints) {
    m_numberOfVariables = numberOfVariables;
    m_c = c;
    m_Q = Q;
    m_constraints = constraints;
    m_integralityConstraints = integralityConstraints;
}

void OptimizationProblem::print(ostream& os) const {
    if (m_numberOfVariables <= 1)
        os << "Optimization problem with " << m_numberOfVariables
           << " variable" << endl;
    else
        os << "Optimization problem with " << m_numberOfVariables
           << " variables" << endl;
    os << endl << "min \t";
    os << m_c;
    if (m_Q.size() > 0 and m_c.size() > 0)
        os << " + ";
    os << m_Q;
    os << endl << "s.t.\t";
    for (indexx i = 0; i < m_constraints.size(); i++) {
        os << m_constraints[i];
        os << endl << "    \t";
    }
    os << endl;
    os << "integer variables: ";
    for (indexx const& integerVariable : m_integralityConstraints)
        os << integerVariable << " ";
    os << endl;
}


int OptimizationProblem::getNumberOfVariables() const {
    return m_numberOfVariables;
}


MySparseVector OptimizationProblem::getC() {
    return m_c;
}

MySparseMatrix OptimizationProblem::getQ() {
    return m_Q;
}

vector<Constraint> OptimizationProblem::getConstraints() {
    return m_constraints;
}

vector<indexx> OptimizationProblem::getIntegralityConstraints() {
    return m_integralityConstraints;
}

void OptimizationProblem::setC(MySparseVector const& c) {
    m_c = c;
}

void OptimizationProblem::setQ(MySparseMatrix const& Q) {
    m_Q = Q;
}

void OptimizationProblem::setConstraints(vector<Constraint> const& constraints) {
    m_constraints = constraints;
}

void OptimizationProblem::setIntegralityConstraints(
        vector<indexx> const& integralityConstraints) {
    m_integralityConstraints = integralityConstraints;
}

void OptimizationProblem::AddConstraint(Constraint const &constraint) {
    m_constraints.push_back(constraint);
}

bool OptimizationProblem::checkPointFeasibility(vector<double> const &point) const {
    bool test = true;
    for (auto const& constraint : m_constraints) {
        if (constraint.checkConstraintWithPoint(point, 1)) {
            // relaunch test just for printing numerical values
            test = !constraint.checkConstraintWithPoint(point, 2);
            cout << "point is not feasible for node problem because of constraint "
                 << constraint << endl;
            break;
        }
    }
    if (test)
        cout << "point is feasible for node problem, of value "
             << evalObjective(point) << endl;
    return test;
}

double OptimizationProblem::evalObjective(vector<double> const &point) const {
    return m_c * point + point * m_Q * point;
}

std::shared_ptr<OptimizationProblem> OptimizationProblem::clone() const {
    return make_shared<OptimizationProblem>(*this);
}

void OptimizationProblem::updateRhsCuts(double rhsIncrease, int numberOfCuts) {
    // auto const& first = m_constraints.end()-numberOfCuts;
    // auto const& last = m_constraints.end();
    // for (vector<Constraint>& cutsToChange(first,last); auto& cut : cutsToChange)
    // cut.increaseB(rhsIncrease);
    int constraintSize = static_cast<int>(m_constraints.size());
    for (indexx i = 0; i < numberOfCuts; i++) {
        m_constraints[constraintSize - numberOfCuts + i].increaseB(rhsIncrease);
    }
}

int OptimizationProblem::removeNonlinearCutsAsConstraints(int numberOfCuts) {
    int constraintSize = static_cast<int>(m_constraints.size());
    int numberOfCutsDropped = 0;
    vector<indexx> cutsToDelete = {};
    for (indexx i = 0; i < numberOfCuts; i++) {
        indexx index = constraintSize - numberOfCuts + i;
        if (m_constraints[index].getXQx().size() > 0) {
            // m_constraints.erase(m_constraints.begin()+index);
            numberOfCutsDropped += 1;
            cutsToDelete.push_back(index);
        }
    }
    // delete cuts memorized
    for (indexx i = static_cast<indexx>(cutsToDelete.size())-1; i >= 0; i--) {
        m_constraints.erase(m_constraints.begin()+cutsToDelete[i]);
    }
    return numberOfCutsDropped;
}

void OptimizationProblem::printConstraintsAndLhsEval(vector<double> const &point) const {
    for (auto const& constraint : m_constraints)
        cout << constraint << "\t" << constraint.evalLhs(point) << endl;
}

ostream& operator<<(ostream& os, OptimizationProblem const& optimizationProblem) {
    optimizationProblem.print(os);
    return os;
}