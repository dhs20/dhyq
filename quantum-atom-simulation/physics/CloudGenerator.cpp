#include "quantum/physics/CloudGenerator.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <random>

namespace quantum::physics {
namespace {

struct CandidatePoint {
    CloudPoint point;
    float score = 0.0f;
};

struct AdaptiveSamplingSettings {
    double candidateMultiplier = 2.5;
    int radialCdfSamples = 4096;
    int angularScanResolution = 48;
    int monteCarloSamples = 4096;
};

double nowMs() {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration<double, std::milli>(now).count();
}

bool matchesNumericalRadial(const NumericalRadialCache& cache, const SuperpositionComponent& component) {
    return cache.valid && cache.qn.n == component.qn.n && cache.qn.l == component.qn.l &&
           std::abs(cache.zeff - component.zeff) <= 1.0e-6;
}

double interpolateNumericalRadial(const NumericalRadialCache& cache, double radiusM) {
    if (!cache.valid || cache.radiiMeters.empty() || cache.radialValues.empty()) {
        return 0.0;
    }
    if (radiusM <= cache.radiiMeters.front()) {
        return cache.radialValues.front();
    }
    if (radiusM >= cache.radiiMeters.back()) {
        return 0.0;
    }

    const auto upper = std::lower_bound(cache.radiiMeters.begin(), cache.radiiMeters.end(), radiusM);
    if (upper == cache.radiiMeters.begin()) {
        return cache.radialValues.front();
    }
    const std::size_t index = static_cast<std::size_t>(std::distance(cache.radiiMeters.begin(), upper));
    const double r0 = cache.radiiMeters[index - 1];
    const double r1 = cache.radiiMeters[index];
    const double alpha = (radiusM - r0) / std::max(1.0e-20, r1 - r0);
    return cache.radialValues[index - 1] + alpha * (cache.radialValues[index] - cache.radialValues[index - 1]);
}

double componentRadialProbabilityDensity(int nuclearCharge,
                                         const SuperpositionComponent& component,
                                         double nuclearMassKg,
                                         double radiusM,
                                         const NumericalRadialCache& cache) {
    if (matchesNumericalRadial(cache, component)) {
        const double radial = interpolateNumericalRadial(cache, radiusM);
        return radiusM * radiusM * radial * radial;
    }
    return radialProbabilityDensity(nuclearCharge, component.qn, component.zeff, nuclearMassKg, radiusM);
}

std::complex<double> componentWavefunction(int nuclearCharge,
                                           const SuperpositionComponent& component,
                                           double nuclearMassKg,
                                           double radiusM,
                                           double theta,
                                           double phi,
                                           const NumericalRadialCache& cache) {
    if (matchesNumericalRadial(cache, component)) {
        return interpolateNumericalRadial(cache, radiusM) * sphericalHarmonicValue(component.qn, theta, phi);
    }
    return hydrogenicWavefunction(nuclearCharge, component.qn, component.zeff, nuclearMassKg, radiusM, theta, phi);
}

ComplexWaveSample evaluateSuperpositionAt(int nuclearCharge,
                                          const std::vector<SuperpositionComponent>& components,
                                          double nuclearMassKg,
                                          double radiusM,
                                          double theta,
                                          double phi,
                                          const NumericalRadialCache& cache) {
    ComplexWaveSample sample;
    for (const auto& component : components) {
        const auto coefficient = std::polar(component.amplitude, component.phaseRadians);
        sample.psi += coefficient *
                      componentWavefunction(nuclearCharge, component, nuclearMassKg, radiusM, theta, phi, cache);
    }
    sample.density = std::norm(sample.psi);
    sample.phaseRadians = std::arg(sample.psi);
    return sample;
}

std::vector<double> buildRadialCdf(int nuclearCharge,
                                   const SuperpositionComponent& component,
                                   double nuclearMassKg,
                                   double maxRadiusM,
                                   int sampleCount,
                                   const NumericalRadialCache& cache) {
    std::vector<double> cdf(static_cast<std::size_t>(sampleCount), 0.0);
    double accumulator = 0.0;
    double previousDensity = 0.0;
    const double dr = maxRadiusM / static_cast<double>(sampleCount - 1);
    for (int index = 0; index < sampleCount; ++index) {
        const double radius = dr * static_cast<double>(index);
        const double density = componentRadialProbabilityDensity(nuclearCharge, component, nuclearMassKg, radius, cache);
        if (index > 0) {
            accumulator += 0.5 * (density + previousDensity) * dr;
        }
        cdf[static_cast<std::size_t>(index)] = accumulator;
        previousDensity = density;
    }
    if (accumulator > 0.0) {
        for (double& value : cdf) {
            value /= accumulator;
        }
    }
    return cdf;
}

double sampleRadius(const std::vector<double>& cdf, double maxRadiusM, double unitSample) {
    const auto upper = std::lower_bound(cdf.begin(), cdf.end(), unitSample);
    if (upper == cdf.begin()) {
        return 0.0;
    }
    if (upper == cdf.end()) {
        return maxRadiusM;
    }
    const std::size_t index = static_cast<std::size_t>(std::distance(cdf.begin(), upper));
    const double x1 = cdf[index - 1];
    const double x2 = cdf[index];
    const double alpha = (unitSample - x1) / std::max(1e-12, x2 - x1);
    const double t = (static_cast<double>(index - 1) + alpha) / static_cast<double>(cdf.size() - 1);
    return t * maxRadiusM;
}

std::array<double, 2> sampleAngles(int nuclearCharge,
                                   const SuperpositionComponent& component,
                                   double nuclearMassKg,
                                   int scanResolution,
                                   const NumericalRadialCache& cache,
                                   std::mt19937_64& rng) {
    std::uniform_real_distribution<double> uniform01(0.0, 1.0);
    std::uniform_real_distribution<double> uniformPhi(0.0, 2.0 * constants::pi);

    double peakDensity = 1e-8;
    const int resolution = std::max(8, scanResolution);
    for (int i = 0; i < resolution; ++i) {
        const double theta = constants::pi * static_cast<double>(i) / static_cast<double>(resolution - 1);
        for (int j = 0; j < resolution; ++j) {
            const double phi = 2.0 * constants::pi * static_cast<double>(j) / static_cast<double>(resolution - 1);
            const auto psi = componentWavefunction(
                nuclearCharge, component, nuclearMassKg, constants::bohrRadiusM, theta, phi, cache);
            peakDensity = std::max(peakDensity, std::norm(psi));
        }
    }

    for (int attempt = 0; attempt < 256; ++attempt) {
        const double cosTheta = 2.0 * uniform01(rng) - 1.0;
        const double theta = std::acos(std::clamp(cosTheta, -1.0, 1.0));
        const double phi = uniformPhi(rng);
        const auto psi = componentWavefunction(
            nuclearCharge, component, nuclearMassKg, constants::bohrRadiusM, theta, phi, cache);
        if (uniform01(rng) <= std::norm(psi) / peakDensity) {
            return {theta, phi};
        }
    }

    const double cosTheta = 2.0 * uniform01(rng) - 1.0;
    return {std::acos(std::clamp(cosTheta, -1.0, 1.0)), uniformPhi(rng)};
}

AdaptiveSamplingSettings deriveSamplingSettings(const CloudRequest& request, std::size_t componentCount) {
    AdaptiveSamplingSettings settings;
    settings.candidateMultiplier = std::max(1.05, request.candidateMultiplier);
    settings.radialCdfSamples = std::max(256, request.radialCdfSamples);
    settings.angularScanResolution = std::max(8, request.angularScanResolution);
    settings.monteCarloSamples = std::max(512, request.monteCarloSamples);

    if (!request.adaptiveSampling) {
        return settings;
    }

    const std::size_t extraComponents = (componentCount > 0) ? (componentCount - 1) : 0;
    const double componentFactor = 1.0 + 0.20 * static_cast<double>(extraComponents);
    const double volumeFactor = request.buildVolume ? 1.15 : 1.0;
    const double pointFactor =
        (request.pointCount >= 100000) ? 1.10 : ((request.pointCount >= 50000) ? 1.05 : 1.0);
    const double scale = componentFactor * volumeFactor * pointFactor;

    settings.candidateMultiplier = std::clamp(settings.candidateMultiplier * scale, settings.candidateMultiplier, 5.5);
    settings.radialCdfSamples =
        std::clamp(static_cast<int>(std::lround(static_cast<double>(settings.radialCdfSamples) * componentFactor)),
                   256,
                   16384);
    settings.angularScanResolution =
        std::clamp(static_cast<int>(std::lround(static_cast<double>(settings.angularScanResolution) * std::sqrt(scale))),
                   8,
                   96);
    settings.monteCarloSamples =
        std::clamp(static_cast<int>(std::lround(static_cast<double>(settings.monteCarloSamples) * scale)),
                   512,
                   32768);
    return settings;
}

} // namespace

CloudData ProbabilityCloudGenerator::generate(const CloudRequest& request) const {
    CloudData cloud;
    cloud.normalizedComponents = normalizedComponents(request.components);
    cloud.stats.requestedPointCount = request.pointCount;
    for (const auto& component : cloud.normalizedComponents) {
        if (matchesNumericalRadial(request.numericalRadial, component)) {
            ++cloud.stats.numericalComponentCount;
        }
    }
    cloud.stats.usedNumericalRadial = cloud.stats.numericalComponentCount > 0;
    const AdaptiveSamplingSettings sampling = deriveSamplingSettings(request, cloud.normalizedComponents.size());
    cloud.stats.candidateMultiplier = sampling.candidateMultiplier;
    cloud.stats.radialCdfSamples = sampling.radialCdfSamples;
    cloud.stats.angularScanResolution = sampling.angularScanResolution;
    cloud.stats.monteCarloSamples = sampling.monteCarloSamples;

    double maxN = 1.0;
    double averageZeff = 0.0;
    for (const auto& component : cloud.normalizedComponents) {
        maxN = std::max(maxN, static_cast<double>(component.qn.n));
        averageZeff += component.zeff;
    }
    averageZeff /= static_cast<double>(cloud.normalizedComponents.size());
    cloud.extentMeters =
        constants::bohrRadiusM * request.extentScale * maxN * maxN / std::max(averageZeff, 1e-6);

    std::vector<std::vector<double>> cdfs;
    cdfs.reserve(cloud.normalizedComponents.size());
    for (const auto& component : cloud.normalizedComponents) {
        cdfs.push_back(buildRadialCdf(request.nuclearCharge,
                                     component,
                                     request.nuclearMassKg,
                                     cloud.extentMeters,
                                     sampling.radialCdfSamples,
                                     request.numericalRadial));
    }

    std::vector<double> weights;
    weights.reserve(cloud.normalizedComponents.size());
    for (const auto& component : cloud.normalizedComponents) {
        weights.push_back(component.amplitude * component.amplitude);
    }
    std::discrete_distribution<int> componentDistribution(weights.begin(), weights.end());
    std::uniform_real_distribution<double> uniform01(0.0, 1.0);

    const double startPointMs = nowMs();
    std::mt19937_64 rng(42);
    const int candidateCount =
        static_cast<int>(std::ceil(static_cast<double>(request.pointCount) * sampling.candidateMultiplier));
    cloud.stats.candidateCount = candidateCount;

    std::vector<CandidatePoint> candidates;
    candidates.reserve(static_cast<std::size_t>(candidateCount));
    for (int index = 0; index < candidateCount; ++index) {
        const int componentIndex = componentDistribution(rng);
        const auto& component = cloud.normalizedComponents[componentIndex];
        const double radius = sampleRadius(cdfs[componentIndex], cloud.extentMeters, uniform01(rng));
        const auto angles =
            sampleAngles(request.nuclearCharge,
                         component,
                         request.nuclearMassKg,
                         sampling.angularScanResolution,
                         request.numericalRadial,
                         rng);
        const double theta = angles[0];
        const double phi = angles[1];
        const double sinTheta = std::sin(theta);

        const double x = radius * sinTheta * std::cos(phi);
        const double y = radius * sinTheta * std::sin(phi);
        const double z = radius * std::cos(theta);
        const auto sample = evaluateSuperpositionAt(request.nuclearCharge,
                                                    cloud.normalizedComponents,
                                                    request.nuclearMassKg,
                                                    radius,
                                                    theta,
                                                    phi,
                                                    request.numericalRadial);

        candidates.push_back({CloudPoint{static_cast<float>(x),
                                         static_cast<float>(y),
                                         static_cast<float>(z),
                                         static_cast<float>(sample.density),
                                         static_cast<float>(sample.phaseRadians)},
                              static_cast<float>(sample.density)});
    }

    std::sort(candidates.begin(), candidates.end(), [](const CandidatePoint& lhs, const CandidatePoint& rhs) {
        return lhs.score > rhs.score;
    });

    const int acceptedCount = std::min<int>(request.pointCount, static_cast<int>(candidates.size()));
    cloud.stats.acceptedCount = acceptedCount;
    cloud.points.reserve(static_cast<std::size_t>(acceptedCount));

    float maxDensity = 1.0f;
    for (int index = 0; index < acceptedCount; ++index) {
        maxDensity = std::max(maxDensity, candidates[index].point.density);
    }
    for (int index = 0; index < acceptedCount; ++index) {
        auto point = candidates[index].point;
        point.density = std::sqrt(std::max(0.0f, point.density / maxDensity));
        cloud.points.push_back(point);
    }
    cloud.stats.pointBuildMs = nowMs() - startPointMs;

    double monteCarloEstimate = 0.0;
    const int integrationSamples = sampling.monteCarloSamples;
    for (int index = 0; index < integrationSamples; ++index) {
        const double radius = cloud.extentMeters * uniform01(rng);
        const double cosTheta = 2.0 * uniform01(rng) - 1.0;
        const double theta = std::acos(std::clamp(cosTheta, -1.0, 1.0));
        const double phi = 2.0 * constants::pi * uniform01(rng);
        const auto sample = evaluateSuperpositionAt(request.nuclearCharge,
                                                    cloud.normalizedComponents,
                                                    request.nuclearMassKg,
                                                    radius,
                                                    theta,
                                                    phi,
                                                    request.numericalRadial);
        monteCarloEstimate += sample.density * radius * radius * std::sin(theta);
    }
    monteCarloEstimate *= 4.0 * constants::pi * cloud.extentMeters / static_cast<double>(integrationSamples);
    cloud.stats.normalizationEstimate = monteCarloEstimate;
    cloud.stats.normalizationError = std::abs(1.0 - monteCarloEstimate);

    if (request.buildVolume && request.volumeResolution > 0) {
        const double startVolumeMs = nowMs();
        cloud.volume.resolution = request.volumeResolution;
        cloud.volume.extent = static_cast<float>(cloud.extentMeters);
        const std::size_t voxelCount = static_cast<std::size_t>(request.volumeResolution) *
                                       static_cast<std::size_t>(request.volumeResolution) *
                                       static_cast<std::size_t>(request.volumeResolution) * 2ULL;
        cloud.volume.voxels.assign(voxelCount, 0.0f);

        const double voxelStep = (2.0 * cloud.extentMeters) / static_cast<double>(request.volumeResolution - 1);
        float densityPeak = 1e-6f;
        for (int z = 0; z < request.volumeResolution; ++z) {
            for (int y = 0; y < request.volumeResolution; ++y) {
                for (int x = 0; x < request.volumeResolution; ++x) {
                    const double px = -cloud.extentMeters + voxelStep * static_cast<double>(x);
                    const double py = -cloud.extentMeters + voxelStep * static_cast<double>(y);
                    const double pz = -cloud.extentMeters + voxelStep * static_cast<double>(z);
                    const double radius = std::sqrt(px * px + py * py + pz * pz);
                    if (radius > cloud.extentMeters) {
                        continue;
                    }
                    const double theta = (radius > 1e-20) ? std::acos(std::clamp(pz / radius, -1.0, 1.0)) : 0.0;
                    const double phi = std::atan2(py, px);
                    const auto sample = evaluateSuperpositionAt(request.nuclearCharge,
                                                                cloud.normalizedComponents,
                                                                request.nuclearMassKg,
                                                                radius,
                                                                theta,
                                                                phi,
                                                                request.numericalRadial);
                    const std::size_t baseIndex =
                        (static_cast<std::size_t>(z) * request.volumeResolution * request.volumeResolution +
                         static_cast<std::size_t>(y) * request.volumeResolution + static_cast<std::size_t>(x)) *
                        2ULL;
                    cloud.volume.voxels[baseIndex] = static_cast<float>(sample.density);
                    cloud.volume.voxels[baseIndex + 1] = static_cast<float>(sample.phaseRadians);
                    densityPeak = std::max(densityPeak, static_cast<float>(sample.density));
                }
            }
        }
        for (std::size_t index = 0; index < cloud.volume.voxels.size(); index += 2) {
            cloud.volume.voxels[index] = std::sqrt(std::max(0.0f, cloud.volume.voxels[index] / densityPeak));
        }
        cloud.stats.volumeBuildMs = nowMs() - startVolumeMs;
    }

    return cloud;
}

} // namespace quantum::physics
