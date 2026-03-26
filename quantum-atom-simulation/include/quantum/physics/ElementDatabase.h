#pragma once

#include "quantum/physics/AtomicPhysics.h"

#include <filesystem>
#include <unordered_map>
#include <vector>

namespace quantum::physics {

class ElementDatabase {
public:
    bool loadFromJson(const std::filesystem::path& path);
    void loadBuiltInSubset();

    [[nodiscard]] const ElementRecord& elementByAtomicNumber(int atomicNumber) const;
    [[nodiscard]] const std::vector<ElementRecord>& elements() const;

private:
    void rebuildIndex();

    std::vector<ElementRecord> elements_;
    std::unordered_map<int, std::size_t> byAtomicNumber_;
};

} // namespace quantum::physics
