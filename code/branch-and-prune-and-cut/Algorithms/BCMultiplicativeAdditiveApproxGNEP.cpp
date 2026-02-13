//
// Created by alois-duguet on 11/13/25.
//

#include "BCMultiplicativeAdditiveApproxGNEP.h"

#include "GameParserAndBuilder.h"
#include "NEPMultAddApprox.h"

using namespace std;

BCMultiplicativeAdditiveApproxGNEP::BCMultiplicativeAdditiveApproxGNEP(
        vector<double> & multiplicativeApprox,
        vector<double> & additiveApprox,
        shared_ptr<NashEquilibriumProblem> const& NEPInit,
        string const& gameTypeString,
        bool globalCutSwitch,
        string const& modelTypeString,
        string const& whichCutString,
        string const& branchingRuleString,
        string const& algorithmVariant,
        string const& instanceName,
        bool enumerateNE,
        double timeLimit,
        int verbosity,
        string const& resultFilename) :
            BranchAndCutMultiplicativeGNEP(
                multiplicativeApprox,
                NEPInit,
                gameTypeString,
                globalCutSwitch,
                modelTypeString,
                whichCutString,
                branchingRuleString,
                algorithmVariant,
                instanceName,
                enumerateNE,
                timeLimit,
                verbosity,
                resultFilename) {
    auto startTime = startChrono();
    // give proper size to additiveApprox in case it is defined as a vector of 1 element
    // indeed, it is then a shortcut for same additive approx value for each player
    cout << "additiveApprox before extension: " << additiveApprox << endl;
    if (additiveApprox.size() == 1 and NEPInit->getNumberOfPlayer() > 1) {
        for (indexx player = 1; player < NEPInit->getNumberOfPlayer(); player++)
            additiveApprox.push_back(additiveApprox[0]);
    }

    if (modelTypeString == "manyEtasGurobi") {
        if (gameTypeString != "implementationGame") {
            m_NEP = make_shared<NEPMultAddApprox>(multiplicativeApprox,
                                              additiveApprox,
                                              NEPInit->getBestResponseProblems());
        } else {
            // one more argument for the Lipschitz NE-free set
            m_NEP = make_shared<NEPMultAddApprox>(multiplicativeApprox,
                                              additiveApprox,
                                              NEPInit->getBestResponseProblems(),
                                              instanceName);
        }

    } else
        throw runtime_error("Wrong argument: NEP type '" + modelTypeString + "' not recognized.");

    m_timeSpent += elapsedChrono(startTime);
}

NEPBranchOutput* BCMultiplicativeAdditiveApproxGNEP::solve() {
    // initialize timer for this method
    auto startTime = startChrono();
    initialize();
    writeRootNode();

    // print the approximate NE looked for
    m_NEP->printEquilibriaSearched(cout);

    try {
        while ((m_nodeSelector->nodeListSize() > 0 or m_reuseNode) and checkTimeLimit(startTime)) {
            getNewNode();
            printNodeInformation(elapsedChrono(startTime));

            solveNode(computeSolverTimeLimit(startTime));
            checkTime(startTime);

            if (!m_node->isSolved()) {
                // handles unsolved nodes
                unsolvedNodeHandler();
            } else if (!compareApproxLess(m_node->getSolution().objective, m_NEP->getThresholdPruning(), m_pruningZeroObjTolerance)) {
                // prune the node because objective > delta
                if (m_verbosity >= 2)
                    cout << "pruning: node obj > threshold" << endl;
            }
            else {
                if (m_node->isIntegerSolution(m_integralityTolerance)) {
                    auto solution = computeBestResponseHandler(computeSolverTimeLimit(startTime));
                    checkBestResponseValueSigns();
                    checkTime(startTime);
                    detectCycling(solution);
                    if (m_NEP->checkNE(solution, m_NETolerance, m_verbosity)) {
                        if (NEHandler(solution))
                            break;
                    } else {
                        // add a cut and iterate on the same Node
                        addCuts(solution);
                    }
                } else {
                    // usual branching case
                    branchingHandler();
                }
                updateLastNodeSolution();
            }
        }

        auto output = returnHandler(startTime);
        return output;

    } catch (std::exception const& e) {
        auto output = returnHandler(startTime, e.what());
        return output;
    }
}

bool BCMultiplicativeAdditiveApproxGNEP::NEHandler(vector<double> const &solution) {
    // tolerance not the default one because when converging to an NE,
    // the node can be pruned by eta values too close to BR values. That makes pruning
    // happen before the case where an NE is found

    if (m_verbosity >= 1) {
        m_NEP->printApproximation(cout);
        cout << endl;
    }
    if (m_verbosity >= 3) {
        for (int player = 0; player < m_NEP->getNumberOfPlayer(); player++)
            cout << "cost of player " << player << ": "
                 << m_NEP->evalResponseValue(player, m_node->getSolution().solution) << endl;
    }
    m_nodeSelector->incrementSolutionList(m_NEP,
                                          solution,
                                          computeSocialWelfare(m_node->getSolution()),
                                          m_NEP->differencesToBestResponseValues(solution),
                                          m_NETolerance);
    m_NEFound = true;
    if (m_verbosity >= 1)
        cout << "stopping at first NE found" << endl << endl;

    logNEFound(solution);

    return true;
}

void BCMultiplicativeAdditiveApproxGNEP::writeOneLineResult(
        NEPBranchOutput const &output,
        ostream &os) const {
    // separator
    string const sep1 = ",";

    // start of the line
    os << "result,";

    // problem solved: multiplicative-additive approx-NE
    os << "multiplicativeAdditiveNE-";
    m_NEP->printApproximation(os);
    os << sep1;

    writeEndOfOneLineResult(output, os);
}

void BCMultiplicativeAdditiveApproxGNEP::logNEFound(vector<double> const& solution) {
    if (m_verbosity >= 1) {
        if (m_gameTypeString == "GNEP-maxFlowGame") {
            // print number of players sending nonzero flows
            // by counting the flow exiting the source node
            auto [numberOfPlayers,
            numberOfNodes,
            numberOfEdges,
            network,
            capacities,
            sourceSinks,
            loadCosts,
            olddemands,
            oldpMax,
            oldu] = parseImplementationGameGNEP(m_instanceName);
            int nonzeroFlow = 0;
            for (indexx player = 0; player < numberOfPlayers; player++) {
                auto sourcePlayer = static_cast<indexx>(sourceSinks[player].front());
                double flowPlayer = 0;
                for (indexx networkInfoIndex = 0; networkInfoIndex < network.size(); networkInfoIndex++)
                    if (network.getRowIndex(networkInfoIndex) == sourcePlayer) {
                        flowPlayer += network.getValue(networkInfoIndex)*solution[
                            player*numberOfEdges + network.getColIndex(networkInfoIndex)];
                    }
                if (flowPlayer > 0) {
                    nonzeroFlow++;
                }
                cout << "player " << player << " has exiting flow " << flowPlayer << endl;
            }
            cout << "number of players with nonzero flows: " << nonzeroFlow
                 << " / " << numberOfPlayers << endl;
        }
    }
}
