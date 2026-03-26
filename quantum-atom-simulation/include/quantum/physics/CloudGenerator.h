#pragma once

#include "quantum/physics/AtomicPhysics.h"

#include <vector>

namespace quantum::physics {

struct CloudPoint {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float density = 0.0f;
    float phase = 0.0f;
};

struct VolumeGrid {
    int resolution = 0;
    float extent = 0.0f;
    std::vector<float> voxels;
};

struct CloudGenerationStats {
    int requestedPointCount = 0;
    int candidateCount = 0;
    int acceptedCount = 0;
    double candidateMultiplier = 0.0;
    int radialCdfSamples = 0;
    int angularScanResolution = 0;
    int monteCarloSamples = 0;
    double pointBuildMs = 0.0;
    double volumeBuildMs = 0.0;
    double normalizationEstimate = 1.0;
    double normalizationError = 0.0;
};

struct CloudData {
    std::vector<CloudPoint> points;
    VolumeGrid volume;
    std::vector<SuperpositionComponent> normalizedComponents;
    double extentMeters = 0.0;
    CloudGenerationStats stats;
};

struct NumericalRadialCache {
    bool valid = false;
    QuantumNumbers qn;
    double zeff = 1.0;
    std::vector<double> radiiMeters;
    std::vector<double> radialValues;
};

struct CloudRequest {
    int nuclearCharge = 1;
    double nuclearMassKg = 0.0;
    bool useReducedMass = true;
    int pointCount = 100000;
    int volumeResolution = 48;
    double extentScale = 14.0;
    bool buildVolume = true;
    bool adaptiveSampling = true;
    double candidateMultiplier = 2.5;
    int radialCdfSamples = 4096;
    int angularScanResolution = 48;
    int monteCarloSamples = 4096;
    std::vector<SuperpositionComponent> components;
    NumericalRadialCache numericalRadial;
};

class ProbabilityCloudGenerator {
public:
    [[nodiscard]] CloudData generate(const CloudRequest& request) const;
};

} // namespace quantum::physics
