//
// Created by alois-duguet on 11/14/25.
//

#ifndef NEPMULTADDAPPROX_H
#define NEPMULTADDAPPROX_H

#include "NEPMultiplicativeApprox.h"

class NEPMultAddApprox : public NEPMultiplicativeApprox {
public:
    NEPMultAddApprox(
        std::vector<double> const& factor,
        std::vector<double> const& additiveApprox,
        std::vector<std::shared_ptr<OptimizationProblem>> const& bestResponseProblems,
        std::string const& instanceName = "");

    // change print to display the approximation parameters
    void print(std::ostream& os) const override;

    // print the terms defining the approximate NE
    void printApproximation(std::ostream& os) const override;

    // return the additive terms defining the approximate NE
    double getAdditiveApproximationPlayer(indexx player) const override;

    // return the multiplicative terms defining the approximate NE
    double getMultiplicativeApproximationPlayer(indexx player) const override;

    // print the type of equilibria searched
    void printEquilibriaSearched(std::ostream& os) const override;

    // build constraint involving lambda and player
    [[nodiscard]] Constraint buildLambdaConstraint(indexx player) const override;

    // do not use computeRegrets because
    // it is not directly adaptable to this derived class
    [[nodiscard]] std::vector<double> computeRegrets(
        std::vector<double> const &point) const override;

    // check if condition for being an NE is validated for player
    [[nodiscard]] bool checkPlayerNECondition(std::vector<double> const &point,
                           indexx player,
                           double tolerance) const override;

    // derive an equilibrium cut for each player
    [[nodiscard]] std::vector<Cut> deriveEquilibriumCutForEachPlayer(
        std::shared_ptr<GRBModel> const& model,
        std::vector<double> const& point,
        double tolerance,
        int verbosity) const override;

protected:
    std::vector<double> m_additiveApprox;
};



#endif //NEPMULTADDAPPROX_H
