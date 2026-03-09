# Instances

This file describes how the instances of the different types of games are written to describe a game.

## NEP knapsack instances

Folder `NEP_knapsack_instances` contains one file for each instance of NEP knapsack game.
The C++ parser of such a file is function `parseNEPKnapsack` in file `GameParserAndBuilder.cpp` in folder `code/branch-and-prune-and-cut/Parsers`.

The name of the file indicates the main characteristics of the instance.
The format of a file name is:<br>
`N_m_c_corr_k.txt`<br>
with:
- N is the number of players
- m is the number of items
- c is used to compute the capacity of a player: capacity = c/10*{sum of weights of items of the player}
- corr indicates the type of correlation between weights and profits of items, cf Pisinger's knapsack problem generator [^1]
- k is a counter for instances with the same other characteristics

The information of the instance inside the file is described below.
The two first lines are:<br>
`N m`<br>
`b_1 ... b_N`<br>
with b_i the capacity of player i.<br>
Each following line describes the information related to one object j in such a way:<br>
`j p_1,j	w_1,j	...	p_N,j	w_N,j	C_1,2,j	... C_1,N,j	C_2,1,j ...	C_2,N,j ...	C_N,1,j ...	C_N,N-1,j c_j`<br>
with:
- p_i,j the profit of player j with item j
- w_i,j the weight of player j with item j
- C_i,k,j the interaction coefficient of player i w.r.t. player k and item j for k != i (only useful in NEP knapsack games)
- c_j the number of copy of the item available (only useful in GNEP knapsack games)

The indices of items that are indivisible, leading to integrality constraints, are not written in this file because they depend on the type of NEP knapsack game.
For integer NEP knapsack games, all items are indivisible; for mixed-integer NEP knapsack games, approximately half of the items are indivisible: all items with an even index are indivisible, with the starting index being 0.

[^1]: Silvano, M., D. Pisinger, and P. Toth (1999). "Dynamic Programming and Strong Bounds for the 0-1 Knapsack Problem."" In: Management Science. doi: 10.1287/mnsc.45.3.414.

## GNEP knapsack instances

Folder `GNEP_knapsack_instances` contains one file for each instance of GNEP knapsack games.
The C++ parser of such a file is function `parseNEPKnapsack` in file `GameParserAndBuilder.cpp` in folder `code/branch-and-prune-and-cut/Parsers`.

The name of the file indicates the main characteristics of the instance, with the same name format as for NEP knapsack instances.
The information of the instance is also given as for the NEP knapsack instances.

## Implementation game instances

Folder `implementation_games` contains one file for each instance of NEP knapsack game. 
Those instances are the same as the ones used in [^2] as JCDFG.
The C++ parser of such a file is function `parseImplementationGameGNEP` in file `GameParserAndBuilder.cpp` in folder `code/branch-and-prune-and-cut/Parsers`.

The name of the file indicates the main characteristics of the instance.
The format of a file name is:<br>
`I_n_v_a_b_k.txt`
with: 
- n the number of players
- v the number of nodes of the graph
- a = ss or a = mm indicates if the problem is single- or multi-source/sink (i.e., if there are different sources and sink among players)
- b determines the range 1, ..., b in which the weights of the players have been randomly chosen
- k is a counter for instances with the same other characteristics

The information of the instance inside the file is described below.
The following objects are each described in one line, either in sparse format, or in full format:<br>
- Network (sparse format, rows are nodes and columns are edges of the network)
- Capacities (full format)
- sources/sinks (full format)
- U (full format)
- demands (full format)
- pMax (full format)
- u (full format)

The sparse format has the following format:<br>
`size i1 i2 nnz m value r_1 ... r_m c_1 ... c_m v_1 v_m`<br>
with:
- "size", "nnz" and "value" as keywords
- i1 and i2 the number of rows and columns of the matrix
- m the number of nonzero elements
- r_j the row index of nonzero element j
- c_j the column index of nonzero element j
- v_j the value of nonzero element j

The full format is a plain 2D matrix format, except for sources/sinks.
Indeed, they have a 3D matrix format with a first dimension of size 1, so they are still written as a 2D matrix.
The full format for 2D matrix is as such:<br>
`size i1 i2 value v_1,1 ... v_i1,1 v1,2 ... v_i1,2 ... v_i1,1 ... v_i1,i2`<br>
with:
- "size" and "value" as keywords
- i1 and i2 the number of rows and columns of the matrix
- v_i,j is the value of row i and column j

[^2]: Harks, T. and J. Schwarz (2025). "Generalized Nash equilibrium problems with mixed-integer variables." In: Mathematical Programming 209, pp. 231-277. doi: 10.1007/s10107-024-02063-6.

## integer NEP with quadratic objectives

Folder `Schwarze23DiscreteNEP` contains one file for each instance of NEP knapsack game. 
Those instances are the same as the ones used in [^3].
The C++ parser of such a file is function `parseSchwarze23DiscreteNEPInstance` in file `GameParserAndBuilder.cpp` in folder `code/branch-and-prune-and-cut/Parsers`.

The name of the file indicates the main characteristics of the instance.
The format of a file name is: <br>
`XAB_k`<br>
with:
- X = C if each player's optimization problem is player convex or X = N if not
- A the number of players
- B the number of variables of each player
- k is a counter for instances with the same other characteristics

The information of the instance inside the file is described below.
Each player's optimization problem is described in the order of players.
Lines starting with "#" are comments and thus are not parsed.
Each line describes one object used to build the optimization problem of the corresponding player (i) as a 2D matrix in full format (just as described in implementation games above), with the order:<br>
- A_i
- b_i
- bounds_i
- C_i
- Q_i
- d_i
- conv (not used)\
with:
- A_i x_i <= b_i describing the constraints (x_i the strategy of player i)
- 0.5 x_i^T Q_i x_i + (C_i x_{-i} + d_i) x_i the objective function of player i (x_{-i} the strategies of all other players)
- bounds_i[1,j] <= x_i,j <= bounds_i[2,j] the bounds on the variables of the strategy of player i
- conv a vector. If conv_i = 1, then the objective function of player i is convex. Else it is not convex.

[^3]: Schwarze, S. and O. Stein (2023). "A branch-and-prune algorithm for discrete Nash equilibrium problems." In: Computational Optimization and Applications 86.2, pp. 491-519. doi: 10.1007/s10589-023-00500-4.
