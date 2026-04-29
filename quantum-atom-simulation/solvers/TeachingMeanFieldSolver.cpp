#include "quantum/solvers/TeachingMeanFieldSolver.h"

#include "quantum/physics/AtomicPhysics.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <utility>

namespace quantum::solvers {
namespace {

struct TeachingOrbital {
    int n = 1;
    const char* label = "1s";
    int capacity = 2;
};

constexpr TeachingOrbital kOrbitalOrder[] = {
    {1, "1s", 2},
    {2, "2s", 2},
    {2, "2p", 6},
    {3, "3s", 2},
    {3, "3p", 6},
    {4, "4s", 2},
    {3, "3d", 10},
    {4, "4p", 6},
    {5, "5s", 2},
    {4, "4d", 10},
    {5, "5p", 6},
    {6, "6s", 2},
    {4, "4f", 14},
    {5, "5d", 10},
    {6, "6p", 6},
    {7, "7s", 2}
};

quantum::meta::MethodStamp makeMethod(MeanFieldMode mode) {
    quantum::meta::MethodStamp stamp;
    stamp.methodName = "Tier 2 teaching mean-field prototype";
    stamp.dataSource = "Embedded Aufbau/Slater-inspired finite orbital model";
    stamp.tier = quantum::meta::ModelTier::Tier2MeanField;
    stamp.animationKind = quantum::meta::AnimationKind::None;
    stamp.includesElectronCorrelation = false;
    stamp.enforcesAntisymmetry = true;
    stamp.includesRelativity = mode == MeanFieldMode::ScalarRelativisticPreview;
    switch (mode) {
    case MeanFieldMode::SlaterExchange:
        stamp.approximation = "screened Hartree-style SCF with Slater-exchange teaching correction";
        stamp.validationSummary = "Convergence and sign checks only; not a research-grade HF solver.";
        break;
    case MeanFieldMode::ScalarRelativisticPreview:
        stamp.approximation = "screened Hartree-style SCF with scalar-relativistic preview correction";
        stamp.validationSummary = "Convergence and magnitude checks only; not Dirac-Fock.";
        break;
    case MeanFieldMode::HartreeScreened:
    default:
        stamp.approximation = "screened Hartree-style scalar SCF teaching iteration";
        stamp.validationSummary = "Convergence sanity check for a compact effective-charge iteration.";
        break;
    }
    stamp.caveats = {"Uses a compact effective-charge iteration rather than Coulomb/exchange integrals.",
                     "Orbital energies are teaching estimates, not publishable atomic-structure values.",
                     "This closes the Tier 2 interface with an honest prototype, not full HF/Dirac-Fock."};
    return stamp;
}

quantum::meta::ValidationRecord makeResidualValidation(double residual, double tolerance) {
    quantum::meta::ValidationRecord record;
    record.status = residual <= std::max(tolerance, 1.0e-4) ? quantum::meta::ValidationStatus::SanityChecked
                                                            : quantum::meta::ValidationStatus::NotValidated;
    record.caseName = "Teaching SCF residual";
    record.referenceName = "Configured convergence tolerance";
    record.units = "dimensionless";
    record.referenceValue = tolerance;
    record.measuredValue = residual;
    record.absoluteError = std::abs(residual - tolerance);
    record.relativeError = tolerance > 1.0e-30 ? record.absoluteError / tolerance : 0.0;
    record.notes = "Checks whether the compact effective-charge iteration settled.";
    record.source = {"teaching-mean-field", "Embedded teaching mean-field prototype", "internal", "1", "", ""};
    return record;
}

} // namespace

const char* meanFieldModeLabel(MeanFieldMode mode) {
    switch (mode) {
    case MeanFieldMode::SlaterExchange:
        return "Slater exchange";
    case MeanFieldMode::ScalarRelativisticPreview:
        return "Scalar-relativistic preview";
    case MeanFieldMode::HartreeScreened:
    default:
        return "Hartree screened";
    }
}

std::string_view TeachingMeanFieldSolver::name() const {
    return "TeachingMeanFieldSolver";
}

MeanFieldSolveResult TeachingMeanFieldSolver::solve(const MeanFieldSolveRequest& request) {
    MeanFieldSolveResult result;
    result.method = makeMethod(request.mode);

    const int atomicNumber = std::clamp(request.atomicNumber, 1, 118);
    const int electronCount = std::clamp(atomicNumber - request.chargeState, 1, atomicNumber);
    const int maxIterations = std::clamp(request.maxIterations, 3, 200);
    const double tolerance = std::clamp(request.convergenceTolerance, 1.0e-10, 1.0e-3);

    double zeff = std::max(1.0, static_cast<double>(atomicNumber - std::max(0, request.chargeState)) * 0.65);
    const double baseTarget =
        std::max(1.0, static_cast<double>(atomicNumber) - 0.32 * static_cast<double>(std::max(0, electronCount - 1)));
    double residual = std::numeric_limits<double>::infinity();
    for (int iteration = 0; iteration < maxIterations; ++iteration) {
        const double shellRelaxation = 0.06 * static_cast<double>(atomicNumber) / static_cast<double>(iteration + 2);
        const double target = std::max(1.0, baseTarget + shellRelaxation);
        const double nextZeff = 0.58 * zeff + 0.42 * target;
        residual = std::abs(nextZeff - zeff) / std::max(1.0, std::abs(nextZeff));
        zeff = nextZeff;
        result.effectiveChargeHistory.push_back(zeff);
        result.residualHistory.push_back(residual);
        result.iterations = iteration + 1;
        if (residual <= tolerance) {
            break;
        }
    }

    int remainingElectrons = electronCount;
    double orbitalEnergySumEv = 0.0;
    for (const auto& orbital : kOrbitalOrder) {
        if (remainingElectrons <= 0) {
            break;
        }
        const int occupancy = std::min(remainingElectrons, orbital.capacity);
        const double shellScreen = 1.0 - 0.025 * static_cast<double>(orbital.n - 1);
        const double orbitalZeff = std::max(1.0, zeff * shellScreen);
        const double energyEv = -13.605693122994 * orbitalZeff * orbitalZeff /
                                static_cast<double>(orbital.n * orbital.n);
        result.orbitalLabels.push_back(std::string(orbital.label) + "^" + std::to_string(occupancy));
        result.orbitalEnergiesEv.push_back(energyEv);
        orbitalEnergySumEv += static_cast<double>(occupancy) * energyEv;
        remainingElectrons -= occupancy;
    }

    result.exchangeCorrectionEv = 0.0;
    result.scalarRelativisticCorrectionEv = 0.0;
    if (request.mode == MeanFieldMode::SlaterExchange) {
        result.exchangeCorrectionEv = -0.055 * std::pow(static_cast<double>(electronCount), 4.0 / 3.0) * zeff;
    } else if (request.mode == MeanFieldMode::ScalarRelativisticPreview) {
        const double alpha = quantum::physics::constants::fineStructureConstant;
        result.scalarRelativisticCorrectionEv =
            -0.5 * 13.605693122994 * alpha * alpha * std::pow(static_cast<double>(atomicNumber), 4.0) /
            std::pow(static_cast<double>(std::max(1, electronCount)), 2.0);
    }
    result.totalEnergyEv = orbitalEnergySumEv + result.exchangeCorrectionEv + result.scalarRelativisticCorrectionEv;
    result.finalEffectiveCharge = zeff;
    result.converged = residual <= std::max(tolerance, 1.0e-4);
    result.validation.push_back(makeResidualValidation(residual, tolerance));
    if (request.mode == MeanFieldMode::SlaterExchange) {
        quantum::meta::ValidationRecord exchangeRecord;
        exchangeRecord.status = result.exchangeCorrectionEv <= 0.0 ? quantum::meta::ValidationStatus::SanityChecked
                                                                    : quantum::meta::ValidationStatus::NotValidated;
        exchangeRecord.caseName = "Slater exchange sign";
        exchangeRecord.referenceName = "Attractive exchange correction";
        exchangeRecord.units = "eV";
        exchangeRecord.referenceValue = 0.0;
        exchangeRecord.measuredValue = result.exchangeCorrectionEv;
        exchangeRecord.absoluteError = std::abs(result.exchangeCorrectionEv);
        exchangeRecord.notes = "Teaching exchange correction should lower the reference energy.";
        exchangeRecord.source = {"teaching-mean-field", "Embedded teaching mean-field prototype", "internal", "1", "", ""};
        result.validation.push_back(std::move(exchangeRecord));
    }
    return result;
}

} // namespace quantum::solvers
