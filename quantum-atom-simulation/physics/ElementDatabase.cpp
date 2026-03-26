#include "quantum/physics/ElementDatabase.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>

namespace quantum::physics {

bool ElementDatabase::loadFromJson(const std::filesystem::path& path) {
    std::ifstream stream(path);
    if (!stream.is_open()) {
        return false;
    }

    const auto json = nlohmann::json::parse(stream, nullptr, true, true);
    elements_.clear();
    for (const auto& item : json.at("elements")) {
        elements_.push_back(ElementRecord{
            .atomicNumber = item.at("atomic_number").get<int>(),
            .symbol = item.at("symbol").get<std::string>(),
            .name = item.at("name").get<std::string>(),
            .atomicMassU = item.at("atomic_mass_u").get<double>()});
    }

    rebuildIndex();
    return !elements_.empty();
}

void ElementDatabase::loadBuiltInSubset() {
    elements_ = {
        {1, "H", "Hydrogen", 1.00784},
        {2, "He", "Helium", 4.002602},
        {3, "Li", "Lithium", 6.94},
        {4, "Be", "Beryllium", 9.0121831},
        {5, "B", "Boron", 10.81},
        {6, "C", "Carbon", 12.011},
        {7, "N", "Nitrogen", 14.007},
        {8, "O", "Oxygen", 15.999},
        {9, "F", "Fluorine", 18.998403163},
        {10, "Ne", "Neon", 20.1797},
        {11, "Na", "Sodium", 22.98976928},
        {12, "Mg", "Magnesium", 24.305},
        {13, "Al", "Aluminium", 26.9815385},
        {14, "Si", "Silicon", 28.085},
        {15, "P", "Phosphorus", 30.973761998},
        {16, "S", "Sulfur", 32.06},
        {17, "Cl", "Chlorine", 35.45},
        {18, "Ar", "Argon", 39.948}};
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

void ElementDatabase::rebuildIndex() {
    byAtomicNumber_.clear();
    for (std::size_t index = 0; index < elements_.size(); ++index) {
        byAtomicNumber_[elements_[index].atomicNumber] = index;
    }
}

} // namespace quantum::physics
