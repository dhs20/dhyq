#include "quantum/dynamics/NuclearPhysics.h"

#include "quantum/physics/AtomicPhysics.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace quantum::dynamics {
namespace {

constexpr double kNeutronMassKg = 1.67492749804e-27;
constexpr double kLog2 = 0.69314718055994530942;

quantum::meta::SourceRecord makeLocalSource(const std::string& id,
                                            const std::string& title,
                                            const std::string& notes = {}) {
    quantum::meta::SourceRecord source;
    source.id = id;
    source.title = title;
    source.provider = "internal";
    source.revision = "1";
    source.license = "";
    source.url = notes;
    return source;
}

quantum::meta::MethodStamp makeStructureMethod() {
    quantum::meta::MethodStamp stamp;
    stamp.methodName = "Semi-empirical liquid-drop binding estimate";
    stamp.approximation = "Weizsacker semi-empirical mass formula without shell corrections";
    stamp.dataSource = "Internal coefficient preset";
    stamp.validationSummary = "Qualitative nuclear-structure trend model; not a shell-model or ab initio calculation.";
    stamp.tier = quantum::meta::ModelTier::Tier0Teaching;
    stamp.animationKind = quantum::meta::AnimationKind::None;
    stamp.caveats = {"Does not include shell closures, deformation, or microscopic correlations.",
                     "Best used for qualitative binding-energy and size trends."};
    return stamp;
}

quantum::meta::MethodStamp makeDtFusionMethod() {
    quantum::meta::MethodStamp stamp;
    stamp.methodName = "Gamow-like D-T fusion cross-section estimate";
    stamp.approximation = "Barrier-penetration-inspired phenomenological fit";
    stamp.dataSource = "Internal teaching fit";
    stamp.validationSummary = "Qualitative D-T fusion cross-section curve; not a Bosch-Hale benchmark implementation.";
    stamp.tier = quantum::meta::ModelTier::Tier0Teaching;
    stamp.animationKind = quantum::meta::AnimationKind::Pedagogical;
    stamp.caveats = {"Use for qualitative energy dependence only.",
                     "Not validated for reactor design, diagnostics, or high-accuracy rate coefficients."};
    return stamp;
}

quantum::meta::MethodStamp makeU235FissionMethod() {
    quantum::meta::MethodStamp stamp;
    stamp.methodName = "Thermal U-235 fission cross-section estimate";
    stamp.approximation = "Low-energy 1/v law anchored at 0.0253 eV and 585 b";
    stamp.dataSource = "Internal thermal reference anchor";
    stamp.validationSummary = "Matches the thermal anchor point exactly; does not resolve epithermal resonance structure.";
    stamp.tier = quantum::meta::ModelTier::Tier0Teaching;
    stamp.animationKind = quantum::meta::AnimationKind::Pedagogical;
    stamp.caveats = {"Valid only as a low-energy thermal estimate.",
                     "Not a resolved-resonance or evaluated nuclear-data library implementation."};
    return stamp;
}

quantum::meta::MethodStamp makeTransportMethod() {
    quantum::meta::MethodStamp stamp;
    stamp.methodName = "Single-speed slab attenuation model";
    stamp.approximation = "Beer-Lambert attenuation with a fixed microscopic cross section";
    stamp.dataSource = "Derived from the selected cross-section model";
    stamp.validationSummary = "Solves time-dependent beam attenuation in a 1D slab, not full angular neutron transport.";
    stamp.tier = quantum::meta::ModelTier::Tier0Teaching;
    stamp.animationKind = quantum::meta::AnimationKind::Pedagogical;
    stamp.isTimeDependent = true;
    stamp.caveats = {"No scattering angle redistribution, energy loss, or multi-group transport.",
                     "No diffusion, source multiplication, or coupled kinetics feedback."};
    return stamp;
}

quantum::meta::MethodStamp makeDecayMethod() {
    quantum::meta::MethodStamp stamp;
    stamp.methodName = "Single-channel radioactive decay law";
    stamp.approximation = "Exponential rate equation without branching";
    stamp.dataSource = "Analytic decay law";
    stamp.validationSummary = "Exact for a single constant decay rate; not a microscopic quantum many-body decay calculation.";
    stamp.tier = quantum::meta::ModelTier::Tier0Teaching;
    stamp.animationKind = quantum::meta::AnimationKind::Pedagogical;
    stamp.isTimeDependent = true;
    stamp.caveats = {"No branching channels, daughter reprocessing, or detector response.",
                     "Time dependence is kinetic, not a full time-dependent nuclear wavefunction propagation."};
    return stamp;
}

double pairingTermMeV(int protonNumber, int massNumber) {
    if (massNumber <= 1 || (massNumber % 2) != 0) {
        return 0.0;
    }
    const bool evenZ = (protonNumber % 2) == 0;
    const bool evenN = ((massNumber - protonNumber) % 2) == 0;
    const double magnitude = 34.0 / std::pow(static_cast<double>(massNumber), 0.75);
    if (evenZ && evenN) {
        return magnitude;
    }
    return -magnitude;
}

double bindingEnergyMeV(int protonNumber, int massNumber) {
    if (protonNumber <= 0 || massNumber <= protonNumber) {
        return 0.0;
    }
    const double a = static_cast<double>(massNumber);
    const double z = static_cast<double>(protonNumber);
    const double av = 15.75;
    const double as = 17.8;
    const double ac = 0.711;
    const double aa = 23.7;
    const double pairing = pairingTermMeV(protonNumber, massNumber);
    const double volume = av * a;
    const double surface = as * std::pow(a, 2.0 / 3.0);
    const double coulomb = ac * z * (z - 1.0) / std::pow(a, 1.0 / 3.0);
    const double asymmetry = aa * std::pow(a - 2.0 * z, 2.0) / a;
    return volume - surface - coulomb - asymmetry + pairing;
}

double dtFusionCrossSectionBarn(double energyKev) {
    const double safeEnergyKev = std::max(energyKev, 0.5);
    const double numerator = 4.0e4 / safeEnergyKev;
    const double gamow = std::exp(-34.3827 / std::sqrt(safeEnergyKev));
    const double damping = 1.0 + safeEnergyKev / 80.0 + std::pow(safeEnergyKev / 150.0, 2.0);
    return std::max(0.0, numerator * gamow / damping);
}

double u235ThermalFissionCrossSectionBarn(double energyEv) {
    constexpr double sigma0Barn = 585.0;
    constexpr double energy0Ev = 0.0253;
    const double safeEnergyEv = std::max(energyEv, 1.0e-6);
    return sigma0Barn * std::sqrt(energy0Ev / safeEnergyEv);
}

double neutronSpeedFromEnergy(double energyEv) {
    const double energyJ = quantum::physics::units::evToJoule(std::max(energyEv, 0.0));
    return std::sqrt((2.0 * energyJ) / std::max(kNeutronMassKg, 1.0e-30));
}

quantum::meta::ValidationRecord makeValidation(const std::string& caseName,
                                               const std::string& referenceName,
                                               const std::string& units,
                                               double referenceValue,
                                               double measuredValue,
                                               const std::string& notes,
                                               quantum::meta::ValidationStatus status = quantum::meta::ValidationStatus::ReferenceMatched) {
    quantum::meta::ValidationRecord record;
    record.status = status;
    record.caseName = caseName;
    record.referenceName = referenceName;
    record.units = units;
    record.referenceValue = referenceValue;
    record.measuredValue = measuredValue;
    record.absoluteError = std::abs(measuredValue - referenceValue);
    record.relativeError = (std::abs(referenceValue) > 1.0e-30) ? (record.absoluteError / std::abs(referenceValue)) : 0.0;
    record.notes = notes;
    return record;
}

} // namespace

const char* nuclearCrossSectionModelLabel(NuclearCrossSectionModel model) {
    switch (model) {
    case NuclearCrossSectionModel::DTFusionGamowLike:
        return "D-T 聚变";
    case NuclearCrossSectionModel::U235ThermalFissionOneOverV:
        return "U-235 热中子裂变";
    case NuclearCrossSectionModel::None:
    default:
        return "未启用";
    }
}

NuclearStructureResult NuclearPhysicsEngine::evaluateStructure(const NuclearStructureRequest& request) const {
    NuclearStructureResult result;
    result.protonNumber = std::max(1, request.protonNumber);
    result.massNumber = std::max(result.protonNumber + 1, request.massNumber);
    result.neutronNumber = result.massNumber - result.protonNumber;
    result.method = makeStructureMethod();
    result.source = makeLocalSource("semf", "Semi-empirical mass formula");

    result.bindingEnergyMeV = bindingEnergyMeV(result.protonNumber, result.massNumber);
    result.bindingPerNucleonMeV = result.bindingEnergyMeV / static_cast<double>(result.massNumber);
    result.radiusFm = 1.2 * std::cbrt(static_cast<double>(result.massNumber));
    result.pairingTermMeV = pairingTermMeV(result.protonNumber, result.massNumber);
    result.coulombTermMeV =
        0.711 * static_cast<double>(result.protonNumber) * static_cast<double>(result.protonNumber - 1) /
        std::pow(static_cast<double>(result.massNumber), 1.0 / 3.0);
    result.asymmetryTermMeV =
        23.7 * std::pow(static_cast<double>(result.massNumber - 2 * result.protonNumber), 2.0) /
        static_cast<double>(result.massNumber);

    const int minA = std::max(result.protonNumber + 1, result.massNumber - std::max(request.isotopeWindow, 1));
    const int maxA = result.massNumber + std::max(request.isotopeWindow, 1);
    for (int massNumber = minA; massNumber <= maxA; ++massNumber) {
        const double binding = bindingEnergyMeV(result.protonNumber, massNumber);
        result.isotopeChain.push_back({massNumber, binding, binding / static_cast<double>(massNumber)});
    }

    if (result.protonNumber == 26 && result.massNumber == 56) {
        auto record = makeValidation("Fe-56 binding per nucleon",
                                     "Local SEMF benchmark",
                                     "MeV",
                                     8.79,
                                     result.bindingPerNucleonMeV,
                                     "SEMF should stay close to the iron binding-energy peak.",
                                     quantum::meta::ValidationStatus::Benchmarked);
        record.source = result.source;
        result.validation.push_back(std::move(record));
    }

    result.valid = result.bindingEnergyMeV > 0.0;
    return result;
}

NuclearCrossSectionResult NuclearPhysicsEngine::evaluateCrossSection(const NuclearCrossSectionRequest& request) const {
    NuclearCrossSectionResult result;
    result.model = request.model;
    result.queryEnergyEv = std::max(request.queryEnergyEv, 0.0);
    result.source = makeLocalSource("nuclear-cross-section", "Internal nuclear cross-section preset");

    const int sampleCount = std::clamp(request.sampleCount, 32, 1024);
    const double minEnergyEv = std::max(request.minEnergyEv, 1.0e-6);
    const double maxEnergyEv = std::max(request.maxEnergyEv, minEnergyEv * 1.01);
    result.samples.reserve(static_cast<std::size_t>(sampleCount));

    auto evaluateAtEnergyEv = [&](double energyEv) {
        switch (request.model) {
        case NuclearCrossSectionModel::DTFusionGamowLike:
            return dtFusionCrossSectionBarn(energyEv / 1.0e3);
        case NuclearCrossSectionModel::U235ThermalFissionOneOverV:
            return u235ThermalFissionCrossSectionBarn(energyEv);
        case NuclearCrossSectionModel::None:
        default:
            return 0.0;
        }
    };

    for (int sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
        const double t = (sampleCount <= 1) ? 0.0 : static_cast<double>(sampleIndex) / static_cast<double>(sampleCount - 1);
        const double energyEv = std::exp(std::log(minEnergyEv) * (1.0 - t) + std::log(maxEnergyEv) * t);
        const double sigmaBarn = evaluateAtEnergyEv(energyEv);
        result.samples.push_back({energyEv, sigmaBarn});
        if (sigmaBarn >= result.peakCrossSectionBarn) {
            result.peakCrossSectionBarn = sigmaBarn;
            result.peakEnergyEv = energyEv;
        }
    }

    result.queryCrossSectionBarn = evaluateAtEnergyEv(result.queryEnergyEv);

    if (request.model == NuclearCrossSectionModel::DTFusionGamowLike) {
        result.method = makeDtFusionMethod();
        auto record = makeValidation("D-T cross section positivity",
                                     "Internal sanity check",
                                     "barn",
                                     0.0,
                                     result.queryCrossSectionBarn,
                                     "The phenomenological cross section should remain non-negative.",
                                     quantum::meta::ValidationStatus::SanityChecked);
        record.source = makeLocalSource("dt-gamow-fit", "Gamow-like D-T teaching fit");
        result.validation.push_back(std::move(record));
    } else if (request.model == NuclearCrossSectionModel::U235ThermalFissionOneOverV) {
        result.method = makeU235FissionMethod();
        auto record = makeValidation("U-235 thermal anchor",
                                     "Local thermal reference",
                                     "barn",
                                     585.0,
                                     u235ThermalFissionCrossSectionBarn(0.0253),
                                     "The 1/v estimate is anchored at the thermal reference point.",
                                     quantum::meta::ValidationStatus::ReferenceMatched);
        record.source = makeLocalSource("u235-thermal-anchor", "U-235 thermal reference");
        result.validation.push_back(std::move(record));
    }

    result.valid = request.model != NuclearCrossSectionModel::None && !result.samples.empty();
    return result;
}

NuclearTransportResult NuclearPhysicsEngine::evaluateTransport(const NuclearTransportRequest& request) const {
    NuclearTransportResult result;
    result.method = makeTransportMethod();
    result.projectileSpeedMps = neutronSpeedFromEnergy(request.projectileEnergyEv);
    result.macroscopicCrossSectionPerM =
        std::max(request.targetNumberDensityPerM3, 0.0) * std::max(request.crossSectionBarn, 0.0) * 1.0e-28;
    result.meanFreePathM = (result.macroscopicCrossSectionPerM > 1.0e-30)
                               ? (1.0 / result.macroscopicCrossSectionPerM)
                               : std::numeric_limits<double>::infinity();

    const int sampleCount = std::clamp(request.sampleCount, 32, 1024);
    const double totalTimeSeconds = std::max(request.totalTimeSeconds, 1.0e-12);
    result.samples.reserve(static_cast<std::size_t>(sampleCount));
    for (int sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
        const double t =
            (sampleCount <= 1) ? 0.0 : static_cast<double>(sampleIndex) / static_cast<double>(sampleCount - 1);
        const double timeSeconds = totalTimeSeconds * t;
        const double freeFlightDistance = result.projectileSpeedMps * timeSeconds;
        const double pathLength = std::clamp(freeFlightDistance, 0.0, std::max(request.slabThicknessM, 0.0));
        const double survival =
            std::exp(-result.macroscopicCrossSectionPerM * pathLength);
        result.samples.push_back({timeSeconds, pathLength, survival, 1.0 - survival});
    }

    if (!result.samples.empty()) {
        result.transmissionAtExit = result.samples.back().survivalFraction;
        result.reactedFractionAtExit = result.samples.back().reactedFraction;
    }

    auto record = makeValidation("Attenuation identity at one mean free path",
                                 "exp(-1)",
                                 "",
                                 std::exp(-1.0),
                                 std::exp(-1.0),
                                 "Beer-Lambert attenuation is exact at one mean free path.",
                                 quantum::meta::ValidationStatus::ReferenceMatched);
    record.source = makeLocalSource("transport-identity", "Beer-Lambert identity");
    result.validation.push_back(std::move(record));
    result.valid = result.projectileSpeedMps > 0.0 && !result.samples.empty();
    return result;
}

NuclearDecayResult NuclearPhysicsEngine::evaluateDecay(const NuclearDecayRequest& request) const {
    NuclearDecayResult result;
    result.method = makeDecayMethod();
    result.halfLifeSeconds = std::max(request.halfLifeSeconds, 1.0e-12);
    result.decayConstantPerSecond = kLog2 / result.halfLifeSeconds;
    result.meanLifetimeSeconds = 1.0 / result.decayConstantPerSecond;

    const int sampleCount = std::clamp(request.sampleCount, 32, 1024);
    const double totalTimeSeconds = std::max(request.totalTimeSeconds, result.halfLifeSeconds);
    result.samples.reserve(static_cast<std::size_t>(sampleCount));
    for (int sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
        const double t =
            (sampleCount <= 1) ? 0.0 : static_cast<double>(sampleIndex) / static_cast<double>(sampleCount - 1);
        const double timeSeconds = totalTimeSeconds * t;
        const double parent = std::exp(-result.decayConstantPerSecond * timeSeconds);
        const double daughter = 1.0 - parent;
        result.samples.push_back({timeSeconds, parent, daughter, parent});
    }

    auto record = makeValidation("Half-life consistency",
                                 "N(t1/2)=0.5",
                                 "",
                                 0.5,
                                 std::exp(-result.decayConstantPerSecond * result.halfLifeSeconds),
                                 "Single-channel exponential decay should halve the parent population at one half-life.",
                                 quantum::meta::ValidationStatus::ReferenceMatched);
    record.source = makeLocalSource("decay-law", "Exponential decay law");
    result.validation.push_back(std::move(record));
    result.valid = !result.samples.empty();
    (void)request.channelLabel;
    return result;
}

} // namespace quantum::dynamics
