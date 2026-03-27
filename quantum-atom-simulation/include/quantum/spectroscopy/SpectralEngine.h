#pragma once

#include "quantum/data/Records.h"

#include <string_view>
#include <vector>

namespace quantum::spectroscopy {

struct SpectralRequest {
    int atomicNumber = 1;
    int chargeState = 0;
    int maxPrincipalNumber = 6;
    bool applySelectionRules = true;
};

struct SpectralResult {
    std::vector<quantum::data::TransitionRecord> transitions;
    quantum::meta::MethodStamp method;
    std::vector<quantum::meta::ValidationRecord> validation;
};

class SpectralEngine {
public:
    virtual ~SpectralEngine() = default;

    [[nodiscard]] virtual std::string_view name() const = 0;
    [[nodiscard]] virtual SpectralResult build(const SpectralRequest& request) const = 0;
};

} // namespace quantum::spectroscopy
