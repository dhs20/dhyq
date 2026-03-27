#include "quantum/physics/AtomicPhysics.h"
#include "quantum/physics/CentralField.h"
#include "quantum/physics/CloudGenerator.h"
#include "quantum/physics/ElementDatabase.h"
#include "quantum/physics/NumericalSolver.h"
#include "quantum/spectroscopy/HydrogenicCorrections.h"
#include "quantum/validation/ValidationReportWriter.h"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

int failures = 0;

std::filesystem::path locateProjectRoot() {
    auto path = std::filesystem::current_path();
    for (int depth = 0; depth < 6; ++depth) {
        if (std::filesystem::exists(path / "assets" / "data" / "elements.json")) {
            return path;
        }
        if (!path.has_parent_path()) {
            break;
        }
        path = path.parent_path();
    }
    return std::filesystem::current_path();
}

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

quantum::physics::CloudRequest makeCloudRequest(int nuclearCharge,
                                                double nuclearMassKg,
                                                bool useReducedMass,
                                                int pointCount,
                                                int volumeResolution,
                                                double extentScale,
                                                bool buildVolume,
                                                bool adaptiveSampling,
                                                double candidateMultiplier,
                                                int radialCdfSamples,
                                                int angularScanResolution,
                                                int monteCarloSamples,
                                                std::vector<quantum::physics::SuperpositionComponent> components,
                                                quantum::physics::NumericalRadialCache numericalRadial = {}) {
    quantum::physics::CloudRequest request;
    request.nuclearCharge = nuclearCharge;
    request.nuclearMassKg = nuclearMassKg;
    request.useReducedMass = useReducedMass;
    request.pointCount = pointCount;
    request.targetPointCount = pointCount;
    request.volumeResolution = volumeResolution;
    request.targetVolumeResolution = volumeResolution;
    request.extentScale = extentScale;
    request.buildVolume = buildVolume;
    request.adaptiveSampling = adaptiveSampling;
    request.candidateMultiplier = candidateMultiplier;
    request.radialCdfSamples = radialCdfSamples;
    request.angularScanResolution = angularScanResolution;
    request.monteCarloSamples = monteCarloSamples;
    request.components = std::move(components);
    request.numericalRadial = std::move(numericalRadial);
    return request;
}

} // namespace

int main() {
    using namespace quantum::physics;

    ElementDatabase database;
    const auto projectRoot = locateProjectRoot();
    const auto elementsPath = projectRoot / "assets" / "data" / "elements.json";
    const auto referencesPath = projectRoot / "assets" / "data" / "nist_reference_lines.csv";
    if (!database.loadFromJson(elementsPath)) {
        database.loadBuiltInSubset();
    }
    (void)database.loadReferenceTransitions(referencesPath);
    const auto hydrogen = database.elementByAtomicNumber(1);
    const auto oganesson = database.elementByAtomicNumber(118);
    expectTrue("Element metadata method", !hydrogen.method.methodName.empty());
    expectTrue("Element metadata source", !hydrogen.source.id.empty());
    expectTrue("Periodic metadata coverage", oganesson.symbol == "Og");
    expectTrue("Hydrogen Chinese name", hydrogen.localizedNameZh == "氢");
    expectTrue("Oganesson Chinese name", !oganesson.localizedNameZh.empty());

    expectNear("Bohr radius", constants::bohrRadiusM, 5.29177210903e-11, 1e-20);
    expectNear("eV->J", units::evToJoule(13.6), 2.17896022224e-18, 1e-21);

    const double hydrogenMass = nuclearMassKg(hydrogen);
    const auto hydrogenicGround = computeHydrogenicMetrics(1, 1, 1.0, hydrogenMass, true);
    expectNear("Hydrogen 1s energy (eV)", hydrogenicGround.energyEv, -13.6, 0.05);
    const auto heliumIonGround = computeHydrogenicMetrics(2, 1, 2.0, hydrogenMass, true);
    expectNear("He+ 1s energy Z^2 scaling (eV)", heliumIonGround.energyEv, -54.4, 0.2);

    const auto lymanAlpha = computeTransition({{2, 1, 0}, {1, 0, 0}, TransitionRuleSet::ElectricDipole, false, 1, 1.0, 1.0, hydrogenMass, true});
    expectTrue("Lyman-alpha allowed", lymanAlpha.allowed);
    expectNear("Lyman-alpha wavelength (nm)", lymanAlpha.wavelengthNm, 121.567, 0.15);
    expectTrue("Lyman-alpha method metadata", !lymanAlpha.method.methodName.empty());
    expectTrue("Lyman-alpha validation record", !lymanAlpha.validation.empty());

    const auto balmerAlpha = computeTransition({{3, 1, 0}, {2, 0, 0}, TransitionRuleSet::ElectricDipole, false, 1, 1.0, 1.0, hydrogenMass, true});
    expectTrue("Balmer-alpha allowed", balmerAlpha.allowed);
    expectNear("Balmer-alpha wavelength (nm)", balmerAlpha.wavelengthNm, 656.279, 0.4);

    const auto spectrum = generateSpectrum(1, 4, 1.0, hydrogenMass, TransitionRuleSet::ElectricDipole, false, true);
    expectTrue("Spectrum line provenance", !spectrum.empty() && !spectrum.front().provenance.recordId.empty());
    expectTrue("Spectrum line source", !spectrum.empty() && !spectrum.front().source.id.empty());

    const auto levels = generateEnergyLevels(1, 3, 1.0, hydrogenMass, true);
    expectTrue("Energy level metadata", !levels.empty() && !levels.front().method.methodName.empty());
    expectTrue("Energy level provenance", !levels.empty() && !levels.front().provenance.recordId.empty());

    const auto forbidden = computeTransition({{2, 0, 0}, {1, 0, 0}, TransitionRuleSet::ElectricDipole, false, 1, 1.0, 1.0, hydrogenMass, true});
    expectTrue("2s->1s forbidden", !forbidden.allowed);

    const auto referenceTransitions = database.referenceTransitions(1);
    expectTrue("Reference transitions available", !referenceTransitions.empty());
    if (!referenceTransitions.empty()) {
        expectTrue("Reference transition method metadata", !referenceTransitions.front().method.methodName.empty());
        expectTrue("Reference transition provenance", !referenceTransitions.front().provenance.recordId.empty());
    }

    const auto neonConfig = buildElectronConfiguration(10, 0);
    expectTrue("Ne configuration", neonConfig.notation == "1s2 2s2 2p6");
    expectTrue("Configuration method metadata", !neonConfig.method.methodName.empty());

    const auto slater = computeSlaterShielding(10, neonConfig, 2, 1);
    expectNear("Ne 2p Z_eff", slater.zeff, 5.85, 0.3);
    expectTrue("Slater method metadata", !slater.method.methodName.empty());
    expectTrue("Slater validation record", !slater.validation.empty());

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

    const double lightNucleusMass = 2.0 * constants::electronMassKg;
    const auto reducedMassOff = computeHydrogenicMetrics(1, 1, 1.0, lightNucleusMass, false);
    const auto reducedMassOn = computeHydrogenicMetrics(1, 1, 1.0, lightNucleusMass, true);
    expectTrue("Reduced mass changes bound-state energy", std::abs(reducedMassOn.energyEv - reducedMassOff.energyEv) > 1.0);
    const auto psiReducedOff = hydrogenicWavefunction(1, {1, 0, 0}, 1.0, lightNucleusMass, constants::bohrRadiusM, 0.0, 0.0, false);
    const auto psiReducedOn = hydrogenicWavefunction(1, {1, 0, 0}, 1.0, lightNucleusMass, constants::bohrRadiusM, 0.0, 0.0, true);
    const double psiDifference = std::abs(std::abs(psiReducedOn) - std::abs(psiReducedOff));
    expectTrue("Reduced mass propagates into wavefunction amplitude", psiDifference > 1.0e13);

    ProbabilityCloudGenerator cloudGenerator;
    const auto fixedSamplingCloud =
        cloudGenerator.generate(makeCloudRequest(1,
                                                 hydrogenMass,
                                                 true,
                                                 2000,
                                                 0,
                                                 12.0,
                                                 false,
                                                 false,
                                                 2.0,
                                                 2048,
                                                 24,
                                                 2048,
                                                 {{{2, 1, 0}, 1.0, 0.0, 1.0}}));
    expectNear("Fixed sampling multiplier", fixedSamplingCloud.stats.candidateMultiplier, 2.0, 1e-9);
    expectTrue("Fixed candidate count", fixedSamplingCloud.stats.candidateCount == 4000);
    expectTrue("Fixed radial CDF samples", fixedSamplingCloud.stats.radialCdfSamples == 2048);
    expectTrue("Fixed angular scan resolution", fixedSamplingCloud.stats.angularScanResolution == 24);
    expectTrue("Fixed monte carlo samples", fixedSamplingCloud.stats.monteCarloSamples == 2048);
    expectTrue("Fixed accepted count", fixedSamplingCloud.stats.acceptedCount == 2000);

    const auto adaptiveSamplingCloud =
        cloudGenerator.generate(makeCloudRequest(1,
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
                                                 {{{2, 1, 0}, 0.8, 0.0, 1.0}, {{2, 1, 1}, 0.6, 1.2, 1.0}}));
    expectTrue("Adaptive multiplier increases", adaptiveSamplingCloud.stats.candidateMultiplier > 2.5);
    expectTrue("Adaptive candidate count increases", adaptiveSamplingCloud.stats.candidateCount > 3750);
    expectTrue("Adaptive radial CDF increases", adaptiveSamplingCloud.stats.radialCdfSamples > 4096);
    expectTrue("Adaptive angular scan increases", adaptiveSamplingCloud.stats.angularScanResolution >= 48);
    expectTrue("Adaptive monte carlo increases", adaptiveSamplingCloud.stats.monteCarloSamples > 4096);

    const auto analyticCloud =
        cloudGenerator.generate(makeCloudRequest(1,
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
                                                 {{{1, 0, 0}, 1.0, 0.0, 1.0}}));
    NumericalRadialCache syntheticCache;
    syntheticCache.valid = true;
    syntheticCache.qn = {1, 0, 0};
    syntheticCache.zeff = 1.0;
    syntheticCache.radiiMeters = {0.0, 2.0 * constants::bohrRadiusM, 4.0 * constants::bohrRadiusM, 8.0 * constants::bohrRadiusM};
    syntheticCache.radialValues = {0.0, 0.0, 1.0e5, 0.0};
    const auto numericalCloud =
        cloudGenerator.generate(makeCloudRequest(1,
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
                                                 syntheticCache));
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

    CentralFieldParameters hydrogenicField;
    hydrogenicField.mode = CentralFieldMode::HydrogenicCoulomb;
    hydrogenicField.nuclearCharge = 1;
    hydrogenicField.ionCharge = 0;
    hydrogenicField.zeff = 1.0;
    const auto fieldProfile = sampleCentralFieldProfile(hydrogenicField, 80.0, 80);
    expectTrue("Central field samples exist", !fieldProfile.samples.empty());
    expectNear("Central field near charge", fieldProfile.samples.front().effectiveCharge, 1.0, 1e-3);
    expectTrue("Central field validation metadata", !fieldProfile.validation.empty());

    SchrodingerNumericalSolver solver;
    NumericalSolverRequest solverRequest;
    solverRequest.nuclearCharge = 1;
    solverRequest.zeff = 1.0;
    solverRequest.nuclearMassKg = hydrogenMass;
    solverRequest.useReducedMass = true;
    solverRequest.centralField = hydrogenicField;
    solverRequest.qn = {1, 0, 0};
    solverRequest.gridPoints = 4096;
    solverRequest.convergencePasses = 4;
    solverRequest.maxScaledRadius = 120.0;
    const auto numerical = solver.solve(solverRequest);
    expectTrue("Numerical solver converged", numerical.converged);
    expectNear("Numerical 1s error (eV)", numerical.errorEv, 0.0, 0.2);
    expectTrue("Numerical solver is purely numerical", numerical.message.find("fallback") == std::string::npos);
    expectTrue("Convergence samples exist", !numerical.convergence.empty());
    expectTrue("Numerical solver method metadata", !numerical.method.methodName.empty());
    expectTrue("Numerical solver validation record", !numerical.validation.empty());
    expectTrue("Numerical solver central field profile", !numerical.centralFieldProfile.samples.empty());
    if (numerical.convergence.size() >= 2) {
        expectTrue("Convergence improves with refinement",
                   numerical.convergence.back().errorEv <= numerical.convergence.front().errorEv + 1e-4);
        expectTrue("Convergence validation metadata", !numerical.convergence.front().validation.caseName.empty());
    }

    quantum::spectroscopy::SpectroscopySettings spectroscopySettings;
    spectroscopySettings.applyFineStructure = true;
    spectroscopySettings.applyZeeman = true;
    spectroscopySettings.magneticFieldTesla = 2.0;
    const auto correctedTransition = quantum::spectroscopy::evaluateHydrogenicCorrections(
        {{2, 1, 1}, {1, 0, 0}, TransitionRuleSet::ElectricDipole, true, 1, 1.0, 1.0, hydrogenMass, true},
        lymanAlpha,
        spectroscopySettings,
        hydrogenMass);
    expectTrue("Tier 3 spectroscopy applied", correctedTransition.applied);
    expectTrue("Tier 3 spectroscopy method metadata", !correctedTransition.method.methodName.empty());
    expectTrue("Tier 3 spectroscopy validation", !correctedTransition.validation.empty());
    expectTrue("Tier 3 corrected wavelength positive", correctedTransition.correctedWavelengthNm > 0.0);

    quantum::validation::ValidationReportWriter reportWriter;
    quantum::validation::ValidationReportInput reportInput;
    reportInput.title = "Test Validation Report";
    reportInput.subject = "Hydrogen";
    reportInput.summary = "Regression test snapshot";
    reportInput.approximationWarning = "Strict hydrogenic test case";
    reportInput.outputStem = "test-validation-report";
    reportInput.methods.push_back({"Transition", lymanAlpha.method});
    reportInput.methods.push_back({"Solver", numerical.method});
    reportInput.records = lymanAlpha.validation;
    reportInput.records.insert(reportInput.records.end(), numerical.validation.begin(), numerical.validation.end());
    const auto reportPath =
        std::filesystem::temp_directory_path() / "quantum_atom_simulation_test_validation_report.md";
    const auto reportResult = reportWriter.write(reportInput, reportPath);
    expectTrue("Validation report markdown export", reportResult.ok && std::filesystem::exists(reportResult.markdownPath));
    expectTrue("Validation report json export", reportResult.ok && std::filesystem::exists(reportResult.jsonPath));
    if (std::filesystem::exists(reportResult.markdownPath)) {
        std::filesystem::remove(reportResult.markdownPath);
    }
    if (std::filesystem::exists(reportResult.jsonPath)) {
        std::filesystem::remove(reportResult.jsonPath);
    }

    if (failures == 0) {
        std::cout << "All tests passed.\n";
        return EXIT_SUCCESS;
    }

    std::cerr << failures << " test(s) failed.\n";
    return EXIT_FAILURE;
}
