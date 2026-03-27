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
    bool includeDissipation = false;
};

struct DynamicsResult {
    bool solved = false;
    std::vector<double> sampleTimes;
    std::vector<double> observables;
    quantum::meta::MethodStamp method;
    std::vector<quantum::meta::ValidationRecord> validation;
};

class DynamicsEngine {
public:
    virtual ~DynamicsEngine() = default;

    [[nodiscard]] virtual std::string_view name() const = 0;
    [[nodiscard]] virtual DynamicsResult propagate(const DynamicsRequest& request) = 0;
};

} // namespace quantum::dynamics
