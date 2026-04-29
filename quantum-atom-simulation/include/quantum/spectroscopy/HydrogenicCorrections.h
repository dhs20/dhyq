#pragma once

#include "quantum/meta/MethodMetadata.h"
#include "quantum/physics/AtomicPhysics.h"

#include <string>
#include <vector>

namespace quantum::spectroscopy {

struct BranchChoice {
    bool lowerJBranch = false;
};

struct SpectroscopySettings {
    bool applyFineStructure = false;
    bool applyZeeman = false;
    bool applyStark = false;
    bool applyHyperfine = false;
    BranchChoice initialBranch;
    BranchChoice finalBranch;
    double magneticFieldTesla = 0.0;
    double electricFieldVPerM = 0.0;
    double nuclearSpin = 0.5;
    double hyperfineAConstantMicroEv = 5.87433;
};

struct StateCorrection {
    double j = 0.5;
    double mJ = 0.5;
    double f = 1.0;
    double landeG = 2.0;
    double fineStructureShiftEv = 0.0;
    double zeemanShiftEv = 0.0;
    double starkShiftEv = 0.0;
    double hyperfineShiftEv = 0.0;
    double totalShiftEv = 0.0;
    double correctedEnergyEv = 0.0;
    std::string termSymbol;
};

struct SpectroscopyCorrectionResult {
    bool applied = false;
    bool starkApproximationLimited = false;
    std::string notes;
    StateCorrection initial;
    StateCorrection final;
    double correctedDeltaEnergyEv = 0.0;
    double correctedWavelengthNm = 0.0;
    std::string initialTermSymbol;
    std::string finalTermSymbol;
    quantum::meta::MethodStamp method;
    std::vector<quantum::meta::ValidationRecord> validation;
};

[[nodiscard]] SpectroscopyCorrectionResult evaluateHydrogenicCorrections(
    const quantum::physics::TransitionRequest& request,
    const quantum::physics::TransitionResult& baseTransition,
    const SpectroscopySettings& settings,
    double nuclearMassKg);

} // namespace quantum::spectroscopy
