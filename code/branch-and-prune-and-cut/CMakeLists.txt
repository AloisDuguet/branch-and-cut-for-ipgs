cmake_minimum_required(VERSION 3.26)
project(branch_and_prune_and_cut)

set(CMAKE_CXX_STANDARD 20)

add_library(branch_and_prune_and_cut
        Algorithms/BranchAndCutForGNEP.h
        Algorithms/BranchAndCutForGNEP.cpp
        Algorithms/BranchAndCutMultiplicativeGNEP.cpp
        Algorithms/BranchAndCutMultiplicativeGNEP.h
        Algorithms/BestMultiplicativeApproxGNEP.cpp
        Algorithms/BestMultiplicativeApproxGNEP.h
        Algorithms/BCMultiplicativeAdditiveApproxGNEP.cpp
        Algorithms/BCMultiplicativeAdditiveApproxGNEP.h
        Algorithms/BestMultiplicativeAdditiveApproxGNEP.cpp
        Algorithms/BestMultiplicativeAdditiveApproxGNEP.h
        BuildGameFromInstance/BuildGameFromInstance.cpp
        BuildGameFromInstance/BuildGameFromInstance.h
        Helpers/NEPBranchOutput.h
        Helpers/NEPBranchOutput.cpp
        Helpers/MinNEPBranchOutput.cpp
        Helpers/MinNEPBranchOutput.h
        Helpers/BestMultiplicativeBranchOutput.cpp
        Helpers/BestMultiplicativeBranchOutput.h
        Helpers/BestMultiplicativeAdditiveBranchOutput.cpp
        Helpers/BestMultiplicativeAdditiveBranchOutput.h
        Helpers/intersectionRayHalfspace.cpp
        Helpers/intersectionRayHalfspace.h
        Parsers/GameParserAndBuilder.h
        Parsers/GameParserAndBuilder.cpp
        SolverSpecific/NEPManyEtasGurobi.h
        SolverSpecific/NEPManyEtasGurobi.cpp
        SolverSpecific/NodeGurobi.cpp
        SolverSpecific/NodeGurobi.h
        SolverSpecific/NEPMultiplicativeApprox.cpp
        SolverSpecific/NEPMultiplicativeApprox.h
        SolverSpecific/NEPMultAddApprox.cpp
        SolverSpecific/NEPMultAddApprox.h
        Tests/TestInstances.h
        Tests/TestInstances.cpp
)

add_subdirectory(BranchAndCutTools)

set(GUROBI_INCLUDE_DIRS $ENV{GUROBI_HOME}/include/)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

find_package(GUROBI REQUIRED)
find_package(EIGEN REQUIRED)

target_include_directories(branch_and_prune_and_cut PUBLIC
        ${GUROBI_INCLUDE_DIRS}
        Algorithms/
        BuildGameFromInstance/
        Helpers/
        Parsers/
        SolverSpecific/
        Tests/
)

target_link_libraries(branch_and_prune_and_cut PRIVATE gurobi)

# add executable main for exact NE experiments
add_executable(main main.cpp)

target_link_libraries(branch_and_prune_and_cut PUBLIC EIGEN)
target_link_libraries(branch_and_prune_and_cut PUBLIC BranchAndCutTools)
target_link_libraries(main PRIVATE branch_and_prune_and_cut)

# add executable approxNEmain for approx NE experiments
add_executable(approxNEmain approxNEMain.cpp)

target_link_libraries(approxNEmain PRIVATE branch_and_prune_and_cut)

# using library Catch2 to make tests
Include(FetchContent)
FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.3.2 # or a later release
        GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(Catch2)

add_executable(tests Tests/EasyNEP.test.cpp)
target_link_libraries(tests PRIVATE branch_and_prune_and_cut)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)
