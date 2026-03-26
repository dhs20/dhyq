#pragma once

#include "quantum/physics/AtomicPhysics.h"
#include "quantum/physics/CloudGenerator.h"
#include "quantum/physics/NumericalSolver.h"

#include <string>
#include <utility>
#include <vector>

namespace quantum::app {

enum class ModelKind {
    Bohr,
    WaveParticle,
    SchrodingerCloud,
    Compare
};

enum class CloudRenderMode {
    PointCloud,
    VolumeSlices,
    Hybrid
};

enum class SamplingQualityPreset {
    Fast,
    Balanced,
    HighFidelity
};

struct TransitionSelection {
    quantum::physics::QuantumNumbers initial{2, 1, 0};
    quantum::physics::QuantumNumbers final{1, 0, 0};
    bool enforceRules = true;
    bool enforceDeltaM = false;
};

struct BohrSettings {
    int principalN = 2;
    int maxSpectrumN = 6;
    bool useReducedMass = true;
};

struct CloudSettings {
    int pointCount = 100000;
    bool enableLod = true;
    bool reduceDuringInteraction = true;
    int mediumLodPointCount = 60000;
    int farLodPointCount = 30000;
    int interactionPointCount = 18000;
    float mediumLodDistance = 14.0f;
    float farLodDistance = 28.0f;
    bool adaptiveSampling = true;
    SamplingQualityPreset samplingQuality = SamplingQualityPreset::Balanced;
    int volumeResolution = 48;
    double extentScale = 14.0;
    bool buildVolume = true;
    bool showPhase = true;
    bool useNumericalRadial = false;
    std::vector<quantum::physics::SuperpositionComponent> components{{{2, 1, 0}, 1.0, 0.0, 1.0}};
};

struct SolverSettings {
    quantum::physics::QuantumNumbers qn{2, 1, 0};
    int gridPoints = 4096;
    int convergencePasses = 5;
    double maxScaledRadius = 120.0;
};

struct ViewSettings {
    CloudRenderMode cloudRenderMode = CloudRenderMode::Hybrid;
    bool showOrbitRings = true;
    bool showGrid = true;
    float pointSize = 3.0f;
    float exposure = 1.0f;
    bool interactionActive = false;
};

struct ReferenceSettings {
    bool enabled = true;
    std::string csvPath = "assets/data/nist_reference_lines.csv";
};

struct DerivedData {
    quantum::physics::ElementRecord element;
    int electronCount = 1;
    bool strictHydrogenicValid = true;
    std::string approximationWarning;
    quantum::physics::ElectronConfigurationResult configuration;
    quantum::physics::SlaterResult slater;
    double activeZeff = 1.0;
    quantum::physics::HydrogenicMetrics bohrMetrics;
    quantum::physics::TransitionResult transition;
    std::vector<quantum::physics::SpectrumLine> spectrumLines;
    std::vector<quantum::physics::SpectrumLine> referenceLines;
    std::vector<quantum::physics::EnergyLevelSample> energyLevels;
    std::vector<std::pair<double, double>> radialDistribution;
    quantum::physics::CloudData cloud;
    quantum::physics::NumericalSolverResult solver;
};

struct DirtyFlags {
    bool physics = true;
    bool cloud = true;
    bool solver = true;
    bool reference = true;
};

struct SimulationState {
    int atomicNumber = 1;
    int chargeState = 0;
    quantum::physics::ApproximationMode approximationMode =
        quantum::physics::ApproximationMode::StrictHydrogenic;
    ModelKind modelKind = ModelKind::Bohr;
    TransitionSelection transition;
    BohrSettings bohr;
    CloudSettings cloud;
    SolverSettings solver;
    ViewSettings view;
    ReferenceSettings reference;
    DerivedData derived;
    DirtyFlags dirty;
};

} // namespace quantum::app
