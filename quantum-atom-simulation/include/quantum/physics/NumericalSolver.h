#pragma once

#include "quantum/physics/AtomicPhysics.h"

#include <string>
#include <utility>
#include <vector>

namespace quantum::physics {

struct NumericalConvergenceSample {
    int gridPoints = 0;
    double gridStepMeters = 0.0;
    double energyEv = 0.0;
    double errorEv = 0.0;
};

struct NumericalSolverRequest {
    int nuclearCharge = 1;
    double zeff = 1.0;
    double nuclearMassKg = 0.0;
    bool useReducedMass = true;
    QuantumNumbers qn;
    int gridPoints = 4096;
    int convergencePasses = 5;
    double maxScaledRadius = 120.0;
};

struct NumericalSolverResult {
    bool converged = false;
    std::string message;
    double energyEv = 0.0;
    double analyticalEnergyEv = 0.0;
    double errorEv = 0.0;
    std::vector<double> radiiMeters;
    std::vector<double> radialFunction;
    std::vector<std::pair<double, double>> radialDistribution;
    std::vector<NumericalConvergenceSample> convergence;
};

class SchrodingerNumericalSolver {
public:
    [[nodiscard]] NumericalSolverResult solve(const NumericalSolverRequest& request) const;
};

} // namespace quantum::physics
