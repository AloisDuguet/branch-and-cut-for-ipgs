//
// Created by Aloïs Duguet on 12/6/24.
//

#ifndef NEPABSTRACTSOLVER_H
#define NEPABSTRACTSOLVER_H

#include <vector>
#include <memory>

#include "OptimizationProblem.h"
#include "Constraint.h"
#include "CutInformation.h"
#include "SolveOutput.h"
#include "NashEquilibriumProblem.h"

struct NEInfo {
    std::vector<double> NE;
    double socialWelfare;
    std::vector<double> bestResponseValues;
    std::vector<double> differencesToBRValues;

    void print(std::ostream& os) const {
        os << "NE " << NE << std::endl;
        os << "of social welfare " << socialWelfare << std::endl;
        os << "with best response values " << bestResponseValues << std::endl;
        os << "and differences to best response values "
         << differencesToBRValues << std::endl;
    }
};

std::ostream& operator<<(std::ostream& os, NEInfo const& info);
std::ostream& operator<<(std::ostream& os, std::vector<NEInfo> const& infos);

class NEPAbstractSolver : public NashEquilibriumProblem {
public:
    NEPAbstractSolver() = default;

    virtual ~NEPAbstractSolver() = default;

    // initiliaze some attributes
    virtual void initialize();

    // return best response problem SolveOutpu for each player
    // with variables of other players set
    [[nodiscard]] std::vector<SolveOutput> getBestResponses() const;

    // return best response problem SolveOutput of
    // player player with variables of other players set
    [[nodiscard]] SolveOutput getBestResponse(indexx player) const;

    // return solution of best response problem of
    // player player with only its variables set
    [[nodiscard]] std::vector<double> getBestResponseWithOnlyPlayerVariables(indexx player) const;

    // build the root node of our algorithm; not meant
    // to be in this class, better in a subclass of NEP.
    virtual std::shared_ptr<OptimizationProblem> buildOptimizationProblem();

    // return the objective value of player with strategies in point
    [[nodiscard]] virtual double evalResponseValue(indexx player,
                                                   std::vector<double> const& point) const;

    // check if point is an NE by evaluating the response values
    // and comparing them to the best response values
    [[nodiscard]] virtual bool checkNE(std::vector<double> const& point, double tolerance, int verbosity) const;

    // uses computeBestResponse for each player
    // and return a list of best responses
    virtual void computeAllBestResponses(std::vector<double> const& point,
                                         std::shared_ptr<GRBEnv> globalEnv,
                                         double solverTimeLimit,
                                         int verbosity);

    // add an NE to nashEquilibriumList
    virtual std::vector<double> extractNashEquilibrium(std::vector<double> nashEquilibrium,
                                                       double tolerance);

    // add an equilibrium cut as in Dragotto23
    [[nodiscard]] virtual Cut buildDisaggregatedEquilibriumCut(
        indexx player,
        std::vector<double> const& point) const;

    // add an aggregated equilibrium cut to m_cuts (sum of equilibrium cut for all players)
    [[nodiscard]] virtual Cut addAggregatedEquilibriumCut(std::vector<double> const& point) const;

    // return the evaluation of the Nikaido-Isoda function at point
    [[nodiscard]] virtual double evalNikaidoIsoda(std::vector<double> const& point,
                                                  double objectiveValue,
                                                  bool isSolutionInteger = true) const;

    // check if point is a delta-NE
    [[nodiscard]] virtual double computeMaxRegretNE(std::vector<double> const& point,
                                                    int verbosity) const;

    // return the difference between best response values and eta_i for all i
    [[nodiscard]] virtual std::vector<double> differencesToBestResponseValues(
        std::vector<double> const& point) const;

    // return true if eta* == sum of best responses values, else false
    [[nodiscard]] virtual bool testBestEtaPossible(std::vector<double> const& point,
                                                   double const& tolerance,
                                                   int verbosity) const;

    // solves an optimization problem to get a valid LB and UB for eta
    virtual void computeEtasBounds(double mipGap);
    [[nodiscard]] virtual SolveOutput computeEtaBound(indexx player,
                                                      bool isUpperBound, double mipGap) const;
    [[nodiscard]] virtual std::vector<double> getEtaUpperBounds() const;
    [[nodiscard]] virtual std::vector<double> getEtaLowerBounds() const;

    // generic function for deriving a Cut
    virtual CutInformation deriveCut(std::vector<double> const &point,
                                     std::string const &whichCutString,
                                     double const &tolerance,
                                     std::shared_ptr<GRBEnv> const &globalEnv,
                                     std::shared_ptr<GRBModel> const &model,
                                     std::shared_ptr<OptimizationProblem> const &nodeProblem,
                                     int verbosity);

    // return index of eta_player variable
    [[nodiscard]] virtual indexx getEtaIndex(indexx player) const;

    // methods linked with an approximate NE
    [[nodiscard]] virtual double getApproximation() const;
    [[nodiscard]] virtual double getApproximationPlayer(indexx player) const;
    virtual void setApproximation(double delta);
    virtual void setApproximationPlayer(double delta, indexx player);
    virtual void printApproximation(std::ostream& os) const;
    virtual void printEquilibriaSearched(std::ostream& os) const;
    virtual void printRegret(std::ostream& os, std::vector<double> const& point) const;
    virtual std::vector<double> computeRegrets(std::vector<double> const& point) const;
    [[nodiscard]] virtual double getThresholdPruning() const;
    virtual void setPositiveResponses(bool b);
    virtual Constraint buildLambdaConstraint(indexx player) const;
    virtual void setLipschitzConstants(
        std::string const &gameTypeString,
        std::string const &whichCutString,
        std::string const &instanceName);
    virtual double getMultiplicativeApproximationPlayer(indexx player) const;
    virtual double getAdditiveApproximationPlayer(indexx player) const;

protected:
    // list of all found NE
    std::vector<std::vector<double>> m_nashEquilibriumList;

    // list of vectors on k: (x)_k with x_k giving best response as x_k,i and other player variables x_k,-i
    std::vector<SolveOutput> m_bestResponses;

};

#endif //NEPABSTRACTSOLVER_H
