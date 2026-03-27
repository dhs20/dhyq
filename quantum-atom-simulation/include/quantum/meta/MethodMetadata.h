#pragma once

#include <string>
#include <vector>

namespace quantum::meta {

enum class ModelTier {
    Tier0Teaching,
    Tier1CentralField,
    Tier2MeanField,
    Tier3SpectroscopyCorrections,
    Tier4Correlation,
    Tier5Dynamics
};

enum class ValidationStatus {
    NotValidated,
    SanityChecked,
    Benchmarked,
    ReferenceMatched,
    ExperimentalComparison
};

enum class AnimationKind {
    None,
    Illustrative,
    Pedagogical,
    BasisSuperposition,
    TrueTimeDependent
};

struct SourceRecord {
    std::string id;
    std::string title;
    std::string provider;
    std::string revision;
    std::string license;
    std::string url;
};

struct ProvenanceRecord {
    std::string recordId;
    std::string sourceId;
    std::string createdBy;
    std::string notes;
};

struct MethodStamp {
    std::string methodName;
    std::string approximation;
    std::string dataSource;
    std::string validationSummary;
    ModelTier tier = ModelTier::Tier0Teaching;
    AnimationKind animationKind = AnimationKind::None;
    bool includesElectronCorrelation = false;
    bool enforcesAntisymmetry = false;
    bool includesRelativity = false;
    bool includesExternalField = false;
    bool isTimeDependent = false;
    std::vector<std::string> caveats;
};

struct ValidationRecord {
    ValidationStatus status = ValidationStatus::NotValidated;
    std::string caseName;
    std::string referenceName;
    std::string units;
    double referenceValue = 0.0;
    double measuredValue = 0.0;
    double absoluteError = 0.0;
    double relativeError = 0.0;
    std::string notes;
    SourceRecord source;
};

} // namespace quantum::meta
