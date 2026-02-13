//
// Created by Aloïs Duguet on 10/30/24.
//

#include "NashEquilibriumProblem.h"

#include "OptimizationProblem.h"
#include "CommonlyUsedFunctions.h"

using namespace std;

NashEquilibriumProblem::NashEquilibriumProblem() {
    m_numberOfPlayers = 0;
    m_bestResponseProblems = {};
    m_isGNEP = false;
}

NashEquilibriumProblem::NashEquilibriumProblem(
        vector<shared_ptr<OptimizationProblem>> const& bestResponseProblems) {
    m_numberOfPlayers = static_cast<int>(bestResponseProblems.size());
    m_bestResponseProblems = bestResponseProblems;
    m_isGNEP = detectGNEP();
}

bool NashEquilibriumProblem::detectGNEP() const {
    // look for presence of other player variable indices in constraints
    // if yes, return true for GNEP

    for (indexx player = 0; player < m_numberOfPlayers; player++) {
        // compute starting and ending indices for player's variables
        indexx startingVariableIndex = 0;
        for (indexx i = 0; i < player; i++)
            // put startingVariableIndex to its proper value
            startingVariableIndex += getNumberOfVariablesOfPlayer(i);
        indexx endingVariableIndex =
            startingVariableIndex + getNumberOfVariablesOfPlayer(player);
        for (auto const& con :
                m_bestResponseProblems[player]->getConstraints()) {
            auto ax = con.getAx();
            for (indexx j = 0; j < ax.size(); j++) {
                indexx variableIndex = ax.getIndex(j);
                // if other player's variable
                if (variableIndex < startingVariableIndex
                    || variableIndex >= endingVariableIndex)
                    return true;
            }
        }
    }
    // if no constraints contain other player's variables, return false
    return false;
}


void NashEquilibriumProblem::print(ostream& os) const {
    os << endl;

    // variableCount will be used to print the variables of each player
    indexx variableCount = 0;

    os << "NEP with " << m_numberOfPlayers << " players:" << endl << endl;
    for (indexx i = 0; i < m_numberOfPlayers; i++) {
        int numberOfVariables = m_bestResponseProblems[i]->getNumberOfVariables();
        os << "Player " << i << " of variable ";
        if (numberOfVariables == 1) {
            os << "index " << variableCount << endl;
            variableCount += numberOfVariables;
        }
        else {
            os << "indices from " << variableCount;
            variableCount += numberOfVariables;
            os << " to " << variableCount-1 << endl;
        }
        os << *m_bestResponseProblems[i];
    }
}

int NashEquilibriumProblem::getNumberOfPlayer() const {
    return m_numberOfPlayers;
}

int NashEquilibriumProblem::getNumberOfVariablesOfPlayer(indexx player) const {
    return m_bestResponseProblems[player]->getNumberOfVariables();
}

vector<indexx> NashEquilibriumProblem::getNumberOfVariablesPerPlayer() const {
    vector<int> numberOfVariablesPerPlayer = {};
    for (indexx player = 0; player < m_numberOfPlayers; player++)
        numberOfVariablesPerPlayer.push_back(getNumberOfVariablesOfPlayer(player));
    return numberOfVariablesPerPlayer;
}

int NashEquilibriumProblem::getTotalNumberOfVariablesOfPlayers() const {
    vector<int> variablesPerPlayer = getNumberOfVariablesPerPlayer();
    int totalNumberOfVariables = 0;
    for (int val : variablesPerPlayer)
        totalNumberOfVariables += val;
    return totalNumberOfVariables;
}

indexx NashEquilibriumProblem::getStartingPlayerVariableIndex(indexx player) const {
    // compute variable indices of player
    indexx startingPlayerVariableIndex = 0;
    for (indexx p = 0; p < player; p++)
        startingPlayerVariableIndex += m_bestResponseProblems[p]->getNumberOfVariables();
    return startingPlayerVariableIndex;
}


vector<shared_ptr<OptimizationProblem>> NashEquilibriumProblem::getBestResponseProblems() const {
    return m_bestResponseProblems;
}

bool NashEquilibriumProblem::getIsGNEP() const {
    return m_isGNEP;
}

ostream& operator<<(ostream& os, NashEquilibriumProblem const& NEP) {
    NEP.print(os);
    return os;
}
