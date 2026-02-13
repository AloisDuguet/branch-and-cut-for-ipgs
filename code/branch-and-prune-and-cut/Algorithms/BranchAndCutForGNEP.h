//
// Created by Aloïs Duguet on 11/12/24.
//

#ifndef BRANCHANDCUTFORGNEP_H
#define BRANCHANDCUTFORGNEP_H

#include <vector>
#include <memory>
#include <chrono>
#include <bits/stdc++.h>

#include "NashEquilibriumProblem.h"
#include "BranchAndCutTools.h"
#include "NEPBranchOutput.h"

#define checkTime(startTime) if (!checkTimeLimit(startTime)) break;

class BranchAndCutForGNEP {
public:
    explicit BranchAndCutForGNEP(
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

    virtual ~BranchAndCutForGNEP() = default;

    // initialize some attributes that are not initialized in
    // the same way depending on the real algorithm class
    virtual void initialize();

    // print information on instance
    virtual void printParameters(std::ostream& os) const;

    // check that the set of arguments is valid
    virtual void checkValidArguments() const;

    virtual NEPBranchOutput* solve();

    // using m_reuseNode, attribute or not a new node to m_node
    void getNewNode();

    // print node number and cut number for the current node
    void printNodeInformation(double timeSpent) const;

    // solve current node problem
    void virtual solveNode(double solverTimeLimit);

    // handles when the current node problem was not solved
    virtual void unsolvedNodeHandler();

    // compute best responses
    [[nodiscard]] std::vector<double> computeBestResponseHandler(double solverTimeLimit);

    // handles when an NE is found
    virtual bool NEHandler(std::vector<double> const& solution);

    // detect when the algorithm is cycling because of same solution
    // in the last two iterations of the same node
    virtual bool detectCycling(std::vector<double> const& solution);

    // try to produce cuts and add them to m_node
    virtual void addCuts(std::vector<double> const& solution);

    // handle cycling by slightly modifying the new cuts
    void handleCycling(std::vector<Cut> &cuts, std::vector<double> const& solution);

    // check validity of cuts derived and add the valid ones to the node problem
    void checkCutsValidity(std::vector<Constraint> const& cuts, std::vector<double> const& solution);

    // print number of cuts derived
    void logCutToAdd(std::vector<Cut> const& cuts) const;

    // print that a cut was not added
    virtual void logCutNotAdded() const;

    // print information when no cut were derived
    virtual void logNoCutAdded() const;

    // handles branching
    void branchingHandler() const;

    // return a filename with all options and appropriate date and time
    [[nodiscard]] std::string buildFilenameWithOptions(bool withDateAndTime = true) const;

    // write root node
    void writeRootNode() const;

    // write NEP and solutions found in logFiles/m_instanceName
    void writeLog(NEPBranchOutput const& NEPOutput) const;

    // write cuts added with node number and cut number in logCutFiles/m_instanceName
    void writeLogCut(std::vector<Cut> const& cuts) const;

    // write cycling issues in logFiles/m_instanceName
    void writeLogCycling(std::vector<Cut> const& cuts,
                         std::vector<double> const& solution) const;

    // copy current node solution for use in the next iteration
    void updateLastNodeSolution();

    // handle where to write the one-line result
    void oneLineResultHandler(NEPBranchOutput const& output) const;

    // write a one-line result in a file
    virtual void writeOneLineResult(NEPBranchOutput const& output, std::ostream& os) const;

    // write the end of a one-line result in a file
    void writeEndOfOneLineResult(NEPBranchOutput const& output, std::ostream& os) const;

    // build the return of method solve, write log and print informations to console
    virtual NEPBranchOutput* returnHandler(
        std::chrono::time_point<std::chrono::system_clock> startTime,
        std::string errorMsg = "no error");

    // computes social welfare.
    // it should be called only if current node problem has been solved
    [[nodiscard]] double computeSocialWelfare(SolveOutput const& output) const;

    void printSocialWelfare(SolveOutput const& output) const;

    // stop the algorithm with a one-line result if time limit reached
    [[nodiscard]] bool checkTimeLimit(
        std::chrono::time_point<std::chrono::system_clock> const& startTime) const;

    [[nodiscard]] double computeSolverTimeLimit(
        std::chrono::time_point<std::chrono::high_resolution_clock> const& startTime) const;

    // reset static count of node explored of nodeSelector. Do not use during solve without being sure of what it does
    void resetExploredNodeNumber() const;

    // check that for a specific instance, a valid NE is not cut off by a cut derived
    void checkValidNENotCutExactGNEPImplementationGames(std::vector<Cut> const& cuts);

    void checkPointNotCutSpecificInstance(
        std::string const& instanceName,
        std::vector<double> const& point,
        std::vector<Cut> const& cuts);

    // check that a point is not cut off by a group of cuts
    void checkPointNotCut(std::vector<Cut> const& cuts,
                          std::vector<double> const& point);

protected:
    // attributes related to branching
    // std::shared_ptr<OptimizationProblem> m_rootNodeProblem;
    std::shared_ptr<Node> m_rootNode;
    std::shared_ptr<NEPAbstractSolver> m_NEP;
    std::shared_ptr<Node> m_node;
    std::shared_ptr<NodeSelector> m_nodeSelector;
    std::shared_ptr<BranchingRule> m_branchingRule;

    // ----------------------------------------------------------------
    // attributes related to gurobi models

    std::shared_ptr<GRBEnv> m_globalEnv;

    std::shared_ptr<GRBModel> m_nodeModel;

    std::vector<GRBModel> m_bestResponseModels;

    // ----------------------------------------------------------------
    // attributes that are options of the branching structure

    // examples: "NEP-mixedInteger", "NEP-fullInteger",
    // "GNEP-fullInteger", "implementationGame"
    std::string m_gameTypeString;

    bool m_globalCutSwitch;

    // "manyEtasModel" or "oneEtaModel"
    std::string m_modelTypeString;

    // "mostFractional" or "firstFractional" or "random"
    std::string m_branchingRuleString;

    // "eqCuts" or "aggEqCuts"
    std::string m_whichCutString;

    // "basicAlgorithm" or "cuttingBeforeBranching"
    std::string m_algorithmVariant;

    // only matter for "cuttingBeforeBranching" algorithmVariant
    int m_maxNonIntegerCutsPerNode;

    // ----------------------------------------------------------------
    // attributes that are tolerances

    // tolerance of the hat V function under which the profile of strategy is deemed an NE
    double m_NETolerance;

    // tolerance for integrality constraints
    double m_integralityTolerance;

    // tolerance of the objective value of a node problem over which a node is pruned
    double m_pruningZeroObjTolerance;

    // tolerance of the difference between eta_i and best response value
    // over which a cut is derived in some cases
    double m_deriveCutTolerance;

    // feasibility tolerance for constraint used to solve the node problem
    double m_feasibilityTol;

    // ----------------------------------------------------------------
    // attributes related to infos of the algorithm

    double m_timeSpent;
    std::string m_instanceName;

    // print deactivated because of potential errors if multiple threads
    // write at the same time in a file (HPC)
    std::string m_resultFilename;

    int m_totalNumberOfCuts;
    int m_numberOfCutsOfCurrentNode;
    bool m_NEFound;
    bool m_reuseNode;
    bool m_enumerateNE;
    std::vector<double> m_lastIterationSolution;

    // in seconds
    double m_timeLimit;
    std::unordered_map<std::string,double> m_timeCounters;

    // in seconds, corresponds to the absolute time given to
    // operations that do not solve an optimization problem
    double m_bufferTime;

    int m_verbosity;

    // ----------------------------------------------------------------
    // anticycling attributes

    // bool with value false at start and true if cycling detected at least once
    bool m_antiCyclingMeasuresTaken;

    // base bonus to the rhs of a cut causing cycling
    double m_rhsBaseBonusForRestrictedCut;

    // current bonus to the rhs of a cut causing cycling
    double m_rhsBonusForRestrictedCut;

    // factor multiplying the bonus after each failure to solve the cycling
    double m_factorIncreaseForRestrictedCut;
};

#endif //BRANCHANDCUTFORGNEP_H