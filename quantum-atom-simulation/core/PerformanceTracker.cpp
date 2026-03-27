#include "quantum/core/PerformanceTracker.h"

#include <chrono>

namespace quantum::core {
namespace {

double nowSeconds() {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration<double>(now).count();
}

} // namespace

void PerformanceTracker::beginFrame() {
    frameStartSeconds_ = nowSeconds();
}

void PerformanceTracker::endFrame() {
    const double elapsedSeconds = nowSeconds() - frameStartSeconds_;
    lastFrameMs_ = elapsedSeconds * 1000.0;
    frameHistoryMs_.push_back(lastFrameMs_);
    if (frameHistoryMs_.size() > 240) {
        frameHistoryMs_.pop_front();
    }
}

void PerformanceTracker::setCloudBuildMs(double value) {
    cloudBuildMs_ = value;
}

void PerformanceTracker::setVolumeBuildMs(double value) {
    volumeBuildMs_ = value;
}

void PerformanceTracker::setSamplingStats(int requestedPointCount,
                                          int targetPointCount,
                                          int targetVolumeResolution,
                                          int acceptedPointCount,
                                          int candidateCount,
                                          double candidateMultiplier,
                                          int radialCdfSamples,
                                          int angularScanResolution,
                                          int monteCarloSamples,
                                          bool previewStage,
                                          double qualityRatio) {
    requestedPointCount_ = requestedPointCount;
    targetPointCount_ = targetPointCount;
    targetVolumeResolution_ = targetVolumeResolution;
    pointCount_ = acceptedPointCount;
    candidateCount_ = candidateCount;
    candidateMultiplier_ = candidateMultiplier;
    radialCdfSamples_ = radialCdfSamples;
    angularScanResolution_ = angularScanResolution;
    monteCarloSamples_ = monteCarloSamples;
    previewStage_ = previewStage;
    qualityRatio_ = qualityRatio;
}

void PerformanceTracker::setCloudBuildState(bool inFlight, bool queued) {
    cloudBuildInFlight_ = inFlight;
    cloudBuildQueued_ = queued;
}

void PerformanceTracker::setRenderStats(bool gpuTimersSupported,
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
                                        std::size_t framebufferBytes) {
    gpuTimersSupported_ = gpuTimersSupported;
    gpuFrameMs_ = gpuFrameMs;
    gpuPointMs_ = gpuPointMs;
    gpuVolumeMs_ = gpuVolumeMs;
    renderedPointCount_ = renderedPointCount;
    volumeSliceCount_ = volumeSliceCount;
    volumeSliceAxis_ = volumeSliceAxis;
    lodLevel_ = lodLevel;
    cameraDistance_ = cameraDistance;
    pointBufferBytes_ = pointBufferBytes;
    volumeTextureBytes_ = volumeTextureBytes;
    framebufferBytes_ = framebufferBytes;
}

PerformanceSnapshot PerformanceTracker::snapshot() const {
    PerformanceSnapshot snapshot;
    snapshot.cpuFrameMs = lastFrameMs_;
    snapshot.cloudBuildMs = cloudBuildMs_;
    snapshot.volumeBuildMs = volumeBuildMs_;
    snapshot.gpuFrameMs = gpuFrameMs_;
    snapshot.gpuPointMs = gpuPointMs_;
    snapshot.gpuVolumeMs = gpuVolumeMs_;
    snapshot.gpuTimersSupported = gpuTimersSupported_;
    snapshot.cloudBuildInFlight = cloudBuildInFlight_;
    snapshot.cloudBuildQueued = cloudBuildQueued_;
    snapshot.previewStage = previewStage_;
    snapshot.requestedPointCount = requestedPointCount_;
    snapshot.targetPointCount = targetPointCount_;
    snapshot.targetVolumeResolution = targetVolumeResolution_;
    snapshot.pointCount = pointCount_;
    snapshot.candidateCount = candidateCount_;
    snapshot.renderedPointCount = renderedPointCount_;
    snapshot.volumeSliceCount = volumeSliceCount_;
    snapshot.volumeSliceAxis = volumeSliceAxis_;
    snapshot.lodLevel = lodLevel_;
    snapshot.cameraDistance = cameraDistance_;
    snapshot.candidateMultiplier = candidateMultiplier_;
    snapshot.qualityRatio = qualityRatio_;
    snapshot.radialCdfSamples = radialCdfSamples_;
    snapshot.angularScanResolution = angularScanResolution_;
    snapshot.monteCarloSamples = monteCarloSamples_;
    snapshot.pointBufferBytes = pointBufferBytes_;
    snapshot.volumeTextureBytes = volumeTextureBytes_;
    snapshot.framebufferBytes = framebufferBytes_;
    snapshot.trackedGpuBytes = pointBufferBytes_ + volumeTextureBytes_ + framebufferBytes_;
    snapshot.frameHistoryMs.reserve(frameHistoryMs_.size());
    for (const double frameMs : frameHistoryMs_) {
        snapshot.frameHistoryMs.push_back(static_cast<float>(frameMs));
    }
    if (!frameHistoryMs_.empty()) {
        double sum = 0.0;
        for (const double frameMs : frameHistoryMs_) {
            sum += frameMs;
        }
        snapshot.averageFrameMs = sum / static_cast<double>(frameHistoryMs_.size());
        if (snapshot.averageFrameMs > 0.0) {
            snapshot.fps = 1000.0 / snapshot.averageFrameMs;
        }
    }
    return snapshot;
}

} // namespace quantum::core
