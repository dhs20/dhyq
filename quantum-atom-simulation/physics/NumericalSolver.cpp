#include "quantum/physics/NumericalSolver.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <vector>

namespace quantum::physics {
namespace {

using quantum::meta::AnimationKind;
using quantum::meta::MethodStamp;
using quantum::meta::ModelTier;
using quantum::meta::SourceRecord;
using quantum::meta::ValidationRecord;
using quantum::meta::ValidationStatus;

SourceRecord numericalBenchmarkSource() {
    return {"internal-numerical-benchmark", "Internal numerical hydrogenic benchmark", "quantum_atom_tests", "1", "", ""};
}

MethodStamp makeSolverMethodStamp() {
    MethodStamp stamp;
    stamp.methodName = "Finite-difference radial central-field eigen solver";
    stamp.approximation = "single-electron radial central-field solver on a uniform grid";
    stamp.dataSource = "Discretized radial Hamiltonian with configurable central potential";
    stamp.validationSummary =
        "Hydrogenic Coulomb mode is benchmarked against analytic energies; screened mode is a qualitative Tier 1 extension.";
    stamp.tier = ModelTier::Tier1CentralField;
    stamp.animationKind = AnimationKind::None;
    stamp.caveats.push_back("Only radial central-potential problems are solved.");
    stamp.caveats.push_back("This is not a many-electron self-consistent-field or correlated solver.");
    stamp.caveats.push_back("Screened central-field mode is phenomenological and intended for qualitative trends.");
    return stamp;
}

ValidationRecord makeEnergyValidation(const std::string& caseName,
                                      double referenceValue,
                                      double measuredValue,
                                      double tolerance,
                                      const std::string& notes = {}) {
    ValidationRecord record;
    record.status = (std::abs(measuredValue - referenceValue) <= tolerance) ? ValidationStatus::ReferenceMatched
                                                                             : ValidationStatus::SanityChecked;
    record.caseName = caseName;
    record.referenceName = "Hydrogenic analytic energy";
    record.units = "eV";
    record.referenceValue = referenceValue;
    record.measuredValue = measuredValue;
    record.absoluteError = std::abs(measuredValue - referenceValue);
    record.relativeError =
        (std::abs(referenceValue) > 1.0e-30) ? (record.absoluteError / std::abs(referenceValue)) : 0.0;
    record.notes = notes;
    record.source = numericalBenchmarkSource();
    return record;
}

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
    std::vector<double> radiiBohr;
    double stepBohr = 0.0;
};

TridiagonalSystem buildCentralFieldSystem(const NumericalSolverRequest& request, int gridPoints) {
    TridiagonalSystem system;
    system.diagonal.resize(static_cast<std::size_t>(gridPoints));
    system.offDiagonal.assign(static_cast<std::size_t>(std::max(0, gridPoints - 1)), 0.0);
    system.radiiBohr.resize(static_cast<std::size_t>(gridPoints));

    const double maxRadiusBohr = std::max(request.maxScaledRadius, 5.0);
    system.stepBohr = maxRadiusBohr / static_cast<double>(gridPoints + 1);
    const double invH2 = 1.0 / (system.stepBohr * system.stepBohr);
    const double muFactor = reducedMassFactorForSolver(request.nuclearMassKg, request.useReducedMass);
    const double kineticPrefactor = 1.0 / (2.0 * muFactor);
    const double off = -kineticPrefactor * invH2;

    for (int index = 0; index < gridPoints; ++index) {
        const double radiusBohr = system.stepBohr * static_cast<double>(index + 1);
        system.radiiBohr[static_cast<std::size_t>(index)] = radiusBohr;
        const double centrifugal =
            kineticPrefactor * static_cast<double>(request.qn.l * (request.qn.l + 1)) / (radiusBohr * radiusBohr);
        const double potential = centralPotentialHartree(request.centralField, radiusBohr);
        system.diagonal[static_cast<std::size_t>(index)] = (2.0 * kineticPrefactor * invH2) + centrifugal + potential;
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
        result.message = "有限差分求解请求无效";
        return result;
    }

    const auto system = buildCentralFieldSystem(request, gridPoints);
    const double energyHartree = bisectEigenvalue(system, radialStateIndex);
    const auto eigenvector = inverseIterationEigenvector(system, energyHartree);

    const double energyJ = energyHartree * 2.0 * constants::rydbergEnergyJ;
    result.energyEv = units::jouleToEv(energyJ);
    result.errorEv = std::abs(result.energyEv - result.analyticalEnergyEv);
    result.gridStepMeters = system.stepBohr * constants::bohrRadiusM;

    double norm = 0.0;
    for (const double value : eigenvector) {
        norm += value * value * result.gridStepMeters;
    }
    norm = std::sqrt(std::max(norm, 1e-30));

    result.radiiMeters.reserve(eigenvector.size());
    result.radialFunction.reserve(eigenvector.size());
    result.radialDistribution.reserve(eigenvector.size());
    for (std::size_t index = 0; index < eigenvector.size(); ++index) {
        const double radius = system.radiiBohr[index] * constants::bohrRadiusM;
        const double u = eigenvector[index] / norm;
        const double radial = u / std::max(radius, 1e-20);
        result.radiiMeters.push_back(radius);
        result.radialFunction.push_back(radial);
        result.radialDistribution.emplace_back(radius, u * u);
    }

    result.converged = true;
    result.message = (request.centralField.mode == CentralFieldMode::ScreenedCentralField)
                         ? "有限差分中心场本征求解已收敛"
                         : "有限差分库仑本征求解已收敛";
    return result;
}

} // namespace

NumericalSolverResult SchrodingerNumericalSolver::solve(const NumericalSolverRequest& request) const {
    NumericalSolverResult result;
    result.method = makeSolverMethodStamp();
    if (request.qn.n <= 0 || request.qn.l < 0 || request.qn.l >= request.qn.n) {
        result.message = "量子数无效";
        return result;
    }

    result.centralFieldProfile =
        sampleCentralFieldProfile(request.centralField, request.maxScaledRadius, 160);

    const auto base = solveSingle(request, request.gridPoints);
    result.converged = base.converged;
    result.message = base.message;
    result.energyEv = base.energyEv;
    result.analyticalEnergyEv = base.analyticalEnergyEv;
    result.errorEv = base.errorEv;
    result.radiiMeters = base.radiiMeters;
    result.radialFunction = base.radialFunction;
    result.radialDistribution = base.radialDistribution;
    result.validation.insert(result.validation.end(),
                             result.centralFieldProfile.validation.begin(),
                             result.centralFieldProfile.validation.end());
    result.validation.push_back(makeEnergyValidation("Numerical eigen solve",
                                                     result.analyticalEnergyEv,
                                                     result.energyEv,
                                                     0.2,
                                                     "Primary solver result compared against the analytic hydrogenic value."));

    if (!base.converged) {
        return result;
    }

    for (int pass = 0; pass < request.convergencePasses; ++pass) {
        const int divisor = 1 << (request.convergencePasses - pass - 1);
        const int gridPoints = std::max(128, request.gridPoints / divisor);
        const auto sample = solveSingle(request, gridPoints);
        if (sample.converged) {
            NumericalConvergenceSample convergenceSample;
            convergenceSample.gridPoints = gridPoints;
            convergenceSample.gridStepMeters = sample.gridStepMeters;
            convergenceSample.energyEv = sample.energyEv;
            convergenceSample.errorEv = sample.errorEv;
            convergenceSample.validation = makeEnergyValidation("Grid refinement sample",
                                                                sample.analyticalEnergyEv,
                                                                sample.energyEv,
                                                                0.3,
                                                                "Convergence sample for the finite-difference radial solver.");
            result.convergence.push_back(std::move(convergenceSample));
        }
    }

    std::sort(result.convergence.begin(), result.convergence.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.gridStepMeters > rhs.gridStepMeters;
    });
    return result;
}

} // namespace quantum::physics
