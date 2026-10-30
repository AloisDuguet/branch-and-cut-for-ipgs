# Results of [^1]

This file describes how the raw numerical results are organized.

Each folder contains the numerical results from one experiment:
- integer NEP knapsack games in Section 5.4.1 in `NEPKnapsack-fullInteger`
- mixed-integer NEP knapsack games in Section 5.4.1 in `NEPKnapsack-mixedInteger`
- GNEP knapsack games in Section 5.4.2 in `GNEPKnapsack`
- implementation games in Section 5.4.3 in `ImplementationGames`
- knapsack games of Dragotto and Scatamacchia (2023) in Section 5.5.1 in `DragottoKnapsack`
- integer NEP with quadratic objective functions in Section 5.5.2 in `NEPQuadraticSchwarze`
- comparison between Branch-and-Cut, bilevel solver and with non-tailored cuts in Section 5.5.3 in `GNEPknapsackMethodComparison`
- comparison between Branch-and-Cut, bilevel solver and with non-tailored cuts in Section 5.5.3 in `NEPfullIntegerMethodComparison`


Each one of those folders is organized in a similar way with:
- one to three result files in .csv format, one for each method used to tackle the corresponding instances
- a .r file that produces figures and statistics presented in the article

More precisely, in the result file:
- the first line gives a name to each column of the csv file
- each other line describes the important information on the corresponding instance (options of the algorithm + numerical results)

[^1]: Duguet, A., T. Harks, M. Schmidt, and J. Schwarz (2026). Branch-and-Cut for Mixed-Integer Nash Equilibrium Problems. Tech. rep. url: https://optimization-online.org/?p=30556.
