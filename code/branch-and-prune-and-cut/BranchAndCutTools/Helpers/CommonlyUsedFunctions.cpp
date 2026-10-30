//
// Created by Aloïs Duguet on 11/4/24.
//

#include "CommonlyUsedFunctions.h"

#include "gurobi_c++.h"
#include <cmath>
#include <vector>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <format>
#include <numeric>

using namespace std;

chrono::time_point<chrono::high_resolution_clock> startChrono() {
    return chrono::high_resolution_clock::now();
}

double elapsedChrono(chrono::time_point<chrono::high_resolution_clock> start) {
    auto stop = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);
    return static_cast<double>(duration.count()) / 1000000.0;
}

double norm2(vector<double> const& v1, vector<double> const& v2) {
    double res = 0.0;
    for (int i = 0; i < v1.size(); i++)
        res += (v1[i]-v2[i])*(v1[i]-v2[i]);
    return sqrt(res);
}

double norm2(vector<double> const& v) {
    double res = 0.0;
    for (int i = 0; i < v.size(); i++)
        res += v[i]*v[i];
    return sqrt(res);
}

vector<double> operator+(vector<double> const& u, vector<double> const& v) {
    if (u.size() == v.size()) {
        vector<double> result = {};
        for (int i = 0; i < u.size(); i++)
            result.push_back(u[i] + v[i]);
        return result;
    } else
        throw logic_error("While adding two vectors with "
                          "overloaded + operator: vectors of different size");
}

double operator*(vector<double> const& u, vector<double> const& v) {
    if (u.size() != v.size())
        throw logic_error("attempting scalar product with overloaded "
                          "operator* between two vectors of different sizes: "
    + to_string(u.size()) + " and " + to_string(v.size()));
    else { // u and v are of same size
        double result = 0.0;
        for (int i = 0; i < u.size(); i++)
            result += u[i]*v[i];
        return result;
    }
}

vector<double> operator*(double a, vector<double> const &v) {
    vector<double> res = {};
    for (double val : v)
        res.push_back(a*val);
    return res;
}

double sum(vector<double> const& v) {
    return accumulate(v.begin(), v.end(), 0.0);
}

ostream& operator<<(ostream& os, vector<double> const& v) {
    for (indexx i = 0; i < v.size(); i++)
        os << v[i] << " ";
    return os;
}

ostream& operator<<(ostream& os, vector<int> const& v) {
    for (indexx i = 0; i < v.size(); i++)
        os << v[i] << " ";
    return os;
}

ostream& operator<<(ostream& os, vector<bool> const& v) {
    for (auto && i : v)
        os << i << " ";
    return os;
}

void printVector(vector<double> const& v, bool printEndl) {
    for (indexx i = 0; i < v.size(); i++)
        cout << v[i] << " ";
    if (printEndl)
        cout << endl;
}

void printVector(vector<int> const& v, bool printEndl) {
    for (indexx i = 0; i < v.size(); i++)
        cout << v[i] << " ";
    if (printEndl)
        cout << endl;
}

void printVector(vector<bool> const& v, bool printEndl) {
    for (bool i : v)
        cout << static_cast<int>(i) << " ";
    if (printEndl)
        cout << endl;
}

bool isValueInVector(int value, vector<int> v) {
    for (int element : v)
        if (value == element)
            return true;
    return false;
}

bool compareApproxEqual(double x, double y, double const tolerance) {
    if (fabs(x - y) < tolerance)
        return true;
    return false;
}

bool compareApproxEqual(vector<double> const& x,
                        vector<double> const& y,
                        double const tolerance) {
    if (x.size() != y.size())
        throw logic_error("compareApproxEqual with two vector<double> "
                          "of different sizes: "
                          + to_string(x.size()) + " and " + to_string(y.size()));
    for (indexx i = 0; i < x.size(); i++) {
        if (!compareApproxEqual(x[i], y[i], tolerance))
            return false;
    }
    return true;
}

bool isApproxZero(double const x, double const tolerance) {
    if (fabs(x) < tolerance)
        return true;
    return false;
}

bool compareApproxLess(double const x, double const y, double const tolerance) {
    if (x - y < tolerance)
        return true;
    return false;
}

bool compareApproxMore(double const x, double const y, double const tolerance) {
    return compareApproxLess(-x, -y, tolerance);
}

double integralityRounding(double const x, double const tolerance) {
    if (fabs(ceil(x) - x) < tolerance)
        return ceil(x);
    if (fabs(floor(x) - x) < tolerance)
        return floor(x);
    return x;
}

void removeCloseToZero(GRBsvec* x, double tolerance) {
    int len = 0;
    int indices[x->len];
    double values[x->len];

    // find 'real' nonzero values
    for (indexx index = 0; index < x->len; index++) {
        if (!isApproxZero(x->val[index], tolerance)) {
            indices[len] = x->ind[index];
            values[len] = x->val[index];
            len++;
        }
    }

    // enter real nonzero values into x
    if (len != x->len) {
        x->len = len;
        for (indexx index = 0; index < len; index++) {
            x->ind[index] = indices[index];
            x->val[index] = values[index];
        }
    }
}

vector<double> integralityRounding(vector<double> v, double const tolerance) {
    for (indexx i = 0; i < v.size(); i++)
        v[i] = integralityRounding(v[i], tolerance);
    return v;
}

bool isInteger(vector<double> const& v, double tolerance) {
    for (double const val : v) {
        // if val is neither close to floor(val) nor ceil(val), then the vector is not integer
        if (fabs(val - floor(val)) > tolerance and fabs(val - ceil(val)) > tolerance) {
            return false;
        }
    }
    return true;
}

vector<string> splitString(string const& str, char const delimiter) {
    vector<string> result = {};
    string temp;
    stringstream ss(str);
    while (getline(ss, temp, delimiter))
        result.push_back(temp);
    return result;
}

void testSplitString() {
    string str = "test of this,impressive,parser";
    vector<string> result = splitString(str, ' ');
    vector<string> result2 = splitString(str, ',');
    cout << endl << "first splitString" << endl;
    for (const string& s : result)
        cout << s << endl;
    cout << endl << "second splitString" << endl;
    for (const string& s : result2)
        cout << s << endl;
}

vector<vector<double>> parseFlattenedMatrix(string const& str, bool transpose) {
    string temp;
    istringstream iss(str);
    getline(iss,temp,' '); // "size"
    getline(iss,temp,' '); // number of rows
    int numberOfRows = stoi(temp);
    getline(iss,temp,' '); // number of cols
    int numberOfCols = stoi(temp);
    getline(iss,temp,' '); // "value"

    vector<vector<double>> matrix = {};
    if (transpose) {
        for (indexx colIndex = 0; colIndex < numberOfCols; colIndex++) {
            matrix.push_back({}); // start all rows
            for (indexx rowIndex = 0; rowIndex < numberOfRows; rowIndex++)
                matrix[colIndex].push_back(0);
        }
    } else {
        for (indexx rowIndex = 0; rowIndex < numberOfRows; rowIndex++) {
            matrix.push_back({}); // start all rows
            for (indexx colIndex = 0; colIndex < numberOfCols; colIndex++)
                matrix[rowIndex].push_back(0);
        }
    }

    for (indexx colIndex = 0; colIndex < numberOfCols; colIndex++) {
        // fill in col by col
        for (indexx rowIndex = 0; rowIndex < numberOfRows; rowIndex++) {
            getline(iss,temp,' ');
            if (transpose)
                matrix[colIndex][rowIndex] = stod(temp);
            else
                matrix[rowIndex][colIndex] = stod(temp);
        }
    }
    return matrix;
}

vector<double> parseFlattenedVector(string const& str) {
    string temp;
    istringstream iss(str);
    getline(iss,temp,' '); // "size"
    getline(iss,temp,' '); // number of rows
    int numberOfRows = stoi(temp);
    getline(iss,temp,' '); // "1"
    getline(iss,temp,' '); // "value"

    vector<double> vec = {};
    for (indexx rowIndex = 0; rowIndex < numberOfRows; rowIndex++) {
        getline(iss,temp,' ');
        vec.push_back(stod(temp));
    }
    return vec;
}

string findFirstLineWith(ifstream& file, string const& target) {
    string line;
    while (getline(file, line))
        if (line.find(target) != string::npos)
            return line;
    throw runtime_error("string " + target + " not found in file");
    return "";
}

void replaceAll(string& str, const string& from, const string& to)
{
    if (from.empty()) return;

    std::size_t pos = 0;
    while ((pos = str.find(from, pos)) != std::string::npos) {
        str.replace(pos, from.length(), to);
        pos += to.length();  // Move past the replacement
    }
}

bool handleBoolErrorAsMainArgument(char const* arg, int const pos) {
    string const s = arg;
    if ( s.size() != 1 || s[0] < '0' || s[0] > '1' )
        throw invalid_argument("argv["+to_string(pos)+
                               "] should be either 0 or 1 for conversion to bool");
    return ( s[0] == '1' );
}

vector<double> buildVectorFromString(string& s) {
    vector<double> res = {};
    stringstream ss(s);
    string temp;
    while (getline(ss, temp, ',')) {
        res.push_back(stod(temp));
    }
    return res;
}

int getPrecisionFloatFormat(double value, int decimalDigits) {
    if (value < 0)
        value = -value;
    if (value == 0)
        return 1;
    if (value == std::numeric_limits<double>::infinity())
        return decimalDigits;
    return static_cast<int>(ceil(abs(log10(value)))) + decimalDigits + (value < 0);
}


vector<bool> allButLastTrue(int const n) {
    vector<bool> res = {};
    for (indexx i = 0; i < n-1; i++)
        res.push_back(true);
    res.push_back(false);
    return res;
}

string to_string(vector<double> &v) {
    string s;
    for (auto const value : v) {
        s += to_string(value) + " ";
    }
    return s.substr(0, s.size()-1);
}

string to_string(vector<int> &v) {
    string s;
    for (auto const value : v) {
        s += to_string(value) + " ";
    }
    return s.substr(0, s.size()-1);
}

string to_string(vector<bool> &v) {
    string s;
    for (auto value : v) {
        s += to_string(value) + " ";
    }
    return s.substr(0, s.size()-1);
}

std::vector<double> expandToVector(double value, int length) {
    vector<double> v = {};
    for (indexx i = 0; i < length; i++)
        v.push_back(value);
    return v;
}

double randomUniformO1() {
    return rand() / static_cast<double>(RAND_MAX);
}

int randomUniformInt(UniformDistribInt distrib) {
    return rand() % (distrib.range*2+1) + distrib.mean - distrib.range;
}

int randomUniformIntRange(int a, int b) {
    return rand() % (b - a + 1) + a;
}

double randomUniformDouble(UniformDistribDouble distrib) {
    return rand()/ static_cast<double>(RAND_MAX)*distrib.range*2
            + distrib.mean - distrib.range;
}

double distanceMeasure(vector<double> const& v1, vector<double> const& v2) {
    if (v1.size() != v2.size())
        throw invalid_argument("vectors must have the same size");
    double sumOfSquare = 0;
    for (indexx i = 0; i < v1.size(); i++)
        sumOfSquare += pow(v1[i] - v2[i], 2);
    return sqrt(sumOfSquare);
}

string getCurrentTimeAndDate()
{
    auto const time = chrono::current_zone()
        ->to_local(chrono::system_clock::now());
    return format("{:%Y-%m-%d-%X}", time);
}

std::vector<std::vector<double> > symmetricPositiveDefiniteGeneration(
    int size,
    Bounds rangeLowerTriangularDiagonal,
    Bounds rangeLowerPart,
    double sparsityLowerPart,
    unsigned int seed,
    bool isInteger) {
    if (!isInteger)
        throw logic_error("float symmetric positive-definite generation not implemented");
    if (rangeLowerTriangularDiagonal.lb <= 0)
        throw logic_error("SPD matrices need strictly positive diagonal value");

    srand(seed);

    // generation of the lower-triangular matrix
    vector<vector<double>> lowerTriangular;
    for (indexx i = 0; i < size; i++) {
        vector<double> temp = {};
        for (indexx j = 0; j < i; j++) {
            if (rand() < sparsityLowerPart*static_cast<double>(RAND_MAX))
                temp.push_back(randomUniformIntRange(
                    rangeLowerPart.lb,
                    rangeLowerPart.ub));
            else
                temp.push_back(0);
        }
        temp.push_back(randomUniformIntRange(
            rangeLowerTriangularDiagonal.lb,
            rangeLowerTriangularDiagonal.ub));
        for (indexx j = i+1; j < size; j++)
            temp.push_back(0);
        lowerTriangular.push_back(temp);
    }

    // multiplication of the lower triangular L with its symmetric: L L^T
    vector<vector<double>> spd = {};
    for (indexx i = 0; i < size; i++) {
        vector<double> temp = {};
        for (indexx j = 0; j < size; j++) {
            double val = 0;
            for (indexx k = 0; k < size; k++)
                val += lowerTriangular[i][k] * lowerTriangular[j][k];
            temp.push_back(val);
        }
        spd.push_back(temp);
        cout << temp << endl;
    }

    return spd;
}

unordered_map<string, double> operator+=(
        unordered_map<string, double> &t1,
        unordered_map<string, double> const& t2) {
    for (const auto & it2 : t2) {
        bool elementFound = false;
        // if t1 has it2.first, add it2.second to the value in t1
        for (auto it1 : t1) {
            if (it2.first == it1.first) {
                t1[it2.first] += it2.second;
                elementFound = true;
                break;
            }
        }
        // else, create new element in t1
        if (!elementFound)
            t1.insert({it2.first, it2.second});
    }

    return t1;
}

ostream& operator<<(ostream& os, unordered_map<string, double> const& timeValues) {
    for (auto const& it : timeValues)
        os << it.first << ": " << it.second << " seconds" << endl;
    return os;
}