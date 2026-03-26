#include "quantum/physics/NumericalSolver.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <vector>

namespace quantum::physics {
namespace {

double reducedMassFactorForSolver(double nuclearMassKg, bool useReducedMass) {
    if (!useReducedMass || nuclearMassKg <= 0.0) {
        return 1.0;
    }
    const double mu =
        (constants::electronMassKg * nuclearMassKg) / (constants::electronMassKg + nuclearMassKg);
    return mu / constants::electronMassKg;
}

struct TridiagonalSystem {
    std::vector<double> diagonal;
    std::vector<double> offDiagonal;
    std::vector<double> radiiScaled;
    double stepScaled = 0.0;
};

TridiagonalSystem buildHydrogenicSystem(const NumericalSolverRequest& request, int gridPoints) {
    TridiagonalSystem system;
    system.diagonal.resize(static_cast<std::size_t>(gridPoints));
    system.offDiagonal.assign(static_cast<std::size_t>(std::max(0, gridPoints - 1)), 0.0);
    system.radiiScaled.resize(static_cast<std::size_t>(gridPoints));

    const double maxRadiusScaled = request.maxScaledRadius;
    system.stepScaled = maxRadiusScaled / static_cast<double>(gridPoints + 1);
    const double invH2 = 1.0 / (system.stepScaled * system.stepScaled);
    const double off = -0.5 * invH2;

    for (int index = 0; index < gridPoints; ++index) {
        const double radius = system.stepScaled * static_cast<double>(index + 1);
        system.radiiScaled[static_cast<std::size_t>(index)] = radius;
        const double centrifugal =
            static_cast<double>(request.qn.l * (request.qn.l + 1)) / (2.0 * radius * radius);
        const double coulomb = -1.0 / radius;
        system.diagonal[static_cast<std::size_t>(index)] = invH2 + centrifugal + coulomb;
        if (index < gridPoints - 1) {
            system.offDiagonal[static_cast<std::size_t>(index)] = off;
        }
    }

    return system;
}

int sturmCount(const TridiagonalSystem& system, double lambda) {
    int count = 0;
    double q = system.diagonal[0] - lambda;
    if (q < 0.0) {
        ++count;
    }
    const double epsilon = 1e-14;
    if (std::abs(q) < epsilon) {
        q = (q < 0.0) ? -epsilon : epsilon;
    }

    for (std::size_t index = 1; index < system.diagonal.size(); ++index) {
        const double off = system.offDiagonal[index - 1];
        q = system.diagonal[index] - lambda - (off * off) / q;
        if (q < 0.0) {
            ++count;
        }
        if (std::abs(q) < epsilon) {
            q = (q < 0.0) ? -epsilon : epsilon;
        }
    }
    return count;
}

double gershgorinLowerBound(const TridiagonalSystem& system) {
    double bound = std::numeric_limits<double>::max();
    for (std::size_t index = 0; index < system.diagonal.size(); ++index) {
        const double radius =
            ((index > 0) ? std::abs(system.offDiagonal[index - 1]) : 0.0) +
            ((index + 1 < system.diagonal.size()) ? std::abs(system.offDiagonal[index]) : 0.0);
        bound = std::min(bound, system.diagonal[index] - radius);
    }
    return bound;
}

double gershgorinUpperBound(const TridiagonalSystem& system) {
    double bound = -std::numeric_limits<double>::max();
    for (std::size_t index = 0; index < system.diagonal.size(); ++index) {
        const double radius =
            ((index > 0) ? std::abs(system.offDiagonal[index - 1]) : 0.0) +
            ((index + 1 < system.diagonal.size()) ? std::abs(system.offDiagonal[index]) : 0.0);
        bound = std::max(bound, system.diagonal[index] + radius);
    }
    return bound;
}

double bisectEigenvalue(const TridiagonalSystem& system, int eigenIndex) {
    double low = gershgorinLowerBound(system);
    double high = gershgorinUpperBound(system);

    for (int iteration = 0; iteration < 96; ++iteration) {
        const double mid = 0.5 * (low + high);
        const int count = sturmCount(system, mid);
        if (count <= eigenIndex) {
            low = mid;
        } else {
            high = mid;
        }
    }
    return 0.5 * (low + high);
}

std::vector<double> solveShiftedTridiagonal(const TridiagonalSystem& system,
                                            double shift,
                                            const std::vector<double>& rhs) {
    const std::size_t n = system.diagonal.size();
    std::vector<double> cPrime(n > 1 ? n - 1 : 0, 0.0);
    std::vector<double> dPrime(n, 0.0);
    std::vector<double> solution(n, 0.0);

    double denominator = system.diagonal[0] - shift;
    if (std::abs(denominator) < 1e-12) {
        denominator = (denominator < 0.0) ? -1e-12 : 1e-12;
    }
    if (n > 1) {
        cPrime[0] = system.offDiagonal[0] / denominator;
    }
    dPrime[0] = rhs[0] / denominator;

    for (std::size_t index = 1; index < n; ++index) {
        denominator = system.diagonal[index] - shift - system.offDiagonal[index - 1] * cPrime[index - 1];
        if (std::abs(denominator) < 1e-12) {
            denominator = (denominator < 0.0) ? -1e-12 : 1e-12;
        }
        if (index < n - 1) {
            cPrime[index] = system.offDiagonal[index] / denominator;
        }
        dPrime[index] = (rhs[index] - system.offDiagonal[index - 1] * dPrime[index - 1]) / denominator;
    }

    solution[n - 1] = dPrime[n - 1];
    for (std::size_t index = n - 1; index-- > 0;) {
        solution[index] = dPrime[index] - cPrime[index] * solution[index + 1];
    }
    return solution;
}

std::vector<double> inverseIterationEigenvector(const TridiagonalSystem& system, double eigenvalue) {
    std::vector<double> vector(system.diagonal.size(), 1.0);
    const double shift = eigenvalue + ((eigenvalue < 0.0) ? -1.0e-10 : 1.0e-10);

    for (int iteration = 0; iteration < 16; ++iteration) {
        vector = solveShiftedTridiagonal(system, shift, vector);
        const double norm = std::sqrt(std::inner_product(vector.begin(), vector.end(), vector.begin(), 0.0));
        if (norm <= 1e-30) {
            break;
        }
        for (double& value : vector) {
            value /= norm;
        }
    }
    return vector;
}

struct SingleSolveResult {
    bool converged = false;
    double energyEv = 0.0;
    double analyticalEnergyEv = 0.0;
    double errorEv = 0.0;
    double gridStepMeters = 0.0;
    std::vector<double> radiiMeters;
    std::vector<double> radialFunction;
    std::vector<std::pair<double, double>> radialDistribution;
    std::string message;
};

SingleSolveResult solveSingle(const NumericalSolverRequest& request, int gridPoints) {
    SingleSolveResult result;
    const auto analytical = computeHydrogenicMetrics(
        request.nuclearCharge, request.qn.n, request.zeff, request.nuclearMassKg, request.useReducedMass);
    result.analyticalEnergyEv = analytical.energyEv;

    const int radialStateIndex = request.qn.n - request.qn.l - 1;
    if (gridPoints < 32 || radialStateIndex < 0) {
        result.message = "Invalid finite-difference solver request";
        return result;
    }

    const double muFactor = reducedMassFactorForSolver(request.nuclearMassKg, request.useReducedMass);
    const double effectiveBohrRadius =
        constants::bohrRadiusM * (1.0 / muFactor) / std::max(request.zeff, 1e-6);

    const auto system = buildHydrogenicSystem(request, gridPoints);
    const double eigenvalueScaledHartree = bisectEigenvalue(system, radialStateIndex);
    const auto eigenvector = inverseIterationEigenvector(system, eigenvalueScaledHartree);

    const double energyHartree = eigenvalueScaledHartree * muFactor * request.zeff * request.zeff;
    const double energyJ = energyHartree * 2.0 * constants::rydbergEnergyJ;
    result.energyEv = units::jouleToEv(energyJ);
    result.errorEv = std::abs(result.energyEv - result.analyticalEnergyEv);
    result.gridStepMeters = system.stepScaled * effectiveBohrRadius;

    double norm = 0.0;
    for (const double value : eigenvector) {
        norm += value * value * result.gridStepMeters;
    }
    norm = std::sqrt(std::max(norm, 1e-30));

    result.radiiMeters.reserve(eigenvector.size());
    result.radialFunction.reserve(eigenvector.size());
    result.radialDistribution.reserve(eigenvector.size());
    for (std::size_t index = 0; index < eigenvector.size(); ++index) {
        const double radius = system.radiiScaled[index] * effectiveBohrRadius;
        const double u = eigenvector[index] / norm;
        const double radial = u / std::max(radius, 1e-20);
        result.radiiMeters.push_back(radius);
        result.radialFunction.push_back(radial);
        result.radialDistribution.emplace_back(radius, u * u);
    }

    result.converged = true;
    result.message = "Finite-difference tridiagonal eigen solve converged";
    return result;
}

} // namespace

NumericalSolverResult SchrodingerNumericalSolver::solve(const NumericalSolverRequest& request) const {
    NumericalSolverResult result;
    if (request.qn.n <= 0 || request.qn.l < 0 || request.qn.l >= request.qn.n) {
        result.message = "Invalid quantum numbers";
        return result;
    }

    const auto base = solveSingle(request, request.gridPoints);
    result.converged = base.converged;
    result.message = base.message;
    result.energyEv = base.energyEv;
    result.analyticalEnergyEv = base.analyticalEnergyEv;
    result.errorEv = base.errorEv;
    result.radiiMeters = base.radiiMeters;
    result.radialFunction = base.radialFunction;
    result.radialDistribution = base.radialDistribution;

    if (!base.converged) {
        return result;
    }

    for (int pass = 0; pass < request.convergencePasses; ++pass) {
        const int divisor = 1 << (request.convergencePasses - pass - 1);
        const int gridPoints = std::max(128, request.gridPoints / divisor);
        const auto sample = solveSingle(request, gridPoints);
        if (sample.converged) {
            result.convergence.push_back({gridPoints, sample.gridStepMeters, sample.energyEv, sample.errorEv});
        }
    }

    std::sort(result.convergence.begin(), result.convergence.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.gridStepMeters > rhs.gridStepMeters;
    });
    return result;
}

} // namespace quantum::physics
