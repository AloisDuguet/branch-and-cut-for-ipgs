//
// Created by alois-duguet on 11/13/25.
//

#ifndef BCMULTIPLICATIVEADDITIVEAPPROXGNEP_H
#define BCMULTIPLICATIVEADDITIVEAPPROXGNEP_H

#include "BranchAndCutMultiplicativeGNEP.h"

class BCMultiplicativeAdditiveApproxGNEP : virtual public BranchAndCutMultiplicativeGNEP {
public:
    // constructor with adapted approx parameters and derived NEP class
    BCMultiplicativeAdditiveApproxGNEP(
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

    // compute additive delta-NE
    NEPBranchOutput* solve() override;

    // print approximation value when delta-NE found
    bool NEHandler(std::vector<double> const& solution) override;

    // write one-line result with algorithm
    void writeOneLineResult(NEPBranchOutput const& output, std::ostream& os) const override;

    // write logs when an NE is found
    void logNEFound(std::vector<double> const& solution);
};



#endif //BCMULTIPLICATIVEADDITIVEAPPROXGNEP_H
