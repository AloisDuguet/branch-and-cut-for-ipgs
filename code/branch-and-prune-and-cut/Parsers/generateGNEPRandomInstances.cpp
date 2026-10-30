//
// Created by alois-duguet on 9/24/26.
//

#include "generateGNEPRandomInstances.h"

#include <iostream>
#include <fstream>
#include <random>
#include <string>
#include <algorithm>
#include <vector>
#include <filesystem>
#include <cmath>

using namespace std;

bool randomGNEPInstance(std::string const& filename,
                        int nPlayers,
                        int nConstraints,
                        int nVariables,
                        double nonzeroProportion);

bool generateBenchmarkRandomGNEP(
    string const& foldername,
    vector<int> const& nPlayersRange,
    vector<int> const& nConstraintsRange,
    vector<int> const& nVariablesRange,
    vector<double> const& nonzeroProportionRange,
    int nRepetitions);

int main()
{
    // no '/' at the end
    const std::string foldername = "../../../instances/GNEPRandomInstances";
    const std::vector<int> nPlayersRange = {2, 3, 4};
    const std::vector<int> nConstraintsRange = {5};
    const std::vector<int> nVariablesRange = {3, 6, 10};
    const std::vector<double> nonzeroProportionRange = {0.10, 0.25, 0.50};
    const int nRepetitions = 10;
    const bool success = generateBenchmarkRandomGNEP(
        foldername,
        nPlayersRange,
        nConstraintsRange,
        nVariablesRange,
        nonzeroProportionRange,
        nRepetitions
    );

    return success ? 0 : 1;
}



bool randomGNEPInstance(string const& filename,
                        int nPlayers,
                        int nConstraints,
                        int nVariables,
                        double nonzeroProportion)
{
    // Basic parameter validation.
    if (nPlayers <= 0 ||
        nConstraints <= 0 ||
        nVariables <= 0 ||
        nonzeroProportion < 0.0 ||
        nonzeroProportion > 1.0)
    {
        cout << "invalid parameters in randomGNEPInstance:" << endl;
        cout << nPlayers << " ";
        cout << nConstraints << " ";
        cout << nVariables << " ";
        cout << nonzeroProportion << endl;
        return false;
    }

    ofstream out(filename);
    if (!out.is_open())
        return false;

    // Random number generator.
    random_device rd;
    mt19937 gen(rd());

    // Objective and nonzero LHS coefficients.
    uniform_int_distribution<int> objectiveDist(-10, -1);
    uniform_int_distribution<int> coefficientDist(1, 10);

    // Probability of a nonzero LHS coefficient.
    bernoulli_distribution nonzeroDist(nonzeroProportion);

    // RHS coefficients.
    uniform_int_distribution<int> rhsDist(10, 100);

    // First line.
    out << nPlayers << ' '
        << nVariables << ' '
        << nConstraints << '\n';

    const int totalVariables = nPlayers * nVariables;

    for (int player = 0; player < nPlayers; ++player)
    {
        // ------------------------------------------------------------
        // Objective function coefficients.
        // ------------------------------------------------------------
        for (int j = 0; j < totalVariables; ++j)
        {
            if (j > 0)
                out << ' ';

            out << objectiveDist(gen);
        }
        out << '\n';

        // ------------------------------------------------------------
        // Constraints.
        // ------------------------------------------------------------
        for (int c = 0; c < nConstraints; ++c)
        {
            vector<int> coefficients(totalVariables, 0);

            // Generate all LHS coefficients.
            for (int j = 0; j < totalVariables; ++j)
            {
                if (nonzeroDist(gen))
                    coefficients[j] = coefficientDist(gen);
            }

            // Ensure that the constraint contains at least one
            // nonzero coefficient among this player's own variables.
            const int firstOwnVariable = player * nVariables;
            const int lastOwnVariable = firstOwnVariable + nVariables;

            bool hasOwnCoefficient = false;

            for (int j = firstOwnVariable; j < lastOwnVariable; ++j)
            {
                if (coefficients[j] != 0)
                {
                    hasOwnCoefficient = true;
                    break;
                }
            }

            if (!hasOwnCoefficient)
            {
                // Force one randomly selected own-variable coefficient
                // to be nonzero.
                uniform_int_distribution<int> ownVariableDist(
                    firstOwnVariable, lastOwnVariable - 1);

                coefficients[ownVariableDist(gen)] = coefficientDist(gen);
            }

            // Write LHS coefficients in global variable order.
            for (int j = 0; j < totalVariables; ++j)
            {
                if (j > 0)
                    out << ' ';

                out << coefficients[j];
            }

            // Positive RHS.
            out << ' ' << rhsDist(gen) << '\n';
        }
    }

    return out.good();
}

bool generateBenchmarkRandomGNEP(
    string const& foldername,
    vector<int> const& nPlayersRange,
    vector<int> const& nConstraintsRange,
    vector<int> const& nVariablesRange,
    vector<double> const& nonzeroProportionRange,
    int nRepetitions)
{
    namespace fs = filesystem;

    if (nPlayersRange.empty() ||
        nConstraintsRange.empty() ||
        nVariablesRange.empty() ||
        nonzeroProportionRange.empty() ||
        nRepetitions <= 0)
    {
        cout << "parameters of generateBenchmarkRandomGNEP invalid" << endl;
        return false;
    }

    // Create the output directory if necessary.
    error_code ec;
    fs::create_directories(foldername, ec);

    if (ec)
        return false;

    for (int nPlayers : nPlayersRange)
    {
        for (int nConstraints : nConstraintsRange)
        {
            for (int nVariables : nVariablesRange)
            {
                for (double nonzeroProportion : nonzeroProportionRange)
                {
                    // Check that nonzeroProportion is in [0,1].
                    if (nonzeroProportion < 0.0 ||
                        nonzeroProportion > 1.0)
                    {
                        cout << "nonzeroProportion does not satisfy bounds" << endl;
                        return false;
                    }

                    // Check that nonzeroProportion is a multiple of 0.01.
                    const double scaled = 100.0 * nonzeroProportion;
                    const double rounded = round(scaled);

                    if (abs(scaled - rounded) > 1e-9) {
                        cout << "nonzeroProportion is not a multiple of 0.01" << endl;
                        return false;
                    }

                    const int percentage =
                        static_cast<int>(rounded);

                    for (int repetition = 1;
                         repetition <= nRepetitions;
                         ++repetition)
                    {
                        const string filename =
                            foldername +
                            "/randomGNEP" +
                            to_string(nPlayers) +
                            "-" +
                            to_string(nVariables) +
                            "-" +
                            to_string(nConstraints) +
                            "-" +
                            to_string(percentage) +
                            "-" +
                            to_string(repetition) +
                            ".txt";

                        if (!randomGNEPInstance(
                                filename,
                                nPlayers,
                                nConstraints,
                                nVariables,
                                nonzeroProportion))
                        {
                            cout << "randomGNEPInstance did not finish properly with arguments:" << endl;
                            cout << filename << endl;
                            cout << nPlayers << " ";
                            cout << nConstraints << " ";
                            cout << nVariables << " ";
                            cout << nonzeroProportion << endl;
                            return false;
                        }
                    }
                }
            }
        }
    }

    return true;
}