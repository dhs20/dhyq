#pragma once

#include "quantum/meta/MethodMetadata.h"

#include <cstddef>
#include <string_view>
#include <vector>

namespace quantum::solvers {

struct OrbitalSolveRequest {
    int atomicNumber = 1;
    int chargeState = 0;
    int electronCount = 1;
    int n = 1;
    int l = 0;
    double effectiveCharge = 1.0;
    bool useReducedMass = true;
    std::size_t gridPoints = 4096;
};

struct OrbitalSolveResult {
    bool converged = false;
    double energyEv = 0.0;
    std::vector<double> radiiMeters;
    std::vector<double> radialValues;
    quantum::meta::MethodStamp method;
    std::vector<quantum::meta::ValidationRecord> validation;
};

class OrbitalSolver {
public:
    virtual ~OrbitalSolver() = default;

    [[nodiscard]] virtual std::string_view name() const = 0;
    [[nodiscard]] virtual OrbitalSolveResult solve(const OrbitalSolveRequest& request) = 0;
};

} // namespace quantum::solvers
