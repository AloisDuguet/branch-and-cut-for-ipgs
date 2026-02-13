//
// Created by Aloïs Duguet on 11/28/24.
//

#include "Constraint.h"

#include <iostream>
#include <cmath>
#include <format>

using namespace std;

Constraint::Constraint() {
    m_ax = {};
    m_xQx = {};
    m_b = 0;
}

Constraint::Constraint(MySparseVector const& v, MySparseMatrix const& M, double rhs) {
    m_ax = v;
    m_xQx = M;
    m_b = rhs;
}

void Constraint::print(ostream& os, bool endLine) const {
    os << m_ax;
    if (m_xQx.size() > 0 and m_ax.size() > 0)
        os << " + ";
    os << m_xQx;
    os << " <= " << format("{:.6f}", m_b);
    if (endLine)
        os << endl;
}

MySparseVector Constraint::getAx() const {
    return m_ax;
}

MySparseMatrix Constraint::getXQx() const {
    return m_xQx;
}

double Constraint::getB() const {
    return m_b;
}

void Constraint::increaseB(double value) {
    m_b += value;
}


double Cut::distanceMeasure(Cut const& cut) const {
    double sumOfSquare = pow(m_b-cut.m_b, 2);
    sumOfSquare += pow(m_ax.distanceMeasure(cut.m_ax), 2);
    sumOfSquare += pow(m_xQx.distanceMeasure(cut.m_xQx), 2);
    return sqrt(sumOfSquare);
}

bool Cut::areCutsEqual(Cut const& cut) const {
    cout << "distance between those two cuts: " << distanceMeasure(cut) << endl;
    if (m_b != cut.m_b)
        return false;
    if (m_ax != cut.m_ax)
        return false;
    if (m_xQx != cut.m_xQx)
        return false;
    return true;
}

bool Constraint::checkConstraintWithPoint(std::vector<double> const& point,
                                          int verbosity,
                                          double const& tolerance) const {
    double lhsEval = 0;
    for (indexx i = 0; i < m_ax.size(); i++) {
        if (point.size() < m_ax.getIndex(i))
            throw logic_error("in checkConstraintWithPoint, "
                              "a point of wrong size has been given");
        lhsEval += m_ax.getValue(i)*point[m_ax.getIndex(i)];
    }
    for (indexx i = 0; i < m_xQx.size(); i++)
        lhsEval +=
            m_xQx.getValue(i)*point[m_xQx.getRowIndex(i)]*point[m_xQx.getColIndex(i)];
    // useful print
    // tolerance should be set in accordance with solver tolerance
    // BUT it does not exactly work by setting the same value
    // cf commentary in function declaration
    if (lhsEval > m_b + tolerance) {
        if (verbosity >= 2) {
            cout << "new cut not satisfied as expected: "
                 << format("{:.7f}", lhsEval)
                 << " <= " << format("{:.7f}", m_b)
                 << " by a margin of " << lhsEval-m_b << endl;
        }
        if (verbosity >= 3) {
            if (m_xQx.size() == 0) {
                // print a standardized value meaning by how much the cut cuts off solution
                // to do that, divide the margin by the geometric mean of a linear coefficient of the cut
                double margin = lhsEval-m_b;
                double standardizedMargin = margin/norm2(m_ax.getValues());
                cout << "distance to line ax == b: " << standardizedMargin << endl;
            }
        }
        return true;
    } else {
        /*if (verbosity == 1) {
            cout << "margin of " << lhsEval-m_b << endl;
        }*/
        if (verbosity >= 2) {
            string s = "new cut satisfied (up to a tolerance of "
                     + to_string(tolerance) + "): "
                     + format("{:.7f}", lhsEval)
                     + " <= " + format("{:.7f}", m_b)
                     + ", margin of "
                     + to_string(lhsEval-m_b);
            cout << s << endl;
        }
        return false;
    }
}

double Constraint::evalLhs(vector<double> const &point) const {
    double res = 0;
    for (indexx i = 0; i < m_ax.size(); i++) {
        res += m_ax.getValue(i)*point[m_ax.getIndex(i)];
    }
    for (indexx i = 0; i < m_xQx.size(); i++) {
        res += m_xQx.getValue(i)*point[m_xQx.getRowIndex(i)]*point[m_xQx.getColIndex(i)];
    }
    return res;
}

ostream& operator<<(ostream& os, Constraint const& constraint) {
    constraint.print(os);
    return os;
}

bool operator==(Constraint const& c1, Constraint const& c2) {
    if (c1.getB() != c2.getB())
        return false;
    if (c1.getAx() != c2.getAx())
        return false;
    if (c1.getXQx() != c2.getXQx())
        return false;
    return true;
}