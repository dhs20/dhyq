#pragma once

#include "quantum/data/Records.h"

#include <optional>
#include <vector>

namespace quantum::data {

class DataProvider {
public:
    virtual ~DataProvider() = default;

    [[nodiscard]] virtual std::vector<ElementRecord> elements() const = 0;
    [[nodiscard]] virtual std::optional<ElementRecord> elementByAtomicNumber(int atomicNumber) const = 0;
    [[nodiscard]] virtual std::vector<TransitionRecord> referenceTransitions(int atomicNumber) const = 0;
};

} // namespace quantum::data
