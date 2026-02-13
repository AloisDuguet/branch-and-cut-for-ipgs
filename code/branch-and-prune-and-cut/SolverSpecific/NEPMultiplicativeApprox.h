//
// Created by alois-duguet on 9/19/25.
//

#ifndef NEPMULTIPLICATIVEAPPROX_H
#define NEPMULTIPLICATIVEAPPROX_H

#include "NEPManyEtasGurobi.h"
#include "TestInstances.h"

class NEPMultiplicativeApprox : public NEPManyEtasGurobi {
public:
    NEPMultiplicativeApprox(
        std::vector<double> const& factor,
        std::vector<std::shared_ptr<OptimizationProblem>> const& bestResponseProblems,
        std::string const& instanceName = "");

    // setter and getter for m_factor
    [[nodiscard]] double getApproximationPlayer(indexx player) const override;
    void setApproximationPlayer(double approximation, indexx player) override;

    // change print to display m_factor
    void print(std::ostream& os) const override;

    // print the terms defining the approximate NE
    void printApproximation(std::ostream& os) const override;

    // print the type of equilibria searched
    void printEquilibriaSearched(std::ostream& os) const override;

    // get index of variable lambda
    [[nodiscard]] indexx getLambdaIndex() const;

    // get index of variable xi for player player
    [[nodiscard]] indexx getXiIndex(indexx player) const;

    // build the root node of the algorithm
    std::shared_ptr<OptimizationProblem> buildOptimizationProblem() override;

    // build constraint involving lambda and player
    [[nodiscard]] Constraint buildLambdaConstraint(indexx player) const override;

    // return the regrets per player
    [[nodiscard]] std::vector<double> computeRegrets(std::vector<double> const& point) const override;

    // return max regret among players
    [[nodiscard]] double computeMaxRegretNE(std::vector<double> const& point,
                                             int verbosity) const override;

    // print the regret for each player
    void printRegret(std::ostream& os,
                     std::vector<double> const& point) const override;

    // return the numerical threshold for pruning a node
    [[nodiscard]] double getThresholdPruning() const override;

    // check if condition for being an NE is validated for player
    [[nodiscard]] virtual bool checkPlayerNECondition(std::vector<double> const &point,
                           indexx player,
                           double tolerance) const;

    // check if solution is a factor-NE
    [[nodiscard]] bool checkNE(std::vector<double> const& point, double tolerance, int verbosity) const override;

    // extract NE from node solution
    std::vector<double> extractNashEquilibrium(std::vector<double> nashEquilibrium,
                                               double tolerance) override;

    // derive an equilibrium cut for each player
    [[nodiscard]] std::vector<Cut> deriveEquilibriumCutForEachPlayer(
        std::shared_ptr<GRBModel> const& model,
        std::vector<double> const& point,
        double tolerance,
        int verbosity) const override;

    // check if xi_i variable is at the value wanted (pi_i(x))
    [[nodiscard]] bool testBestXiPossible(indexx player,
                           std::vector<double> const& point,
                           double const& tolerance,
                           int verbosity = 0) const;

    // compute NE-free set for IC for xi when pi_i is concave in x and linear in x_-i
    [[nodiscard]] std::vector<Constraint> getNEFreeSetXiConcave(
        indexx player) const;

    // compute NE-free set for IC for xi when pi_i is convex in x
    [[nodiscard]] std::vector<Constraint> getNEFreeSetXiConvex(
        indexx player,
        std::vector<double> const &point) const;

    // compute NE-free set for IC for xi when pi_i is convex in x_-i and
    // lipschitz continuous in x
    [[nodiscard]] std::vector<Constraint> getNEFreeSetXiLipschitz(
        indexx player,
        std::vector<double> const& point);

    // derive an intersection cut for each player
    [[nodiscard]] std::vector<Cut> deriveICForEachPlayer(
        std::shared_ptr<GRBModel> const& model,
        std::vector<double> const& point,
        std::string const& whichCutString,
        double tolerance, int verbosity);

    // derive cuts
    CutInformation deriveCut(std::vector<double> const &point,
                             std::string const &whichCutString,
                             double const &tolerance,
                             std::shared_ptr<GRBEnv> const &globalEnv,
                             std::shared_ptr<GRBModel> const &model,
                             std::shared_ptr<OptimizationProblem> const &nodeProblem,
                             int verbosity) override;

    // set m_positiveResponses
    void setPositiveResponses(bool b) override;

    // set lipschitz constants for NE-free set
    void setLipschitzConstants(
        std::string const &gameTypeString,
        std::string const &whichCutString,
        std::string const &instanceName) override;

protected:
    // parameter of the multiplicative approximation of a Nash equilibrium: factor-NE
    std::vector<double> m_factor;

    // parameter set to true if the response values are positive, to false if negative
    // really important because it changes the constraint linking
    // lambda and the responses
    bool m_positiveResponses;

    // contains the lipschitz constants of costs of the player
    // only used for GNEP implementation games,
    // so that the NE-free set using the lipschitz constant works
    std::vector<double> m_lipschitzConstants;
    // instance name is useful for deriving local lipschitz constants
    GNEPImplementationGameInstance m_instanceObject;
};



#endif //NEPMULTIPLICATIVEAPPROX_H
