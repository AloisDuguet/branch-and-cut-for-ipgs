## Results

This file describes how the raw numerical results are organized.

Each folder inside the folder results contains the numerical results from the experiments on one type of games:
- integer NEP knapsack games in `NEPKnapsackFullInteger`
- mixed-integer NEP knapsack games in `NEPKnapsackMixedInteger`
- GNEP knapsack games in `GNEPKnapsack`
- implementation games in `ImplementationGames`
- integer NEP with quadratic objectives in `NEPDiscreteSchwarze23_BranchAndPrune_BranchAndCut`

Each one of those folders is organized in a similar way with:
- one or two result files in .csv format, one for each method used to tackle the corresponding instances
- a .r file that produces figures and statistics presented in the article

More precisely, in the result file:
- the first line gives a name to each column of the csv file
- each other line describes the important information on the instance (options of the algorithm + numerical results)
