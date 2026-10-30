//
// Created by alois-duguet on 6/17/26.
//

#include "LipschitzComputation.h"

#include "GameParserAndBuilder.h"

using namespace std;

vector<double> computeAllLipschitzConstantImplementationGame(
        string const& filename,
        vector<shared_ptr<OptimizationProblem>> const& bestResponseProblems) {
    auto [numberOfPlayers,
        numberOfNodes,
        numberOfEdges,
        network,
        capacities,
        sourceSinks,
        U,
        demands,
        pMax,
        u] = parseImplementationGameGNEP(filename);

    // vector with the lipschitz constants for each player
    vector<double> lipschitzVector;

    for (indexx player = 0; player < numberOfPlayers+1; player++) {
        lipschitzVector.push_back(computeLipschitzConstantImplementationGame(
            player,
            bestResponseProblems[player],
            numberOfEdges,
            numberOfPlayers,
            U,
            capacities,
            pMax,
            u));
    }

    return lipschitzVector;
}

double getMaxTermLipschitzImplementationGame(double const uValue, double pmin, double pmax) {
    pmin = max(0.0,pmin);
    return max(abs(pmin - uValue), abs(pmax - uValue));
}

double computeLipschitzConstantImplementationGame(
        indexx const player,
        shared_ptr<OptimizationProblem> const& playerProblem,
        int numberOfEdges,
        int numberOfRealPlayers,
        vector<vector<double>> const& U,
        vector<vector<double>> const& capacities,
        vector<double> const& pMax,
        vector<double> const& u,
        string const& mode,
        vector<double> const& point,
        double ballSize) {

    if (mode != "global" and mode != "local")
        throw logic_error("mode must be 'global' or 'local'");

    double lipschitzConstant = 0;
    double pmax;
    // formula of the real players is different from the one of the last player (central authority)
    if (player != numberOfRealPlayers) {
        // pi_i(x,p) = (p-U_i)x_i
        // formula derived from the gradient of pi_i
        // L_i = max_(x,p) sqrt(sum_{e in E} (p_e-U_{i,e})^2 + x_{i,e}^2)
        for (indexx edge = 0; edge < numberOfEdges; edge++) {
            indexx variableIndex = player*(numberOfEdges+1) + edge;
            // (p_e-U_{i,e})^2, p_e in [0,pMax_e]
            if (mode == "global")
                pmax = pMax[edge];
            else if (mode == "local") {
                indexx startingIndexCentralAuthority = numberOfRealPlayers*(numberOfEdges+1);
                pmax = point[startingIndexCentralAuthority+edge]+ballSize;
                pmax = min(pmax, pMax[edge]);
            }
            double maxTerm = getMaxTermLipschitzImplementationGame(U[edge][player], 0, pmax);
            // double maxTerm = max(abs(-U[edge][player]),
            // abs(pMax[edge] - U[edge][player]));
            lipschitzConstant += maxTerm*maxTerm;
            // x_{i,e}^2, x_{i,e} in [0,capacities_{i,e}]
            lipschitzConstant += capacities[edge][player]*capacities[edge][player];
        }
    } else {
        // player is central authority
        // pi_N(x,p) = (u-l(x))p
        // where l(x) = (l_1(x),...,l_numberOfEdges(x))
        // and l_e(x) = sum_i x_{i,e}
        // formula derived from the gradient of pi_N
        // L_i = sqrt(sum_e (N*pMax_e^2 + max(abs(u_e), abs(u_e-l_e^max(x)))^2))
        for (indexx edge = 0; edge < numberOfEdges; edge++) {
            // N*pMax_e^2
            if (mode == "global")
                pmax = pMax[edge];
            else if (mode == "local") {
                indexx startingIndexCentralAuthority = numberOfRealPlayers*(numberOfEdges+1);
                pmax = point[startingIndexCentralAuthority+edge]+ballSize;
                pmax = min(pmax, pMax[edge]);
            }
            lipschitzConstant += numberOfRealPlayers*pmax*pmax;
            // each player has the same capacity on an edge
            double maxEdgeLoad = capacities[edge][0]; // l_e(x) <= capacity_edge
            for (indexx p = 0; p < numberOfRealPlayers; p++) {
                if (maxEdgeLoad != capacities[edge][p]) {
                    throw logic_error("capacities are not the same for each player, "
                        "lipschitz constant of central authority not computed properly");
                }
            }
            // max(abs(u_e), abs(u_e-l_e^max(x)))^2
            double maxTerm = max(abs(u[edge]), abs(u[edge] - maxEdgeLoad));
            lipschitzConstant += maxTerm*maxTerm;
        }
    }
    lipschitzConstant = sqrt(lipschitzConstant);

    return lipschitzConstant;
}