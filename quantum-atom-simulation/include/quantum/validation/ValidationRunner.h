#pragma once

#include "quantum/meta/MethodMetadata.h"

#include <string>
#include <string_view>
#include <vector>

namespace quantum::validation {

struct ValidationRunRequest {
    std::string suiteName;
    std::string caseFilter;
};

struct ValidationRunResult {
    bool passed = false;
    std::vector<quantum::meta::ValidationRecord> records;
    quantum::meta::MethodStamp method;
};

class ValidationRunner {
public:
    virtual ~ValidationRunner() = default;

    [[nodiscard]] virtual std::string_view name() const = 0;
    [[nodiscard]] virtual ValidationRunResult run(const ValidationRunRequest& request) const = 0;
};

} // namespace quantum::validation
