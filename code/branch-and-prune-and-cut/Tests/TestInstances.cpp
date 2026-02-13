//
// Created by Aloïs Duguet on 11/7/24.
//

#include "TestInstances.h"

#include <vector>
#include <cstdlib>
#include <ctime>

#include "GameParserAndBuilder.h"
#include "BranchAndCutTools.h"

using namespace std;

OptimizationProblem testEasyMIQP() {
    // explicit definition of root node
    // which here is chosen because our algorithm would produce two cuts and then find an NE in the third node:
    // min_{x1,x2,eta}  x1x2 - eta
    // s.t.             x1,x2 in [-1,1]
    //                  eta in [-3,3]
    // optimal solutions: {-1,1,3} and {1,-1,3}

    int numberOfVariables = 3;
    MySparseVector c = {{-1.0}, {2}};
    MySparseMatrix Q = MySparseMatrix({1.0}, {1}, {0});
    vector<Constraint> constraints(
        {{{{1.0}, {0}}, {}, 1.0},
        {{{1.0}, {1}}, {}, 1.0},
        {{{1.0}, {2}}, {}, 3.0},
        {{{-1.0}, {0}}, {}, 1.0},
        {{{-1.0}, {1}}, {}, 1.0},
        {{{-1.0}, {2}}, {}, 3.0}});
    vector<indexx> integralityConstraints({1});
    OptimizationProblem rootNodeProblem = OptimizationProblem(numberOfVariables, c, Q, constraints, integralityConstraints);
    return rootNodeProblem;
}

OptimizationProblem testMILP() {
    // min_{x1,x2}  x1 + x2
    // s.t.         x1 in [-0.5,1]
    //              x2 in [-0.5,1]
    // unique optimal solution: [0,0]
    int numberOfVariables = 2;
    MySparseVector c = {{1.0,1.0}, {0,1}};
    MySparseMatrix Q = MySparseMatrix();
    vector<Constraint> constraints(
        {{{{1.0}, {0}}, {}, 1.0},
        {{{1.0}, {1}}, {}, 1.0},
        {{{-1.0}, {0}}, {}, 0.5},
        {{{-1.0}, {1}}, {}, 0.5}});
    vector<indexx> integralityConstraints({0,1});
    OptimizationProblem rootNodeProblem = OptimizationProblem(numberOfVariables, c, Q, constraints, integralityConstraints);
    return rootNodeProblem;
}

shared_ptr<NashEquilibriumProblem> testNEP1() {
    // example of 2-player game with only NE = (0,0)
    vector<int> numberOfVariablesPerPlayer = {1,1};

    // player 1 with variable x1
    // min_x1   x1x2
    // s.t.     -1 <= x1 <= 1
    MySparseVector c1 = {{},{}};
    MySparseMatrix Q1 = MySparseMatrix({-1.0},{1},{0});
    /*vector<Constraint> constraints1({
        {{{1.0},{0}}, {},1.0},
        {{{-1.0},{0}}, {},1.0}});*/
    vector<Constraint> constraints1 = {};
    constraints1.push_back(Constraint(MySparseVector({1.0},{0}), MySparseMatrix(), 1.0));
    constraints1.push_back(Constraint(MySparseVector({-1.0},{0}), MySparseMatrix(), 1.0));
    vector<indexx> integrality1 = {};
    shared_ptr<OptimizationProblem> opt1 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[0], c1, Q1, constraints1, integrality1);

    // player 2 with variable x2
    // min_x1   2x1x2
    // s.t.     -1 <= x2 <= 1
    MySparseVector c2 = {{},{}};
    MySparseMatrix Q2 = MySparseMatrix({2.0},{1},{0});
    vector<Constraint> constraints2;
    constraints2.push_back(Constraint(MySparseVector({1.0},{1}), MySparseMatrix(), 1.0));
    constraints2.push_back(Constraint(MySparseVector({-1.0},{1}), MySparseMatrix(), 1.0));
    vector<indexx> integrality2 = {1};
    shared_ptr<OptimizationProblem> opt2 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[1], c2, Q2, constraints2, integrality2);

    vector<shared_ptr<OptimizationProblem>> bestResponseProblems = {opt1,opt2};

    // instantiate NEP, one NE to find: {0,0}
    shared_ptr<NashEquilibriumProblem> NEP = make_shared<NashEquilibriumProblem>(bestResponseProblems);
    return NEP;
}

shared_ptr<NashEquilibriumProblem> testNEP2() {
    // example of 2-player game with no NE
    vector<int> numberOfVariablesPerPlayer = {1,1};

    // player 1 with variable x1
    // min_x1   x1x2
    // s.t.     -1 <= x1 <= 1
    MySparseVector c1 = {{2},{0}};
    MySparseMatrix Q1 = MySparseMatrix({-3.0},{1},{0});
    vector<Constraint> constraints1({
        {{{1.0},{0}}, {},1.0},
        {{{-1.0},{0}}, {},1.0}});
    vector<indexx> integrality1 = {};
    shared_ptr<OptimizationProblem> opt1 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[0], c1, Q1, constraints1, integrality1);

    // player 2 with variable x2
    // min_x1   2x1x2
    // s.t.     -1 <= x2 <= 1
    MySparseVector c2 = {{2},{1}};
    MySparseMatrix Q2 = MySparseMatrix({3.0},{1},{0});
    vector<Constraint> constraints2({
        {{{1.0},{1}}, {},1.0},
        {{{-1.0},{1}}, {},1.0}});
    vector<indexx> integrality2 = {1};
    shared_ptr<OptimizationProblem> opt2 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[1], c2, Q2, constraints2, integrality2);

    vector<shared_ptr<OptimizationProblem>> bestResponseProblems = {opt1,opt2};

    // instantiate NEP, one NE to find: {0,0}
    shared_ptr<NashEquilibriumProblem> NEP = make_shared<NashEquilibriumProblem>(bestResponseProblems);
    return NEP;
}

shared_ptr<NashEquilibriumProblem> testNEP3() {
    // example of 2-player game with only NE = (0,0)
    vector<int> numberOfVariablesPerPlayer = {1,1};

    // player 1 with variable x1
    // min_x1   x1x2
    // s.t.     -1 <= x1 <= 1
    MySparseVector c1 = {};
    MySparseMatrix Q1 = MySparseMatrix({-3.0},{1},{0});
    vector<Constraint> constraints1({
        {{{1.0},{0}}, {},1.0},
        {{{-1.0},{0}}, {},1.0}});
    vector<indexx> integrality1 = {};
    shared_ptr<OptimizationProblem> opt1 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[0], c1, Q1, constraints1, integrality1);

    // player 2 with variable x2
    // min_x1   2x1x2
    // s.t.     -1 <= x2 <= 1
    MySparseVector c2 = {};
    MySparseMatrix Q2 = MySparseMatrix({3.0},{1},{0});
    vector<Constraint> constraints2({
        {{{1.0},{1}}, {},1.0},
        {{{-1.0},{1}}, {},1.0}});
    vector<indexx> integrality2 = {1};
    shared_ptr<OptimizationProblem> opt2 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[1], c2, Q2, constraints2, integrality2);

    vector<shared_ptr<OptimizationProblem>> bestResponseProblems = {opt1,opt2};

    // instantiate NEP, one NE to find: {0,0}
    shared_ptr<NashEquilibriumProblem> NEP = make_shared<NashEquilibriumProblem>(bestResponseProblems);
    return NEP;
}

shared_ptr<NashEquilibriumProblem> testNEP4() {
    // example of 2-player game with only NE = (0,0)
    vector<int> numberOfVariablesPerPlayer = {1,1};

    // player 1 with variable x1
    // min_x1   x1x2
    // s.t.     -1 <= x1 <= 1
    MySparseVector c1 = {};
    MySparseMatrix Q1 = MySparseMatrix({-1.0},{1},{0});
    vector<Constraint> constraints1({
        {{{1.0},{0}}, {},10.0},
        {{{-1.0},{0}}, {},10.0}});
    vector<indexx> integrality1 = {};
    shared_ptr<OptimizationProblem> opt1 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[0], c1, Q1, constraints1, integrality1);

    // player 2 with variable x2
    // min_x1   2x1x2
    // s.t.     -1 <= x2 <= 1
    MySparseVector c2 = {};
    MySparseMatrix Q2 = MySparseMatrix({2.0},{1},{0});
    vector<Constraint> constraints2({
        {{{1.0},{1}}, {},1.0},
        {{{-1.0},{1}}, {},1.0}});
    vector<indexx> integrality2 = {1};
    shared_ptr<OptimizationProblem> opt2 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[1], c2, Q2, constraints2, integrality2);

    vector<shared_ptr<OptimizationProblem>> bestResponseProblems = {opt1,opt2};

    // instantiate NEP, one NE to find: {0,0}
    shared_ptr<NashEquilibriumProblem> NEP = make_shared<NashEquilibriumProblem>(bestResponseProblems);
    return NEP;
}

shared_ptr<NashEquilibriumProblem> testNEP5() {
    // example of 2-player game with NE = (-1,1)
    vector<int> numberOfVariablesPerPlayer = {1,1};

    // player 1 with variable x1
    // min_x1   x1x2
    // s.t.     -1 <= x1 <= 1
    MySparseVector c1 = {{1.0}, {0}};
    MySparseMatrix Q1 = MySparseMatrix({1.0},{1},{0});
    vector<Constraint> constraints1({
        {{{1.0},{0}}, {},1.0},
        {{{-1.0},{0}}, {},1.0}});
    vector<indexx> integrality1 = {};
    shared_ptr<OptimizationProblem> opt1 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[0], c1, Q1, constraints1, integrality1);

    // player 2 with variable x2
    // min_x1   2x1x2
    // s.t.     -1 <= x2 <= 1
    MySparseVector c2 = {{1.0}, {1}};
    MySparseMatrix Q2 = MySparseMatrix({1.0},{1},{0});
    vector<Constraint> constraints2({
        {{{1.0},{1}}, {},1.0},
        {{{-1.0},{1}}, {},1.0}});
    vector<indexx> integrality2 = {1};
    shared_ptr<OptimizationProblem> opt2 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[1], c2, Q2, constraints2, integrality2);

    vector<shared_ptr<OptimizationProblem>> bestResponseProblems = {opt1,opt2};

    // instantiate NEP, one NE to find: {0,0}
    shared_ptr<NashEquilibriumProblem> NEP = make_shared<NashEquilibriumProblem>(bestResponseProblems);
    return NEP;
}

shared_ptr<NashEquilibriumProblem> testNEP6() {
    // example of 2-player game with no NE
    vector<int> numberOfVariablesPerPlayer = {1,1};

    // player 1 with variable x1
    // min_x1   x1x2
    // s.t.     -1 <= x1 <= 1
    MySparseVector c1 = {{0.5}, {0}};
    MySparseMatrix Q1 = MySparseMatrix({1.0},{1},{0});
    vector<Constraint> constraints1({
        {{{1.0},{0}}, {},1.0},
        {{{-1.0},{0}}, {},1.0}});
    vector<indexx> integrality1 = {};
    shared_ptr<OptimizationProblem> opt1 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[0], c1, Q1, constraints1, integrality1);

    // player 2 with variable x2
    // min_x1   2x1x2
    // s.t.     -1 <= x2 <= 1
    MySparseVector c2 = {{0.5}, {1}};
    MySparseMatrix Q2 = MySparseMatrix({-1.0},{1},{0});
    vector<Constraint> constraints2({
        {{{1.0},{1}}, {},1.0},
        {{{-1.0},{1}}, {},1.0}});
    vector<indexx> integrality2 = {1};
    shared_ptr<OptimizationProblem> opt2 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[1], c2, Q2, constraints2, integrality2);

    vector<shared_ptr<OptimizationProblem>> bestResponseProblems = {opt1,opt2};

    // instantiate NEP, one NE to find: {0,0}
    shared_ptr<NashEquilibriumProblem> NEP = make_shared<NashEquilibriumProblem>(bestResponseProblems);
    return NEP;
}

shared_ptr<NashEquilibriumProblem> testNEP7() {
    // example of 2-player game
    vector<int> numberOfVariablesPerPlayer = {1,1};

    // player 1 with variable x1
    // min_x1   x1x2
    // s.t.     -1 <= x1 <= 1
    MySparseVector c1 = {{-2.0}, {0}};
    MySparseMatrix Q1 = MySparseMatrix({-1.0,1.0},{1,0},{0,0});
    vector<Constraint> constraints1({
        {{{1.0},{0}}, {},1.0},
        {{{-1.0},{0}}, {},1.0}});
    vector<indexx> integrality1 = {};
    shared_ptr<OptimizationProblem> opt1 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[0], c1, Q1, constraints1, integrality1);

    // player 2 with variable x2
    // min_x1   2x1x2
    // s.t.     -1 <= x2 <= 1
    MySparseVector c2 = {{1.8}, {1}};
    MySparseMatrix Q2 = MySparseMatrix({-1.0,1.0},{1,1},{0,1});
    vector<Constraint> constraints2({
        {{{1.0},{1}}, {},2.0},
        {{{-1.0},{1}}, {},2.0}});
    vector<indexx> integrality2 = {1};
    shared_ptr<OptimizationProblem> opt2 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[1], c2, Q2, constraints2, integrality2);

    vector<shared_ptr<OptimizationProblem>> bestResponseProblems = {opt1,opt2};

    // instantiate NEP, one NE to find: {0,0}
    shared_ptr<NashEquilibriumProblem> NEP = make_shared<NashEquilibriumProblem>(bestResponseProblems);
    return NEP;
}

shared_ptr<NashEquilibriumProblem> testNEP8() {
    // example of 2-player game with one NE (1,1.4)
    vector<int> numberOfVariablesPerPlayer = {1,1};

    // player 1 with variable x1
    // min_x1   x1x2
    // s.t.     -1 <= x1 <= 1
    MySparseVector c1 = {{-2.0}, {0}};
    MySparseMatrix Q1 = MySparseMatrix({-1.0,1.0},{1,0},{0,0});
    vector<Constraint> constraints1({
        {{{1.0},{0}}, {},1.0},
        {{{-1.0},{0}}, {},1.0}});
    vector<indexx> integrality1 = {};
    shared_ptr<OptimizationProblem> opt1 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[0], c1, Q1, constraints1, integrality1);

    // player 2 with variable x2
    // min_x1   2x1x2
    // s.t.     -1 <= x2 <= 1
    MySparseVector c2 = {{-1.8}, {1}};
    MySparseMatrix Q2 = MySparseMatrix({-1.0,1.0},{1,1},{0,1});
    vector<Constraint> constraints2({
        {{{1.0},{1}}, {},2.0},
        {{{-1.0},{1}}, {},2.0}});
    vector<indexx> integrality2 = {};
    shared_ptr<OptimizationProblem> opt2 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[1], c2, Q2, constraints2, integrality2);

    vector<shared_ptr<OptimizationProblem>> bestResponseProblems = {opt1,opt2};

    // instantiate NEP, one NE to find: {0,0}
    shared_ptr<NashEquilibriumProblem> NEP = make_shared<NashEquilibriumProblem>(bestResponseProblems);
    return NEP;
}

shared_ptr<NashEquilibriumProblem> testNEP9() {
    // example of 2-player game with one NE (1,1)
    vector<int> numberOfVariablesPerPlayer = {1,1};

    // player 1 with variable x1
    // min_x1   x1x2
    // s.t.     -1 <= x1 <= 1
    MySparseVector c1 = {{-2.0}, {0}};
    MySparseMatrix Q1 = MySparseMatrix({-1.0,1.0},{1,0},{0,0});
    vector<Constraint> constraints1({
        {{{1.0},{0}}, {},1.0},
        {{{-1.0},{0}}, {},1.0}});
    vector<indexx> integrality1 = {};
    shared_ptr<OptimizationProblem> opt1 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[0], c1, Q1, constraints1, integrality1);

    // player 2 with variable x2
    // min_x1   2x1x2
    // s.t.     -1 <= x2 <= 1, x2 integer
    MySparseVector c2 = {{-1.8}, {1}};
    MySparseMatrix Q2 = MySparseMatrix({-1.0,1.0},{1,1},{0,1});
    vector<Constraint> constraints2({
        {{{1.0},{1}}, {},2.0},
        {{{-1.0},{1}}, {},2.0}});
    vector<indexx> integrality2 = {1};
    shared_ptr<OptimizationProblem> opt2 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[1], c2, Q2, constraints2, integrality2);

    vector<shared_ptr<OptimizationProblem>> bestResponseProblems = {opt1,opt2};

    // instantiate NEP, one NE to find: {0,0}
    shared_ptr<NashEquilibriumProblem> NEP = make_shared<NashEquilibriumProblem>(bestResponseProblems);
    return NEP;
}

shared_ptr<NashEquilibriumProblem> testGNEP1() {
    // example of 2-player game with no GNE
    vector<int> numberOfVariablesPerPlayer = {1,1};

    // player 1 with variable x1
    // min_x1   x1+x2
    // s.t.     -1 <= x1 <= 1
    //          x1+x2 == 1
    MySparseVector c1 = {{1,1},{0,1}};
    MySparseMatrix Q1 = MySparseMatrix({});
    vector<Constraint> constraints1 = {};
    constraints1.push_back(Constraint(MySparseVector({1.0},{0}), MySparseMatrix(), 1.0));
    constraints1.push_back(Constraint(MySparseVector({-1.0},{0}), MySparseMatrix(), 1.0));
    constraints1.push_back(Constraint(MySparseVector({-1,-1},{0,1}), MySparseMatrix(), -1));
    constraints1.push_back(Constraint(MySparseVector({1,1},{0,1}), MySparseMatrix(), 1));
    vector<indexx> integrality1 = {0};
    shared_ptr<OptimizationProblem> opt1 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[0], c1, Q1, constraints1, integrality1);

    // player 2 with variable x2
    // min_x1   2x1+x2
    // s.t.     -1 <= x2 <= 1
    MySparseVector c2 = {{2,1},{0,1}};
    MySparseMatrix Q2 = MySparseMatrix({});
    vector<Constraint> constraints2;
    constraints2.push_back(Constraint(MySparseVector({1.0},{1}), MySparseMatrix(), 1.0));
    constraints2.push_back(Constraint(MySparseVector({-1.0},{1}), MySparseMatrix(), 1.0));
    vector<indexx> integrality2 = {1};
    shared_ptr<OptimizationProblem> opt2 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[1], c2, Q2, constraints2, integrality2);

    vector<shared_ptr<OptimizationProblem>> bestResponseProblems = {opt1,opt2};

    // instantiate NEP, one NE to find: {0,0}
    shared_ptr<NashEquilibriumProblem> NEP = make_shared<NashEquilibriumProblem>(bestResponseProblems);
    return NEP;
}

shared_ptr<NashEquilibriumProblem> testGNEP2() {
    // example of 2-player game, no GNE
    vector<int> numberOfVariablesPerPlayer = {1,1};

    // player 1 with variable x1
    MySparseVector c1 = {{1,1},{0,1}};
    MySparseMatrix Q1 = MySparseMatrix({});
    vector<Constraint> constraints1 = {};
    constraints1.push_back(Constraint(MySparseVector({1.0},{0}), MySparseMatrix(), 1.0));
    constraints1.push_back(Constraint(MySparseVector({-1.0},{0}), MySparseMatrix(), 1.0));
    // constraints1.push_back(Constraint(SparseVector({-1,-1},{0,1}), SparseMatrix(), -1));
    constraints1.push_back(Constraint(MySparseVector({1,1},{0,1}), MySparseMatrix(), 1));
    vector<indexx> integrality1 = {0};
    shared_ptr<OptimizationProblem> opt1 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[0], c1, Q1, constraints1, integrality1);

    // player 2 with variable x2
    MySparseVector c2 = {{2,1},{0,1}};
    MySparseMatrix Q2 = MySparseMatrix({});
    vector<Constraint> constraints2;
    constraints2.push_back(Constraint(MySparseVector({-1,-1},{0,1}), MySparseMatrix(), -1));
    constraints2.push_back(Constraint(MySparseVector({1.0},{1}), MySparseMatrix(), 1.0));
    constraints2.push_back(Constraint(MySparseVector({-1.0},{1}), MySparseMatrix(), 1.0));
    // constraints2.push_back(Constraint(SparseVector({1,1},{0,1}), SparseMatrix(), 1));
    vector<indexx> integrality2 = {};
    shared_ptr<OptimizationProblem> opt2 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[1], c2, Q2, constraints2, integrality2);

    vector<shared_ptr<OptimizationProblem>> bestResponseProblems = {opt1,opt2};

    // instantiate NEP, one NE to find: {0,0}
    shared_ptr<NashEquilibriumProblem> NEP = make_shared<NashEquilibriumProblem>(bestResponseProblems);
    return NEP;
}

shared_ptr<NashEquilibriumProblem> testGNEP3() {
    // example of 2-player game
    vector<int> numberOfVariablesPerPlayer = {1,1};

    // player 1 with variable x1
    MySparseVector c1 = {{1,1},{0,1}};
    MySparseMatrix Q1 = MySparseMatrix({});
    vector<Constraint> constraints1 = {};
    constraints1.push_back(Constraint(MySparseVector({1.0},{0}), MySparseMatrix(), 1.0));
    constraints1.push_back(Constraint(MySparseVector({-1.0},{0}), MySparseMatrix(), 1.0));
    constraints1.push_back(Constraint(MySparseVector({-1,-1},{0,1}), MySparseMatrix(), -1));
    constraints1.push_back(Constraint(MySparseVector({1,1},{0,1}), MySparseMatrix(), 1));
    vector<indexx> integrality1 = {0};
    shared_ptr<OptimizationProblem> opt1 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[0], c1, Q1, constraints1, integrality1);

    // player 2 with variable x2
    MySparseVector c2 = {{2,1},{0,1}};
    MySparseMatrix Q2 = MySparseMatrix({});
    vector<Constraint> constraints2;
    constraints2.push_back(Constraint(MySparseVector({1.0},{1}), MySparseMatrix(), 1.0));
    constraints2.push_back(Constraint(MySparseVector({-1.0},{1}), MySparseMatrix(), 1.0));
    constraints2.push_back(Constraint(MySparseVector({-1,-1},{0,1}), MySparseMatrix(), -1));
    // constraints2.push_back(Constraint(SparseVector({1,1},{0,1}), SparseMatrix(), 1));
    vector<indexx> integrality2 = {};
    shared_ptr<OptimizationProblem> opt2 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[1], c2, Q2, constraints2, integrality2);

    vector<shared_ptr<OptimizationProblem>> bestResponseProblems = {opt1,opt2};

    // instantiate NEP, one NE to find: {0,0}
    shared_ptr<NashEquilibriumProblem> NEP = make_shared<NashEquilibriumProblem>(bestResponseProblems);
    return NEP;
}

shared_ptr<NashEquilibriumProblem> testGNEPConforti() {
    // example of 2-player game
    vector<int> numberOfVariablesPerPlayer = {2,1};

    // player 1 with variable x1
    MySparseVector c1 = {{-0.5},{1}};
    MySparseMatrix Q1 = MySparseMatrix({});
    vector<Constraint> constraints1 = {};
    constraints1.push_back(Constraint(MySparseVector({1,1,1},{0,1,2}), MySparseMatrix(), 2.0));
    constraints1.push_back(Constraint(MySparseVector({-1.0,0.5},{0,2}), MySparseMatrix(), 0.0));
    constraints1.push_back(Constraint(MySparseVector({-1.0,0.5},{1,2}), MySparseMatrix(), 0.0));
    constraints1.push_back(Constraint(MySparseVector({1.0,0.5},{0,2}), MySparseMatrix(), 1.0));
    constraints1.push_back(Constraint(MySparseVector({-1,1,1},{0,1,2}), MySparseMatrix(), 1));
    // constraints1.push_back(Constraint(SparseVector({1},{0}), SparseMatrix(), 2.0));
    // constraints1.push_back(Constraint(SparseVector({1},{1}), SparseMatrix(), 2.0));
    // constraints1.push_back(Constraint(SparseVector({1},{2}), SparseMatrix(), 2.0));
    // constraints1.push_back(Constraint(SparseVector({-1},{0}), SparseMatrix(), 0.0));
    // constraints1.push_back(Constraint(SparseVector({-1},{1}), SparseMatrix(), 0.0));
    // constraints1.push_back(Constraint(SparseVector({-1},{2}), SparseMatrix(), 0.0));
    vector<indexx> integrality1 = {0,1};
    shared_ptr<OptimizationProblem> opt1 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[0], c1, Q1, constraints1, integrality1);

    // player 2 with variable x2
    MySparseVector c2 = {{-1},{2}};
    MySparseMatrix Q2 = MySparseMatrix({});
    vector<Constraint> constraints2;
    constraints2.push_back(Constraint(MySparseVector({1,1,1},{0,1,2}), MySparseMatrix(), 2.0));
    constraints2.push_back(Constraint(MySparseVector({-1.0,0.5},{0,2}), MySparseMatrix(), 0.0));
    constraints2.push_back(Constraint(MySparseVector({-1.0,0.5},{1,2}), MySparseMatrix(), 0.0));
    constraints2.push_back(Constraint(MySparseVector({1.0,0.5},{0,2}), MySparseMatrix(), 1.0));
    constraints2.push_back(Constraint(MySparseVector({-1,1,1},{0,1,2}), MySparseMatrix(), 1));
    // constraints2.push_back(Constraint(SparseVector({1},{0}), SparseMatrix(), 2.0));
    // constraints2.push_back(Constraint(SparseVector({1},{1}), SparseMatrix(), 2.0));
    constraints2.push_back(Constraint(MySparseVector({1},{2}), MySparseMatrix(), 2.0));
    // constraints2.push_back(Constraint(SparseVector({-1},{0}), SparseMatrix(), 0.0));
    // constraints2.push_back(Constraint(SparseVector({-1},{1}), SparseMatrix(), 0.0));
    constraints2.push_back(Constraint(MySparseVector({-1},{2}), MySparseMatrix(), 0.0));
    vector<indexx> integrality2 = {2};
    shared_ptr<OptimizationProblem> opt2 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[1], c2, Q2, constraints2, integrality2);

    vector<shared_ptr<OptimizationProblem>> bestResponseProblems = {opt1,opt2};

    // instantiate NEP, one NE to find: {0,0}
    shared_ptr<NashEquilibriumProblem> NEP = make_shared<NashEquilibriumProblem>(bestResponseProblems);
    return NEP;
}

shared_ptr<NashEquilibriumProblem> testGNEPJulianICExample() {
    // example of 3-player game
    vector<int> numberOfVariablesPerPlayer = {2,2,2};

    // player 1 with variable x0 and x1
    MySparseVector c1 = {{-1},{0}};
    MySparseMatrix Q1 = MySparseMatrix({});
    vector<Constraint> constraints1 = {};
    constraints1.push_back(Constraint(MySparseVector({-1.0},{0}), MySparseMatrix(), 0.0));
    constraints1.push_back(Constraint(MySparseVector({-1.0},{1}), MySparseMatrix(), 0.0));
    constraints1.push_back(Constraint(MySparseVector({1,1,1},{0,2,4}), MySparseMatrix(), 2));
    constraints1.push_back(Constraint(MySparseVector({1.0,1.0},{0,1}), MySparseMatrix(), 1.0));
    constraints1.push_back(Constraint(MySparseVector({-1.0,-1.0},{0,1}), MySparseMatrix(), -1.0));
    vector<indexx> integrality1 = {0,1};
    shared_ptr<OptimizationProblem> opt1 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[0], c1, Q1, constraints1, integrality1);

    // player 2 with variable x2 and x2=3
    MySparseVector c2 = {{-1,1},{3,0}};
    MySparseMatrix Q2 = MySparseMatrix({});
    vector<Constraint> constraints2;
    constraints2.push_back(Constraint(MySparseVector({-1.0},{2}), MySparseMatrix(), 0.0));
    constraints2.push_back(Constraint(MySparseVector({-1.0},{3}), MySparseMatrix(), 0.0));
    // constraints2.push_back(Constraint(SparseVector({1,1,1},{0,2,4}), SparseMatrix(), 2));
    constraints2.push_back(Constraint(MySparseVector({1.0,1.0},{2,3}), MySparseMatrix(), 1.0));
    constraints2.push_back(Constraint(MySparseVector({-1.0,-1.0},{2,3}), MySparseMatrix(), -1.0));
    vector<indexx> integrality2 = {2,3};
    shared_ptr<OptimizationProblem> opt2 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[1], c2, Q2, constraints2, integrality2);

    // player 3 with variable x4 and x5
    MySparseVector c3 = {{-1,1},{5,0}};
    MySparseMatrix Q3 = MySparseMatrix({});
    vector<Constraint> constraints3;
    constraints3.push_back(Constraint(MySparseVector({-1.0},{4}), MySparseMatrix(), 0.0));
    constraints3.push_back(Constraint(MySparseVector({-1.0},{5}), MySparseMatrix(), 0.0));
    // constraints3.push_back(Constraint(SparseVector({1,1,1},{0,2,4}), SparseMatrix(), 2));
    constraints3.push_back(Constraint(MySparseVector({1.0,1.0},{4,5}), MySparseMatrix(), 1.0));
    constraints3.push_back(Constraint(MySparseVector({-1.0,-1.0},{4,5}), MySparseMatrix(), -1.0));
    vector<indexx> integrality3 = {4,5};
    shared_ptr<OptimizationProblem> opt3 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[2], c3, Q3, constraints3, integrality3);

    vector<shared_ptr<OptimizationProblem>> bestResponseProblems = {opt1,opt2,opt3};

    // instantiate NEP, one NE to find: {0,0}
    shared_ptr<NashEquilibriumProblem> NEP = make_shared<NashEquilibriumProblem>(bestResponseProblems);
    return NEP;
}

shared_ptr<NashEquilibriumProblem> testGNEPJulianICExampleModified() {
    // example of 3-player game
    vector<int> numberOfVariablesPerPlayer = {2,2,2};

    // player 1 with variable x0 and x1
    MySparseVector c1 = {{-1},{0}};
    MySparseMatrix Q1 = MySparseMatrix({});
    vector<Constraint> constraints1 = {};
    constraints1.push_back(Constraint(MySparseVector({-1.0},{0}), MySparseMatrix(), 0.0));
    // constraints1.push_back(Constraint(SparseVector({-1.0},{1}), SparseMatrix(), 0.0));
    constraints1.push_back(Constraint(MySparseVector({1,1,1},{0,2,4}), MySparseMatrix(), 2));
    constraints1.push_back(Constraint(MySparseVector({1.0,1.0},{0,1}), MySparseMatrix(), 1.0));
    // constraints1.push_back(Constraint(SparseVector({-1.0,-1.0},{0,1}), SparseMatrix(), -1.0));
    vector<indexx> integrality1 = {0,1};
    shared_ptr<OptimizationProblem> opt1 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[0], c1, Q1, constraints1, integrality1);

    // player 2 with variable x2 and x2=3
    MySparseVector c2 = {{-1,1},{3,0}};
    MySparseMatrix Q2 = MySparseMatrix({});
    vector<Constraint> constraints2;
    constraints2.push_back(Constraint(MySparseVector({-1.0},{2}), MySparseMatrix(), 0.0));
    constraints2.push_back(Constraint(MySparseVector({-1.0},{3}), MySparseMatrix(), 0.0));
    // constraints2.push_back(Constraint(SparseVector({1,1,1},{0,2,4}), SparseMatrix(), 2));
    constraints2.push_back(Constraint(MySparseVector({1.0,1.0},{2,3}), MySparseMatrix(), 1.0));
    // constraints2.push_back(Constraint(SparseVector({-1.0,-1.0},{2,3}), SparseMatrix(), -1.0));
    vector<indexx> integrality2 = {2,3};
    shared_ptr<OptimizationProblem> opt2 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[1], c2, Q2, constraints2, integrality2);

    // player 3 with variable x4 and x5
    MySparseVector c3 = {{-1,1},{5,0}};
    MySparseMatrix Q3 = MySparseMatrix({});
    vector<Constraint> constraints3;
    constraints3.push_back(Constraint(MySparseVector({-1.0},{4}), MySparseMatrix(), 0.0));
    constraints3.push_back(Constraint(MySparseVector({-1.0},{5}), MySparseMatrix(), 0.0));
    // constraints3.push_back(Constraint(SparseVector({1,1,1},{0,2,4}), SparseMatrix(), 2));
    constraints3.push_back(Constraint(MySparseVector({1.0,1.0},{4,5}), MySparseMatrix(), 1.0));
    // constraints3.push_back(Constraint(SparseVector({-1.0,-1.0},{4,5}), SparseMatrix(), -1.0));
    vector<indexx> integrality3 = {4,5};
    shared_ptr<OptimizationProblem> opt3 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[2], c3, Q3, constraints3, integrality3);

    vector<shared_ptr<OptimizationProblem>> bestResponseProblems = {opt1,opt2,opt3};

    // instantiate NEP, one NE to find: {0,0}
    shared_ptr<NashEquilibriumProblem> NEP = make_shared<NashEquilibriumProblem>(bestResponseProblems);
    return NEP;
}

shared_ptr<NashEquilibriumProblem> testGNEPtoNEPJulianICExample() {
    // example of 3-player game
    vector<int> numberOfVariablesPerPlayer = {2,2,2};

    // player 1 with variable x0 and x1
    MySparseVector c1 = {{-1},{0}};
    MySparseMatrix Q1 = MySparseMatrix({});
    vector<Constraint> constraints1 = {};
    constraints1.push_back(Constraint(MySparseVector({-1.0},{0}), MySparseMatrix(), 0.0));
    constraints1.push_back(Constraint(MySparseVector({-1.0},{1}), MySparseMatrix(), 0.0));
    constraints1.push_back(Constraint(MySparseVector({1.0,1.0},{0,1}), MySparseMatrix(), 1.0));
    constraints1.push_back(Constraint(MySparseVector({-1.0,-1.0},{0,1}), MySparseMatrix(), -1.0));
    vector<indexx> integrality1 = {0,1};
    shared_ptr<OptimizationProblem> opt1 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[0], c1, Q1, constraints1, integrality1);

    // player 2 with variable x2 and x2=3
    MySparseVector c2 = {{-1,1},{3,0}};
    MySparseMatrix Q2 = MySparseMatrix({});
    vector<Constraint> constraints2;
    constraints2.push_back(Constraint(MySparseVector({-1.0},{2}), MySparseMatrix(), 0.0));
    constraints2.push_back(Constraint(MySparseVector({-1.0},{3}), MySparseMatrix(), 0.0));
    // constraints2.push_back(Constraint(SparseVector({1,1,1},{0,2,4}), SparseMatrix(), 2));
    constraints2.push_back(Constraint(MySparseVector({1.0,1.0},{2,3}), MySparseMatrix(), 1.0));
    constraints2.push_back(Constraint(MySparseVector({-1.0,-1.0},{2,3}), MySparseMatrix(), -1.0));
    vector<indexx> integrality2 = {2,3};
    shared_ptr<OptimizationProblem> opt2 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[1], c2, Q2, constraints2, integrality2);

    // player 3 with variable x4 and x5
    MySparseVector c3 = {{-1,1},{5,0}};
    MySparseMatrix Q3 = MySparseMatrix({});
    vector<Constraint> constraints3;
    constraints3.push_back(Constraint(MySparseVector({-1.0},{4}), MySparseMatrix(), 0.0));
    constraints3.push_back(Constraint(MySparseVector({-1.0},{5}), MySparseMatrix(), 0.0));
    // constraints3.push_back(Constraint(SparseVector({1,1,1},{0,2,4}), SparseMatrix(), 2));
    constraints3.push_back(Constraint(MySparseVector({1.0,1.0},{4,5}), MySparseMatrix(), 1.0));
    constraints3.push_back(Constraint(MySparseVector({-1.0,-1.0},{4,5}), MySparseMatrix(), -1.0));
    vector<indexx> integrality3 = {4,5};
    shared_ptr<OptimizationProblem> opt3 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[2], c3, Q3, constraints3, integrality3);

    vector<shared_ptr<OptimizationProblem>> bestResponseProblems = {opt1,opt2,opt3};

    // instantiate NEP, one NE to find: {0,0}
    shared_ptr<NashEquilibriumProblem> NEP = make_shared<NashEquilibriumProblem>(bestResponseProblems);
    return NEP;
}

shared_ptr<NashEquilibriumProblem> testNEP4playerwithNE() {
    // example of 4-player game
    vector<int> numberOfVariablesPerPlayer = {1,1,1,1};

    // player 1 with variable x1
    // min_x1   x1x2
    // s.t.     -1 <= x1 <= 1
    MySparseVector c1 = {{},{}};
    MySparseMatrix Q1 = MySparseMatrix({-1.0},{1},{0});
    vector<Constraint> constraints1({
        {{{1.0},{0}}, {},1.0},
        {{{-1.0},{0}}, {},1.0}});
    vector<indexx> integrality1 = {0};
    shared_ptr<OptimizationProblem> opt1 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[0], c1, Q1, constraints1, integrality1);

    // player 2 with variable x2
    // min_x1   2x2x3
    // s.t.     -1 <= x2 <= 1
    MySparseVector c2 = {{},{}};
    MySparseMatrix Q2 = MySparseMatrix({2.0},{1},{2});
    vector<Constraint> constraints2({
        {{{1.0},{1}}, {},1.0},
        {{{-1.0},{1}}, {},1.0}});
    vector<indexx> integrality2 = {1};
    shared_ptr<OptimizationProblem> opt2 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[1], c2, Q2, constraints2, integrality2);

    // player 3 with variable x3
    // min_x1   x3x4
    // s.t.     -1 <= x3 <= 1
    MySparseVector c3 = {{},{}};
    MySparseMatrix Q3 = MySparseMatrix({-1.0},{2},{3});
    vector<Constraint> constraints3({
        {{{1.0},{2}}, {},1.0},
        {{{-1.0},{2}}, {},1.0}});
    vector<indexx> integrality3 = {2};
    shared_ptr<OptimizationProblem> opt3 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[2], c3, Q3, constraints3, integrality3);

    // player 4 with variable x4
    // min_x1   2x4x1
    // s.t.     -1 <= x4 <= 1
    MySparseVector c4 = {{},{}};
    MySparseMatrix Q4 = {{2.0},{3},{0}};
    vector<Constraint> constraints4({
        {{{1.0},{3}}, {},1.0},
        {{{-1.0},{3}}, {},1.0}});
    vector<indexx> integrality4 = {};
    shared_ptr<OptimizationProblem> opt4 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[3], c4, Q4, constraints4, integrality4);

    vector<shared_ptr<OptimizationProblem>> bestResponseProblems = {opt1,opt2,opt3,opt4};

    // instantiate NEP, one NE to find: to check
    shared_ptr<NashEquilibriumProblem> NEP = make_shared<NashEquilibriumProblem>(bestResponseProblems);
    return NEP;
}

shared_ptr<NashEquilibriumProblem> testRandomNEP(
    int numberOfPlayers,
    UniformDistribInt distribVariables,
    UniformDistribDouble distribC,
    UniformDistribDouble distribQ,
    UniformDistribDouble distribBounds,
    double probaIntegrality, // proba of a variable to be integer
    double probaNonzero,
    int seed) {
    // build random NEP (all variables are bounded, and no other constraints)
    // probaNonzero is a probability to have a nonzero value for each choice of coefficient

    vector<shared_ptr<OptimizationProblem>> bestResponseProblems = {};

    // initialize random generator with srand
    time_t timer = time(nullptr);
    if (seed != -1)
        srand(seed); // putting 123 to not have random instances for now so that we can debug
    else
        srand(timer);
    // generation of the number of variables per player
    vector<int> numberOfVariablesPerPlayer = {};
    int totalNumberOfVariables = 0;
    for (indexx player = 0; player < numberOfPlayers;player++) {
        numberOfVariablesPerPlayer.push_back(randomUniformInt(distribVariables));
        totalNumberOfVariables += numberOfVariablesPerPlayer[player];
    }

    // indexx to get the true index of variable
    indexx bonusVariableIndex = 0; // proper initialization
    for (indexx i = 0; i < numberOfPlayers; i++) {
        if (i != 0)
            bonusVariableIndex += numberOfVariablesPerPlayer[i-1]; // add the number of variable of current player
        // generation of c
        MySparseVector c = {};
        for (indexx j = 0; j < numberOfVariablesPerPlayer[i]; j++) {
            if (randomUniformO1() < probaNonzero) {
                c.addCoefficient(randomUniformDouble(distribC), j+bonusVariableIndex);
            } // else coef = 0
        }
        // generation of Q
        MySparseMatrix Q = MySparseMatrix();
        for (indexx j = 0; j < numberOfVariablesPerPlayer[i]; j++) {
            for (indexx k = 0; k < totalNumberOfVariables; k++) {
                if (randomUniformO1() < probaNonzero) {
                    Q.addCoefficient(randomUniformDouble(distribQ), j+bonusVariableIndex, k);
                } // else coef = 0
            }
        }
        // generation of constraints (which are only bounds)
        vector<Constraint> constraints = {};
        for (indexx j = 0; j < numberOfVariablesPerPlayer[i]; j++) {
            // generate two double values, order them, add +1 if equal and set the bounds with those values
            double a = randomUniformDouble(distribBounds);
            double b = randomUniformDouble(distribBounds);
            if (b < a) {
                double temp = a;
                a = b;
                b = temp;
            } else if (a == b)
                b++;
            constraints.push_back({{{-1.0},{j+bonusVariableIndex}}, {}, -a});
            constraints.push_back({{{1.0},{j+bonusVariableIndex}}, {}, b});
        }
        // generation of integral variables
        vector<indexx> integrality = {};
        for (indexx j = 0; j < numberOfVariablesPerPlayer[i]; j++) {
            if (randomUniformO1() < probaIntegrality) {
                integrality.push_back(j+bonusVariableIndex);
            } // else continuous
        }
        // instantiate OptimizationProblem
        bestResponseProblems.push_back(make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[i], c, Q, constraints, integrality));
    }

    // instantiate NEP, one NE to find: {0,0}
    shared_ptr<NashEquilibriumProblem> NEP = make_shared<NashEquilibriumProblem>(bestResponseProblems);
    return NEP;
}

shared_ptr<NashEquilibriumProblem> testKnapsackNEP() {
    // example of 2-player game NEP knapsack extracted from instance 2-25-2-cij-n.txt of Dragotto23 with no PNE
    vector<int> numberOfVariablesPerPlayer = {7,7};

    // player 1 with variable x1
    MySparseVector c1 = {{86,52,1,97,73,14,92}, {0,1,2,3,4,5,6}};
    MySparseMatrix Q1 = MySparseMatrix({6,-70,-97,-23,86,9,55},{0,1,2,3,4,5,6},{7,8,9,10,11,12,13});
    vector<Constraint> constraints1({
            {{{1.0},{0}}, {},1.0},
            {{{1.0},{1}}, {},1.0},
            {{{1.0},{2}}, {},1.0},
            {{{1.0},{3}}, {},1.0},
            {{{1.0},{4}}, {},1.0},
            {{{1.0},{5}}, {},1.0},
            {{{1.0},{6}}, {},1.0},
            {{{-1.0},{0}}, {},0.0},
            {{{-1.0},{1}}, {},0.0},
            {{{-1.0},{2}}, {},0.0},
            {{{-1.0},{3}}, {},0.0},
            {{{-1.0},{4}}, {},0.0},
            {{{-1.0},{5}}, {},0.0},
            {{{-1.0},{6}}, {},0.0},
        {{{11,50,81,72,81,52,19},{0,1,2,3,4,5,6}}, {},77}});
    vector<indexx> integrality1 = {0,1,2,3,4,5,6};
    shared_ptr<OptimizationProblem> opt1 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[0], c1, Q1, constraints1, integrality1);

    // player 2 with variable x2
    // min_x1   2x1x2
    // s.t.     -1 <= x2 <= 1, x2 integer
    MySparseVector c2 = {{35,1,52,69,22,63,16}, {7,8,9,10,11,12,13}};
    MySparseMatrix Q2 = MySparseMatrix({-17,-98,61,17,33,-80,23},{7,8,9,10,11,12,13},{0,1,2,3,4,5,6});
    vector<Constraint> constraints2({
            {{{1.0},{7}}, {},1.0},
            {{{1.0},{8}}, {},1.0},
            {{{1.0},{9}}, {},1.0},
            {{{1.0},{10}}, {},1.0},
            {{{1.0},{11}}, {},1.0},
            {{{1.0},{12}}, {},1.0},
            {{{1.0},{13}}, {},1.0},
            {{{-1.0},{7}}, {},0.0},
            {{{-1.0},{8}}, {},0.0},
            {{{-1.0},{9}}, {},0.0},
            {{{-1.0},{10}}, {},0.0},
            {{{-1.0},{11}}, {},0.0},
            {{{-1.0},{12}}, {},0.0},
            {{{-1.0},{13}}, {},0.0},
        {{{68,22,15,51,7,62,17},{7,8,9,10,11,12,13}}, {}, 48}});
    vector<indexx> integrality2 = {7,8,9,10,11,12,13};
    shared_ptr<OptimizationProblem> opt2 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[1], c2, Q2, constraints2, integrality2);

    vector<shared_ptr<OptimizationProblem>> bestResponseProblems = {opt1,opt2};

    // instantiate NEP, one NE to find: {0,0}
    shared_ptr<NashEquilibriumProblem> NEP = make_shared<NashEquilibriumProblem>(bestResponseProblems);
    return NEP;
}

std::shared_ptr<NashEquilibriumProblem> testInfiniteNEP() {
    vector<int> numberOfVariablesPerPlayer = {1,1};

    // player 1 with variable x1
    MySparseVector c1 = {{0},{1}};
    MySparseMatrix Q1 = MySparseMatrix({1,-1},{0,0},{0,1});
    vector<Constraint> constraints1({
                {{{1.0},{0}}, {},1.0},
                {{{-1.0},{0}}, {},1.0}});
    vector<indexx> integrality1 = {};
    shared_ptr<OptimizationProblem> opt1 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[0], c1, Q1, constraints1, integrality1);

    // player 2 with variable x2
    // min_x1   2x1x2
    // s.t.     -1 <= x2 <= 1, x2 integer
    MySparseVector c2 = {{0},{0}};
    MySparseMatrix Q2 = MySparseMatrix({1,-2},{1,1},{1,0});
    vector<Constraint> constraints2({
                {{{1.0},{1}}, {},1.0},
                {{{-1.0},{1}}, {},1.0}});
    vector<indexx> integrality2 = {};
    shared_ptr<OptimizationProblem> opt2 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[1], c2, Q2, constraints2, integrality2);

    vector<shared_ptr<OptimizationProblem>> bestResponseProblems = {opt1,opt2};

    // instantiate NEP, one NE to find: {0,0}
    shared_ptr<NashEquilibriumProblem> NEP = make_shared<NashEquilibriumProblem>(bestResponseProblems);
    return NEP;
}

std::shared_ptr<NashEquilibriumProblem> testInfiniteNEP2() {
    vector<int> numberOfVariablesPerPlayer = {2,2};

    // player 1 with variable x1
    MySparseVector c1 = {};
    MySparseMatrix Q1 = MySparseMatrix({1,-2,1,1},{0,0,2,1},{0,2,2,2});
    vector<Constraint> constraints1({
                {{{1.0},{0}}, {},1.0},
                {{{-1.0},{0}}, {},1.0},
                {{{1.0},{1}}, {},1.0},
                {{{-1.0},{1}}, {},1.0}});
    vector<indexx> integrality1 = {1};
    shared_ptr<OptimizationProblem> opt1 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[0], c1, Q1, constraints1, integrality1);

    // player 2 with variable x2
    // min_x1   2x1x2
    // s.t.     -1 <= x2 <= 1, x2 integer
    MySparseVector c2 = {};
    MySparseMatrix Q2 = MySparseMatrix({1,-2,1,1},{2,2,0,3},{2,0,0,0});
    vector<Constraint> constraints2({
                {{{1.0},{2}}, {},1.0},
                {{{-1.0},{2}}, {},1.0},
                {{{1.0},{3}}, {},1.0},
                {{{-1.0},{3}}, {},1.0}});
    vector<indexx> integrality2 = {3};
    shared_ptr<OptimizationProblem> opt2 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[1], c2, Q2, constraints2, integrality2);

    vector<shared_ptr<OptimizationProblem>> bestResponseProblems = {opt1,opt2};

    // instantiate NEP, one NE to find: {0,0}
    shared_ptr<NashEquilibriumProblem> NEP = make_shared<NashEquilibriumProblem>(bestResponseProblems);
    return NEP;
}

shared_ptr<NashEquilibriumProblem> createGNEPInstanceNegativeBounds(
    string const& filename) {
    cout << "building model of GNEP game derived from a knapsack instance "
            "but with negative lower bounds, to test extreme ray computation"
            ". Maybe I forgot to change the bounds of gurobi variables."
         << endl;
    string const& optionIntegrality = "fullInteger";

    // parse filename
    NEPKnapsackInstance const& instance = parseNEPKnapsack(filename);

    int numberOfPlayers = instance.numberOfPlayers;
    int numberOfItems = instance.numberOfItems;

    vector<int> numberOfVariablesPerPlayer = {};
    for (int player = 0; player < numberOfPlayers; player++)
        numberOfVariablesPerPlayer.push_back(numberOfItems);

    // initialize vector of optimizationProblem and other things
    vector<shared_ptr<OptimizationProblem>> bestResponseProblems = {};
    int countNumberOfVariables = 0;

    // for each player, build OptimizationProblem
    for (int player = 0; player < numberOfPlayers; player++) {
        // build c, multiply all coefficients by -1 because
        // it is a maximization problem but we will minimize it
        vector<double> cValues = {};
        vector<int> cIndices = {};
        for (int item = 0; item < numberOfItems; item++) {
            // minus sign for maximization
            cValues.push_back(-instance.profits[player][item]);
            cIndices.push_back(countNumberOfVariables++);
        }
        MySparseVector c = {cValues, cIndices};

        // build Q
        MySparseMatrix Q = {{}, {}, {}};

        // build constraints
        vector<Constraint> constraints = {};
        // constraints for bounds of binary vectors
        for (int item = 0; item < numberOfItems; item++) {
            constraints.push_back({{{1.0}, {numberOfItems*player+item}}, {}, 1.0});
            constraints.push_back({{{-1.0}, {numberOfItems*player+item}}, {}, 1.0});
        }
        // constraints forcing an item to be taken by maximum its number of copies
        for (int item = 0; item < numberOfItems; item++) {
            vector<double> ones = {};
            vector<indexx> sameItemVariables = {};
            for (indexx j = 0; j < numberOfPlayers; j++) {
                ones.push_back(1.0);
                sameItemVariables.push_back(numberOfItems*j+item);
            }
            MySparseVector v = {ones, sameItemVariables};
            constraints.push_back({v, {}, static_cast<double>(instance.itemsAvailable[item])});
        }
        // capacity constraint
        vector<double> weightValues = {};
        vector<int> capacityIndices = {};
        for (int item = 0; item < numberOfItems; item++) {
            weightValues.push_back(instance.weights[player][item]);
            capacityIndices.push_back(numberOfItems*player+item);
        }
        constraints.push_back({{weightValues, capacityIndices}, {}, static_cast<double>(instance.capacities[player])});

        // build OptimizationProblem, reuse capacityIndices as integrality variable indices
        vector<indexx> integralityIndices = {};
        if (optionIntegrality == "fullInteger") {
            // all variables are integer
            integralityIndices = capacityIndices;
        }
        else if (optionIntegrality == "halfInteger") {
            // all even variable indices are integer, so approximately half of them
            for (indexx i = 0; i < capacityIndices.size(); i += 2)
                integralityIndices.push_back(capacityIndices[i]);
        }
        bestResponseProblems.push_back(
            make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[player],
                                              c,
                                              Q,
                                              constraints,
                                              integralityIndices));
    }

    // instantiate GNEP
    shared_ptr<NashEquilibriumProblem> GNEP = make_shared<NashEquilibriumProblem>(bestResponseProblems);

    return GNEP;
}


shared_ptr<NashEquilibriumProblem> testGNEPNegativeBounds() {
    shared_ptr<NashEquilibriumProblem> NEP =
        createGNEPInstanceNegativeBounds("../../../instances/GNEP_knapsack_instances/4-50-5-weakcorr-0.txt");
    return NEP;
}

std::shared_ptr<NashEquilibriumProblem> testBilevelCounterexample() {
    // unique NE: (0,0)
    vector<int> numberOfVariablesPerPlayer = {1,1};

    // player 1 with variable x
    // min_x    x-y
    // s.t.     0 <= x <= 1
    MySparseVector c1 = {{1,-1}, {0,1}};
    MySparseMatrix Q1 = {};
    vector<Constraint> constraints1({
                {{{1.0},{0}}, {},1.0},
                {{{-1.0},{0}}, {},0.0}});
    vector<indexx> integrality1 = {};
    shared_ptr<OptimizationProblem> opt1 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[0], c1, Q1, constraints1, integrality1);

    // player 2 with variable y
    // min_y    y
    // s.t.     y >= x
    //          0 <= y <= 1
    //          y integer
    MySparseVector c2 = {{1}, {1}};
    MySparseMatrix Q2 = {};
    vector<Constraint> constraints2({
                {{{1.0},{1}}, {},1.0},
                {{{-1.0},{1}}, {},0.0},
                {{{1,-1},{0,1}},{},0.0}});
    vector<indexx> integrality2 = {1};
    shared_ptr<OptimizationProblem> opt2 = make_shared<OptimizationProblem>(numberOfVariablesPerPlayer[1], c2, Q2, constraints2, integrality2);

    vector<shared_ptr<OptimizationProblem>> bestResponseProblems = {opt1,opt2};

    // instantiate NEP
    shared_ptr<NashEquilibriumProblem> NEP = make_shared<NashEquilibriumProblem>(bestResponseProblems);
    return NEP;
}