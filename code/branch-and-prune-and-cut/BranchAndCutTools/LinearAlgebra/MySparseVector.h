//
// Created by Aloïs Duguet on 11/14/24.
//

#ifndef MYSPARSEVECTOR_H
#define MYSPARSEVECTOR_H

#include <vector>
#include <iostream>

#include "CommonlyUsedFunctions.h"

class MySparseMatrix;

class MySparseVector {
    // a SparseVector should be interpreted as a linear combination
    // where only nonzero coefficients are represented.
    // example: 9x0 + 3x4 is represented as a list [9,3] of
    // values and a list [0,4] of indices of a vector.
    // the vector is expected to be clear with the utilisation of the SparseVector
    //
    // properties wanted for this class:
    // all indices are different (unicity of indices),
    // same size for index and value because they are interpreted
    // together as one coef value[i]*index[i],
    // no coefficients of value 0

public:
    MySparseVector();

    MySparseVector(std::vector<double> const& values,
                   std::vector<indexx> const& indices);

    // print SparseVector and work with operator<< overload
    void print(std::ostream& os = std::cout, bool isEndl = false) const;

    [[nodiscard]] int size() const;

    // v is copied on purpose to modify and return it
    MySparseVector operator+(MySparseVector v) const;

    // minus unary operator
    MySparseVector operator-() const;

    // sparse vector minus sparse vector
    MySparseVector operator-(MySparseVector const& v) const;

    bool operator==(MySparseVector const& v) const;

    bool operator!=(MySparseVector const& v) const;

    // add a coefficient to the instance of SparseVector
    void addCoefficient(double value, indexx index);

    void deleteCoefficient(indexx index);

    [[nodiscard]] double getValue(indexx k) const;

    [[nodiscard]] std::vector<double> getValues() const;

    [[nodiscard]] indexx getIndex(indexx k) const;

    // computes the euclidean norm between coefficients of v and *this
    [[nodiscard]] double distanceMeasure(MySparseVector const& v) const;

    double operator*(std::vector<double> const& v) const;

    double operator*(MySparseVector const& v) const;

    [[nodiscard]] MySparseVector operator*(MySparseMatrix const& Q) const;

    // return true if ||x-y|| < tolerance. 1e-6 is
    // the default feasibility tolerance of gurobi
    [[nodiscard]] bool compareApproxEqual(MySparseVector const& y,
                                          double tolerance = 1e-6) const;

protected:
    std::vector<double> m_values;
    std::vector<indexx> m_indices;
};

std::ostream& operator<<(std::ostream& os, MySparseVector const& v);

// test different operations of SparseVector objects
void testSparseVector();

#endif //MYSPARSEVECTOR_H
