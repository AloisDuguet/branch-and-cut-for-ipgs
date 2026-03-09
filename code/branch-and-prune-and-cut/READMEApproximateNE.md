# Computation of approximate Nash equilibria

## Running

In a terminal, use the following command:<br>
`./<main> <problemVariant> <instance> <approxNEParameters> <gameType> <algorithmVariant> <cutType> <globalCut> <enumerateNE> <modelType> <branchingRule> <timeLimit> <verbosity> <resultFilename>`<br>
<br>
where:
- `main` is the path to the executable approxNEMain
- `problemVariant` is the type of approximate NE to produce
    possible values: 
    - `multiplicativeNE` for multiplicative approximate NE,
    - `addMultNE` for additive and multiplicative approximate NE, 
    - `minMultiplicativeNE` for finding the minimum alpha such that there exists an alpha-NE with alpha a multiplicative approximation, 
    - `minMultFixedAddNE` for finding the minimum alpha such that there exists an (alpha,beta)-NE with alpha a multiplicative approximation and beta an additive approximation
- `instance` is the path to the instance including the name of the instance
- `approxNEParameters` gives the numerical values describing an approximate NE, for example the multiplicative approximation alpha and the additive approximation beta. For each of these potential two arguments, the value for each player is given in a csv fashion: `2,3,5,8` meaning 2 for the first player, 3 for the second, 5 for the third and 8 for the fourth. Another possibility for each argument is to give only one numeric value: `4` which will then be the value for each player.
    possible values:
    - `alpha` for problemVariant == `multiplicativeNE`
    - `alpha-beta` for problemVariant == `addMultNE`, alpha describing the multiplicative approximation followed by beta describing the additive approximation
    - `alphaStart` for problemVariant == `minMultiplicativeNE`, to start the search for min alpha from value alphaStart
    - `alphaStart-beta` for problemVariant == `minMultFixedAddNE`, to start the search for min alpha from value alphaStart, with additive approximation fixed to beta
    examples:
    - `2,2,3` for problemVariant == `multiplicativeNE` and three players
    - `2-10,20` for problemVariant == `addMultNE` and two players
    - `4` for problemVariant == `minMultiplicativeNE`, 4 for each player
    - `3,2-5,10` for problemVariant == `minMultFixedAddNE` and two players
- `gameType` is the name of the type of game of the instance, so that the correct parser is called to produce the (G)NEP<br>
    possible values: `NEP-fullInteger`, `NEP-mixedInteger`, `GNEP-fullInteger`, `implementationGame`, `NEPdiscrete-Schwarze23`, `NEPImplementationGame`, ...
- `algorithmVariant` is the variant of the algorithm to use<br>
    possible values: 
    - `basicAlgorithm` is mandatory if `problemVariant` is either `multiplicativeNE` or `addMultNE`
    - `basicAlgorithm` or `reuseTreeSearch` or `reuseWithoutCuts` if `problemVariant` is either `minMultiplicativeNE` or `minMultFixedAddNE`, they respectively corresponds to the simple binary search method, the variant `singleTree+Cuts` and the variant `singleTree` in the preprint
    
- `cutType` is the type of cut to derive. For now only one type of cuts can be derived by the algorithm. Be careful with `intersectionCuts` as the code is not generic. It works only for models in which variable bounds are given as constraints<br>
    possible values: `eqCuts`, `aggEqCuts`, `intersectionCuts`, `noGoodCuts`<br>
    Recommended: `eqCuts` for NEP and `intersectionCuts` for GNEP
- `globalCut` indicates if the cuts are added only in the subtree (local cuts) or everywhere in the branching tree (global cuts)<br>
    possible values: `1` for global cuts, `0` for local cuts
- `enumerateNE` indicates if one wants to compute one NE and stop or compute all NEs of the instance. For now, only computing one NE has been implemented, there is also no theoretical results about computing all NEs discussed in the preprint<br>
    mandatory value: `0` for stopping after the first NE is found
- `modelType` is the type of model of the node problem. The model discussed everywhere is `manyEtasModel` while a second model is only discussed in Remark 3.2<br>
    mandatory value: `manyEtasGurobi`
- `branchingRule` is the type of rule used to branch.<br>
    possible values: `mostFractional`, `firstFractional`, `random`
- `timeLimit` is the number of seconds given to the algorithm to return a solution<br>
    possible values: any positive value
- `verbosity` controls the amount of information printed. The higher the number, the more information is displayed<br>
    possible values:
    - `0` for printing only the model and the output
    - `1` for a print every 1000 nodes and each time a node is solved with the total number of cuts derived being a multiple of 100
    - `2` for adding few node information like node number, if cuts were added in the node, pruning situations, optimal value of problems
    - `3` for more node information like solution of problems and cuts derived
    - `4` for some debug information
    - `5` for more debug information
- `resultFilename` indicates in which file the one-line result of the solve is written.<br>
    possible values: `noWriteResult` to not write the one-line result in a file, any other value writes in file `resultFilename`

### Forbidden parameters or combinations

Some combinations of parameters are not allowed, such as `eqCuts` and `GNEP-fullInteger` because the equilibrium cuts are only valid in NEP. Pay attention to the choice of `gameType`, `cutType`, `globalCut` and `modelType` for such combinations.
Many forbidden combinations have been coded so that an error is issued if it is used. You can find in function `checkValidArguments` of files `BranchAndCutForGNEP.cpp` and `BranchAndCutMultiplicativeGNEP.cpp` a list of forbidden parameters or pairs of parameters and why they are forbidden.

### Preferred combinations

In general:<br>
`algorithmVariant` = "basicAlgorithm"<br>
`modelType` = "manyEtasModel"<br>
`enumerateNE` = false

For mixed-integer and integer NEPs:<br>
`cutType` = "eqCuts"<br>
`globalCut` = true

For integer GNEPs (mandatory parameters):<br>
`cutType` = "intersectionCuts"<br>
`globalCut` = false

### Examples

Compute a multiplicative-additive approximate NE of a NEP implementation game:<br>
Open a terminal in folder `branch-and-cut-for-ipgs/code/branch-and-prune-and-cut` and run the following command line<br>
`./build/approxNEmain addMultNE ../../instances/implementation_games/I_2_10_mm_10_10.txt 4-25 NEPImplementationGame basicAlgorithm eqCuts 1 0 manyEtasGurobi mostFractional 3600 1 noWriteResult`

Compute a minimum value alpha such that there exists a multiplicative alpha-NE to a NEP implementation game:<br>
Open a terminal in folder `branch-and-cut-for-ipgs/code/branch-and-prune-and-cut` and run the following command line<br>
`./build/approxNEmain minMultiplicativeNE ../../instances/implementation_games/I_2_10_mm_10_10.txt 4-25 NEPImplementationGame reuseWithoutCuts eqCuts 1 0 manyEtasGurobi mostFractional 3600 1 noWriteResult`

### Using new game models

The workflow to use the branch-and-cut algorithm is the following:
1. instantiate an object of class `NashEquilibriumProblem` corresponding to the game model as defined in file `NashEquilibriumProblem.h`,
2. instantiate an object of class `shared_ptr<NashEquilibriumProblem>` from the `NashEquilibriumProblem` instance,
3. instantiate an object of the class wanted, cf `approxNEMain.cpp` to associate problemVariant to the class name, with the `shared_ptr<NashEquilibriumProblem>` together with other parameters as required to call the constructor,
4. run the solve method of the corresponding class.

An example of this procedure is implemented in file `approxNEMain.cpp` where the game model is produced via function `buildGameFromInstance`, defined in file `BuildGameFromInstance.cpp`.
Function `buildGameFromInstance` produces a game model from an instance used in the preprint or from a C++ function.

If one wants to use a game model that is not in an instance, one can either:
- write an instance in the appropriate file format if there is already a parser, cf folder `Instances` for examples, or
- implement its custom parser as well as the instance file, cf `Parsers/GameParserAndBuilder.cpp` for examples, or
- directly implement an instance in C++, cf file `TestInstances.h` for examples.

After that, it suffices to call the function building an instance of class `shared_ptr<NashEquilibriumProblem>` related to the instance file wanted and do steps 3 and 4.

## Experiments of the preprint [^2]

There was one experiment in the preprint which is with NEP implementation games. To reproduce the results, from inside folder `branch-and-cut-for-ipgs/code/branch-and-prune-and-cut` do:
For each instance in folder `../../instances/implementation_games` and each variant in {`basicAlgorithm`, `reuseTreeSearch`, `reuseWithoutCuts`} launch command<br>
`./cmake-build-debug/approxNEmain minMultiplicativeNE <instance> 10 NEPImplementationGame <variant> eqCuts 1 0 manyEtasGurobi mostFractional 3600 1 results.txt`

[^2]: Duguet, A., T. Harks, M. Schmidt, and J. Schwarz (2025). Branch-and-Cut for Approximate Equilibria of Mixed-Integer Generalized Nash Games. Tech. rep. url: https://optimization-online.org/?p=32468.
