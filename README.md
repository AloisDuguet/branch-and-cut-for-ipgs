# Branch-and-Cut for Mixed-Integer Generalized Nash Equilibrium Problems

This project contains a C++ implementation of two preprints:
The branch-and-cut approach for exact Nash equilibria described in the preprint [^1] accessible [here](https://optimization-online.org/?p=30556), and
the branch-and-cut approach for approximate Nash equilibria described in the preprint [^2] accessible [here](https://optimization-online.org/?p=32468).
It is able to produce exact and approximate Nash equilibria for specific classes of mixed-integer generalized Nash equilibrium problems.
The implementation continues to be updated. 
Optimization subroutines (QPs and MIQPs) are solved with [Gurobi](https://www.gurobi.com/) using its C++ API.
[^1]: Duguet, A., T. Harks, M. Schmidt, and J. Schwarz (2025). Branch-and-Cut for Mixed-Integer Generalized Nash Equilibrium Problems. Tech. rep. url: https://optimization-online.org/?p=30556.
[^2]: Duguet, A., T. Harks, M. Schmidt, and J. Schwarz (2025). Branch-and-Cut for Approximate Equilibria of Mixed-Integer Generalized Nash Games. Tech. rep. url: https://optimization-online.org/?p=32468.

## License

This code is distributed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.

## Code

To compile the C++ code, see specific [README.md](https://gitlab.uni-trier.de/nlopt/branch-and-cut-for-ipgs/-/blob/master/code/branch-and-prune-and-cut/README.md).
To compute exact Nash equilibria, see specific [READMEExactNE.md](https://gitlab.uni-trier.de/nlopt/branch-and-cut-for-ipgs/-/blob/master/code/branch-and-prune-and-cut/READMEExactNE.md).
To compute approximate Nash equilibria, see specific [READMEApproximateNE.md](https://gitlab.uni-trier.de/nlopt/branch-and-cut-for-ipgs/-/blob/master/code/branch-and-prune-and-cut/READMEApproximateNE.md).

## Instances

See specific [README.md](https://gitlab.uni-trier.de/nlopt/branch-and-cut-for-ipgs/-/blob/master/instances/README.md).

## Numerical results

See specific [README.md](https://gitlab.uni-trier.de/nlopt/branch-and-cut-for-ipgs/-/blob/master/results/README.md).

## Current version of the paper

Compiled [PDF](https://gitlab.uni-trier.de/api/v4/projects/nlopt%2Fbranch-and-cut-for-ipgs/jobs/artifacts/master/raw/paper/branch-and-cut-for-ipgs-preprint.pdf?job=build).
