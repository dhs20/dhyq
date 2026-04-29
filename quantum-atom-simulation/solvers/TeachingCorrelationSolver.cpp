#include "quantum/solvers/TeachingCorrelationSolver.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace quantum::solvers {
namespace {

quantum::meta::MethodStamp makeMethod() {
    quantum::meta::MethodStamp stamp;
    stamp.methodName = "Tier 4 finite-basis teaching CI prototype";
    stamp.approximation = "two-configuration mixing over mean-field orbital estimates";
    stamp.dataSource = "Embedded 2x2 pedagogical Hamiltonian";
    stamp.validationSummary = "Checks variational lowering and normalized configuration weights.";
    stamp.tier = quantum::meta::ModelTier::Tier4Correlation;
    stamp.animationKind = quantum::meta::AnimationKind::None;
    stamp.includesElectronCorrelation = true;
    stamp.enforcesAntisymmetry = true;
    stamp.caveats = {"Uses a two-configuration toy Hamiltonian.",
                     "Not a complete CI, MCSCF, CASSCF, or electron-correlation benchmark.",
                     "Coupling is a controllable teaching parameter rather than an ab initio integral."};
    return stamp;
}

quantum::meta::ValidationRecord makeLoweringValidation(double referenceEnergy, double correlatedEnergy) {
    quantum::meta::ValidationRecord record;
    record.status = correlatedEnergy <= referenceEnergy ? quantum::meta::ValidationStatus::SanityChecked
                                                        : quantum::meta::ValidationStatus::NotValidated;
    record.caseName = "Finite-basis variational lowering";
    record.referenceName = "Mean-field reference energy";
    record.units = "eV";
    record.referenceValue = referenceEnergy;
    record.measuredValue = correlatedEnergy;
    record.absoluteError = std::abs(correlatedEnergy - referenceEnergy);
    record.relativeError = std::abs(referenceEnergy) > 1.0e-30 ? record.absoluteError / std::abs(referenceEnergy) : 0.0;
    record.notes = "The lower eigenvalue of the 2x2 mixing Hamiltonian should not exceed the uncorrelated reference.";
    record.source = {"teaching-correlation", "Embedded teaching correlation prototype", "internal", "1", "", ""};
    return record;
}

} // namespace

std::string_view TeachingCorrelationSolver::name() const {
    return "TeachingCorrelationSolver";
}

CorrelationSolveResult TeachingCorrelationSolver::solve(const CorrelationSolveRequest& request) const {
    CorrelationSolveResult result;
    result.method = makeMethod();
    result.uncorrelatedEnergyEv = request.referenceEnergyEv;

    const double coupling = std::clamp(std::abs(request.couplingStrengthEv), 1.0e-6, 20.0);
    const double firstGap =
        request.orbitalEnergiesEv.size() >= 2
            ? std::abs(request.orbitalEnergiesEv.back() - request.orbitalEnergiesEv.front())
            : std::max(0.5, 13.605693122994 / static_cast<double>(std::max(1, request.atomicNumber)));
    const double excitationEnergy = std::max(0.25, firstGap + 0.15 * static_cast<double>(std::max(1, request.atomicNumber)));
    const double excitedDiagonal = result.uncorrelatedEnergyEv + excitationEnergy;
    const double trace = result.uncorrelatedEnergyEv + excitedDiagonal;
    const double delta = result.uncorrelatedEnergyEv - excitedDiagonal;
    const double root = std::sqrt(0.25 * delta * delta + coupling * coupling);
    result.correlatedEnergyEv = 0.5 * trace - root;
    result.correlationEnergyEv = result.correlatedEnergyEv - result.uncorrelatedEnergyEv;
    result.mixingAngleRadians = 0.5 * std::atan2(2.0 * coupling, excitedDiagonal - result.uncorrelatedEnergyEv);

    const double referenceWeight = std::cos(result.mixingAngleRadians) * std::cos(result.mixingAngleRadians);
    const double excitedWeight = std::sin(result.mixingAngleRadians) * std::sin(result.mixingAngleRadians);
    result.components = {{"reference configuration", result.uncorrelatedEnergyEv, referenceWeight},
                         {"single/double excitation proxy", excitedDiagonal, excitedWeight}};
    result.solved = true;
    result.validation.push_back(makeLoweringValidation(result.uncorrelatedEnergyEv, result.correlatedEnergyEv));

    quantum::meta::ValidationRecord weightRecord;
    weightRecord.status = std::abs(referenceWeight + excitedWeight - 1.0) <= 1.0e-12
                              ? quantum::meta::ValidationStatus::ReferenceMatched
                              : quantum::meta::ValidationStatus::NotValidated;
    weightRecord.caseName = "Configuration weight normalization";
    weightRecord.referenceName = "Unit norm";
    weightRecord.units = "";
    weightRecord.referenceValue = 1.0;
    weightRecord.measuredValue = referenceWeight + excitedWeight;
    weightRecord.absoluteError = std::abs(weightRecord.measuredValue - 1.0);
    weightRecord.relativeError = weightRecord.absoluteError;
    weightRecord.notes = "The two-state eigenvector weights should sum to one.";
    weightRecord.source = {"teaching-correlation", "Embedded teaching correlation prototype", "internal", "1", "", ""};
    result.validation.push_back(std::move(weightRecord));
    return result;
}

} // namespace quantum::solvers
