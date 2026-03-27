#pragma once

#include "quantum/meta/MethodMetadata.h"

#include <string>
#include <vector>

namespace quantum::data {

struct ElementRecord {
    int atomicNumber = 0;
    std::string symbol;
    std::string name;
    std::string localizedNameZh;
    double atomicMassU = 0.0;
    std::vector<int> supportedChargeStates;
    quantum::meta::MethodStamp method;
    quantum::meta::SourceRecord source;
};

struct IsotopeRecord {
    int atomicNumber = 0;
    int massNumber = 0;
    double atomicMassU = 0.0;
    double naturalAbundance = 0.0;
    quantum::meta::SourceRecord source;
};

struct ConfigurationRecord {
    int atomicNumber = 0;
    int chargeState = 0;
    std::string notation;
    std::vector<std::string> occupiedSubshells;
    quantum::meta::MethodStamp method;
    quantum::meta::ValidationRecord validation;
};

struct StateRecord {
    int atomicNumber = 0;
    int chargeState = 0;
    std::string label;
    int n = 0;
    int l = 0;
    int m = 0;
    double energyEv = 0.0;
    quantum::meta::MethodStamp method;
    quantum::meta::ValidationRecord validation;
    quantum::meta::ProvenanceRecord provenance;
};

struct TransitionRecord {
    std::string label;
    StateRecord upper;
    StateRecord lower;
    double wavelengthNm = 0.0;
    double frequencyHz = 0.0;
    double photonEnergyEv = 0.0;
    bool allowed = false;
    quantum::meta::MethodStamp method;
    quantum::meta::ValidationRecord validation;
    quantum::meta::ProvenanceRecord provenance;
};

struct ScenePresetRecord {
    std::string name;
    std::string approximationMode;
    std::string modelKind;
    int atomicNumber = 0;
    int chargeState = 0;
    quantum::meta::MethodStamp method;
    quantum::meta::SourceRecord source;
    quantum::meta::ProvenanceRecord provenance;
};

} // namespace quantum::data
