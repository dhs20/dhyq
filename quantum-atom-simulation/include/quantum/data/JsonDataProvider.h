#pragma once

#include "quantum/data/DataProvider.h"

#include <filesystem>
#include <optional>
#include <unordered_map>
#include <vector>

namespace quantum::data {

class JsonDataProvider final : public DataProvider {
public:
    bool loadElements(const std::filesystem::path& path);
    bool loadReferenceTransitions(const std::filesystem::path& path);
    void loadBuiltInSubset();

    [[nodiscard]] std::vector<ElementRecord> elements() const override;
    [[nodiscard]] std::optional<ElementRecord> elementByAtomicNumber(int atomicNumber) const override;
    [[nodiscard]] std::vector<TransitionRecord> referenceTransitions(int atomicNumber) const override;

private:
    void rebuildElementIndex();

    std::vector<ElementRecord> elements_;
    std::unordered_map<int, std::size_t> byAtomicNumber_;
    std::vector<TransitionRecord> referenceTransitions_;
};

} // namespace quantum::data
