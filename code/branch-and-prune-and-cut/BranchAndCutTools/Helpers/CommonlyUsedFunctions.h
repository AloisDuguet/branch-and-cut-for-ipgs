//
// Created by Aloïs Duguet on 11/4/24.
//

#ifndef COMMONLYUSEDFUNCTIONS_H
#define COMMONLYUSEDFUNCTIONS_H

#include <string>
#include <vector>
#include <chrono>
#include "gurobi_c++.h"
#include "bits/stdc++.h"

// macros
#define MY_INF std::numeric_limits<double>::infinity()

// typedefs
typedef int indexx;

// ----------------------------------------------------------------
// time-related functions

// start a chrono with <chrono> library
std::chrono::time_point<std::chrono::high_resolution_clock> startChrono();

// return a double representing a time between start and now()
double elapsedChrono(std::chrono::time_point<std::chrono::high_resolution_clock> start);

// return date and time in a formatted way without spaces
std::string getCurrentTimeAndDate();

// ----------------------------------------------------------------
// string and bool functions

// fill in a vector<string> with str and delimiter
std::vector<std::string> splitString(std::string const& str, char delimiter = ' ');

void testSplitString();

// handle possible input errors for bool arguments of main function
bool handleBoolErrorAsMainArgument(char const* arg, int pos);

// return a vector<double> from a string where double are separated by ','
std::vector<double> buildVectorFromString(std::string& s);

// return a string from a vector<double>
std::string to_string(std::vector<double>& v);

// ----------------------------------------------------------------
// printing functions
std::ostream& operator<<(std::ostream&, std::vector<double> const& v);
std::ostream& operator<<(std::ostream&, std::vector<int> const& v);
std::ostream& operator<<(std::ostream&, std::vector<bool> const& v);

// print all elements of v with a space between them. printEndl adds a line return if true.
void printVector(std::vector<double> const& v, bool printEndl = true);

// print all elements of v with a space between them. printEndl adds a line return if true.
void printVector(std::vector<int> const& v, bool printEndl = true);

// print vector<bool>
void printVector(std::vector<bool> const& v, bool printEndl = true);

// ----------------------------------------------------------------
// maths functions

// returns the norm 2 of v1-v2
double norm2(std::vector<double> const& v1, std::vector<double> const& v2);

// returns the norm 2 of v
double norm2(std::vector<double> const& v);

// operator+ overloading
std::vector<double> operator+(std::vector<double> const& u, std::vector<double> const& v);

// return the sum of elements of a vector
double sum(std::vector<double> const& v);

// scalar product for equally-sized vectors
double operator*(std::vector<double> const& u, std::vector<double> const& v);

// product of a scalar and a vector
std::vector<double> operator*(double a, std::vector<double> const& v);

// return true if value is in vector
bool isValueInVector(int value, std::vector<int> v);

// possible cause for numerical issues
// return true if |x-y| < tolerance. 1e-6 is the default feasibility tolerance of gurobi
bool compareApproxEqual(double x, double y, double tolerance = 1e-6);

// return true if ||x-y|| < tolerance. 1e-6 is the default feasibility tolerance of gurobi
bool compareApproxEqual(std::vector<double> const& x,
                        std::vector<double> const& y,
                        double tolerance = 1e-6);

// return true if |x| < tolerance. 1e-6 is the default feasibility tolerance of gurobi
bool isApproxZero(double x, double tolerance = 1e-6);

// return true if x < y + tolerance. 1e-6 is the default feasibility tolerance of gurobi
bool compareApproxLess(double x, double y, double tolerance = 1e-6);

// return true if x > y - tolerance. 1e-6 is the default feasibility tolerance of gurobi
bool compareApproxMore(double x, double y, double tolerance = 1e-6);

// possible cause for numerical issues
// return a double rounded to the closest integral if it is closer to tolerance.
// 1e-5 is the default IntFeasTol value of gurobi
double integralityRounding(double x, double tolerance = 1e-5);

// calls integralityRounding for each double of v.
std::vector<double> integralityRounding(std::vector<double> v, double tolerance = 1e-5);

// check if a vector is integer
bool isInteger(std::vector<double> const& v, double tolerance = 1e-5);

// remove values close to zero in vector
void removeCloseToZero(GRBsvec* x, double tolerance = 1e-10);

// euclidean norm between vector<double>
double distanceMeasure(std::vector<double> const& v1, std::vector<double> const& v2);

// parse a special form of matrix of double
// transpose == true makes a transpose of the result
std::vector<std::vector<double>> parseFlattenedMatrix(std::string const& str,
                                                      bool transpose = false);

// parse a special form of matrix of double
std::vector<double> parseFlattenedVector(std::string const& str);

// ----------------------------------------------------------------
// random functions

// structure for 1D distribution with a mean and a range
// Elements should be bound by [mean-range,mean+range].
struct UniformDistribDouble {
    double mean;
    double range;

    explicit UniformDistribDouble(double mean = 0, double range = 1) :
        mean(mean), range(range) {}
};

// structure for 1D integral distribution with a mean and a range
// elements should be bound by [mean-range,mean+range].
struct UniformDistribInt {
    int mean;
    int range;

    explicit UniformDistribInt(int mean = 0, int range = 1) :
        mean(mean), range(range) {}
};

// uniform between O and 1 included
double randomUniformO1();

// int uniform between mean-range and mean+range
int randomUniformInt(UniformDistribInt distrib);

// return random integer between a and b included.
int randomUniformIntRange(int a, int b);

// double uniform between mean-range and mean+range
double randomUniformDouble(UniformDistribDouble distrib);

// --------------------------------------------------------------
// functions with unordered maps
std::unordered_map<std::string, double> operator+=(
    std::unordered_map<std::string, double> &t1,
    std::unordered_map<std::string, double> const& t2);

std::ostream& operator<<(std::ostream& os, std::unordered_map<std::string, double> const& timeValues);

#endif //COMMONLYUSEDFUNCTIONS_H