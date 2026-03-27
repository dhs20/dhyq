#pragma once

#include "quantum/data/JsonDataProvider.h"
#include "quantum/physics/AtomicPhysics.h"

#include <filesystem>
#include <unordered_map>
#include <vector>

namespace quantum::physics {

class ElementDatabase {
public:
    bool loadFromJson(const std::filesystem::path& path);
    bool loadReferenceTransitions(const std::filesystem::path& path);
    void loadBuiltInSubset();

    [[nodiscard]] const ElementRecord& elementByAtomicNumber(int atomicNumber) const;
    [[nodiscard]] const std::vector<ElementRecord>& elements() const;
    [[nodiscard]] const quantum::data::ElementRecord* elementMetadataByAtomicNumber(int atomicNumber) const;
    [[nodiscard]] std::vector<quantum::data::TransitionRecord> referenceTransitions(int atomicNumber) const;

private:
    void rebuildIndex();

    std::vector<ElementRecord> elements_;
    std::unordered_map<int, std::size_t> byAtomicNumber_;
    quantum::data::JsonDataProvider provider_;
};

} // namespace quantum::physics
