#pragma once

#include "quantum/meta/MethodMetadata.h"

#include <string>
#include <string_view>
#include <vector>

namespace quantum::dynamics {

struct DynamicsRequest {
    std::string modelLabel;
    double totalTimeSeconds = 0.0;
    double timeStepSeconds = 0.0;
    std::string externalFieldLabel;
    double couplingEv = 0.0;
    double detuningEv = 0.0;
    double dampingRatePerSecond = 0.0;
    bool includeDissipation = false;
};

struct DynamicsResult {
    bool solved = false;
    std::vector<double> sampleTimes;
    std::vector<double> observables;
    std::vector<double> lowerPopulation;
    std::vector<double> upperPopulation;
    std::vector<double> dipoleExpectation;
    quantum::meta::MethodStamp method;
    std::vector<quantum::meta::ValidationRecord> validation;
};

class DynamicsEngine {
public:
    virtual ~DynamicsEngine() = default;

    [[nodiscard]] virtual std::string_view name() const = 0;
    [[nodiscard]] virtual DynamicsResult propagate(const DynamicsRequest& request) = 0;
};

class TeachingDynamicsEngine final : public DynamicsEngine {
public:
    [[nodiscard]] std::string_view name() const override;
    [[nodiscard]] DynamicsResult propagate(const DynamicsRequest& request) override;
};

} // namespace quantum::dynamics
