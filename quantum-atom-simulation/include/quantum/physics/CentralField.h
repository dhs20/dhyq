#pragma once

#include "quantum/meta/MethodMetadata.h"

#include <string>
#include <vector>

namespace quantum::physics {

enum class CentralFieldMode {
    HydrogenicCoulomb,
    ScreenedCentralField
};

struct CentralFieldParameters {
    CentralFieldMode mode = CentralFieldMode::HydrogenicCoulomb;
    int nuclearCharge = 1;
    int ionCharge = 0;
    double zeff = 1.0;
    bool autoResidualCharge = true;
    double residualCharge = 1.0;
    double screeningLengthBohr = 1.5;
};

struct CentralFieldSample {
    double radiusBohr = 0.0;
    double effectiveCharge = 1.0;
    double potentialHartree = 0.0;
};

struct CentralFieldProfile {
    CentralFieldParameters parameters;
    std::string description;
    std::vector<CentralFieldSample> samples;
    quantum::meta::MethodStamp method;
    std::vector<quantum::meta::ValidationRecord> validation;
};

[[nodiscard]] double effectiveResidualCharge(const CentralFieldParameters& parameters);
[[nodiscard]] double effectiveChargeAtBohr(const CentralFieldParameters& parameters, double radiusBohr);
[[nodiscard]] double centralPotentialHartree(const CentralFieldParameters& parameters, double radiusBohr);
[[nodiscard]] CentralFieldProfile sampleCentralFieldProfile(const CentralFieldParameters& parameters,
                                                            double maxRadiusBohr,
                                                            int samples);

} // namespace quantum::physics
