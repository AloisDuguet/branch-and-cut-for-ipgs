//
// Created by Aloïs Duguet on 11/27/24.
//

#include "MySparseMatrix.h"

#include <vector>
#include <iostream>
#include <cmath>

#include "MySparseVector.h"

using namespace std;

MySparseMatrix::MySparseMatrix() {
    m_values = {};
    m_rowIndices = {};
    m_colIndices = {};
}

MySparseMatrix::MySparseMatrix(std::vector<double> const& values,
                               std::vector<indexx> const& rowIndices,
                               std::vector<indexx> const& colIndices) {
    if (values.size() != rowIndices.size() || values.size() != colIndices.size()) {
        // attributes of SparseMatrix of different sizes at initialization
        throw logic_error("attributes of SparseMatrix of "
                          "different sizes at initialization");
    }
    m_values = {};
    m_rowIndices = {};
    m_colIndices = {};
    // check for coefficients of value 0
    // WARNING: removing a coefficient with deleteCoefficient when looping
    // may skip the next element from consideration in a for loop
    for (int i = 0; i < values.size(); i++) {
        if (values[i] != 0) {
            m_values.push_back(values[i]);
            m_rowIndices.push_back(rowIndices[i]);
            m_colIndices.push_back(colIndices[i]);
        }
    }
}

void MySparseMatrix::print(ostream& os, bool isEndl) const {
    for (indexx i = 0; i < m_rowIndices.size(); i++) {
        if (i > 0) {
            if (m_values[i] > 0) {
                if (m_values[i] == 1)
                    os  << " + x_" << m_rowIndices[i] << "x_" << m_colIndices[i];
                else
                    os << " + " << m_values[i] << " x_"
                       << m_rowIndices[i] << "x_" << m_colIndices[i];
            }
            else { // m_values[i] < 0
                if (m_values[i] == -1)
                    os << " - x_" << m_rowIndices[i] << "x_" << m_colIndices[i];
                else if (m_values[i] < 0) // should print nothing if == 0
                    os << " - " << -m_values[i] << " x_"
                       << m_rowIndices[i] << "x_" << m_colIndices[i];
            }
        }
        else {
            if (m_values[i] == 1)
                os << "x_" << m_rowIndices[i] << "x_" << m_colIndices[i];
            else if (m_values[i] == -1)
                os << "-x_" << m_rowIndices[i] << "x_" << m_colIndices[i];
            else if (m_values[i] != 0) // should print nothing if == 0
                os << m_values[i] << " x_" << m_rowIndices[i] << "x_" << m_colIndices[i];
        }
    }
    if (isEndl)
        os << endl;
}

int MySparseMatrix::size() const {
    return static_cast<int>(m_rowIndices.size());
}

bool MySparseMatrix::operator==(MySparseMatrix const& M) const {
    if (m_values == M.m_values and m_rowIndices == M.m_rowIndices and m_colIndices == M.m_colIndices)
        return true;
    return false;
}

bool MySparseMatrix::operator!=(MySparseMatrix const& M) const {
    if (this->m_values != M.m_values)
        return true;
    if (this->m_rowIndices != M.m_rowIndices)
        return true;
    if (this->m_colIndices != M.m_colIndices)
        return true;
    return false;
}

MySparseMatrix MySparseMatrix::operator+(MySparseMatrix M) const {
    for (indexx i = 0; i < m_rowIndices.size(); i++) {
        bool coef_added = false;
        for (indexx j = 0; j < M.m_rowIndices.size(); j++) {
            if (m_rowIndices[i] == M.m_rowIndices[j] and m_colIndices[i] == M.m_colIndices[j] ) {
                M.m_values[j] += m_values[i];
                if (M.m_values[j] == 0) {
                    // safe deleteCoefficient (no index skipped)
                    // because the loop is broken after
                    M.deleteCoefficient(j);
                }
                coef_added = true;
                break; // all indices should be different
            }
        }
        if (!coef_added) {
            // the SparseMatrix 'this' is trusted not to
            // have coefficients with value 0
            M.m_values.push_back(m_values[i]);
            M.m_rowIndices.push_back(m_rowIndices[i]);
            M.m_colIndices.push_back(m_colIndices[i]);
        }
    }
    return M;
}

void MySparseMatrix::addCoefficient(double value,
                                    indexx rowIndex,
                                    indexx colIndex) {
    // check if index already has a coefficient. If yes, add value to the old value.
    // If no, add a new index with this value
    if (value != 0) {
        bool isNewIndex = true;
        for (indexx i = 0; i < m_rowIndices.size(); i++) {
            if (m_rowIndices[i] == rowIndex and m_colIndices[i] == colIndex) {
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
            m_rowIndices.push_back(rowIndex);
            m_colIndices.push_back(colIndex);
            m_values.push_back(value);
        }
    }
}

void MySparseMatrix::deleteCoefficient(indexx const index) {
    m_values.erase(m_values.begin() + index);
    m_rowIndices.erase(m_rowIndices.begin() + index);
    m_colIndices.erase(m_colIndices.begin() + index);
}

double MySparseMatrix::getValue(indexx k) const {
    return m_values[k];
}

indexx MySparseMatrix::getRowIndex(indexx k) const {
    return m_rowIndices[k];
}

indexx MySparseMatrix::getColIndex(indexx k) const {
    return m_colIndices[k];
}

double MySparseMatrix::distanceMeasure(MySparseMatrix const& M) const {
    double sumOfSquare = 0;
    for (indexx i = 0; i < m_rowIndices.size(); i++) {
        bool coef_found = false;
        for (indexx j = 0; j < M.m_rowIndices.size(); j++) {
            if (m_rowIndices[i] == M.m_rowIndices[j]
                and m_colIndices[i] == M.m_colIndices[j] ) {
                // found matched coefficients of M and this
                sumOfSquare += pow(M.m_values[j] - m_values[i], 2);
                coef_found = true;
                break; // all indices should be different
            }
        }
        if (!coef_found) {
            // found unmatched coefficients of this
            sumOfSquare += pow(m_values[i], 2);
        }
    }
    // finding unmatched coefficients of v
    for (indexx i = 0; i < M.m_rowIndices.size(); i++) {
        bool coef_found = false;
        for (indexx j = 0; j < m_rowIndices.size(); j++) {
            if (M.m_rowIndices[i] == m_rowIndices[j]
                and M.m_colIndices[i] == m_colIndices[j] ) {
                coef_found = true;
                break; // all indices should be different
            }
        }
        if (!coef_found) {
            // found unmatched coefficients of v
            sumOfSquare += pow(M.m_values[i], 2);
        }
    }
    return sqrt(sumOfSquare);
}

ostream& operator<<(ostream& os, MySparseMatrix const& M) {
    M.print(os);
    return os;
}

MySparseVector operator*(std::vector<double> const &v, MySparseMatrix const &M) {
    // initialize result
    MySparseVector res({},{});
    // loop through all pairs of coefficients of v and M
    // and add coefficient to res when it matches
    for (indexx i = 0; i < v.size(); i++) {
        for (indexx j = 0; j < M.size(); j++) {
            // check if index of v corresponds to row index of M
            // safe addCoefficient (wrt deleting indices in a loop)
            // because not looping on the coefs of the object with addCoefficient
            if (i == M.getRowIndex(j))
                res.addCoefficient(v[i]*M.getValue(j), M.getColIndex(j));
        }
    }

    return res;
}


void testSparseMatrix() {
    MySparseMatrix M1 = {{1.2,3.4}, {2,5}, {0,1}};
    MySparseMatrix M2 = {{-3.4, 0.5}, {5,2}, {1,0}};
    cout << "M1: ";
    // M1.print(true);
    cout << M1 << endl;
    cout << "M2: ";
    cout << M2 << endl;
    M1.addCoefficient(-1.2, 2, 0);
    cout << "M1 + coef(-1.2,2,0): ";
    // M1.print(true);
    cout << M1 << endl;
    M1.addCoefficient(-1.0, 4, 0);
    cout << "M1 + coef(-1.0,4,0): ";
    // M1.print(true);
    cout << M1 << endl;
    bool eqMat = (M1 == M2);
    bool diffMat = (M1 != M2);
    MySparseMatrix M3 = M1 + M2;
    cout << "M1==M2? " << eqMat << endl;
    cout << "M1!=M2? " << diffMat << endl;
    cout << "M1+M2 = ";
    // M3.print(true);
    cout << M3 << endl << endl;

    cout << "test vector<double> * SparseMatrix" << endl;
    vector<double> v({1,2,0,4});
    MySparseMatrix M4 = {{-3, 0.5}, {3,2}, {1,0}};
    cout << v << endl;
    cout << "*" << endl;
    cout << M4 << endl;
    cout << "= " << v*M4 << endl;
    cout << "same with the transpose of the SparseMatrix:" << endl << endl;

    MySparseMatrix M5 = {{-3, 0.5}, {1,0}, {3,2}};
    cout << v << endl;
    cout << "*" << endl;
    cout << M5 << endl;
    cout << "= " << v*M5 << endl << endl;
}