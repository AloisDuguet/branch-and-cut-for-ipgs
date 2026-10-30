cmake_minimum_required(VERSION 3.26)
project(branch_and_prune_and_cut)

set(CMAKE_CXX_STANDARD 20)

add_library(branch_and_prune_and_cut
        Algorithms/BranchAndCutForGNEP.h
        Algorithms/BranchAndCutForGNEP.cpp
        BuildGameFromInstance/BuildGameFromInstance.cpp
        BuildGameFromInstance/BuildGameFromInstance.h
        InstanceGenerator/GenerateToyInstanceWithErrors.cpp
        Parsers/GameParserAndBuilder.h
        Parsers/GameParserAndBuilder.cpp
        SolverSpecific/NEPManyEtasGurobi.h
        SolverSpecific/NEPManyEtasGurobi.cpp
        SolverSpecific/NodeGurobi.cpp
        SolverSpecific/NodeGurobi.h
        Tests/TestInstances.h
        Tests/TestInstances.cpp
        Helpers/intersectionRayHalfspace.cpp
        Helpers/intersectionRayHalfspace.h
        SolverSpecific/NEPMultiplicativeApprox.cpp
        SolverSpecific/NEPMultiplicativeApprox.h
        Algorithms/BranchAndCutMultiplicativeGNEP.cpp
        Algorithms/BranchAndCutMultiplicativeGNEP.h
        Algorithms/BestMultiplicativeApproxGNEP.cpp
        Algorithms/BestMultiplicativeApproxGNEP.h
        Algorithms/BCMultiplicativeAdditiveApproxGNEP.cpp
        Algorithms/BCMultiplicativeAdditiveApproxGNEP.h
        SolverSpecific/NEPMultAddApprox.cpp
        SolverSpecific/NEPMultAddApprox.h
        Helpers/NEPBranchOutput.h
        Helpers/NEPBranchOutput.cpp
        Helpers/MinNEPBranchOutput.cpp
        Helpers/MinNEPBranchOutput.h
        Helpers/BestMultiplicativeBranchOutput.cpp
        Helpers/BestMultiplicativeBranchOutput.h
        Algorithms/BestMultiplicativeAdditiveApproxGNEP.cpp
        Algorithms/BestMultiplicativeAdditiveApproxGNEP.h
        Helpers/BestMultiplicativeAdditiveBranchOutput.cpp
        Helpers/BestMultiplicativeAdditiveBranchOutput.h
        Algorithms/BestPartialMinMultFixedAdd.cpp
        Algorithms/BestPartialMinMultFixedAdd.h
        Helpers/BestPartialMinMultFixedAddBranchOutput.cpp
        Helpers/BestPartialMinMultFixedAddBranchOutput.h
        Algorithms/BestAdditiveMultiplicativeApproxGNEP.cpp
        Algorithms/BestAdditiveMultiplicativeApproxGNEP.h
        Helpers/BestAdditiveMultiplicativeBranchOutput.cpp
        Helpers/BestAdditiveMultiplicativeBranchOutput.h
        Helpers/LipschitzComputation.cpp
        Helpers/LipschitzComputation.h
        Algorithms/MibSForGNEP.cpp
        Algorithms/MibSForGNEP.h
)

add_subdirectory(BranchAndCutTools)
add_subdirectory(InstanceGenerator)

set(GUROBI_INCLUDE_DIRS $ENV{GUROBI_HOME}/include/)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

# how to add a library:
# 1 - find_package() to locate the folder with C++ files
# 2 - write Find{PACKAGE}.cmake so that find_package() finds the folder
# 3 - target_link_libraries() to link the library with my project
find_package(GUROBI REQUIRED)
find_package(EIGEN REQUIRED)

message("GUROBI_INCLUDE_DIRS: ${GUROBI_INCLUDE_DIRS}")

# CLion specific comment:
# GUROBI_INCLUDE_DIRS is a "CMake variable"
# a "$ENV{...}" is an environment variable defined in CLion in settings>Build,Execution,Deployment>CMake>environment
# it works as an environment variable in a .bashrc, but CLion protects their use so that we need to define them explicitly in this way
message("--> ${GUROBI_LIBRARY}")
message("--> $ENV{GUROBI_LIBRARY}")
message("--> ${GUROBI_CXX_LIBRARY}")
message("--> $ENV{GUROBI_CXX_LIBRARY}")


message("CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(branch_and_prune_and_cut PUBLIC
        ${GUROBI_INCLUDE_DIRS}
        Algorithms/
        BuildGameFromInstance/
        Helpers/
        InstanceGenerator/
        Parsers/
        SolverSpecific/
        Tests/
)
target_link_libraries(branch_and_prune_and_cut PRIVATE $ENV{GUROBI_CXX_LIBRARY})
target_link_libraries(branch_and_prune_and_cut PRIVATE $ENV{GUROBI_LIBRARY})

add_executable(main main.cpp)

# adding Eigen to the list of libraries to do the "-I path/to/Eigen" option of compilation
# PUBLIC so that if main uses eigen, it still knows it
target_link_libraries(branch_and_prune_and_cut PUBLIC EIGEN)
target_link_libraries(branch_and_prune_and_cut PUBLIC BranchAndCutTools)
target_link_libraries(main PRIVATE branch_and_prune_and_cut)

# add executable approxNEmain for approx NE experiments
add_executable(approxNEmain approxNEMain.cpp)

target_link_libraries(approxNEmain PRIVATE branch_and_prune_and_cut)

# add executable generateGNEPRandomInstances
add_executable(generateGNEPRandomInstances Parsers/generateGNEPRandomInstances.cpp)

# using library Catch2 to make tests
Include(FetchContent)
FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.3.2 # or a later release
        GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(Catch2)

add_executable(tests Tests/EasyNEP.test.cpp
        Tests/IntersectionCutTests.cpp)
target_link_libraries(tests PRIVATE branch_and_prune_and_cut)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)