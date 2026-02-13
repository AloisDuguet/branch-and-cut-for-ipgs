//
// Created by Aloïs Duguet on 10/31/24.
//

#include "SolveOutput.h"

#include "CommonlyUsedFunctions.h"

#include <iostream>

using namespace std;

void printSolveOutput(SolveOutput output, int verbosity) {
    if (verbosity >= 3) {
        cout << "(status = " << output.status << ", sol = " << output.solution;
        cout << ", obj = " << output.objective << ")" << endl;
    }
    else if (verbosity == 2)
        cout << "obj = " << output.objective << endl;
}