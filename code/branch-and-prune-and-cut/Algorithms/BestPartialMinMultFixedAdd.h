//
// Created by alois-duguet on 3/27/26.
//

#ifndef BESTPARTIALMINMULTFIXEDADD_H
#define BESTPARTIALMINMULTFIXEDADD_H

#include "BestMultiplicativeAdditiveApproxGNEP.h"
#include "BestPartialMinMultFixedAddBranchOutput.h"

class BestPartialMinMultFixedAdd : public BestMultiplicativeAdditiveApproxGNEP {
public:
    BestPartialMinMultFixedAdd(
        std::vector<bool> const& multToMinimize,
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

    void initialize() override;

    void checkValidArguments() const override;

    // print basic information on instance
    void printParameters(std::ostream& os) const override;

    // set (multiplicative) approximation for each player for which we minimize it to value
    void setApproximation(double approximationValue) const override;

    // get current multiplicative approximation (which is the same for each player for which we minimize it)
    double getApproximation() const override;

    // remove in not used -> m_NEP->printApproximation(cout) does the same
    std::string getCurrentApproxString() const;

    void logNoNEFound() const override;
    void logNEFound(double maxRegret) const override;

    BestPartialMinMultFixedAddBranchOutput* returnHandler(
        std::chrono::time_point<std::chrono::system_clock> startTime,
        std::string errorMsg = "no error");

    // handle where to write the one-line result
    void oneLineResultHandler(BestPartialMinMultFixedAddBranchOutput const& output) const;

    // prints output of the algorithm in one line
    void writeOneLineResult(
        BestPartialMinMultFixedAddBranchOutput const &output,
        std::ostream &os) const;

protected:
    std::vector<bool> m_multToMinimize;
    std::vector<int> m_listWithMultApproxToMinimize;
};



#endif //BESTPARTIALMINMULTFIXEDADD_H
