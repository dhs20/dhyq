#pragma once

#include <cstddef>
#include <deque>

namespace quantum::core {

struct PerformanceSnapshot {
    double fps = 0.0;
    double averageFrameMs = 0.0;
    double cpuFrameMs = 0.0;
    double cloudBuildMs = 0.0;
    double volumeBuildMs = 0.0;
    double gpuFrameMs = 0.0;
    double gpuPointMs = 0.0;
    double gpuVolumeMs = 0.0;
    bool gpuTimersSupported = false;
    bool cloudBuildInFlight = false;
    bool cloudBuildQueued = false;
    int requestedPointCount = 0;
    int pointCount = 0;
    int candidateCount = 0;
    int renderedPointCount = 0;
    int volumeSliceCount = 0;
    int volumeSliceAxis = 2;
    int lodLevel = 0;
    double cameraDistance = 0.0;
    double candidateMultiplier = 0.0;
    int radialCdfSamples = 0;
    int angularScanResolution = 0;
    int monteCarloSamples = 0;
    std::size_t pointBufferBytes = 0;
    std::size_t volumeTextureBytes = 0;
    std::size_t framebufferBytes = 0;
    std::size_t trackedGpuBytes = 0;
};

class PerformanceTracker {
public:
    void beginFrame();
    void endFrame();
    void setCloudBuildMs(double value);
    void setVolumeBuildMs(double value);
    void setSamplingStats(int requestedPointCount,
                          int acceptedPointCount,
                          int candidateCount,
                          double candidateMultiplier,
                          int radialCdfSamples,
                          int angularScanResolution,
                          int monteCarloSamples);
    void setCloudBuildState(bool inFlight, bool queued);
    void setRenderStats(bool gpuTimersSupported,
                        double gpuFrameMs,
                        double gpuPointMs,
                        double gpuVolumeMs,
                        int renderedPointCount,
                        int volumeSliceCount,
                        int volumeSliceAxis,
                        int lodLevel,
                        double cameraDistance,
                        std::size_t pointBufferBytes,
                        std::size_t volumeTextureBytes,
                        std::size_t framebufferBytes);

    [[nodiscard]] PerformanceSnapshot snapshot() const;

private:
    double frameStartSeconds_ = 0.0;
    double lastFrameMs_ = 0.0;
    double cloudBuildMs_ = 0.0;
    double volumeBuildMs_ = 0.0;
    double gpuFrameMs_ = 0.0;
    double gpuPointMs_ = 0.0;
    double gpuVolumeMs_ = 0.0;
    bool gpuTimersSupported_ = false;
    bool cloudBuildInFlight_ = false;
    bool cloudBuildQueued_ = false;
    int requestedPointCount_ = 0;
    int pointCount_ = 0;
    int candidateCount_ = 0;
    int renderedPointCount_ = 0;
    int volumeSliceCount_ = 0;
    int volumeSliceAxis_ = 2;
    int lodLevel_ = 0;
    double cameraDistance_ = 0.0;
    double candidateMultiplier_ = 0.0;
    int radialCdfSamples_ = 0;
    int angularScanResolution_ = 0;
    int monteCarloSamples_ = 0;
    std::size_t pointBufferBytes_ = 0;
    std::size_t volumeTextureBytes_ = 0;
    std::size_t framebufferBytes_ = 0;
    std::deque<double> frameHistoryMs_;
};

} // namespace quantum::core
