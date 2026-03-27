#include "quantum/data/JsonDataProvider.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <sstream>

namespace quantum::data {
namespace {

quantum::meta::SourceRecord defaultElementSource(const std::filesystem::path& path) {
    return {"elements-json",
            "Element metadata asset",
            "local-asset",
            "1",
            "",
            path.generic_string()};
}

quantum::meta::SourceRecord defaultReferenceSource(const std::filesystem::path& path) {
    return {"reference-lines-csv",
            "Reference spectral lines asset",
            "local-asset",
            "1",
            "",
            path.generic_string()};
}

quantum::meta::MethodStamp elementImportMethod() {
    quantum::meta::MethodStamp stamp;
    stamp.methodName = "Static element metadata import";
    stamp.approximation = "metadata import";
    stamp.dataSource = "JSON asset";
    stamp.validationSummary = "Schema-level import without external online validation.";
    stamp.tier = quantum::meta::ModelTier::Tier0Teaching;
    stamp.animationKind = quantum::meta::AnimationKind::None;
    stamp.caveats.push_back("Metadata coverage may exceed validated physics-model coverage.");
    return stamp;
}

quantum::meta::MethodStamp referenceImportMethod() {
    quantum::meta::MethodStamp stamp;
    stamp.methodName = "Reference transition import";
    stamp.approximation = "local reference mapping";
    stamp.dataSource = "CSV asset";
    stamp.validationSummary = "Imported reference lines are used for local comparisons only.";
    stamp.tier = quantum::meta::ModelTier::Tier0Teaching;
    stamp.animationKind = quantum::meta::AnimationKind::None;
    stamp.caveats.push_back("This is not a live NIST ASD integration.");
    return stamp;
}

quantum::meta::SourceRecord parseSourceRecord(const nlohmann::json& node,
                                              const quantum::meta::SourceRecord& fallback) {
    quantum::meta::SourceRecord source = fallback;
    if (!node.is_object()) {
        return source;
    }
    source.id = node.value("id", source.id);
    source.title = node.value("title", source.title);
    source.provider = node.value("provider", source.provider);
    source.revision = node.value("revision", source.revision);
    source.license = node.value("license", source.license);
    source.url = node.value("url", source.url);
    return source;
}

std::vector<int> parseSupportedChargeStates(const nlohmann::json& item, int atomicNumber) {
    std::vector<int> states;
    if (item.contains("supported_charge_states") && item.at("supported_charge_states").is_array()) {
        states = item.at("supported_charge_states").get<std::vector<int>>();
    } else {
        states.reserve(static_cast<std::size_t>(atomicNumber));
        for (int charge = 0; charge < atomicNumber; ++charge) {
            states.push_back(charge);
        }
    }
    return states;
}

std::pair<int, int> inferAtomicNumberAndChargeState(const std::string& seriesName) {
    if (seriesName.rfind("He+", 0) == 0) {
        return {2, 1};
    }
    if (seriesName.rfind("Li2+", 0) == 0) {
        return {3, 2};
    }
    return {1, 0};
}

std::string transitionRecordId(int atomicNumber,
                               int chargeState,
                               int ni,
                               int li,
                               int nf,
                               int lf) {
    return "reference-" + std::to_string(atomicNumber) + "-" + std::to_string(chargeState) + "-" +
           std::to_string(ni) + "-" + std::to_string(li) + "-" + std::to_string(nf) + "-" + std::to_string(lf);
}

} // namespace

bool JsonDataProvider::loadElements(const std::filesystem::path& path) {
    std::ifstream stream(path);
    if (!stream.is_open()) {
        return false;
    }

    const auto json = nlohmann::json::parse(stream, nullptr, true, true);
    const auto source = parseSourceRecord(json.value("source", nlohmann::json::object()), defaultElementSource(path));
    const auto importMethod = elementImportMethod();

    elements_.clear();
    for (const auto& item : json.at("elements")) {
        ElementRecord record;
        record.atomicNumber = item.at("atomic_number").get<int>();
        record.symbol = item.at("symbol").get<std::string>();
        record.name = item.at("name").get<std::string>();
        record.localizedNameZh = item.value("name_zh", std::string{});
        record.atomicMassU = item.at("atomic_mass_u").get<double>();
        record.supportedChargeStates = parseSupportedChargeStates(item, record.atomicNumber);
        record.method = importMethod;
        record.source = source;
        elements_.push_back(std::move(record));
    }

    rebuildElementIndex();
    return !elements_.empty();
}

bool JsonDataProvider::loadReferenceTransitions(const std::filesystem::path& path) {
    std::ifstream stream(path);
    if (!stream.is_open()) {
        return false;
    }

    const auto source = defaultReferenceSource(path);
    const auto method = referenceImportMethod();

    referenceTransitions_.clear();
    std::string line;
    std::getline(stream, line);
    while (std::getline(stream, line)) {
        if (line.empty()) {
            continue;
        }
        std::stringstream row(line);
        std::string token;
        std::vector<std::string> tokens;
        while (std::getline(row, token, ',')) {
            tokens.push_back(token);
        }
        if (tokens.size() < 7) {
            continue;
        }

        const auto [atomicNumber, chargeState] = inferAtomicNumberAndChargeState(tokens[0]);
        TransitionRecord record;
        record.label = tokens[0] + " " + tokens[1] + "->" + tokens[3];
        record.upper.atomicNumber = atomicNumber;
        record.upper.chargeState = chargeState;
        record.upper.label = tokens[0] + " upper";
        record.upper.n = std::stoi(tokens[1]);
        record.upper.l = std::stoi(tokens[2]);
        record.upper.method = method;
        record.upper.provenance = {transitionRecordId(atomicNumber,
                                                      chargeState,
                                                      record.upper.n,
                                                      record.upper.l,
                                                      std::stoi(tokens[3]),
                                                      std::stoi(tokens[4])) + "-upper",
                                   source.id,
                                   "data::JsonDataProvider",
                                   "Imported from local reference CSV."};
        record.lower.atomicNumber = atomicNumber;
        record.lower.chargeState = chargeState;
        record.lower.label = tokens[0] + " lower";
        record.lower.n = std::stoi(tokens[3]);
        record.lower.l = std::stoi(tokens[4]);
        record.lower.method = method;
        record.lower.provenance = {transitionRecordId(atomicNumber,
                                                      chargeState,
                                                      record.upper.n,
                                                      record.upper.l,
                                                      record.lower.n,
                                                      record.lower.l) + "-lower",
                                   source.id,
                                   "data::JsonDataProvider",
                                   "Imported from local reference CSV."};
        record.wavelengthNm = std::stod(tokens[5]);
        record.photonEnergyEv = std::stod(tokens[6]);
        record.frequencyHz = (record.wavelengthNm > 0.0)
                                 ? (299792458.0 / (record.wavelengthNm * 1.0e-9))
                                 : 0.0;
        record.allowed = true;
        record.method = method;
        record.validation = {quantum::meta::ValidationStatus::ReferenceMatched,
                             tokens[0] + " import",
                             path.filename().string(),
                             "nm",
                             record.wavelengthNm,
                             record.wavelengthNm,
                             0.0,
                             0.0,
                             "Imported local reference line.",
                             source};
        record.provenance = {transitionRecordId(atomicNumber,
                                                chargeState,
                                                record.upper.n,
                                                record.upper.l,
                                                record.lower.n,
                                                record.lower.l),
                             source.id,
                             "data::JsonDataProvider",
                             "Imported from local reference CSV."};
        referenceTransitions_.push_back(std::move(record));
    }

    return !referenceTransitions_.empty();
}

void JsonDataProvider::loadBuiltInSubset() {
    const auto source = quantum::meta::SourceRecord{"elements-builtin",
                                                    "Built-in light-element subset",
                                                    "embedded",
                                                    "1",
                                                    "",
                                                    ""};
    const auto method = elementImportMethod();
    elements_ = {
        {1, "H", "Hydrogen", "氢", 1.00784, {0}, method, source},
        {2, "He", "Helium", "氦", 4.002602, {0, 1}, method, source},
        {3, "Li", "Lithium", "锂", 6.94, {0, 1, 2}, method, source},
        {4, "Be", "Beryllium", "铍", 9.0121831, {0, 1, 2, 3}, method, source},
        {5, "B", "Boron", "硼", 10.81, {0, 1, 2, 3, 4}, method, source},
        {6, "C", "Carbon", "碳", 12.011, {0, 1, 2, 3, 4, 5}, method, source},
        {7, "N", "Nitrogen", "氮", 14.007, {0, 1, 2, 3, 4, 5, 6}, method, source},
        {8, "O", "Oxygen", "氧", 15.999, {0, 1, 2, 3, 4, 5, 6, 7}, method, source},
        {9, "F", "Fluorine", "氟", 18.998403163, {0, 1, 2, 3, 4, 5, 6, 7, 8}, method, source},
        {10, "Ne", "Neon", "氖", 20.1797, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, method, source},
        {11, "Na", "Sodium", "钠", 22.98976928, {0}, method, source},
        {12, "Mg", "Magnesium", "镁", 24.305, {0}, method, source},
        {13, "Al", "Aluminium", "铝", 26.9815385, {0}, method, source},
        {14, "Si", "Silicon", "硅", 28.085, {0}, method, source},
        {15, "P", "Phosphorus", "磷", 30.973761998, {0}, method, source},
        {16, "S", "Sulfur", "硫", 32.06, {0}, method, source},
        {17, "Cl", "Chlorine", "氯", 35.45, {0}, method, source},
        {18, "Ar", "Argon", "氩", 39.948, {0}, method, source}};
    rebuildElementIndex();
}

std::vector<ElementRecord> JsonDataProvider::elements() const {
    return elements_;
}

std::optional<ElementRecord> JsonDataProvider::elementByAtomicNumber(int atomicNumber) const {
    const auto it = byAtomicNumber_.find(atomicNumber);
    if (it == byAtomicNumber_.end()) {
        return std::nullopt;
    }
    return elements_.at(it->second);
}

std::vector<TransitionRecord> JsonDataProvider::referenceTransitions(int atomicNumber) const {
    std::vector<TransitionRecord> result;
    for (const auto& record : referenceTransitions_) {
        if (record.upper.atomicNumber == atomicNumber) {
            result.push_back(record);
        }
    }
    return result;
}

void JsonDataProvider::rebuildElementIndex() {
    byAtomicNumber_.clear();
    for (std::size_t index = 0; index < elements_.size(); ++index) {
        byAtomicNumber_[elements_[index].atomicNumber] = index;
    }
}

} // namespace quantum::data
