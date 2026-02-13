//
// Created by alois-duguet on 9/19/25.
//

#ifndef BRANCHANDCUTMULTIPLICATIVEGNEP_H
#define BRANCHANDCUTMULTIPLICATIVEGNEP_H

#include "BranchAndCutForGNEP.h"

class BranchAndCutMultiplicativeGNEP : public BranchAndCutForGNEP {
public:
    BranchAndCutMultiplicativeGNEP(
        std::vector<double> & factor,
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

    // initialize some attributes that are not initialized in
    // the same way depending on the real algorithm class
    void initialize() override;

    // set positive response of NEP with gameTypeString
    void setPositiveResponse();

    // checkValidArguments: interdict intersection cuts
    // as long as the NE-free set is not recomputed
    void checkValidArguments() const override;

    // print basic information on instance
    void printParameters(std::ostream& os) const override;

    // solve node problem
    void solveNode(double solverTimeLimit) override;

    // compute additive delta-NE
    NEPBranchOutput* solve() override;

    // check sign of best response values for some categories of instances
    // because the definition of NE depends on that
    void checkBestResponseValueSigns() const;

    // print approximation value when delta-NE found
    bool NEHandler(std::vector<double> const& solution) override;

    // detect if the previous node solution is close to the current node solution
    bool detectCycling(std::vector<double> const& solution) override;

    // write one-line result with algorithm
    void writeOneLineResult(NEPBranchOutput const& output, std::ostream& os) const override;

    // print that a cut was not added
    void logCutNotAdded() const override;

    // print information when no cut were derived
    void logNoCutAdded() const override;
};



#endif //BRANCHANDCUTMULTIPLICATIVEGNEP_H
