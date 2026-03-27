#include "quantum/physics/ElementDatabase.h"

#include <stdexcept>

namespace quantum::physics {
namespace {

ElementRecord toPhysicsElement(const quantum::data::ElementRecord& record) {
    ElementRecord element;
    element.atomicNumber = record.atomicNumber;
    element.symbol = record.symbol;
    element.name = record.name;
    element.localizedNameZh = record.localizedNameZh;
    element.atomicMassU = record.atomicMassU;
    element.method = record.method;
    element.source = record.source;
    element.provenance = {"element-" + std::to_string(record.atomicNumber),
                          record.source.id,
                          "physics::ElementDatabase",
                          "Mapped from quantum::data::ElementRecord"};
    return element;
}

} // namespace

bool ElementDatabase::loadFromJson(const std::filesystem::path& path) {
    if (!provider_.loadElements(path)) {
        return false;
    }

    elements_.clear();
    for (const auto& record : provider_.elements()) {
        elements_.push_back(toPhysicsElement(record));
    }

    rebuildIndex();
    return !elements_.empty();
}

bool ElementDatabase::loadReferenceTransitions(const std::filesystem::path& path) {
    return provider_.loadReferenceTransitions(path);
}

void ElementDatabase::loadBuiltInSubset() {
    provider_.loadBuiltInSubset();
    elements_.clear();
    for (const auto& record : provider_.elements()) {
        elements_.push_back(toPhysicsElement(record));
    }
    rebuildIndex();
}

const ElementRecord& ElementDatabase::elementByAtomicNumber(int atomicNumber) const {
    const auto it = byAtomicNumber_.find(atomicNumber);
    if (it == byAtomicNumber_.end()) {
        throw std::out_of_range("Unknown atomic number");
    }
    return elements_.at(it->second);
}

const std::vector<ElementRecord>& ElementDatabase::elements() const {
    return elements_;
}

const quantum::data::ElementRecord* ElementDatabase::elementMetadataByAtomicNumber(int atomicNumber) const {
    const auto result = provider_.elementByAtomicNumber(atomicNumber);
    if (!result.has_value()) {
        return nullptr;
    }

    static thread_local quantum::data::ElementRecord cache;
    cache = *result;
    return &cache;
}

std::vector<quantum::data::TransitionRecord> ElementDatabase::referenceTransitions(int atomicNumber) const {
    return provider_.referenceTransitions(atomicNumber);
}

void ElementDatabase::rebuildIndex() {
    byAtomicNumber_.clear();
    for (std::size_t index = 0; index < elements_.size(); ++index) {
        byAtomicNumber_[elements_[index].atomicNumber] = index;
    }
}

} // namespace quantum::physics
