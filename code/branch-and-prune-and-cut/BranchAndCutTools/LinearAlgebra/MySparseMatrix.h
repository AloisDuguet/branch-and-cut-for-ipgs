//
// Created by Aloïs Duguet on 11/27/24.
//

#ifndef MYSPARSEMATRIX_H
#define MYSPARSEMATRIX_H

#include <iostream>

#include "CommonlyUsedFunctions.h"
#include "MySparseVector.h"

class MySparseMatrix {
    // a SparseMatrix should be interpreted as a quadratic monomial
    // where only nonzero coefficients are represented.
    // example: 9x0x2 + 3x4x1 is represented as:
    // a list value=[9,3] of numerical values,
    // a list rowIndex=[0,4] of row indices and
    // a list colIndex=[2,1] of col indices.
    // the vector is expected to be clear with the utilisation of the SparseVector
    //
    // properties wanted for this class;
    // - all pairs (rowIndex, colIndex) are different (unicity of indices)
    // - same size for rowIndex, colIndex and value
    // because they are interpreted together
    // as one coef value[i]*row_index[i]col_index[i]
    // - no coefficients of value 0

public:
    MySparseMatrix();

    MySparseMatrix(std::vector<double> const& values,
                   std::vector<indexx> const& rowIndices,
                   std::vector<indexx> const& colIndices);

    // print instance
    void print(std::ostream& os = std::cout, bool isEndl = false) const;

    [[nodiscard]] int size() const;

    // M is copied on purpose to modify and return it
    MySparseMatrix operator+(MySparseMatrix M) const;

    bool operator==(MySparseMatrix const& M) const;

    bool operator!=(MySparseMatrix const& M) const;

    // add a coefficient to the instance of SparseMatrix
    void addCoefficient(double value, indexx rowIndex, indexx colIndex);

    // remove coefficient
    void deleteCoefficient(indexx index);

    [[nodiscard]] double getValue(indexx k) const;

    [[nodiscard]] indexx getRowIndex(indexx k) const;

    [[nodiscard]] indexx getColIndex(indexx k) const;

    // computes the euclidean norm between coefficients of v and *this
    [[nodiscard]] double distanceMeasure(MySparseMatrix const& M) const;

protected:
    std::vector<double> m_values;
    std::vector<indexx> m_rowIndices;
    std::vector<indexx> m_colIndices;
};

std::ostream& operator<<(std::ostream& os, MySparseMatrix const& M);

// vector<double> multiplied by MySparseMatrix
MySparseVector operator*(std::vector<double> const& v, MySparseMatrix const& M);

// test different operations of SparseMatrix objects
void testSparseMatrix();

#endif //MYSPARSEMATRIX_H
