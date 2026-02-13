//
// Created by alois-duguet on 10/6/25.
//

#ifndef BESTMULTIPLICATIVEAPPROXGNEP_H
#define BESTMULTIPLICATIVEAPPROXGNEP_H

#include "BranchAndCutMultiplicativeGNEP.h"

#include "BestMultiplicativeBranchOutput.h"

class BestMultiplicativeApproxGNEP : virtual public BranchAndCutMultiplicativeGNEP {
public:
    BestMultiplicativeApproxGNEP(
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

    // print basic information on instance
    void printParameters(std::ostream& os) const override;

    // set approximation for each player to value
    void setApproximation(double approximationValue) const;

    // get approximation (which is the same for each player)
    double getApproximation() const;

    // compute minimum additive delta-NE
    BestMultiplicativeBranchOutput* solve() override;

    // while loop of the solve function
    void whileLoopSolve(std::chrono::time_point<std::chrono::system_clock> startTime);

    // check if current integral node solution is an approximate NE
    bool checkNE(std::vector<double> const& solution) const;

    // compute minimum factor such that a (minFactor,additiveApprox)-NE has been found
    [[nodiscard]] virtual double computeNEWithMinMultiplicativeAndFixedAdditive(
        std::vector<double> const& solution,
        int verbosity) const;

    // rewrite NEHandler to return false if there is hope to find a better delta, else true
    bool NEHandler(std::vector<double> const &solution) override;

    // print information when an NE has been found
    virtual void logNEFound(double maxRegret) const;

    // check if binary search on approximation value can continue and handle the changes
    bool checkAndChangeApproximation(double maxRegret);

    // if no new delta-NE with a smaller delta is found,
    // retrieve queue state from last NE and start again
    // search with a bigger delta
    void noNEHandler();

    // print message saying that no NE with current approximation was found
    virtual void logNoNEFound() const;

    // rebuild root node and empty nodeSelector object with current solution list
    std::shared_ptr<Node> resetBranchingWithSolutionList();

    // recover queue state
    void recoverQueueState();

    // adapt constraints involving lambda after approximation value changed
    void adaptLambdaConstraints(bool removeCuts = false) const;

    // override the base method to handle out of memory in subsolver via branching
    void unsolvedNodeHandler(std::chrono::time_point<std::chrono::system_clock> startTime);

    // add outputs to the return
    BestMultiplicativeBranchOutput* returnHandler(
        std::chrono::time_point<std::chrono::system_clock> startTime,
        std::string errorMsg = "no error") override;

    // handle where to write the one-line result
    void oneLineResultHandler(BestMultiplicativeBranchOutput const& output) const;

    // prints output of the algorithm in one line
    void writeOneLineResult(
        BestMultiplicativeBranchOutput const &output, std::ostream &os) const;

    // build string optimizationStatus for the return of solve
    std::string buildOptimizationStatusString(
        std::chrono::time_point<std::chrono::system_clock> startTime,
        std::string errorMsg = "no error");

protected:
    // name of problem solved with initial approximation value
    std::string m_tagProblemPlusInitialApproximationValue;

    // minimum and maximum values of approximation factor possible for the moment
    double m_minApproximationPossible;
    double m_bestApproximationFound;

    // stopping criterion for the tolerance on the bounds on approximation factor
    double m_toleranceMinApproximation;

    // true if an exact NE is possible,
    // false if it was proved that there is no exact NE
    bool m_exactNEPossible;

    // number of cuts dropped when restarting the search of a factor-NE
    int m_currentNumberOfCutsDropped;

    // queueState will hold deep copies of the unexplored nodes when a delta'-NE
    // is searched after a delta-NE has been found
    // hiddenNodes will contain deep copies of the hidden nodes of
    // the tree structure of the branch and cut
    // together, queueState and hiddenNodes are meant to 'restart' the search
    // in the tree if no delta'-NE is found, but an intermediate delta"-NE is searched
    std::unique_ptr<NodeSelector> m_queueState;

    // value of delta for last computation of a delta-NE that found one
    // it corresponds to the rhs of the cuts saved in queue state
    double m_approximationValueOfLambdaConstraintsInQueueState;
};



#endif //BESTMULTIPLICATIVEAPPROXGNEP_H
