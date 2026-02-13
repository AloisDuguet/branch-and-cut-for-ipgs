//
// Created by Aloïs Duguet on 10/31/24.
//

#ifndef SOLVEOUTPUT_H
#define SOLVEOUTPUT_H

#include <vector>

struct SolveOutput {
    // status code of the optimization by gurobi
    int status = -1;

    // solution if status == 2, else empty vector
    std::vector<double> solution = {};

    // objective value if status == 2, else 1e15 which is
    // close to the max for a double (assuming minimization)
    double objective = 1e15;
};

void printSolveOutput(SolveOutput output, int verbosity = 2);


#endif //SOLVEOUTPUT_H