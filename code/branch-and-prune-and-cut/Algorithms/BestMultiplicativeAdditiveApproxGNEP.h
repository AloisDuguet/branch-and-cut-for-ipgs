//
// Created by alois-duguet on 12/3/25.
//

#ifndef BESTMULTIPLICATIVEADDITIVEAPPROXGNEP_H
#define BESTMULTIPLICATIVEADDITIVEAPPROXGNEP_H

#include "BCMultiplicativeAdditiveApproxGNEP.h"
#include "BestMultiplicativeApproxGNEP.h"

#include "BestMultiplicativeAdditiveBranchOutput.h"

class BestMultiplicativeAdditiveApproxGNEP :
                                        public BCMultiplicativeAdditiveApproxGNEP,
                                        public BestMultiplicativeApproxGNEP {
public:
    BestMultiplicativeAdditiveApproxGNEP(
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

    // print basic information on instance
    void printParameters(std::ostream& os) const override;

    // compute minimum additive delta-NE
    BestMultiplicativeAdditiveBranchOutput* solve() override;

    // rewrite NEHandler to return false if there is hope to find a better delta, else true
    bool NEHandler(std::vector<double> const &solution) override;

    // print information when an NE has been found
    void logNEFound(double maxRegret) const override;

    // print message saying that no NE with current approximation was found
    void logNoNEFound() const override;

    // add outputs to the return
    BestMultiplicativeAdditiveBranchOutput* returnHandler(
        std::chrono::time_point<std::chrono::system_clock> startTime,
        std::string errorMsg = "no error") override;

    // handle where to write the one-line result
    void oneLineResultHandler(BestMultiplicativeAdditiveBranchOutput const& output) const;

    // prints output of the algorithm in one line
    void writeOneLineResult(
        BestMultiplicativeAdditiveBranchOutput const &output,
        std::ostream &os) const;
};



#endif //BESTMULTIPLICATIVEADDITIVEAPPROXGNEP_H
