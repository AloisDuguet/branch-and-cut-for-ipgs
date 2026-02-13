cmake_minimum_required(VERSION 3.26)
project(BranchAndCutTools)

add_library(BranchAndCutTools
        BranchAndCutTools.h
        BranchingRule/BranchingRule.cpp
        BranchingRule/BranchingRule.h
        BranchingRule/BranchingRuleFirstFractional.cpp
        BranchingRule/BranchingRuleFirstFractional.h
        BranchingRule/BranchingRuleMostFractional.h
        BranchingRule/BranchingRuleMostFractional.cpp
        BranchingRule/BranchingRuleRandom.h
        BranchingRule/BranchingRuleRandom.cpp
        Helpers/CommonlyUsedFunctions.h
        Helpers/CommonlyUsedFunctions.cpp
        Helpers/CutInformation.cpp
        Helpers/CutInformation.h
        LinearAlgebra/MySparseVector.h
        LinearAlgebra/MySparseVector.cpp
        LinearAlgebra/MySparseMatrix.h
        LinearAlgebra/MySparseMatrix.cpp
        Node/Node.cpp
        Node/Node.h
        NodeSelection/NodeSelector.cpp
        NodeSelection/NodeSelector.h
        NodeSelection/DFS.cpp
        NodeSelection/DFS.h
        OptimizationProblem/NashEquilibriumProblem.cpp
        OptimizationProblem/NashEquilibriumProblem.h
        OptimizationProblem/OptimizationProblem.cpp
        OptimizationProblem/OptimizationProblem.h
        OptimizationProblem/Constraint.cpp
        OptimizationProblem/Constraint.h
        OptimizationProblem/NEPAbstractSolver.h
        OptimizationProblem/NEPAbstractSolver.cpp
        OptimizationProblem/SolveOutput.cpp
        OptimizationProblem/SolveOutput.h
)

set(CMAKE_CXX_STANDARD 20)

# add Gurobi
set(GUROBI_INCLUDE_DIRS $ENV{GUROBI_HOME}/include/)
# add folder of FindGUROBI.cmake to CMAKE_MODULE_PATH
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")

find_package(GUROBI REQUIRED)
target_link_libraries(branch_and_prune_and_cut PRIVATE gurobi)

target_include_directories(BranchAndCutTools PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        BranchingRule/
        Helpers/
        LinearAlgebra/
        Node/
        NodeSelection/
        OptimizationProblem/
        ${GUROBI_INCLUDE_DIRS}
)
