//
// Created by alois-duguet on 7/8/26.
//

#ifndef MIBSFORGNEP_H
#define MIBSFORGNEP_H
#include "BranchAndCutForGNEP.h"

struct MibSSolutionInfo {
    std::string optimizationStatus;
    double cost;
    std::vector<double> solution;
    int numberOfNodes;
    int numberOfCuts;
    double timeOfCuts;
};

class MibSForGNEP : public BranchAndCutForGNEP {
public:
    explicit MibSForGNEP(
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

    // build bilevel model, solve it via MibS and extract solution as a NEPBranchOutput*
    NEPBranchOutput* solve() override;

    NEPBranchOutput* returnHandler(
        std::chrono::time_point<std::chrono::system_clock> startTime,
        std::string errorMsg = "no error") override;

    // parse log and solution file of MibS for important information
    MibSSolutionInfo parseMibSSolutionFile() const;

protected:
    std::string m_mibsInstanceFilename;
    std::string m_mibsSolutionFilename;
    std::string m_mibsLogFilename;
};



#endif //MIBSFORGNEP_H
