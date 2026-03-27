#pragma once

#include "quantum/meta/MethodMetadata.h"

#include <string>
#include <vector>

namespace quantum::physics {

struct ConfigurationBuildRequest {
    int atomicNumber = 1;
    int chargeState = 0;
    bool allowExceptions = true;
};

struct ConfigurationBuildResult {
    std::string notation;
    std::vector<std::string> occupiedSubshells;
    quantum::meta::MethodStamp method;
    std::vector<quantum::meta::ValidationRecord> validation;
};

class ConfigurationBuilder {
public:
    virtual ~ConfigurationBuilder() = default;
    [[nodiscard]] virtual ConfigurationBuildResult build(const ConfigurationBuildRequest& request) const = 0;
};

} // namespace quantum::physics
