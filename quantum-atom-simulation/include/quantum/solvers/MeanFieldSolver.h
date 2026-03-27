#pragma once

#include "quantum/meta/MethodMetadata.h"

#include <string>
#include <string_view>
#include <vector>

namespace quantum::solvers {

struct MeanFieldSolveRequest {
    int atomicNumber = 1;
    int chargeState = 0;
    std::string configurationHint;
    int maxIterations = 50;
    double convergenceTolerance = 1.0e-8;
};

struct MeanFieldSolveResult {
    bool converged = false;
    int iterations = 0;
    double totalEnergyEv = 0.0;
    std::vector<double> orbitalEnergiesEv;
    quantum::meta::MethodStamp method;
    std::vector<quantum::meta::ValidationRecord> validation;
};

class MeanFieldSolver {
public:
    virtual ~MeanFieldSolver() = default;

    [[nodiscard]] virtual std::string_view name() const = 0;
    [[nodiscard]] virtual MeanFieldSolveResult solve(const MeanFieldSolveRequest& request) = 0;
};

} // namespace quantum::solvers
