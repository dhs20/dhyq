#pragma once

#include "quantum/meta/MethodMetadata.h"

#include <cstddef>
#include <string>
#include <vector>

namespace quantum::physics {

struct HamiltonianAssemblyRequest {
    int atomicNumber = 1;
    int electronCount = 1;
    std::string basisLabel;
    bool includeRelativity = false;
    bool includeExternalField = false;
};

struct HamiltonianAssemblyResult {
    std::size_t dimension = 0;
    std::vector<double> diagonal;
    quantum::meta::MethodStamp method;
    std::vector<quantum::meta::ValidationRecord> validation;
};

class HamiltonianAssembler {
public:
    virtual ~HamiltonianAssembler() = default;
    [[nodiscard]] virtual HamiltonianAssemblyResult assemble(const HamiltonianAssemblyRequest& request) const = 0;
};

} // namespace quantum::physics
