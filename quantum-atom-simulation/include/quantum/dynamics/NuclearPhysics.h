#pragma once

#include "quantum/meta/MethodMetadata.h"

#include <string>
#include <vector>

namespace quantum::dynamics {

enum class NuclearCrossSectionModel {
    None,
    DTFusionGamowLike,
    U235ThermalFissionOneOverV
};

struct NuclearStructureRequest {
    int protonNumber = 92;
    int massNumber = 235;
    int isotopeWindow = 12;
};

struct IsotopeStructureSample {
    int massNumber = 0;
    double bindingEnergyMeV = 0.0;
    double bindingPerNucleonMeV = 0.0;
};

struct NuclearStructureResult {
    bool valid = false;
    int protonNumber = 0;
    int massNumber = 0;
    int neutronNumber = 0;
    double bindingEnergyMeV = 0.0;
    double bindingPerNucleonMeV = 0.0;
    double radiusFm = 0.0;
    double pairingTermMeV = 0.0;
    double asymmetryTermMeV = 0.0;
    double coulombTermMeV = 0.0;
    std::vector<IsotopeStructureSample> isotopeChain;
    quantum::meta::MethodStamp method;
    std::vector<quantum::meta::ValidationRecord> validation;
    quantum::meta::SourceRecord source;
};

struct CrossSectionSample {
    double energyEv = 0.0;
    double crossSectionBarn = 0.0;
};

struct NuclearCrossSectionRequest {
    NuclearCrossSectionModel model = NuclearCrossSectionModel::None;
    double queryEnergyEv = 0.0;
    double minEnergyEv = 0.0;
    double maxEnergyEv = 0.0;
    int sampleCount = 256;
};

struct NuclearCrossSectionResult {
    bool valid = false;
    NuclearCrossSectionModel model = NuclearCrossSectionModel::None;
    double queryEnergyEv = 0.0;
    double queryCrossSectionBarn = 0.0;
    double peakEnergyEv = 0.0;
    double peakCrossSectionBarn = 0.0;
    std::vector<CrossSectionSample> samples;
    quantum::meta::MethodStamp method;
    std::vector<quantum::meta::ValidationRecord> validation;
    quantum::meta::SourceRecord source;
};

struct TransportSample {
    double timeSeconds = 0.0;
    double positionM = 0.0;
    double survivalFraction = 1.0;
    double reactedFraction = 0.0;
};

struct NuclearTransportRequest {
    double crossSectionBarn = 0.0;
    double projectileEnergyEv = 0.0253;
    double targetNumberDensityPerM3 = 1.0e28;
    double slabThicknessM = 0.01;
    double totalTimeSeconds = 1.0e-6;
    int sampleCount = 256;
};

struct NuclearTransportResult {
    bool valid = false;
    double projectileSpeedMps = 0.0;
    double macroscopicCrossSectionPerM = 0.0;
    double meanFreePathM = 0.0;
    double transmissionAtExit = 1.0;
    double reactedFractionAtExit = 0.0;
    std::vector<TransportSample> samples;
    quantum::meta::MethodStamp method;
    std::vector<quantum::meta::ValidationRecord> validation;
};

struct DecaySample {
    double timeSeconds = 0.0;
    double parentFraction = 1.0;
    double daughterFraction = 0.0;
    double activityFraction = 1.0;
};

struct NuclearDecayRequest {
    std::string channelLabel = "alpha";
    double halfLifeSeconds = 1.0;
    double totalTimeSeconds = 5.0;
    int sampleCount = 256;
};

struct NuclearDecayResult {
    bool valid = false;
    double halfLifeSeconds = 1.0;
    double decayConstantPerSecond = 0.0;
    double meanLifetimeSeconds = 0.0;
    std::vector<DecaySample> samples;
    quantum::meta::MethodStamp method;
    std::vector<quantum::meta::ValidationRecord> validation;
};

class NuclearPhysicsEngine {
public:
    [[nodiscard]] NuclearStructureResult evaluateStructure(const NuclearStructureRequest& request) const;
    [[nodiscard]] NuclearCrossSectionResult evaluateCrossSection(const NuclearCrossSectionRequest& request) const;
    [[nodiscard]] NuclearTransportResult evaluateTransport(const NuclearTransportRequest& request) const;
    [[nodiscard]] NuclearDecayResult evaluateDecay(const NuclearDecayRequest& request) const;
};

[[nodiscard]] const char* nuclearCrossSectionModelLabel(NuclearCrossSectionModel model);

} // namespace quantum::dynamics
