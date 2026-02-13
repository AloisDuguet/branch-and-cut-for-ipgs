//
// Created by alois-duguet on 1/15/26.
//

#ifndef CUTINFORMATION_H
#define CUTINFORMATION_H

#include <vector>
#include <bits/stdc++.h>

#include <Constraint.h>

class CutInformation {
public:
    CutInformation() = default;
    CutInformation(std::vector<Cut> const& cuts,
                   std::unordered_map<std::string, double> const& timeValues);

    std::vector<Cut> getCuts() const;

    std::unordered_map<std::string, double> getTimeValues() const;

    std::vector<Cut> cuts;
    std::unordered_map<std::string, double> timeValues;
};


#endif //CUTINFORMATION_H
