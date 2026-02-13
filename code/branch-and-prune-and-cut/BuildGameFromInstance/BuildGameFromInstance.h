//
// Created by Aloïs Duguet on 7/8/25.
//

#ifndef BUILDGAMEFROMINSTANCE_H
#define BUILDGAMEFROMINSTANCE_H

#include "BranchAndCutTools.h"

// build internal model of the game from game type and instance file name
std::shared_ptr<NashEquilibriumProblem> buildGameFromInstance(
    std::string const& gameTypeString,
    std::string const& filename);

// for test purposes. Return instance file name from a type of game
std::string getFilenameInstance(std::string const& gameTypeString);

// return a vector<vector<double>> with two elements from a string
// where the first element is a vector for the multiplicative approx,
// and the second element is a vector for the additive approx
std::vector<std::vector<double>> buildMultiplicativeAdditiveApproxVector(std::string& s);

#endif //BUILDGAMEFROMINSTANCE_H
