//
// Created by alois-duguet on 1/15/26.
//

#include "CutInformation.h"

#include <vector>
#include <bits/stdc++.h>

using namespace std;

CutInformation::CutInformation(
        vector<Cut> const& cuts,
        unordered_map<string, double> const& timeValues) {
    this->cuts = cuts;
    this->timeValues = timeValues;
}

vector<Cut> CutInformation::getCuts() const {
    return cuts;
}

unordered_map<string, double> CutInformation::getTimeValues() const {
    return timeValues;
}