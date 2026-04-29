#include "quantum/spectroscopy/HydrogenicCorrections.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace quantum::spectroscopy {
namespace {

constexpr double kElectronGFactor = 2.00231930436256;
constexpr double kBohrMagnetonEvPerTesla = 5.7883818060e-5;

double selectJ(int l, bool lowerBranch) {
    if (l <= 0) {
        return 0.5;
    }
    return lowerBranch ? std::max(0.5, static_cast<double>(l) - 0.5) : static_cast<double>(l) + 0.5;
}

double selectMJ(const quantum::physics::QuantumNumbers& qn, double j) {
    if (j <= 0.5 && qn.l == 0) {
        return 0.5;
    }
    const double clamped = std::clamp(static_cast<double>(qn.m), -j, j);
    if (std::abs(clamped) < 0.5) {
        return (qn.m >= 0) ? 0.5 : -0.5;
    }
    return clamped;
}

double landeFactor(int l, double j) {
    const double s = 0.5;
    return 1.0 + ((j * (j + 1.0)) + (s * (s + 1.0)) - (static_cast<double>(l) * (l + 1.0))) /
                     (2.0 * j * (j + 1.0));
}

std::string orbitalLetter(int l) {
    switch (l) {
    case 0:
        return "S";
    case 1:
        return "P";
    case 2:
        return "D";
    case 3:
        return "F";
    default:
        return "L" + std::to_string(l);
    }
}

std::string halfIntegerLabel(double value) {
    const int twice = static_cast<int>(std::lround(2.0 * value));
    if ((twice % 2) == 0) {
        return std::to_string(twice / 2);
    }
    return std::to_string(twice) + "/2";
}

std::string termSymbol(const quantum::physics::QuantumNumbers& qn, double j, double f) {
    std::ostringstream stream;
    stream << qn.n << " ^2" << orbitalLetter(qn.l) << "_" << halfIntegerLabel(j) << " F=" << halfIntegerLabel(f);
    return stream.str();
}

double fineStructureShiftEv(const quantum::physics::HydrogenicMetrics& metrics,
                            int nuclearCharge,
                            const quantum::physics::QuantumNumbers& qn,
                            double j) {
    const double zAlpha = static_cast<double>(nuclearCharge) * quantum::physics::constants::fineStructureConstant;
    const double bracket = (1.0 / (j + 0.5)) - (3.0 / (4.0 * static_cast<double>(qn.n)));
    return metrics.energyEv * (zAlpha * zAlpha) * bracket / static_cast<double>(qn.n);
}

double hyperfineShiftEv(const quantum::physics::QuantumNumbers& qn,
                        double nuclearSpin,
                        double j,
                        double f,
                        double hyperfineAConstantMicroEv) {
    const double contactScale = (qn.l == 0) ? 1.0 : 0.12;
    const double aEv = hyperfineAConstantMicroEv * 1.0e-6 * contactScale /
                       std::pow(static_cast<double>(std::max(qn.n, 1)), 3.0);
    return 0.5 * aEv * (f * (f + 1.0) - nuclearSpin * (nuclearSpin + 1.0) - j * (j + 1.0));
}

double starkShiftEv(int nuclearCharge,
                    const quantum::physics::QuantumNumbers& qn,
                    double electricFieldVPerM,
                    bool& limited) {
    if (electricFieldVPerM == 0.0) {
        return 0.0;
    }
    if (!(qn.n == 1 && qn.l == 0)) {
        limited = true;
        return 0.0;
    }

    const double a0 = quantum::physics::constants::bohrRadiusM;
    const double epsilon0 = quantum::physics::constants::epsilon0;
    const double polarizabilitySi =
        4.5 * 4.0 * quantum::physics::constants::pi * epsilon0 * std::pow(a0, 3.0) /
        std::pow(static_cast<double>(std::max(nuclearCharge, 1)), 4.0);
    const double energyJ = -0.5 * polarizabilitySi * electricFieldVPerM * electricFieldVPerM;
    return quantum::physics::units::jouleToEv(energyJ);
}

quantum::meta::MethodStamp makeCorrectionMethodStamp(const SpectroscopySettings& settings) {
    quantum::meta::MethodStamp stamp;
    stamp.methodName = "Tier 3 hydrogenic spectroscopy corrections";
    stamp.approximation = "fine-structure + optional Zeeman/Stark/hyperfine teaching estimates";
    stamp.dataSource = "Embedded perturbative formulas";
    stamp.validationSummary = "Fine, Zeeman, Stark, and hyperfine terms are perturbative teaching-level corrections.";
    stamp.tier = quantum::meta::ModelTier::Tier3SpectroscopyCorrections;
    stamp.animationKind = quantum::meta::AnimationKind::None;
    stamp.includesRelativity = settings.applyFineStructure;
    stamp.includesExternalField = settings.applyZeeman || settings.applyStark;
    stamp.caveats.push_back("Spin is not solved dynamically; j-branch selection is user-controlled.");
    if (settings.applyStark) {
        stamp.caveats.push_back("Current Stark implementation is limited to a scalar 1s estimate.");
    }
    if (settings.applyHyperfine) {
        stamp.caveats.push_back("Hyperfine splitting uses a scalar A-constant teaching estimate, not a full nuclear model.");
    }
    return stamp;
}

} // namespace

SpectroscopyCorrectionResult evaluateHydrogenicCorrections(const quantum::physics::TransitionRequest& request,
                                                           const quantum::physics::TransitionResult& baseTransition,
                                                           const SpectroscopySettings& settings,
                                                           double nuclearMassKg) {
    SpectroscopyCorrectionResult result;
    result.method = makeCorrectionMethodStamp(settings);
    result.applied = settings.applyFineStructure || settings.applyZeeman || settings.applyStark || settings.applyHyperfine;
    if (!result.applied || !baseTransition.allowed) {
        result.notes = "No Tier 3 correction enabled or transition is forbidden.";
        return result;
    }

    const auto initialMetrics = quantum::physics::computeHydrogenicMetrics(
        request.nuclearCharge, request.initial.n, request.initialZeff, nuclearMassKg, request.useReducedMass);
    const auto finalMetrics = quantum::physics::computeHydrogenicMetrics(
        request.nuclearCharge, request.final.n, request.finalZeff, nuclearMassKg, request.useReducedMass);

    result.initial.j = selectJ(request.initial.l, settings.initialBranch.lowerJBranch);
    result.final.j = selectJ(request.final.l, settings.finalBranch.lowerJBranch);
    result.initial.mJ = selectMJ(request.initial, result.initial.j);
    result.final.mJ = selectMJ(request.final, result.final.j);
    result.initial.f = std::max(std::abs(settings.nuclearSpin - result.initial.j), settings.nuclearSpin + result.initial.j);
    result.final.f = std::max(std::abs(settings.nuclearSpin - result.final.j), settings.nuclearSpin + result.final.j);
    result.initial.landeG = landeFactor(request.initial.l, result.initial.j);
    result.final.landeG = landeFactor(request.final.l, result.final.j);
    result.initial.correctedEnergyEv = initialMetrics.energyEv;
    result.final.correctedEnergyEv = finalMetrics.energyEv;
    result.initial.termSymbol = termSymbol(request.initial, result.initial.j, result.initial.f);
    result.final.termSymbol = termSymbol(request.final, result.final.j, result.final.f);
    result.initialTermSymbol = result.initial.termSymbol;
    result.finalTermSymbol = result.final.termSymbol;

    if (settings.applyFineStructure) {
        result.initial.fineStructureShiftEv =
            fineStructureShiftEv(initialMetrics, request.nuclearCharge, request.initial, result.initial.j);
        result.final.fineStructureShiftEv =
            fineStructureShiftEv(finalMetrics, request.nuclearCharge, request.final, result.final.j);
    }
    if (settings.applyZeeman) {
        result.initial.zeemanShiftEv =
            kBohrMagnetonEvPerTesla * result.initial.landeG * result.initial.mJ * settings.magneticFieldTesla;
        result.final.zeemanShiftEv =
            kBohrMagnetonEvPerTesla * result.final.landeG * result.final.mJ * settings.magneticFieldTesla;
    }
    if (settings.applyStark) {
        result.initial.starkShiftEv =
            starkShiftEv(request.nuclearCharge, request.initial, settings.electricFieldVPerM, result.starkApproximationLimited);
        result.final.starkShiftEv =
            starkShiftEv(request.nuclearCharge, request.final, settings.electricFieldVPerM, result.starkApproximationLimited);
    }
    if (settings.applyHyperfine) {
        result.initial.hyperfineShiftEv = hyperfineShiftEv(request.initial,
                                                          settings.nuclearSpin,
                                                          result.initial.j,
                                                          result.initial.f,
                                                          settings.hyperfineAConstantMicroEv);
        result.final.hyperfineShiftEv = hyperfineShiftEv(request.final,
                                                        settings.nuclearSpin,
                                                        result.final.j,
                                                        result.final.f,
                                                        settings.hyperfineAConstantMicroEv);
    }

    result.initial.totalShiftEv =
        result.initial.fineStructureShiftEv + result.initial.zeemanShiftEv + result.initial.starkShiftEv +
        result.initial.hyperfineShiftEv;
    result.final.totalShiftEv =
        result.final.fineStructureShiftEv + result.final.zeemanShiftEv + result.final.starkShiftEv +
        result.final.hyperfineShiftEv;
    result.initial.correctedEnergyEv += result.initial.totalShiftEv;
    result.final.correctedEnergyEv += result.final.totalShiftEv;
    result.correctedDeltaEnergyEv = result.initial.correctedEnergyEv - result.final.correctedEnergyEv;
    result.correctedWavelengthNm =
        (std::abs(result.correctedDeltaEnergyEv) > 1.0e-20)
            ? (1239.8419843320026 / std::abs(result.correctedDeltaEnergyEv))
            : 0.0;

    result.notes = result.starkApproximationLimited
                       ? "Stark correction currently falls back to a 1s scalar estimate only."
                       : "Corrections were evaluated perturbatively.";

    quantum::meta::ValidationRecord record;
    record.status = quantum::meta::ValidationStatus::SanityChecked;
    record.caseName = "Tier 3 corrected transition";
    record.referenceName = "Uncorrected transition baseline";
    record.units = "eV";
    record.referenceValue = baseTransition.deltaEnergyEv;
    record.measuredValue = result.correctedDeltaEnergyEv;
    record.absoluteError = std::abs(result.correctedDeltaEnergyEv - baseTransition.deltaEnergyEv);
    record.relativeError = (std::abs(baseTransition.deltaEnergyEv) > 1.0e-30)
                               ? (record.absoluteError / std::abs(baseTransition.deltaEnergyEv))
                               : 0.0;
    record.notes = "Difference relative to the uncorrected hydrogenic transition.";
    record.source = {"tier3-corrections", "Embedded spectroscopy corrections", "internal", "1", "", ""};
    result.validation.push_back(record);
    return result;
}

} // namespace quantum::spectroscopy
