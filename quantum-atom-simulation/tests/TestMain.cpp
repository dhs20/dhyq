#include "quantum/physics/AtomicPhysics.h"
#include "quantum/physics/CloudGenerator.h"
#include "quantum/physics/ElementDatabase.h"
#include "quantum/physics/NumericalSolver.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {

int failures = 0;

void expectNear(const std::string& label, double actual, double expected, double tolerance) {
    const double error = std::abs(actual - expected);
    if (error > tolerance) {
        std::cerr << "[FAIL] " << label << ": actual=" << actual << ", expected=" << expected << ", tol=" << tolerance
                  << ", error=" << error << '\n';
        ++failures;
    }
}

void expectTrue(const std::string& label, bool condition) {
    if (!condition) {
        std::cerr << "[FAIL] " << label << '\n';
        ++failures;
    }
}

} // namespace

int main() {
    using namespace quantum::physics;

    ElementDatabase database;
    database.loadBuiltInSubset();
    const auto hydrogen = database.elementByAtomicNumber(1);

    expectNear("Bohr radius", constants::bohrRadiusM, 5.29177210903e-11, 1e-20);
    expectNear("eV->J", units::evToJoule(13.6), 2.17896022224e-18, 1e-21);

    const double hydrogenMass = nuclearMassKg(hydrogen);

    const auto lymanAlpha = computeTransition({{2, 1, 0}, {1, 0, 0}, TransitionRuleSet::ElectricDipole, false, 1, 1.0, 1.0, hydrogenMass, true});
    expectTrue("Lyman-alpha allowed", lymanAlpha.allowed);
    expectNear("Lyman-alpha wavelength (nm)", lymanAlpha.wavelengthNm, 121.567, 0.15);

    const auto balmerAlpha = computeTransition({{3, 1, 0}, {2, 0, 0}, TransitionRuleSet::ElectricDipole, false, 1, 1.0, 1.0, hydrogenMass, true});
    expectTrue("Balmer-alpha allowed", balmerAlpha.allowed);
    expectNear("Balmer-alpha wavelength (nm)", balmerAlpha.wavelengthNm, 656.279, 0.4);

    const auto forbidden = computeTransition({{2, 0, 0}, {1, 0, 0}, TransitionRuleSet::ElectricDipole, false, 1, 1.0, 1.0, hydrogenMass, true});
    expectTrue("2s->1s forbidden", !forbidden.allowed);

    const auto neonConfig = buildElectronConfiguration(10, 0);
    expectTrue("Ne configuration", neonConfig.notation == "1s2 2s2 2p6");

    const auto slater = computeSlaterShielding(10, neonConfig, 2, 1);
    expectNear("Ne 2p Z_eff", slater.zeff, 5.85, 0.3);

    const auto radial = sampleRadialDistribution(1, {1, 0, 0}, 1.0, hydrogenMass, 20.0 * constants::bohrRadiusM, 4096);
    double integral = 0.0;
    for (std::size_t index = 1; index < radial.size(); ++index) {
        const double r0 = radial[index - 1].first;
        const double r1 = radial[index].first;
        integral += 0.5 * (radial[index - 1].second + radial[index].second) * (r1 - r0);
    }
    expectNear("1s radial normalization", integral, 1.0, 1e-2);

    double overlap = 0.0;
    const int overlapSamples = 4096;
    const double maxRadius = 40.0 * constants::bohrRadiusM;
    for (int index = 1; index < overlapSamples; ++index) {
        const double r0 = maxRadius * static_cast<double>(index - 1) / static_cast<double>(overlapSamples - 1);
        const double r1 = maxRadius * static_cast<double>(index) / static_cast<double>(overlapSamples - 1);
        const auto psi10_0 = hydrogenicWavefunction(1, {1, 0, 0}, 1.0, hydrogenMass, r0, 0.0, 0.0);
        const auto psi20_0 = hydrogenicWavefunction(1, {2, 0, 0}, 1.0, hydrogenMass, r0, 0.0, 0.0);
        const auto psi10_1 = hydrogenicWavefunction(1, {1, 0, 0}, 1.0, hydrogenMass, r1, 0.0, 0.0);
        const auto psi20_1 = hydrogenicWavefunction(1, {2, 0, 0}, 1.0, hydrogenMass, r1, 0.0, 0.0);
        const double integrand0 = 4.0 * constants::pi * std::real(std::conj(psi10_0) * psi20_0) * r0 * r0;
        const double integrand1 = 4.0 * constants::pi * std::real(std::conj(psi10_1) * psi20_1) * r1 * r1;
        overlap += 0.5 * (integrand0 + integrand1) * (r1 - r0);
    }
    expectNear("1s-2s orthogonality", overlap, 0.0, 2e-2);

    ProbabilityCloudGenerator cloudGenerator;
    const auto fixedSamplingCloud = cloudGenerator.generate(
        {1, hydrogenMass, true, 2000, 0, 12.0, false, false, 2.0, 2048, 24, 2048, {{{2, 1, 0}, 1.0, 0.0, 1.0}}, {}});
    expectNear("Fixed sampling multiplier", fixedSamplingCloud.stats.candidateMultiplier, 2.0, 1e-9);
    expectTrue("Fixed candidate count", fixedSamplingCloud.stats.candidateCount == 4000);
    expectTrue("Fixed radial CDF samples", fixedSamplingCloud.stats.radialCdfSamples == 2048);
    expectTrue("Fixed angular scan resolution", fixedSamplingCloud.stats.angularScanResolution == 24);
    expectTrue("Fixed monte carlo samples", fixedSamplingCloud.stats.monteCarloSamples == 2048);
    expectTrue("Fixed accepted count", fixedSamplingCloud.stats.acceptedCount == 2000);

    const auto adaptiveSamplingCloud = cloudGenerator.generate({1,
                                                                hydrogenMass,
                                                                true,
                                                                1500,
                                                                8,
                                                                12.0,
                                                                true,
                                                                true,
                                                                2.5,
                                                                4096,
                                                                48,
                                                                4096,
                                                                {{{2, 1, 0}, 0.8, 0.0, 1.0},
                                                                 {{2, 1, 1}, 0.6, 1.2, 1.0}},
                                                                {}});
    expectTrue("Adaptive multiplier increases", adaptiveSamplingCloud.stats.candidateMultiplier > 2.5);
    expectTrue("Adaptive candidate count increases", adaptiveSamplingCloud.stats.candidateCount > 3750);
    expectTrue("Adaptive radial CDF increases", adaptiveSamplingCloud.stats.radialCdfSamples > 4096);
    expectTrue("Adaptive angular scan increases", adaptiveSamplingCloud.stats.angularScanResolution >= 48);
    expectTrue("Adaptive monte carlo increases", adaptiveSamplingCloud.stats.monteCarloSamples > 4096);

    const auto analyticCloud = cloudGenerator.generate(
        {1, hydrogenMass, true, 512, 0, 10.0, false, false, 2.0, 1024, 24, 1024, {{{1, 0, 0}, 1.0, 0.0, 1.0}}, {}});
    NumericalRadialCache syntheticCache;
    syntheticCache.valid = true;
    syntheticCache.qn = {1, 0, 0};
    syntheticCache.zeff = 1.0;
    syntheticCache.radiiMeters = {0.0, 2.0 * constants::bohrRadiusM, 4.0 * constants::bohrRadiusM, 8.0 * constants::bohrRadiusM};
    syntheticCache.radialValues = {0.0, 0.0, 1.0e5, 0.0};
    const auto numericalCloud = cloudGenerator.generate({1,
                                                         hydrogenMass,
                                                         true,
                                                         512,
                                                         0,
                                                         10.0,
                                                         false,
                                                         false,
                                                         2.0,
                                                         1024,
                                                         24,
                                                         1024,
                                                         {{{1, 0, 0}, 1.0, 0.0, 1.0}},
                                                         syntheticCache});
    expectTrue("Numerical radial flag propagated", numericalCloud.stats.usedNumericalRadial);
    expectTrue("Numerical radial component count", numericalCloud.stats.numericalComponentCount == 1);
    const auto analyticRadius = std::sqrt(static_cast<double>(analyticCloud.points.front().x) * analyticCloud.points.front().x +
                                          static_cast<double>(analyticCloud.points.front().y) * analyticCloud.points.front().y +
                                          static_cast<double>(analyticCloud.points.front().z) * analyticCloud.points.front().z);
    const auto numericalRadius = std::sqrt(static_cast<double>(numericalCloud.points.front().x) * numericalCloud.points.front().x +
                                           static_cast<double>(numericalCloud.points.front().y) * numericalCloud.points.front().y +
                                           static_cast<double>(numericalCloud.points.front().z) * numericalCloud.points.front().z);
    expectTrue("Numerical radial changes sampled cloud",
               std::abs(numericalRadius - analyticRadius) > 0.5 * constants::bohrRadiusM);

    SchrodingerNumericalSolver solver;
    const auto numerical = solver.solve({1, 1.0, hydrogenMass, true, {1, 0, 0}, 4096, 4, 120.0});
    expectTrue("Numerical solver converged", numerical.converged);
    expectNear("Numerical 1s error (eV)", numerical.errorEv, 0.0, 0.2);
    expectTrue("Numerical solver is purely numerical", numerical.message.find("fallback") == std::string::npos);
    expectTrue("Convergence samples exist", !numerical.convergence.empty());
    if (numerical.convergence.size() >= 2) {
        expectTrue("Convergence improves with refinement",
                   numerical.convergence.back().errorEv <= numerical.convergence.front().errorEv + 1e-4);
    }

    if (failures == 0) {
        std::cout << "All tests passed.\n";
        return EXIT_SUCCESS;
    }

    std::cerr << failures << " test(s) failed.\n";
    return EXIT_FAILURE;
}
