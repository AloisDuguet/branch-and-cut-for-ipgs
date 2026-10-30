//
// Created by alois-duguet on 10/31/25.
//

#include <catch2/catch_all.hpp>
#include "catch2/catch_test_macros.hpp"
#include "catch2/internal/catch_compiler_capabilities.hpp"
#include "catch2/internal/catch_test_registry.hpp"
#include "catch2/internal/catch_result_type.hpp"
#include "catch2/internal/catch_test_macro_impl.hpp"
#include <memory>

#include "TestInstances.h"
#include "BranchAndCutForGNEP.h"
#include "BranchAndCutTools.h"
#include "BuildGameFromInstance.h"
//#include "EasyNEP.test.cpp"

using namespace std;

/*TEST_CASE("Intersection Cut Test JulianICExample") {
    auto NEP = testGNEPJulianICExample();
    auto branchingAlgorithm = BranchAndCutForGNEP(
        NEP, "JulianICExample",
        false,
        "manyEtasGurobi",
        "intersectionCuts",
        "mostFractional",
        "basicAlgorithm",
        "JulianICExample",
        false);
    auto NEPOutput = *branchingAlgorithm.solve();
    vector<vector<double>> solutionExpected = {{1,0,0,1,0,1}};
    auto instances = NEPTestInstances({{NEP},
        {"SOLUTION_FOUND"},
        {solutionExpected}});
    checkGoodOutput(NEPOutput, instances, 0);
}*/