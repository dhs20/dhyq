#include <cmath>
#include "SchrodingerSolver.h"
#include "Utils.h"

std::vector<OrbitalPoint> SchrodingerSolver::sampleOrbitalDistribution(int n, int l, int m, int radialSamples, int angularSamples) {
    std::vector<OrbitalPoint> samples;
    samples.reserve(radialSamples * angularSamples);

    const double maxR = 8.0 * n * Utils::BOHR_RADIUS;

    for (int i = 0; i < radialSamples; ++i) {
        double r = maxR * i / (radialSamples - 1);
        double radial = Utils::radialWaveFunction(n, l, r);
        double radialProbability = radial * radial * r * r;

        for (int j = 0; j < angularSamples; ++j) {
            double theta = Utils::PI * j / (angularSamples - 1);
            double angular = Utils::angularWaveFunction(l, m, theta, 0.0);
            double angularProbability = angular * angular * std::sin(theta);
            samples.push_back({r, radialProbability, theta, angularProbability});
        }
    }

    return samples;
}

std::vector<std::vector<int>> SchrodingerSolver::supportedQuantumStates() {
    return {
        {1, 0, 0},
        {2, 0, 0},
        {2, 1, -1},
        {2, 1, 0},
        {2, 1, 1},
        {3, 0, 0},
        {3, 1, -1},
        {3, 1, 0},
        {3, 2, -2},
        {3, 2, 0}
    };
}
