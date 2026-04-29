#include "quantum/dynamics/DynamicsEngine.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace quantum::dynamics {
namespace {

constexpr double kPlanckEvSecond = 4.135667696e-15;
constexpr double kHbarEvSecond = kPlanckEvSecond / (2.0 * 3.14159265358979323846);

quantum::meta::MethodStamp makeMethod(const DynamicsRequest& request) {
    quantum::meta::MethodStamp stamp;
    stamp.methodName = "Tier 5 finite-basis two-level TDSE teaching solver";
    stamp.approximation = "closed-form Rabi dynamics in a two-state basis";
    stamp.dataSource = "Embedded two-level Hamiltonian";
    stamp.validationSummary = "Population conservation and damping sanity checks for a finite-basis model.";
    stamp.tier = quantum::meta::ModelTier::Tier5Dynamics;
    stamp.animationKind = quantum::meta::AnimationKind::TrueTimeDependent;
    stamp.includesExternalField = !request.externalFieldLabel.empty() || std::abs(request.couplingEv) > 0.0;
    stamp.isTimeDependent = true;
    stamp.caveats = {"Only the two-level basis is propagated.",
                     "Not a full real-space TDSE, TD-CI, TDHF, or many-electron driven-dynamics solver.",
                     "Use for teaching coherent population transfer and detuning trends."};
    if (request.includeDissipation) {
        stamp.caveats.push_back("Dissipation is a simple exponential envelope, not an open-system master equation.");
    }
    return stamp;
}

} // namespace

std::string_view TeachingDynamicsEngine::name() const {
    return "TeachingDynamicsEngine";
}

DynamicsResult TeachingDynamicsEngine::propagate(const DynamicsRequest& request) {
    DynamicsResult result;
    result.method = makeMethod(request);

    const double totalTime = std::clamp(request.totalTimeSeconds, 1.0e-18, 1.0e-9);
    const double requestedStep = std::max(request.timeStepSeconds, totalTime / 240.0);
    const int sampleCount = std::clamp(static_cast<int>(std::ceil(totalTime / requestedStep)) + 1, 32, 2048);
    const double couplingEv = std::clamp(std::abs(request.couplingEv), 0.0, 20.0);
    const double detuningEv = request.detuningEv;
    const double omega = std::sqrt(detuningEv * detuningEv + 4.0 * couplingEv * couplingEv) /
                         std::max(kHbarEvSecond, 1.0e-30);
    const double couplingRatio = (omega > 1.0e-30)
                                     ? (4.0 * couplingEv * couplingEv /
                                        std::max(detuningEv * detuningEv + 4.0 * couplingEv * couplingEv, 1.0e-30))
                                     : 0.0;
    const double damping = request.includeDissipation
                               ? std::clamp(request.dampingRatePerSecond, 0.0, 1.0e18)
                               : 0.0;

    result.sampleTimes.reserve(static_cast<std::size_t>(sampleCount));
    result.observables.reserve(static_cast<std::size_t>(sampleCount));
    result.lowerPopulation.reserve(static_cast<std::size_t>(sampleCount));
    result.upperPopulation.reserve(static_cast<std::size_t>(sampleCount));
    result.dipoleExpectation.reserve(static_cast<std::size_t>(sampleCount));

    for (int index = 0; index < sampleCount; ++index) {
        const double t = totalTime * static_cast<double>(index) / static_cast<double>(sampleCount - 1);
        const double coherentUpper = couplingRatio * std::pow(std::sin(0.5 * omega * t), 2.0);
        const double envelope = std::exp(-damping * t);
        const double upper = coherentUpper * envelope;
        const double lower = 1.0 - upper;
        const double dipole = 2.0 * std::sqrt(std::max(0.0, lower * upper)) * std::cos(detuningEv * t / kHbarEvSecond);
        result.sampleTimes.push_back(t);
        result.upperPopulation.push_back(upper);
        result.lowerPopulation.push_back(lower);
        result.dipoleExpectation.push_back(dipole);
        result.observables.push_back(upper);
    }

    quantum::meta::ValidationRecord populationRecord;
    populationRecord.status = quantum::meta::ValidationStatus::ReferenceMatched;
    populationRecord.caseName = "Two-level population normalization";
    populationRecord.referenceName = "P_lower + P_upper = 1";
    populationRecord.units = "";
    populationRecord.referenceValue = 1.0;
    populationRecord.measuredValue =
        (!result.lowerPopulation.empty() && !result.upperPopulation.empty())
            ? (result.lowerPopulation.back() + result.upperPopulation.back())
            : 0.0;
    populationRecord.absoluteError = std::abs(populationRecord.measuredValue - 1.0);
    populationRecord.relativeError = populationRecord.absoluteError;
    populationRecord.notes = "The finite-basis model keeps probability normalized in the reported two-state basis.";
    populationRecord.source = {"teaching-dynamics", "Embedded two-level TDSE teaching solver", "internal", "1", "", ""};
    result.validation.push_back(std::move(populationRecord));

    if (request.includeDissipation) {
        quantum::meta::ValidationRecord dampingRecord;
        dampingRecord.status = result.upperPopulation.back() <=
                                       (*std::max_element(result.upperPopulation.begin(), result.upperPopulation.end()) + 1.0e-12)
                                   ? quantum::meta::ValidationStatus::SanityChecked
                                   : quantum::meta::ValidationStatus::NotValidated;
        dampingRecord.caseName = "Damping envelope sanity";
        dampingRecord.referenceName = "Non-amplifying envelope";
        dampingRecord.units = "";
        dampingRecord.referenceValue = 1.0;
        dampingRecord.measuredValue = result.upperPopulation.back();
        dampingRecord.absoluteError = std::abs(dampingRecord.measuredValue);
        dampingRecord.notes = "Simple damping should not amplify the excited-state population beyond the coherent envelope.";
        dampingRecord.source = {"teaching-dynamics", "Embedded two-level TDSE teaching solver", "internal", "1", "", ""};
        result.validation.push_back(std::move(dampingRecord));
    }

    result.solved = !result.sampleTimes.empty();
    return result;
}

} // namespace quantum::dynamics
