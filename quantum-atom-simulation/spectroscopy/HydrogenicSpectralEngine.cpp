#include "quantum/spectroscopy/HydrogenicSpectralEngine.h"

#include "quantum/physics/AtomicPhysics.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace quantum::spectroscopy {
namespace {

quantum::meta::MethodStamp makeMethod() {
    quantum::meta::MethodStamp stamp;
    stamp.methodName = "Hydrogenic spectral record engine";
    stamp.approximation = "state/transition records from hydrogenic selection rules";
    stamp.dataSource = "Embedded hydrogenic formulas";
    stamp.validationSummary = "Uses the same transition checks as the analytic hydrogenic test path.";
    stamp.tier = quantum::meta::ModelTier::Tier3SpectroscopyCorrections;
    stamp.animationKind = quantum::meta::AnimationKind::None;
    stamp.caveats = {"Produces record-oriented teaching spectra.",
                     "Complex atoms still require external reference data or higher-fidelity solvers."};
    return stamp;
}

quantum::data::StateRecord makeState(int atomicNumber,
                                     int chargeState,
                                     const quantum::physics::QuantumNumbers& qn,
                                     double zeff,
                                     double nuclearMassKg,
                                     bool upper) {
    quantum::data::StateRecord state;
    state.atomicNumber = atomicNumber;
    state.chargeState = chargeState;
    state.n = qn.n;
    state.l = qn.l;
    state.m = qn.m;
    state.label = std::string(upper ? "upper" : "lower") + " n=" + std::to_string(qn.n) + " l=" + std::to_string(qn.l);
    state.energyEv = quantum::physics::computeHydrogenicMetrics(
                         atomicNumber, qn.n, zeff, nuclearMassKg, true)
                         .energyEv;
    state.method = makeMethod();
    state.validation = {quantum::meta::ValidationStatus::SanityChecked,
                        "Hydrogenic spectral state",
                        "Embedded analytic energy",
                        "eV",
                        state.energyEv,
                        state.energyEv,
                        0.0,
                        0.0,
                        "State generated from record-oriented hydrogenic spectral engine.",
                        {"hydrogenic-spectral-engine", "Hydrogenic spectral record engine", "internal", "1", "", ""}};
    state.provenance = {"spectral-state-" + std::to_string(atomicNumber) + "-" + std::to_string(chargeState) + "-" +
                            std::to_string(qn.n) + "-" + std::to_string(qn.l) + "-" + std::to_string(qn.m),
                        "hydrogenic-spectral-engine",
                        "spectroscopy::HydrogenicSpectralEngine",
                        "Generated from embedded hydrogenic formulas."};
    return state;
}

} // namespace

std::string_view HydrogenicSpectralEngine::name() const {
    return "HydrogenicSpectralEngine";
}

SpectralResult HydrogenicSpectralEngine::build(const SpectralRequest& request) const {
    SpectralResult result;
    result.method = makeMethod();
    const int atomicNumber = std::max(1, request.atomicNumber);
    const int chargeState = std::max(0, request.chargeState);
    const int maxN = std::clamp(request.maxPrincipalNumber, 2, 12);
    const double zeff = static_cast<double>(atomicNumber);
    const double nuclearMassKg =
        static_cast<double>(std::max(1, atomicNumber)) * 1836.15267343 * quantum::physics::constants::electronMassKg;

    for (int ni = 2; ni <= maxN; ++ni) {
        for (int nf = 1; nf < ni; ++nf) {
            quantum::physics::TransitionRequest transitionRequest;
            transitionRequest.initial = {ni, 1, 0};
            transitionRequest.final = {nf, 0, 0};
            transitionRequest.rules = request.applySelectionRules ? quantum::physics::TransitionRuleSet::ElectricDipole
                                                                  : quantum::physics::TransitionRuleSet::None;
            transitionRequest.nuclearCharge = atomicNumber;
            transitionRequest.initialZeff = zeff;
            transitionRequest.finalZeff = zeff;
            transitionRequest.nuclearMassKg = nuclearMassKg;
            transitionRequest.useReducedMass = true;
            const auto transition = quantum::physics::computeTransition(transitionRequest);

            quantum::data::TransitionRecord record;
            record.label = transition.seriesName + " n=" + std::to_string(ni) + "->" + std::to_string(nf);
            record.upper = makeState(atomicNumber, chargeState, transitionRequest.initial, zeff, nuclearMassKg, true);
            record.lower = makeState(atomicNumber, chargeState, transitionRequest.final, zeff, nuclearMassKg, false);
            record.wavelengthNm = transition.wavelengthNm;
            record.frequencyHz = transition.frequencyHz;
            record.photonEnergyEv = std::abs(transition.deltaEnergyEv);
            record.allowed = transition.allowed;
            record.method = result.method;
            record.validation = transition.validation.empty()
                                    ? quantum::meta::ValidationRecord{}
                                    : transition.validation.front();
            record.provenance = {"spectral-transition-" + std::to_string(atomicNumber) + "-" + std::to_string(ni) +
                                     "-" + std::to_string(nf),
                                 "hydrogenic-spectral-engine",
                                 "spectroscopy::HydrogenicSpectralEngine",
                                 "Generated as a structured transition record."};
            result.transitions.push_back(std::move(record));
        }
    }

    quantum::meta::ValidationRecord record;
    record.status = !result.transitions.empty() ? quantum::meta::ValidationStatus::SanityChecked
                                                : quantum::meta::ValidationStatus::NotValidated;
    record.caseName = "Spectral record generation";
    record.referenceName = "Non-empty transition set";
    record.units = "count";
    record.referenceValue = 1.0;
    record.measuredValue = static_cast<double>(result.transitions.size());
    record.absoluteError = std::abs(record.measuredValue - record.referenceValue);
    record.notes = "Structured spectral engine should produce transition records for max n >= 2.";
    record.source = {"hydrogenic-spectral-engine", "Hydrogenic spectral record engine", "internal", "1", "", ""};
    result.validation.push_back(std::move(record));
    return result;
}

} // namespace quantum::spectroscopy
