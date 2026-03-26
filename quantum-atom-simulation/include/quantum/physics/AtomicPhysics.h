#pragma once

#include <complex>
#include <string>
#include <utility>
#include <vector>

namespace quantum::physics {

struct ElementRecord {
    int atomicNumber = 1;
    std::string symbol;
    std::string name;
    double atomicMassU = 1.0;
};

struct QuantumNumbers {
    int n = 1;
    int l = 0;
    int m = 0;
};

enum class ApproximationMode {
    StrictHydrogenic,
    MultiElectronApproximation
};

enum class TransitionRuleSet {
    None,
    ElectricDipole
};

struct HydrogenicMetrics {
    double energyJ = 0.0;
    double energyEv = 0.0;
    double orbitalRadiusM = 0.0;
    double orbitalRadiusNm = 0.0;
    double orbitalSpeedMps = 0.0;
    double zeff = 1.0;
    double reducedMassFactor = 1.0;
};

struct TransitionRequest {
    QuantumNumbers initial;
    QuantumNumbers final;
    TransitionRuleSet rules = TransitionRuleSet::ElectricDipole;
    bool enforceDeltaM = false;
    int nuclearCharge = 1;
    double initialZeff = 1.0;
    double finalZeff = 1.0;
    double nuclearMassKg = 0.0;
    bool useReducedMass = true;
};

struct TransitionResult {
    bool allowed = true;
    std::string reason;
    double deltaEnergyJ = 0.0;
    double deltaEnergyEv = 0.0;
    double frequencyHz = 0.0;
    double wavelengthM = 0.0;
    double wavelengthNm = 0.0;
    std::string seriesName;
};

struct SpectrumLine {
    QuantumNumbers initial;
    QuantumNumbers final;
    bool allowed = true;
    double wavelengthNm = 0.0;
    double photonEnergyEv = 0.0;
    std::string seriesName;
};

struct SubshellOccupancy {
    int n = 1;
    int l = 0;
    int electrons = 0;
};

struct ElectronConfigurationResult {
    int electronCount = 1;
    std::vector<SubshellOccupancy> subshells;
    std::string notation;
    bool usedExceptionRule = false;
    std::string exceptionLabel;
};

struct SlaterResult {
    std::string targetLabel;
    double shieldingSigma = 0.0;
    double zeff = 1.0;
};

struct SuperpositionComponent {
    QuantumNumbers qn;
    double amplitude = 1.0;
    double phaseRadians = 0.0;
    double zeff = 1.0;
};

struct ComplexWaveSample {
    std::complex<double> psi;
    double density = 0.0;
    double phaseRadians = 0.0;
};

struct EnergyLevelSample {
    int n = 1;
    double energyEv = 0.0;
};

namespace constants {

inline constexpr double pi = 3.141592653589793238462643383279502884;
inline constexpr double reducedPlanckJouleSecond = 1.054571817e-34;
inline constexpr double planckJouleSecond = 6.62607015e-34;
inline constexpr double speedOfLight = 299792458.0;
inline constexpr double elementaryCharge = 1.602176634e-19;
inline constexpr double electronMassKg = 9.1093837015e-31;
inline constexpr double epsilon0 = 8.8541878128e-12;
inline constexpr double bohrRadiusM = 5.29177210903e-11;
inline constexpr double rydbergEnergyJ = 2.1798723611035e-18;
inline constexpr double fineStructureConstant = 7.2973525693e-3;
inline constexpr double atomicMassUnitKg = 1.66053906660e-27;

} // namespace constants

namespace units {

double jouleToEv(double joule);
double evToJoule(double ev);
double meterToNm(double meter);
double nmToMeter(double nm);

} // namespace units

[[nodiscard]] double nuclearMassKg(const ElementRecord& element);
[[nodiscard]] std::string subshellLabel(int n, int l);
[[nodiscard]] std::string spectralSeriesName(int finalN);

[[nodiscard]] HydrogenicMetrics computeHydrogenicMetrics(int nuclearCharge,
                                                         int principalQuantumNumber,
                                                         double zeff,
                                                         double nuclearMassKg,
                                                         bool useReducedMass);
[[nodiscard]] TransitionResult computeTransition(const TransitionRequest& request);
[[nodiscard]] std::vector<SpectrumLine> generateSpectrum(int nuclearCharge,
                                                         int maxInitialN,
                                                         double zeff,
                                                         double nuclearMassKg,
                                                         TransitionRuleSet rules,
                                                         bool enforceDeltaM,
                                                         bool useReducedMass);
[[nodiscard]] std::vector<EnergyLevelSample> generateEnergyLevels(int nuclearCharge,
                                                                  int maxN,
                                                                  double zeff,
                                                                  double nuclearMassKg,
                                                                  bool useReducedMass);

[[nodiscard]] ElectronConfigurationResult buildElectronConfiguration(int atomicNumber, int chargeState);
[[nodiscard]] SlaterResult computeSlaterShielding(int atomicNumber,
                                                  const ElectronConfigurationResult& configuration,
                                                  int targetN,
                                                  int targetL);

[[nodiscard]] double radialProbabilityDensity(int nuclearCharge,
                                              const QuantumNumbers& qn,
                                              double zeff,
                                              double nuclearMassKg,
                                              double radiusM);
[[nodiscard]] std::complex<double> sphericalHarmonicValue(const QuantumNumbers& qn, double theta, double phi);
[[nodiscard]] std::complex<double> hydrogenicWavefunction(int nuclearCharge,
                                                          const QuantumNumbers& qn,
                                                          double zeff,
                                                          double nuclearMassKg,
                                                          double radiusM,
                                                          double theta,
                                                          double phi);
[[nodiscard]] ComplexWaveSample evaluateSuperposition(int nuclearCharge,
                                                      const std::vector<SuperpositionComponent>& components,
                                                      double nuclearMassKg,
                                                      double radiusM,
                                                      double theta,
                                                      double phi);
[[nodiscard]] std::vector<SuperpositionComponent> normalizedComponents(const std::vector<SuperpositionComponent>& components);
[[nodiscard]] std::vector<std::pair<double, double>> sampleRadialDistribution(int nuclearCharge,
                                                                              const QuantumNumbers& qn,
                                                                              double zeff,
                                                                              double nuclearMassKg,
                                                                              double maxRadiusM,
                                                                              int samples);

} // namespace quantum::physics
