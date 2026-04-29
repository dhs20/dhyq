#pragma once

#include "quantum/meta/MethodMetadata.h"
#include "quantum/solvers/MeanFieldSolver.h"

#include <string>
#include <vector>

namespace quantum::solvers {

struct CorrelationSolveRequest {
    int atomicNumber = 1;
    int chargeState = 0;
    double referenceEnergyEv = 0.0;
    std::vector<double> orbitalEnergiesEv;
    double couplingStrengthEv = 0.35;
};

struct ConfigurationMixingComponent {
    std::string label;
    double energyEv = 0.0;
    double weight = 0.0;
};

struct CorrelationSolveResult {
    bool solved = false;
    double uncorrelatedEnergyEv = 0.0;
    double correlatedEnergyEv = 0.0;
    double correlationEnergyEv = 0.0;
    double mixingAngleRadians = 0.0;
    std::vector<ConfigurationMixingComponent> components;
    quantum::meta::MethodStamp method;
    std::vector<quantum::meta::ValidationRecord> validation;
};

class TeachingCorrelationSolver {
public:
    [[nodiscard]] std::string_view name() const;
    [[nodiscard]] CorrelationSolveResult solve(const CorrelationSolveRequest& request) const;
};

} // namespace quantum::solvers
