//
// Created by Aloïs Duguet on 12/6/24.
//

#ifndef NEPMANYETASGUROBI_H
#define NEPMANYETASGUROBI_H

#include <vector>
#include <memory>
#include <Eigen/LU>

#include "BranchAndCutTools.h"
#include "CutInformation.h"

using Eigen::MatrixXd;

struct SetupIC {
    std::vector<indexx> basicVariableIndices = {};
    std::vector<indexx> nonbasicVariableIndices = {};
    std::vector<indexx> basicConstraintIndices = {};
    std::vector<indexx> nonbasicConstraintIndices = {};
    int numBasicVariables;
    int numNonbasicVariables;
    int const numDeclaredVariables;
    int const numConstraints;
    GRBConstr* constraints;
};

class NEPManyEtasGurobi : public NEPAbstractSolver {
public:
    explicit NEPManyEtasGurobi(
        std::vector<std::shared_ptr<OptimizationProblem>> const& bestResponseProblems);

    // initiliaze some attributes
    void initialize() override;

    void print(std::ostream& os = std::cout) const override;

    std::vector<std::vector<double>> getNashEquilibriumList();

    // get index of variable eta_i
    [[nodiscard]] indexx getEtaIndex(indexx player) const override;

    // return evaluation of pi_i(point)
    [[nodiscard]] double evalResponseValue(indexx player,
                                           std::vector<double> const& point) const override;

    [[nodiscard]] bool checkNE(std::vector<double> const& point,
                               double tolerance, int verbosity) const override;

    // compute best response of player player with values of point
    // (which contains other players variables values and also this players variables values)
    void computeBestResponse(indexx player,
                             std::vector<double> const& point,
                             std::shared_ptr<GRBEnv> const& globalEnv,
                             double solverTimeLimit,
                             int verbosity);

    // uses computeBestResponse for each player and return a list of best responses
    void computeAllBestResponses(std::vector<double> const& point,
                                 std::shared_ptr<GRBEnv> globalEnv,
                                 double solverTimeLimit,
                                 int verbosity) override;

    // build the root node of our algorithm
    std::shared_ptr<OptimizationProblem> buildOptimizationProblem() override;

    // add an equilibrium cut as in Dragotto23
    [[nodiscard]] Cut buildDisaggregatedEquilibriumCut(
        indexx player,
        std::vector<double> const& point) const override;

    // return the evaluation of the Nikaido-Isoda function at point
    [[nodiscard]] double evalNikaidoIsoda(std::vector<double> const& point,
                                          double objectiveValue,
                                          bool isSolutionInteger = true) const override;

    // return the differences between best response values and response values for all i
    [[nodiscard]] std::vector<double> differencesToBestResponseValues(
        std::vector<double> const& point) const override;

    // return true if eta*_player == best response value of player, else false
    [[nodiscard]] virtual bool testBestEtaPlayerPossible(indexx player,
                                                         std::vector<double> const& point,
                                                         double const& tolerance = 1e-4,
                                                         int verbosity = 0) const;

    // return true if eta* == sum of best responses values, else false
    [[nodiscard]] bool testBestEtaPossible(std::vector<double> const& point,
                                                   double const& tolerance = 1e-5,
                                                   int verbosity = 0) const override;

    // solves a QP to get a valid lower bound or upper bound for eta (via pi_i(x))
    [[nodiscard]] SolveOutput computeEtaBound(indexx player,
                                              bool isUpperBound, double mipGap) const override;

    void computeEtasBounds(double mipGap) override;

    // add an NE to nashEquilibriumList
    std::vector<double> extractNashEquilibrium(std::vector<double> nashEquilibrium,
                                               double tolerance) override;

    // compute vectors of variable indices and counts for IC computation
    [[nodiscard]] static SetupIC initializeSetupIC(
        std::shared_ptr<GRBModel> const& model, int verbosity);

    // build extreme rays for intersection cut
    [[nodiscard]] static MatrixXd buildExtremeRays(
        std::shared_ptr<GRBModel> const& model,
        SetupIC const& setupIC,
        int verbosity = 0);

    // build constraint of NE-free set g(y_istar,x_-i) <= 1
    [[nodiscard]] static Constraint buildPlayerConstraintOfNEFreeSet(
        Constraint const& constraint,
        std::vector<double> const& y_i_star,
        indexx startingPlayerVariableIndex,
        indexx endingPlayerVariableIndex);

    // build equilibrium constraint of NE-free set eta_i >= pi_i(y_istar,x_-i)
    [[nodiscard]] Constraint buildEquilibriumConstraintOfNEFreeSet(
        int player,
        std::vector<double> const& y_i_star,
        indexx startingPlayerVariableIndex,
        indexx endingPlayerVariableIndex,
        indexx endingAllPlayerIndex) const;

    // old way of computing alpha_i for equilibrium constraint
    [[nodiscard]] double oldComputeIntersectionRayEquilibriumConstraint(
        int player,
        indexx i,
        std::vector<double> const& y_i_star,
        std::vector<double> const& solution,
        MatrixXd extremeRays,
        indexx startingPlayerVariableIndex,
        indexx endingPlayerVariableIndex,
        indexx endingAllPlayerIndex,
        double toleranceDenominator,
        int verbosity = 1) const;

    // old way of computing alpha_i for player constraints of the NE-free set
    [[nodiscard]] static double oldComputeIntersectionRayPlayerConstraint(
        indexx i,
        Constraint const& constraint,
        std::vector<double> const& y_i_star,
        std::vector<double> const& solution,
        MatrixXd extremeRays,
        indexx startingPlayerVariableIndex,
        indexx endingPlayerVariableIndex,
        double toleranceDenominator,
        int verbosity);

    // compute NE-free set for IC for eta variables
    [[nodiscard]] std::vector<Constraint> getNEFreeSetEta(indexx player,
                                            indexx numDeclaredVariables) const;

    // build vector alpha with the intersection of the NE-free set and the rays
    [[nodiscard]] static std::vector<double> computeAlphaForIC(
        MatrixXd const& extremeRays,
        indexx player,
        std::vector<double> const& solution,
        int numNonbasicVariables,
        std::vector<Constraint> const &NEFreeSet,
        int verbosity);

    //compute intersection cut from the vector alpha
    [[nodiscard]] static Cut computeICFromAlpha(
        SetupIC const& setupIC,
        std::vector<double> const& alpha,
        std::shared_ptr<GRBModel> const& model);

    // build intersection cut from extreme rays
    [[nodiscard]] static Cut deriveICForOnePlayer(std::vector<Constraint> const &NEFreeSet,
                                           indexx player,
                                           std::vector<double> const& solution,
                                           std::shared_ptr<GRBModel> const& model,
                                           MatrixXd const& extremeRays,
                                           SetupIC const& setupIC,
                                           int verbosity = 0);

    // check that the IC derived cuts off the point wanted
    static void checkICSatisfaction(
        Constraint const &cut,
        indexx player,
        std::vector<double> const &point,
        std::shared_ptr<GRBModel> const &nodeModel,
        MatrixXd const &extremeRays,
        SetupIC const &setupIC, std::vector<Constraint> const &NEFreeSet);

    // derive an equilibrium cut for each player
    [[nodiscard]] virtual std::vector<Cut> deriveEquilibriumCutForEachPlayer(
        std::shared_ptr<GRBModel> const& model,
        std::vector<double> const& point,
        double tolerance,
        int verbosity) const;

    // derive an intersection cut for each player
    [[nodiscard]] virtual CutInformation deriveICForEachPlayer(
        std::shared_ptr<GRBModel> const &model,
        std::vector<double> const &point,
        double tolerance, int verbosity) const;

    // derive a no-good cut for point
    [[nodiscard]] std::vector<Cut> deriveNoGoodCut(
        std::vector<double> const& point,
        int verbosity) const;

    // function for deriving a Cut for this class
    CutInformation deriveCut(std::vector<double> const &point,
                             std::string const &whichCutString,
                             double const &tolerance,
                             std::shared_ptr<GRBEnv> const &globalEnv,
                             std::shared_ptr<GRBModel> const &model,
                             std::shared_ptr<OptimizationProblem> const &nodeProblem,
                             int verbosity) override;

    [[nodiscard]] std::vector<double> getEtaUpperBounds() const override;
    [[nodiscard]] std::vector<double> getEtaLowerBounds() const override;

protected:
    // contains lower bounds on each eta_i
    std::vector<double> m_etaLowerBounds;

    // contains upper bounds on each eta_i
    std::vector<double> m_etaUpperBounds;

    double etaLB = -1e100;
    double etaUB = 1e100;
};

std::ostream& operator<<(std::ostream& os, NEPManyEtasGurobi const& NEP);




#endif //NEPMANYETASGUROBI_H
