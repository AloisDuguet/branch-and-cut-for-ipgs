## Using the code

This document describes the dependencies used, gives some information on compilation of the code, how to run the code, and how to reproduce the results.

### Dependencies

Compiled and ran with the following dependencies*:

- [CMake](https://cmake.org/) version 3.28.3.
- [GCC](https://gcc.gnu.org/) version 13.3.0.
- [Eigen](https://eigen.tuxfamily.org/dox/GettingStarted.html) version 3.4.0.
- [Gurobi](https://www.gurobi.com/) version 12.0.0.

_* reported versions are not minimal required versions but are the ones that have been tested._

### Compiling

To find Gurobi, you can configure the `GUROBI_HOME` environment variable according to [the official Gurobi guidelines](https://support.gurobi.com/hc/en-us/articles/4534161999889-How-do-I-install-Gurobi-Optimizer-).

CMake and Ninja can be used to build the project with something along the line of<br>
`cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_MAKE_PROGRAM=<path-to-ninja> -DCMAKE_CXX_COMPILER=<path-to-compiler> -S . -B build`<br>
`cmake --build build`
