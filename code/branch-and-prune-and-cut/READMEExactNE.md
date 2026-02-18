## Computation of exact Nash equilibria

### Running

In a terminal, use the following command:<br>
`./<main> <instance> <gameType> <algorithmVariant> <cutType> <globalCut> <enumerateNE> <modelType> <branchingRule> <timeLimit> <verbosity> <resultFilename>`<br>
<br>
where:
- `main` is the path to the executable main
- `instance` is the path to the instance including the name of the instance
- `gameType` is the name of the type of game of the instance, so that the correct parser is called to produce the (G)NEP<br>
    possible values: `NEP-fullInteger`, `NEP-mixedInteger`, `GNEP-fullInteger`, `implementationGame`, `NEPdiscrete-Schwarze23`, ...
- `algorithmVariant` is the variant of the algorithm to use<br>
    possible values: `basicAlgorithm`, `cuttingBeforeBranching`<br>
    Recommended: `basicAlgorithm`, because `cuttingBeforeBranching` is less tested, may be less efficient in general, and invalid in some cases like with `cutType` set to `intersectionCuts`
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
- `resultFilename` indicates in which file the one-line result of the solve is written. The relative path to this file starts from the folder of the executable.<br>
    possible values: `noWriteResult` to not write the one-line result in a file, any other value writes in file `resultFilename`

#### Forbidden parameters or combinations

Some combinations of parameters are not allowed, such as `eqCuts` and `GNEP-fullInteger` because the equilibrium cuts are only valid in NEP. Pay attention to the choice of `gameType`, `cutType`, `globalCut` and `modelType` for such combinations.
Many forbidden combinations have been coded so that an error is issued if it is used. You can find in function `checkValidArguments` of file `BranchAndCutForGNEP.cpp` a list of forbidden parameters or pairs of parameters and why they are forbidden.

#### Preferred combinations

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

#### Examples

Integer GNEP:<br>
Open a terminal in folder `branch-and-cut-for-ipgs/code/branch-and-prune-and-cut` and run the following command line<br>
`./build/main ../../instances/GNEP_knapsack_instances/2-5-2-weakcorr-2.txt GNEP-fullInteger basicAlgorithm intersectionCuts 0 0 manyEtasGurobi mostFractional 3600 1 resultFile.txt`

Mixed-integer NEP:<br>
Open a terminal in folder `branch-and-cut-for-ipgs/code/branch-and-prune-and-cut` and run the following command line<br>
`./build/main ../../instances/NEP_knapsack_instances/2-5-5-weakcorr-2.txt NEP-mixedInteger basicAlgorithm eqCuts 1 0 manyEtasGurobi mostFractional 3600 1 noWriteResult`

#### Using new game models

The workflow to use the branch-and-cut algorithm is the following:
1. instantiate an object of class `NashEquilibriumProblem` corresponding to the game model as defined in file `NashEquilibriumProblem.h`,
2. instantiate an object of class `shared_ptr<NashEquilibriumProblem>` from the `NashEquilibriumProblem` instance,
3. instantiate an object of class `BranchAndCutForGNEP` with the `shared_ptr<NashEquilibriumProblem>` together with other parameters as defined in file `BranchAndCutForGNEP.h`,
4. run the solve method of class `BranchAndCutForGNEP`.

An example of this procedure is implemented in file `main.cpp` where the game model is produced via function `buildGameFromInstance`, defined in file `BuildGameFromInstance.cpp`.
Function `buildGameFromInstance` produces a game model from an instance used in the preprint or from a C++ function.

If one wants to use a game model that is not in an instance, one can either:
- write an instance in the appropriate file format if there is already a parser, cf folder `Instances` for examples, or
- implement its custom parser as well as the instance file, cf `Parsers/GameParserAndBuilder.cpp` for examples, or
- directly implement an instance in C++, cf file `TestInstances.h` for examples.

After that, one has to change appropriately function `buildGameFromInstance` so that an instance of class `shared_ptr<NashEquilibriumProblem>` is created with the game model.

### Experiments of the preprint [^1]

There were five experiments in the preprint. To reproduce the results, from inside folder `branch-and-cut-for-ipgs/code/branch-and-prune-and-cut` do:
1. mixed-integer knapsack NEP: for each instance in folder `../../instances/NEP_knapsack_instances` launch command<br>
`./cmake-build-debug/main \<path-to-instance\> NEP-mixedInteger basicAlgorithm eqCuts 1 0 manyEtasGurobi mostFractional 3600 1`

2. integer knapsack NEP: for each instance in folder `../../instances/NEP_knapsack_instances` launch command<br>
`./cmake-build-debug/main \<path-to-instance\> NEP-fullInteger basicAlgorithm eqCuts 1 0 manyEtasGurobi mostFractional 3600 1`

3. integer knapsack GNEP: for each instance in folder `../../instances/GNEP_knapsack_instances` launch command<br>
`./cmake-build-debug/main \<path-to-instance\> GNEP-fullInteger basicAlgorithm intersectionCuts 0 0 manyEtasGurobi mostFractional 3600 1`

4. implementation games (integer GNEP): for each instance in folder `../../instances/implementation_games` launch command<br>
`./cmake-build-debug/main \<path-to-instance\> implementationGame basicAlgorithm intersectionCuts 0 0 manyEtasGurobi mostFractional 3600 1`

5. integer NEP with quadratic objective functions: for each instance in folder `../../instances/Schwarze23DiscreteNEP` launch command<br>
`./cmake-build-debug/main \<path-to-instance\> NEPdiscrete-Schwarze23 basicAlgorithm eqCuts 1 0 manyEtasGurobi mostFractional 3600 1`

[^1]: Duguet, A., T. Harks, M. Schmidt, and J. Schwarz (2025). Branch-and-Cut for Mixed-Integer Generalized Nash Equilibrium Problems. Tech. rep. url: https://optimization-online.org/?p=30556.
