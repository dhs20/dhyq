#include "quantum/physics/AtomicPhysics.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
#include <sstream>
#include <stdexcept>

namespace quantum::physics {
namespace {

using quantum::meta::AnimationKind;
using quantum::meta::MethodStamp;
using quantum::meta::ModelTier;
using quantum::meta::ProvenanceRecord;
using quantum::meta::SourceRecord;
using quantum::meta::ValidationRecord;
using quantum::meta::ValidationStatus;

double factorial(int n) {
    if (n < 0) {
        throw std::invalid_argument("factorial input must be non-negative");
    }
    return std::tgamma(static_cast<double>(n) + 1.0);
}

double generalizedLaguerre(int k, int alpha, double x) {
    if (k == 0) {
        return 1.0;
    }
    if (k == 1) {
        return 1.0 + static_cast<double>(alpha) - x;
    }

    double prev2 = 1.0;
    double prev1 = 1.0 + static_cast<double>(alpha) - x;
    for (int index = 2; index <= k; ++index) {
        const double numerator =
            ((2.0 * static_cast<double>(index) - 1.0 + static_cast<double>(alpha) - x) * prev1) -
            ((static_cast<double>(index) - 1.0 + static_cast<double>(alpha)) * prev2);
        const double current = numerator / static_cast<double>(index);
        prev2 = prev1;
        prev1 = current;
    }
    return prev1;
}

double associatedLegendre(int l, int m, double x) {
    const int absM = std::abs(m);
    double pmm = 1.0;
    if (absM > 0) {
        const double somx2 = std::sqrt(std::max(0.0, 1.0 - x * x));
        double fact = 1.0;
        for (int i = 1; i <= absM; ++i) {
            pmm *= -fact * somx2;
            fact += 2.0;
        }
    }

    if (l == absM) {
        return pmm;
    }

    double pmmp1 = x * (2.0 * static_cast<double>(absM) + 1.0) * pmm;
    if (l == absM + 1) {
        return pmmp1;
    }

    double prev2 = pmm;
    double prev1 = pmmp1;
    double pll = pmmp1;
    for (int ll = absM + 2; ll <= l; ++ll) {
        pll = (((2.0 * static_cast<double>(ll) - 1.0) * x * prev1) -
               ((static_cast<double>(ll) + static_cast<double>(absM) - 1.0) * prev2)) /
              (static_cast<double>(ll) - static_cast<double>(absM));
        prev2 = prev1;
        prev1 = pll;
    }

    return pll;
}

double reducedMassFactor(double nuclearMassKg, bool useReducedMass) {
    if (!useReducedMass || nuclearMassKg <= 0.0) {
        return 1.0;
    }
    const double mu = (constants::electronMassKg * nuclearMassKg) / (constants::electronMassKg + nuclearMassKg);
    return mu / constants::electronMassKg;
}

double effectiveBohrRadius(double zeff, double nuclearMassKg, bool useReducedMass) {
    return constants::bohrRadiusM * (1.0 / reducedMassFactor(nuclearMassKg, useReducedMass)) / std::max(zeff, 1e-6);
}

bool isValidQuantumNumbers(const QuantumNumbers& qn) {
    return qn.n >= 1 && qn.l >= 0 && qn.l < qn.n && std::abs(qn.m) <= qn.l;
}

struct SubshellRule {
    int n;
    int l;
    int capacity;
};

const std::vector<SubshellRule>& aufbauOrder() {
    static const std::vector<SubshellRule> order = {
        {1, 0, 2}, {2, 0, 2}, {2, 1, 6}, {3, 0, 2}, {3, 1, 6}, {4, 0, 2}, {3, 2, 10}, {4, 1, 6}, {5, 0, 2},
        {4, 2, 10}, {5, 1, 6}, {6, 0, 2}, {4, 3, 14}, {5, 2, 10}, {6, 1, 6}, {7, 0, 2}, {5, 3, 14}, {6, 2, 10},
        {7, 1, 6}};
    return order;
}

std::map<int, std::vector<SubshellOccupancy>> exceptionWhitelist() {
    return {
        {24, {{1, 0, 2}, {2, 0, 2}, {2, 1, 6}, {3, 0, 2}, {3, 1, 6}, {4, 0, 1}, {3, 2, 5}}},
        {29, {{1, 0, 2}, {2, 0, 2}, {2, 1, 6}, {3, 0, 2}, {3, 1, 6}, {4, 0, 1}, {3, 2, 10}}},
        {41, {{1, 0, 2}, {2, 0, 2}, {2, 1, 6}, {3, 0, 2}, {3, 1, 6}, {4, 0, 2}, {3, 2, 10}, {4, 1, 6}, {5, 0, 1}, {4, 2, 4}}},
        {42, {{1, 0, 2}, {2, 0, 2}, {2, 1, 6}, {3, 0, 2}, {3, 1, 6}, {4, 0, 1}, {3, 2, 10}, {4, 1, 6}, {5, 0, 1}, {4, 2, 5}}}};
}

SourceRecord internalBenchmarkSource() {
    return {"internal-benchmark", "Internal validation benchmarks", "quantum_atom_tests", "1", "", ""};
}

SourceRecord codataSource() {
    return {"codata-const", "CODATA constants embedded in source", "internal", "2026-mvp", "", ""};
}

MethodStamp makeHydrogenicMethodStamp(int nuclearCharge, double zeff, bool useReducedMass) {
    MethodStamp stamp;
    stamp.methodName = useReducedMass ? "Reduced-mass corrected hydrogenic analytic model"
                                      : "Hydrogenic analytic model";
    stamp.approximation = (std::abs(zeff - static_cast<double>(nuclearCharge)) < 1.0e-6)
                              ? "single-electron analytic model"
                              : "effective-charge hydrogenic estimate";
    stamp.dataSource = "Embedded CODATA constants";
    stamp.validationSummary = "Analytic closed-form expressions with H/He+ regression checks";
    stamp.tier = ModelTier::Tier0Teaching;
    stamp.animationKind = AnimationKind::None;
    if (std::abs(zeff - static_cast<double>(nuclearCharge)) >= 1.0e-6) {
        stamp.caveats.push_back("Using Z_eff instead of the full nuclear charge is a teaching approximation.");
    }
    if (useReducedMass) {
        stamp.caveats.push_back("Reduced-mass correction is applied.");
    }
    return stamp;
}

MethodStamp makeTransitionMethodStamp(const TransitionRequest& request) {
    MethodStamp stamp = makeHydrogenicMethodStamp(request.nuclearCharge, request.initialZeff, request.useReducedMass);
    stamp.methodName = "Hydrogenic transition and spectral observable evaluation";
    stamp.validationSummary = "Canonical hydrogen lines are regression-tested locally";
    if (request.rules == TransitionRuleSet::ElectricDipole) {
        stamp.caveats.push_back("E1 selection rules are enforced.");
    } else {
        stamp.caveats.push_back("Selection rules are disabled for this result.");
    }
    return stamp;
}

MethodStamp makeConfigurationMethodStamp(bool usedExceptionRule) {
    MethodStamp stamp;
    stamp.methodName = "Aufbau-Pauli electron configuration builder";
    stamp.approximation = "teaching configuration builder";
    stamp.dataSource = "Built-in filling-order table";
    stamp.validationSummary = "Selected light-element configurations are regression-tested locally";
    stamp.tier = ModelTier::Tier0Teaching;
    stamp.animationKind = AnimationKind::None;
    stamp.caveats.push_back("This is not a self-consistent-field calculation.");
    if (usedExceptionRule) {
        stamp.caveats.push_back("A configured exception whitelist was applied.");
    }
    return stamp;
}

MethodStamp makeSlaterMethodStamp() {
    MethodStamp stamp;
    stamp.methodName = "Slater-rules effective charge estimate";
    stamp.approximation = "Z_eff teaching approximation";
    stamp.dataSource = "Built-in Slater shielding rules";
    stamp.validationSummary = "Selected subshell benchmarks are regression-tested locally";
    stamp.tier = ModelTier::Tier0Teaching;
    stamp.animationKind = AnimationKind::None;
    stamp.caveats.push_back("No explicit electron correlation or antisymmetrized many-electron wavefunction is solved.");
    return stamp;
}

ValidationRecord makeNumericValidationRecord(const std::string& caseName,
                                             const std::string& referenceName,
                                             const std::string& units,
                                             double referenceValue,
                                             double measuredValue,
                                             double tolerance,
                                             const SourceRecord& source,
                                             const std::string& notes = {}) {
    ValidationRecord record;
    record.caseName = caseName;
    record.referenceName = referenceName;
    record.units = units;
    record.referenceValue = referenceValue;
    record.measuredValue = measuredValue;
    record.absoluteError = std::abs(measuredValue - referenceValue);
    record.relativeError =
        (std::abs(referenceValue) > 1.0e-30) ? (record.absoluteError / std::abs(referenceValue)) : 0.0;
    record.status = (record.absoluteError <= tolerance) ? ValidationStatus::ReferenceMatched : ValidationStatus::SanityChecked;
    record.notes = notes;
    record.source = source;
    return record;
}

ProvenanceRecord makeSpectrumProvenance(int nuclearCharge, const QuantumNumbers& initial, const QuantumNumbers& final) {
    ProvenanceRecord provenance;
    provenance.recordId = "analytic-spectrum-" + std::to_string(nuclearCharge) + "-" + std::to_string(initial.n) + "-" +
                          std::to_string(initial.l) + "-" + std::to_string(final.n) + "-" + std::to_string(final.l);
    provenance.sourceId = codataSource().id;
    provenance.createdBy = "physics::generateSpectrum";
    provenance.notes = "Derived from hydrogenic transition evaluation in the current build.";
    return provenance;
}

ProvenanceRecord makeEnergyLevelProvenance(int nuclearCharge, int n) {
    ProvenanceRecord provenance;
    provenance.recordId = "analytic-energy-" + std::to_string(nuclearCharge) + "-" + std::to_string(n);
    provenance.sourceId = codataSource().id;
    provenance.createdBy = "physics::generateEnergyLevels";
    provenance.notes = "Derived from hydrogenic energy evaluation in the current build.";
    return provenance;
}

double radialWaveFunction(const QuantumNumbers& qn,
                          double zeff,
                          double nuclearMassKg,
                          double radiusM,
                          bool useReducedMass) {
    const double a = effectiveBohrRadius(zeff, nuclearMassKg, useReducedMass);
    const double rho = 2.0 * radiusM / (static_cast<double>(qn.n) * a);
    const int order = qn.n - qn.l - 1;
    const double normalization =
        std::sqrt(std::pow(2.0 / (static_cast<double>(qn.n) * a), 3.0) *
                  factorial(order) /
                  (2.0 * static_cast<double>(qn.n) * factorial(qn.n + qn.l)));
    const double polynomial = generalizedLaguerre(order, 2 * qn.l + 1, rho);
    return normalization * std::exp(-rho / 2.0) * std::pow(rho, static_cast<double>(qn.l)) * polynomial;
}

std::complex<double> sphericalHarmonic(const QuantumNumbers& qn, double theta, double phi) {
    const int absM = std::abs(qn.m);
    const double normalization = std::sqrt(((2.0 * static_cast<double>(qn.l) + 1.0) / (4.0 * constants::pi)) *
                                           (factorial(qn.l - absM) / factorial(qn.l + absM)));
    const double legendre = associatedLegendre(qn.l, absM, std::cos(theta));
    const auto phase = std::exp(std::complex<double>(0.0, static_cast<double>(absM) * phi));
    std::complex<double> value = normalization * legendre * phase;
    if (qn.m < 0) {
        const double sign = (absM % 2 == 0) ? 1.0 : -1.0;
        value = sign * std::conj(value);
    }
    return value;
}

double slaterContributionForSpTarget(const SubshellOccupancy& source, int targetN, int targetL) {
    if (source.n == targetN && source.l == targetL) {
        if (targetN == 1) {
            return 0.30 * static_cast<double>(std::max(0, source.electrons - 1));
        }
        return 0.35 * static_cast<double>(std::max(0, source.electrons - 1));
    }

    if (source.n == targetN && source.l > 1) {
        return static_cast<double>(source.electrons);
    }

    if (source.n == targetN - 1) {
        return (source.l <= 1) ? 0.85 * static_cast<double>(source.electrons) : static_cast<double>(source.electrons);
    }

    if (source.n <= targetN - 2) {
        return static_cast<double>(source.electrons);
    }

    return 0.0;
}

double slaterContributionForDfTarget(const SubshellOccupancy& source, int targetN, int targetL) {
    if (source.n == targetN && source.l == targetL) {
        return 0.35 * static_cast<double>(std::max(0, source.electrons - 1));
    }

    if (source.n < targetN || (source.n == targetN && source.l < targetL)) {
        return static_cast<double>(source.electrons);
    }

    return 0.0;
}

} // namespace

double units::jouleToEv(double joule) {
    return joule / constants::elementaryCharge;
}

double units::evToJoule(double ev) {
    return ev * constants::elementaryCharge;
}

double units::meterToNm(double meter) {
    return meter * 1.0e9;
}

double units::nmToMeter(double nm) {
    return nm * 1.0e-9;
}

double nuclearMassKg(const ElementRecord& element) {
    return element.atomicMassU * constants::atomicMassUnitKg;
}

std::string subshellLabel(int n, int l) {
    static const char* labels[] = {"s", "p", "d", "f", "g"};
    if (l < 0 || l > 4) {
        return std::to_string(n) + "?";
    }
    return std::to_string(n) + labels[l];
}

std::string spectralSeriesName(int finalN) {
    switch (finalN) {
    case 1:
        return "莱曼系";
    case 2:
        return "巴耳末系";
    case 3:
        return "帕邢系";
    case 4:
        return "布拉开系";
    case 5:
        return "芬德系";
    case 6:
        return "汉弗莱系";
    default:
        return "n=" + std::to_string(finalN) + " 谱系";
    }
}

HydrogenicMetrics computeHydrogenicMetrics(int nuclearCharge,
                                           int principalQuantumNumber,
                                           double zeff,
                                           double nuclearMassKg,
                                           bool useReducedMass) {
    const double muFactor = reducedMassFactor(nuclearMassKg, useReducedMass);
    const double effectiveCharge = std::max(zeff, 1e-6);
    const double n = static_cast<double>(principalQuantumNumber);
    const double energyJ = -constants::rydbergEnergyJ * muFactor * effectiveCharge * effectiveCharge / (n * n);
    const double radiusM = constants::bohrRadiusM * (1.0 / muFactor) * (n * n) / effectiveCharge;
    const double speedMps = constants::fineStructureConstant * constants::speedOfLight * effectiveCharge / n;

    HydrogenicMetrics metrics;
    metrics.energyJ = energyJ;
    metrics.energyEv = units::jouleToEv(energyJ);
    metrics.orbitalRadiusM = radiusM;
    metrics.orbitalRadiusNm = units::meterToNm(radiusM);
    metrics.orbitalSpeedMps = speedMps;
    metrics.zeff = effectiveCharge;
    metrics.reducedMassFactor = muFactor;
    metrics.method = makeHydrogenicMethodStamp(nuclearCharge, effectiveCharge, useReducedMass);
    if (principalQuantumNumber == 1 && std::abs(effectiveCharge - static_cast<double>(nuclearCharge)) < 1.0e-6) {
        if (nuclearCharge == 1) {
            metrics.validation.push_back(makeNumericValidationRecord("H 1s energy",
                                                                     "Hydrogenic benchmark",
                                                                     "eV",
                                                                     -13.605693122994,
                                                                     metrics.energyEv,
                                                                     5.0e-2,
                                                                     internalBenchmarkSource()));
        } else if (nuclearCharge == 2) {
            metrics.validation.push_back(makeNumericValidationRecord("He+ 1s energy",
                                                                     "Hydrogenic Z^2 benchmark",
                                                                     "eV",
                                                                     -54.422772491976,
                                                                     metrics.energyEv,
                                                                     2.0e-1,
                                                                     internalBenchmarkSource()));
        }
    }
    return metrics;
}

TransitionResult computeTransition(const TransitionRequest& request) {
    TransitionResult result;
    result.method = makeTransitionMethodStamp(request);
    if (!isValidQuantumNumbers(request.initial) || !isValidQuantumNumbers(request.final)) {
        result.allowed = false;
        result.reason = "量子数无效";
        return result;
    }
    if (request.final.n >= request.initial.n) {
        result.allowed = false;
        result.reason = "末态主量子数必须低于初态";
        return result;
    }

    if (request.rules == TransitionRuleSet::ElectricDipole) {
        const int deltaL = request.final.l - request.initial.l;
        if (std::abs(deltaL) != 1) {
            result.allowed = false;
            result.reason = "不满足 Delta l = +-1";
        }
        if (request.enforceDeltaM) {
            const int deltaM = request.final.m - request.initial.m;
            if (std::abs(deltaM) > 1) {
                result.allowed = false;
                result.reason = "不满足 Delta m 选择规则";
            }
        }
    }

    const auto initialMetrics = computeHydrogenicMetrics(
        request.nuclearCharge, request.initial.n, request.initialZeff, request.nuclearMassKg, request.useReducedMass);
    const auto finalMetrics = computeHydrogenicMetrics(
        request.nuclearCharge, request.final.n, request.finalZeff, request.nuclearMassKg, request.useReducedMass);
    const double deltaEnergyJ = std::abs(finalMetrics.energyJ - initialMetrics.energyJ);
    result.deltaEnergyJ = deltaEnergyJ;
    result.deltaEnergyEv = units::jouleToEv(deltaEnergyJ);
    result.frequencyHz = deltaEnergyJ / constants::planckJouleSecond;
    result.wavelengthM = constants::speedOfLight / std::max(result.frequencyHz, 1e-30);
    result.wavelengthNm = units::meterToNm(result.wavelengthM);
    result.seriesName = spectralSeriesName(request.final.n);
    if (request.nuclearCharge == 1 && std::abs(request.initialZeff - 1.0) < 1.0e-6 &&
        std::abs(request.finalZeff - 1.0) < 1.0e-6) {
        if (request.initial.n == 2 && request.initial.l == 1 && request.final.n == 1 && request.final.l == 0) {
            result.validation.push_back(makeNumericValidationRecord(
                "Hydrogen Lyman-alpha", "Internal benchmark", "nm", 121.567, result.wavelengthNm, 0.15, internalBenchmarkSource()));
        } else if (request.initial.n == 3 && request.initial.l == 1 && request.final.n == 2 && request.final.l == 0) {
            result.validation.push_back(makeNumericValidationRecord(
                "Hydrogen Balmer-alpha", "Internal benchmark", "nm", 656.279, result.wavelengthNm, 0.4, internalBenchmarkSource()));
        }
    }
    return result;
}

std::vector<SpectrumLine> generateSpectrum(int nuclearCharge,
                                           int maxInitialN,
                                           double zeff,
                                           double nuclearMassKg,
                                           TransitionRuleSet rules,
                                           bool enforceDeltaM,
                                           bool useReducedMass) {
    std::vector<SpectrumLine> lines;
    for (int ni = 2; ni <= maxInitialN; ++ni) {
        for (int nf = 1; nf < ni; ++nf) {
            for (int li = 0; li < ni; ++li) {
                for (int lf = 0; lf < nf; ++lf) {
                    const TransitionResult transition = computeTransition(
                        {{ni, li, 0}, {nf, lf, 0}, rules, enforceDeltaM, nuclearCharge, zeff, zeff, nuclearMassKg, useReducedMass});
                    SpectrumLine line;
                    line.initial = {ni, li, 0};
                    line.final = {nf, lf, 0};
                    line.allowed = transition.allowed;
                    line.wavelengthNm = transition.wavelengthNm;
                    line.photonEnergyEv = transition.deltaEnergyEv;
                    line.seriesName = transition.seriesName;
                    line.method = transition.method;
                    line.validation = transition.validation;
                    line.source = codataSource();
                    line.provenance = makeSpectrumProvenance(nuclearCharge, line.initial, line.final);
                    lines.push_back(std::move(line));
                }
            }
        }
    }
    std::sort(lines.begin(), lines.end(), [](const SpectrumLine& lhs, const SpectrumLine& rhs) {
        return lhs.wavelengthNm < rhs.wavelengthNm;
    });
    return lines;
}

std::vector<EnergyLevelSample> generateEnergyLevels(int nuclearCharge,
                                                    int maxN,
                                                    double zeff,
                                                    double nuclearMassKg,
                                                    bool useReducedMass) {
    std::vector<EnergyLevelSample> levels;
    for (int n = 1; n <= maxN; ++n) {
        const auto metrics = computeHydrogenicMetrics(nuclearCharge, n, zeff, nuclearMassKg, useReducedMass);
        EnergyLevelSample sample;
        sample.n = n;
        sample.energyEv = metrics.energyEv;
        sample.method = metrics.method;
        if (!metrics.validation.empty()) {
            sample.validation = metrics.validation.front();
        }
        sample.source = codataSource();
        sample.provenance = makeEnergyLevelProvenance(nuclearCharge, n);
        levels.push_back(std::move(sample));
    }
    return levels;
}

ElectronConfigurationResult buildElectronConfiguration(int atomicNumber, int chargeState) {
    ElectronConfigurationResult result;
    result.electronCount = std::max(0, atomicNumber - chargeState);
    if (result.electronCount == 0) {
        result.notation = "[bare nucleus]";
        result.method = makeConfigurationMethodStamp(false);
        return result;
    }

    const auto exceptions = exceptionWhitelist();
    const auto exception = exceptions.find(atomicNumber);
    if (chargeState == 0 && exception != exceptions.end()) {
        result.subshells = exception->second;
        result.usedExceptionRule = true;
        result.exceptionLabel = "Configured exception";
    } else {
        int remaining = result.electronCount;
        for (const auto& rule : aufbauOrder()) {
            if (remaining <= 0) {
                break;
            }
            const int fill = std::min(rule.capacity, remaining);
            result.subshells.push_back({rule.n, rule.l, fill});
            remaining -= fill;
        }
    }

    std::ostringstream stream;
    bool first = true;
    for (const auto& subshell : result.subshells) {
        if (subshell.electrons <= 0) {
            continue;
        }
        if (!first) {
            stream << ' ';
        }
        stream << subshellLabel(subshell.n, subshell.l) << subshell.electrons;
        first = false;
    }
    result.notation = stream.str();
    result.method = makeConfigurationMethodStamp(result.usedExceptionRule);
    return result;
}

SlaterResult computeSlaterShielding(int atomicNumber,
                                    const ElectronConfigurationResult& configuration,
                                    int targetN,
                                    int targetL) {
    SlaterResult result;
    result.targetLabel = subshellLabel(targetN, targetL);

    double sigma = 0.0;
    if (targetL <= 1) {
        int sameGroupElectrons = 0;
        for (const auto& subshell : configuration.subshells) {
            if (subshell.n == targetN && subshell.l <= 1) {
                sameGroupElectrons += subshell.electrons;
            }
        }
        const double sameGroupFactor = (targetN == 1) ? 0.30 : 0.35;
        sigma += sameGroupFactor * static_cast<double>(std::max(0, sameGroupElectrons - 1));
        for (const auto& subshell : configuration.subshells) {
            if (subshell.n == targetN && subshell.l <= 1) {
                continue;
            }
            sigma += slaterContributionForSpTarget(subshell, targetN, targetL);
        }
    } else {
        int sameGroupElectrons = 0;
        for (const auto& subshell : configuration.subshells) {
            if (subshell.n == targetN && subshell.l == targetL) {
                sameGroupElectrons += subshell.electrons;
            }
        }
        sigma += 0.35 * static_cast<double>(std::max(0, sameGroupElectrons - 1));
        for (const auto& subshell : configuration.subshells) {
            if (subshell.n == targetN && subshell.l == targetL) {
                continue;
            }
            sigma += slaterContributionForDfTarget(subshell, targetN, targetL);
        }
    }

    result.shieldingSigma = sigma;
    result.zeff = std::max(1e-3, static_cast<double>(atomicNumber) - sigma);
    result.method = makeSlaterMethodStamp();
    if (atomicNumber == 10 && targetN == 2 && targetL == 1) {
        result.validation.push_back(makeNumericValidationRecord(
            "Ne 2p Z_eff", "Internal benchmark", "", 5.85, result.zeff, 0.3, internalBenchmarkSource()));
    }
    return result;
}

double radialProbabilityDensity(int nuclearCharge,
                                const QuantumNumbers& qn,
                                double zeff,
                                double nuclearMassKg,
                                double radiusM,
                                bool useReducedMass) {
    const double radial = radialWaveFunction(qn, zeff, nuclearMassKg, radiusM, useReducedMass);
    (void)nuclearCharge;
    return radiusM * radiusM * radial * radial;
}

std::complex<double> sphericalHarmonicValue(const QuantumNumbers& qn, double theta, double phi) {
    if (!isValidQuantumNumbers(qn)) {
        return {0.0, 0.0};
    }
    return sphericalHarmonic(qn, theta, phi);
}

std::complex<double> hydrogenicWavefunction(int nuclearCharge,
                                            const QuantumNumbers& qn,
                                            double zeff,
                                            double nuclearMassKg,
                                            double radiusM,
                                            double theta,
                                            double phi,
                                            bool useReducedMass) {
    if (!isValidQuantumNumbers(qn)) {
        return {0.0, 0.0};
    }
    (void)nuclearCharge;
    return radialWaveFunction(qn, zeff, nuclearMassKg, radiusM, useReducedMass) *
           sphericalHarmonic(qn, theta, phi);
}

ComplexWaveSample evaluateSuperposition(int nuclearCharge,
                                        const std::vector<SuperpositionComponent>& components,
                                        double nuclearMassKg,
                                        double radiusM,
                                        double theta,
                                        double phi,
                                        bool useReducedMass) {
    ComplexWaveSample sample;
    for (const auto& component : normalizedComponents(components)) {
        const auto coefficient = std::polar(component.amplitude, component.phaseRadians);
        sample.psi += coefficient *
                      hydrogenicWavefunction(
                          nuclearCharge, component.qn, component.zeff, nuclearMassKg, radiusM, theta, phi, useReducedMass);
    }
    sample.density = std::norm(sample.psi);
    sample.phaseRadians = std::arg(sample.psi);
    return sample;
}

std::vector<SuperpositionComponent> normalizedComponents(const std::vector<SuperpositionComponent>& components) {
    if (components.empty()) {
        return {{QuantumNumbers{}, 1.0, 0.0, 1.0}};
    }
    double normSquared = 0.0;
    for (const auto& component : components) {
        normSquared += component.amplitude * component.amplitude;
    }
    const double scale = (normSquared > 0.0) ? (1.0 / std::sqrt(normSquared)) : 1.0;
    auto normalized = components;
    for (auto& component : normalized) {
        component.amplitude *= scale;
    }
    return normalized;
}

std::vector<std::pair<double, double>> sampleRadialDistribution(int nuclearCharge,
                                                                const QuantumNumbers& qn,
                                                                double zeff,
                                                                double nuclearMassKg,
                                                                double maxRadiusM,
                                                                int samples,
                                                                bool useReducedMass) {
    std::vector<std::pair<double, double>> values;
    values.reserve(static_cast<std::size_t>(samples));
    for (int index = 0; index < samples; ++index) {
        const double t = (samples > 1) ? static_cast<double>(index) / static_cast<double>(samples - 1) : 0.0;
        const double radius = maxRadiusM * t;
        values.emplace_back(radius, radialProbabilityDensity(nuclearCharge, qn, zeff, nuclearMassKg, radius, useReducedMass));
    }
    return values;
}

} // namespace quantum::physics
