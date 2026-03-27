#include "quantum/physics/CentralField.h"

#include <algorithm>
#include <cmath>

namespace quantum::physics {
namespace {

quantum::meta::MethodStamp makeCentralFieldMethodStamp(const CentralFieldParameters& parameters) {
    quantum::meta::MethodStamp stamp;
    stamp.methodName = (parameters.mode == CentralFieldMode::ScreenedCentralField)
                           ? "Tier 1 screened central field"
                           : "Hydrogenic Coulomb central field";
    stamp.approximation = (parameters.mode == CentralFieldMode::ScreenedCentralField)
                              ? "phenomenological screened single-electron central field"
                              : "analytic Coulomb central field";
    stamp.dataSource = "Embedded central-field model";
    stamp.validationSummary =
        (parameters.mode == CentralFieldMode::ScreenedCentralField)
            ? "Phenomenological model used for qualitative central-field trends."
            : "Matches the hydrogenic Coulomb model exactly.";
    stamp.tier = quantum::meta::ModelTier::Tier1CentralField;
    stamp.animationKind = quantum::meta::AnimationKind::None;
    stamp.caveats.push_back("This is still a single-electron central-field approximation.");
    if (parameters.mode == CentralFieldMode::ScreenedCentralField) {
        stamp.caveats.push_back("Screening length and residual charge are phenomenological parameters.");
    }
    return stamp;
}

quantum::meta::ValidationRecord makePotentialValidation(const CentralFieldParameters& parameters,
                                                        double nearCharge,
                                                        double farCharge) {
    quantum::meta::ValidationRecord record;
    record.caseName = "Central field endpoint behaviour";
    record.referenceName = "Configured near/far charge limits";
    record.units = "dimensionless";
    record.referenceValue = effectiveResidualCharge(parameters);
    record.measuredValue = farCharge;
    record.absoluteError = std::abs(farCharge - record.referenceValue);
    record.relativeError = (std::abs(record.referenceValue) > 1.0e-30)
                               ? (record.absoluteError / std::abs(record.referenceValue))
                               : 0.0;
    record.status = (parameters.mode == CentralFieldMode::HydrogenicCoulomb ||
                     (std::abs(nearCharge - static_cast<double>(parameters.nuclearCharge)) < 1.0e-2 &&
                      record.absoluteError < 1.0e-2))
                        ? quantum::meta::ValidationStatus::SanityChecked
                        : quantum::meta::ValidationStatus::NotValidated;
    record.notes = "Checks that the screened field approaches the expected near-nucleus and far-field charges.";
    record.source = {"central-field-model", "Embedded central-field model", "internal", "1", "", ""};
    return record;
}

} // namespace

double effectiveResidualCharge(const CentralFieldParameters& parameters) {
    if (!parameters.autoResidualCharge) {
        return std::max(parameters.residualCharge, 1.0e-6);
    }
    return std::max(1.0, static_cast<double>(parameters.ionCharge + 1));
}

double effectiveChargeAtBohr(const CentralFieldParameters& parameters, double radiusBohr) {
    const double r = std::max(radiusBohr, 1.0e-8);
    if (parameters.mode == CentralFieldMode::HydrogenicCoulomb) {
        return std::max(parameters.zeff, 1.0e-6);
    }

    const double farCharge = effectiveResidualCharge(parameters);
    const double nearCharge = std::max(static_cast<double>(parameters.nuclearCharge), farCharge);
    const double length = std::max(parameters.screeningLengthBohr, 1.0e-4);
    return farCharge + (nearCharge - farCharge) * std::exp(-r / length);
}

double centralPotentialHartree(const CentralFieldParameters& parameters, double radiusBohr) {
    const double r = std::max(radiusBohr, 1.0e-8);
    return -effectiveChargeAtBohr(parameters, r) / r;
}

CentralFieldProfile sampleCentralFieldProfile(const CentralFieldParameters& parameters, double maxRadiusBohr, int samples) {
    CentralFieldProfile profile;
    profile.parameters = parameters;
    profile.method = makeCentralFieldMethodStamp(parameters);
    profile.description =
        (parameters.mode == CentralFieldMode::ScreenedCentralField)
            ? "Near nucleus the field approaches -Z/r, while far from the atom it tends toward a residual screened charge."
            : "Pure Coulomb central field with a constant effective charge.";

    const int count = std::max(samples, 16);
    const double maxRadius = std::max(maxRadiusBohr, 5.0);
    profile.samples.reserve(static_cast<std::size_t>(count));
    for (int index = 0; index < count; ++index) {
        const double t = static_cast<double>(index) / static_cast<double>(count - 1);
        const double radiusBohr = 1.0e-4 + t * maxRadius;
        profile.samples.push_back({radiusBohr,
                                   effectiveChargeAtBohr(parameters, radiusBohr),
                                   centralPotentialHartree(parameters, radiusBohr)});
    }

    const double nearCharge = profile.samples.front().effectiveCharge;
    const double farCharge = profile.samples.back().effectiveCharge;
    profile.validation.push_back(makePotentialValidation(parameters, nearCharge, farCharge));
    return profile;
}

} // namespace quantum::physics
