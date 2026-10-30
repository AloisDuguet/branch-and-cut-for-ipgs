//
// Created by ChatGPT adaptation.
//

#ifndef BESTADDITIVEMULTIPLICATIVEAPPROXGNEP_H
#define BESTADDITIVEMULTIPLICATIVEAPPROXGNEP_H

#include "BestMultiplicativeAdditiveApproxGNEP.h"

#include "BestMultiplicativeAdditiveBranchOutput.h"

class BestAdditiveMultiplicativeApproxGNEP : public BestMultiplicativeAdditiveApproxGNEP {
public:
    BestAdditiveMultiplicativeApproxGNEP(
        std::vector<double> & multiplicativeApprox,
        std::vector<double> & additiveApprox,
        std::shared_ptr<NashEquilibriumProblem> const& NEPInit,
        std::string const& gameTypeString = "UNK",
        bool globalCutSwitch = false,
        std::string const& modelTypeString = "manyEtasGurobi",
        std::string const& whichCutString = "eqCuts",
        std::string const& branchingRuleString = "mostFractional",
        std::string const& algorithmVariant = "basicAlgorithm",
        std::string const& instanceName = "UNK",
        bool enumerateNE = false,
        double timeLimit = 3600,
        int verbosity = 0,
        std::string const& resultFilename = "results.txt");

    // get current additive approximation parameter
    double getApproximation() const override;
    void setApproximation(double approximationValue) const override;

    // print parameters
    void printParameters(std::ostream& os) const override;

    // compute minimal additive approximation parameter for solution
    double computeNEWithMinAdditiveAndFixedMultiplicative(
        std::vector<double> const &solution) const;

    // handle NE found
    bool NEHandler(std::vector<double> const &solution) override;

    // logging
    void logNEFound(double maxAdditive) const override;
    void logNoNEFound() const override;

    // return handler
    BestAdditiveMultiplicativeBranchOutput* returnHandler(
        std::chrono::time_point<std::chrono::system_clock> startTime,
        std::string errorMsg = "no error") override;

    // one-line output
    void oneLineResultHandler(BestAdditiveMultiplicativeBranchOutput const& output) const;
    void writeOneLineResult(
        BestAdditiveMultiplicativeBranchOutput const &output,
        std::ostream &os) const;
};

#endif //BESTADDITIVEMULTIPLICATIVEAPPROXGNEP_H