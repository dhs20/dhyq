#pragma once

#include "quantum/meta/MethodMetadata.h"
#include "quantum/physics/AtomicPhysics.h"
#include "quantum/physics/CentralField.h"
#include "quantum/physics/CloudGenerator.h"
#include "quantum/physics/NumericalSolver.h"
#include "quantum/spectroscopy/HydrogenicCorrections.h"

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

enum class SceneFitMode {
    Full,
    Nucleus,
    Orbit,
    Cloud
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
    bool progressiveBuild = true;
    int previewPointCount = 18000;
    int previewVolumeResolution = 20;
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
    bool useScreenedCentralField = false;
    bool autoResidualCharge = true;
    double residualCharge = 1.0;
    double screeningLengthBohr = 1.5;
};

struct ViewSettings {
    CloudRenderMode cloudRenderMode = CloudRenderMode::Hybrid;
    bool showOrbitRings = true;
    bool showGrid = true;
    float pointSize = 3.0f;
    float exposure = 1.0f;
    bool interactionActive = false;
    double fitTransitionSeconds = 0.65;
    bool fitLockTarget = false;
};

struct ReferenceSettings {
    bool enabled = true;
    std::string csvPath = "assets/data/nist_reference_lines.csv";
};

struct PlotWindowRange {
    bool hasOverride = false;
    double minX = 0.0;
    double maxX = 1.0;
    double minY = 0.0;
    double maxY = 1.0;
};

struct ReportingSettings {
    bool exportValidationReportRequested = false;
    std::string outputPath = "docs/reports/latest-validation-report.md";
    std::string exportStatus = "尚未导出验证报告";
    bool exportCurrentPlotCsvRequested = false;
    std::string csvOutputPath = "docs/reports/current-plot-window.csv";
    std::string csvExportStatus = "尚未导出当前窗口 CSV";
    bool csvAppendTimestamp = true;
    bool csvSplitByPlot = false;
    PlotWindowRange energyPlotWindow;
    PlotWindowRange spectrumPlotWindow;
    PlotWindowRange radialPlotWindow;
    PlotWindowRange centralFieldPlotWindow;
    PlotWindowRange convergencePlotWindow;
};

struct AutoDemoSettings {
    bool enabled = false;
    bool loop = true;
    bool useScriptPlayback = false;
    double stepDurationSeconds = 6.0;
    bool restartRequested = false;
    bool advanceRequested = false;
    bool loadScriptRequested = false;
    bool saveScriptRequested = false;
    bool captureStepRequested = false;
    bool clearRecordedStepsRequested = false;
    bool exportBuiltInScriptRequested = false;
    int currentStepIndex = 0;
    int loadedStepCount = 0;
    int recordedStepCount = 0;
    double stepProgress = 0.0;
    std::string currentStepName = "手动";
    std::string activeScriptName = "内置演示";
    std::string scriptPath = "assets/scenarios/demo_script.json";
    std::string scriptStatus = "未加载脚本文件";
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
    quantum::spectroscopy::SpectroscopyCorrectionResult spectroscopy;
    std::vector<quantum::physics::SpectrumLine> spectrumLines;
    std::vector<quantum::physics::SpectrumLine> referenceLines;
    std::vector<quantum::physics::EnergyLevelSample> energyLevels;
    std::vector<std::pair<double, double>> radialDistribution;
    quantum::physics::CloudData cloud;
    quantum::physics::CentralFieldProfile centralField;
    quantum::physics::NumericalSolverResult solver;
    std::vector<quantum::meta::MethodStamp> methodStamps;
    std::vector<quantum::meta::ValidationRecord> validationRecords;
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
    quantum::spectroscopy::SpectroscopySettings spectroscopy;
    ViewSettings view;
    ReferenceSettings reference;
    ReportingSettings reporting;
    AutoDemoSettings demo;
    DerivedData derived;
    DirtyFlags dirty;
};

} // namespace quantum::app
