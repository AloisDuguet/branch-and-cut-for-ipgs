//
// Created by Aloïs Duguet on 11/14/24.
//

#include "MySparseVector.h"

#include <vector>
#include <iostream>
#include <cmath>

#include "MySparseMatrix.h"

using namespace std;

MySparseVector::MySparseVector() {
    m_values = {};
    m_indices = {};
}

MySparseVector::MySparseVector(vector<double> const& values,
                               vector<indexx> const& indices) {
    if (values.size() != indices.size()) {
        cout << "Attributes of SparseVector not of same size "
                "at initialization:" << endl;
        cout << "indices: " << indices << endl;
        cout << "values: " << values << endl;
        throw logic_error("attributes of SparseVector of "
                          "different sizes at initialization");
    }
    m_values = {};
    m_indices = {};
    // check for coefficients of value 0
    for (int i = 0; i < values.size(); i++) {
        if (values[i] != 0) {
            m_values.push_back(values[i]);
            m_indices.push_back(indices[i]);
        }
    }
}

void MySparseVector::print(ostream& os, bool isEndl) const {
    for (indexx i = 0; i < m_indices.size(); i++) {
        if (i > 0) {
            if (m_values[i] > 0) {
                if (m_values[i] == 1)
                    os  << " + x_" << m_indices[i];
                else
                    os << " + " << m_values[i] << " x_" << m_indices[i];
            }
            else { // m_values[i] < 0
                if (m_values[i] == -1)
                    os << " - x_" << m_indices[i];
                else if (m_values[i] < 0) // should print nothing if == 0
                    os << " - " << -m_values[i] << " x_" << m_indices[i];
            }
        }
        else {
            if (m_values[i] == 1)
                os  << "x_" << m_indices[i];
            else if (m_values[i] == -1)
                os << "-x_" << m_indices[i];
            else if (m_values[i] != 0) // should print nothing if == 0
                os << m_values[i] << " x_" << m_indices[i];
        }
    }
    if (isEndl)
        os << endl;
}

int MySparseVector::size() const {
    return static_cast<int>(m_indices.size());
}

bool MySparseVector::operator==(MySparseVector const& v) const {
    if (m_values == v.m_values and m_indices == v.m_indices)
        return true;
    return false;
}

bool MySparseVector::operator!=(MySparseVector const& v) const {
    if (this->m_values != v.m_values)
        return true;
    if (this->m_indices != v.m_indices)
        return true;
    return false;
}

MySparseVector MySparseVector::operator+(MySparseVector v) const {
    // v is copied on purpose to modify and return it
    // without modifying the two arguments
    for (indexx i = 0; i < m_indices.size(); i++) {
        bool coef_added = false;
        for (indexx j = 0; j < v.m_indices.size(); j++) {
            if (m_indices[i] == v.m_indices[j]) {
                v.m_values[j] += m_values[i];
                if (v.m_values[j] == 0) {
                    // safe deleteCoefficient (no index skipped)
                    // because the loop is broken after
                    v.deleteCoefficient(j);
                }
                coef_added = true;
                // all indices should be different
                break;
            }
        }
        if (!coef_added) {
            // the SparseVector this is trusted not to have coefficients with value 0
            v.m_values.push_back(m_values[i]);
            v.m_indices.push_back(m_indices[i]);
        }
    }
    return v;
}

MySparseVector MySparseVector::operator-() const {
    MySparseVector res({},{});
    for (indexx i = 0; i < m_indices.size(); i++) {
        res.m_values.push_back(-m_values[i]);
        res.m_indices.push_back(m_indices[i]);
    }
    return res;
}

MySparseVector MySparseVector::operator-(MySparseVector const& v) const {
    return *this + -v;
}


void MySparseVector::addCoefficient(double value, indexx index) {
    // check if index already has a coefficient.
    // If yes, add value to the old value
    // If no, add a new index with this value
    if (value != 0) {
        bool isNewIndex = true;
        for (indexx i = 0; i < m_indices.size(); i++) {
            if (m_indices[i] == index) {
                m_values[i] += value;
                if (m_values[i] == 0) {
                    // safe deleteCoefficient (no index skipped)
                    // because the loop is broken after
                    deleteCoefficient(i);
                }
                isNewIndex = false;
                break;
            }
        }
        if (isNewIndex) {
            // if no coefficient with index index exist, create one
            m_indices.push_back(index);
            m_values.push_back(value);
        }
    }
}

void MySparseVector::deleteCoefficient(indexx const index) {
    m_values.erase(m_values.begin() + index);
    m_indices.erase(m_indices.begin() + index);
}


double MySparseVector::getValue(indexx k) const {
    return m_values[k];
}

std::vector<double> MySparseVector::getValues() const {
    return m_values;
}

indexx MySparseVector::getIndex(indexx k) const {
    return m_indices[k];
}

double MySparseVector::distanceMeasure(MySparseVector const& v) const {
    double sumOfSquare = 0;
    for (indexx i = 0; i < m_indices.size(); i++) {
        bool coef_found = false;
        for (indexx j = 0; j < v.m_indices.size(); j++) {
            if (m_indices[i] == v.m_indices[j]) {
                // found matched coefficients of v and this
                sumOfSquare += pow(v.m_values[j] - m_values[i], 2);
                coef_found = true;
                // all indices should be different
                break;
            }
        }
        if (!coef_found) {
            // found unmatched coefficients of this
            sumOfSquare += pow(m_values[i], 2);
        }
    }
    // finding unmatched coefficients of v
    for (indexx i = 0; i < v.m_indices.size(); i++) {
        bool coef_found = false;
        for (indexx j = 0; j < m_indices.size(); j++) {
            if (v.m_indices[i] == m_indices[j]) {
                coef_found = true;
                break; // all indices should be different
            }
        }
        if (!coef_found) {
            // found unmatched coefficients of v
            sumOfSquare += pow(v.m_values[i], 2);
        }
    }
    return sqrt(sumOfSquare);
}

double MySparseVector::operator*(std::vector<double> const &v) const {
    double scalarProduct = 0;
    for (indexx i = 0; i < m_indices.size(); i++)
        scalarProduct += m_values[i] * v[m_indices[i]];
    return scalarProduct;
}

double MySparseVector::operator*(MySparseVector const &v) const {
    double scalarProduct = 0;
    for (indexx i = 0; i < m_indices.size(); i++) {
        for (indexx j = 0; j < v.m_indices.size(); j++) {
            if (m_indices[i] == v.m_indices[j]) {
                // found matched coefficients of v and this
                scalarProduct += m_values[i]*v.m_values[j];
                break;
            }
        }
    }
    return scalarProduct;
}

MySparseVector MySparseVector::operator*(MySparseMatrix const &Q) const {
    // initialize result
    MySparseVector res({},{});
    // loop through all pairs of coefficients of this and Q
    // and add coefficient to res when it matches
    for (indexx i = 0; i < size(); i++) {
        for (indexx j = 0; j < Q.size(); j++) {
            // check if index of this corresponds to row index of Q
            // safe addCoefficient (wrt deleting indices in a loop)
            // because not looping on the coefs of the object with addCoefficient
            if (getIndex(i) == Q.getRowIndex(j))
                res.addCoefficient(m_values[i]*Q.getValue(j), Q.getColIndex(j));
        }
    }

    return res;
}


bool MySparseVector::compareApproxEqual(MySparseVector const& y,
                                        double const tolerance) const {
    if (this->distanceMeasure(y) > tolerance)
        return false;
    return true;
}


ostream& operator<<(ostream& os, MySparseVector const& v) {
    v.print(os);
    return os;
}

void testSparseVector() {
    MySparseVector v1 = {{1.2,3.4}, {2,5}};
    MySparseVector v2 = {{-1.9, -1.2}, {3,2}};
    cout << "v1: ";
    // v1.print(true);
    cout << v1 << endl;
    cout << "v2: ";
    // v2.print(true);
    cout << v2 << endl;
    v1.addCoefficient(-3.4, 5);
    cout << "v1 + coef(-3.4,5): ";
    // v1.print(true);
    cout << v1 << endl;
    bool eqVec = (v1 == v2);
    bool diffVec = (v1 != v2);
    MySparseVector v3 = v1 + v2;
    cout << "v1==v2? " << eqVec << endl;
    cout << "v1!=v2? " << diffVec << endl;
    cout << "v1+v2 = ";
    // v3.print(true);
    cout << v3 << endl << endl;

    // test operator*
    cout << "test of scalar product between SparseVector (operator*)" << endl;
    MySparseVector v4 = {{2,3}, {2,5}};
    MySparseVector v5 = {{-1, -2}, {3,2}};
    auto v6 = v4 * v5;
    cout << v4 << endl;
    cout << v5 << endl;
    cout << "scalar product of last two vectors:" << endl;
    cout << v6 << endl << endl;

    cout << "test SparseVector * SparseMatrix" << endl;
    MySparseMatrix M1 = {{-3, 0.5}, {5,2}, {1,0}};
    MySparseMatrix M1transpose = {{-3, 0.5}, {1,0}, {5,2}};
    cout << v4 << endl;
    cout << "*" << endl;
    cout << M1 << endl;
    cout << "= " << v4*M1 << endl << endl;
    cout << "same with the transpose of the SparseMatrix:" << endl;
    cout << v4 << endl;
    cout << "*" << endl;
    cout << M1transpose << endl;
    cout << "= " << v4*M1transpose << endl << endl;

    cout << "test of operator- unary and binary" << endl;
    cout << "- (" << v4 << ") = " << -v4 << endl;
    cout << v4 << " - (" << v5 << ") = " << v4-v5 << endl << endl;

    cout << "the deleteCoefficient launched from within "
            "a loop on all indices may not be safe"
            " because it changes the number of elements" << endl;
    auto v7 = MySparseVector({1,2},{0,1});
    auto v8 = MySparseVector({-1,2},{0,1});
    cout << v7 << " + " << v8 << " = " << v7 + v8 << endl << endl;
    auto v9 = MySparseVector({0,1,0,0,1},{0,1,2,3,4});
    cout << "the following vector should not have 0 coefficients "
            "and exactly 2 nonzeros:" << endl << v9 << endl << endl;
}