#include "quantum/validation/ValidationReportWriter.h"

#include <nlohmann/json.hpp>

#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace quantum::validation {
namespace {

const char* modelTierLabel(quantum::meta::ModelTier tier) {
    switch (tier) {
    case quantum::meta::ModelTier::Tier0Teaching:
        return "Tier 0 Teaching";
    case quantum::meta::ModelTier::Tier1CentralField:
        return "Tier 1 Central Field";
    case quantum::meta::ModelTier::Tier2MeanField:
        return "Tier 2 Mean Field";
    case quantum::meta::ModelTier::Tier3SpectroscopyCorrections:
        return "Tier 3 Spectroscopy Corrections";
    case quantum::meta::ModelTier::Tier4Correlation:
        return "Tier 4 Correlation";
    case quantum::meta::ModelTier::Tier5Dynamics:
        return "Tier 5 Dynamics";
    default:
        return "Unknown";
    }
}

const char* validationStatusLabel(quantum::meta::ValidationStatus status) {
    switch (status) {
    case quantum::meta::ValidationStatus::NotValidated:
        return "NotValidated";
    case quantum::meta::ValidationStatus::SanityChecked:
        return "SanityChecked";
    case quantum::meta::ValidationStatus::Benchmarked:
        return "Benchmarked";
    case quantum::meta::ValidationStatus::ReferenceMatched:
        return "ReferenceMatched";
    case quantum::meta::ValidationStatus::ExperimentalComparison:
        return "ExperimentalComparison";
    default:
        return "Unknown";
    }
}

std::string isoTimestampNow() {
    const auto now = std::chrono::system_clock::now();
    const auto nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &nowTime);
#else
    localtime_r(&nowTime, &localTime);
#endif
    std::ostringstream stream;
    stream << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return stream.str();
}

nlohmann::json toJson(const quantum::meta::SourceRecord& source) {
    return {{"id", source.id},
            {"title", source.title},
            {"provider", source.provider},
            {"revision", source.revision},
            {"license", source.license},
            {"url", source.url}};
}

nlohmann::json toJson(const quantum::meta::MethodStamp& stamp) {
    return {{"method_name", stamp.methodName},
            {"approximation", stamp.approximation},
            {"data_source", stamp.dataSource},
            {"validation_summary", stamp.validationSummary},
            {"tier", modelTierLabel(stamp.tier)},
            {"includes_electron_correlation", stamp.includesElectronCorrelation},
            {"enforces_antisymmetry", stamp.enforcesAntisymmetry},
            {"includes_relativity", stamp.includesRelativity},
            {"includes_external_field", stamp.includesExternalField},
            {"is_time_dependent", stamp.isTimeDependent},
            {"caveats", stamp.caveats}};
}

nlohmann::json toJson(const quantum::meta::ValidationRecord& record) {
    return {{"status", validationStatusLabel(record.status)},
            {"case_name", record.caseName},
            {"reference_name", record.referenceName},
            {"units", record.units},
            {"reference_value", record.referenceValue},
            {"measured_value", record.measuredValue},
            {"absolute_error", record.absoluteError},
            {"relative_error", record.relativeError},
            {"notes", record.notes},
            {"source", toJson(record.source)}};
}

void appendMethodSection(std::ostringstream& stream, const NamedMethodStamp& method) {
    stream << "### " << method.label << "\n\n";
    stream << "- method: " << method.stamp.methodName << "\n";
    stream << "- approximation: " << method.stamp.approximation << "\n";
    stream << "- tier: " << modelTierLabel(method.stamp.tier) << "\n";
    stream << "- data source: " << method.stamp.dataSource << "\n";
    stream << "- validation summary: " << method.stamp.validationSummary << "\n";
    stream << "- electron correlation: " << (method.stamp.includesElectronCorrelation ? "yes" : "no") << "\n";
    stream << "- antisymmetry: " << (method.stamp.enforcesAntisymmetry ? "yes" : "no") << "\n";
    stream << "- relativity: " << (method.stamp.includesRelativity ? "yes" : "no") << "\n";
    stream << "- external field: " << (method.stamp.includesExternalField ? "yes" : "no") << "\n";
    stream << "- time-dependent: " << (method.stamp.isTimeDependent ? "yes" : "no") << "\n";
    if (!method.stamp.caveats.empty()) {
        stream << "- caveats:\n";
        for (const auto& caveat : method.stamp.caveats) {
            stream << "  - " << caveat << "\n";
        }
    }
    stream << "\n";
}

} // namespace

ValidationReportOutput ValidationReportWriter::write(const ValidationReportInput& input,
                                                     const std::filesystem::path& markdownPath) const {
    ValidationReportOutput output;
    try {
        std::filesystem::create_directories(markdownPath.parent_path());

        const auto jsonPath = markdownPath.parent_path() / (markdownPath.stem().string() + ".json");
        const std::string timestamp = isoTimestampNow();

        std::ostringstream markdown;
        markdown << "# " << input.title << "\n\n";
        markdown << "- generated: " << timestamp << "\n";
        markdown << "- subject: " << input.subject << "\n";
        if (!input.outputStem.empty()) {
            markdown << "- report id: " << input.outputStem << "\n";
        }
        markdown << "\n## Summary\n\n" << input.summary << "\n\n";
        if (!input.approximationWarning.empty()) {
            markdown << "## Approximation Warning\n\n" << input.approximationWarning << "\n\n";
        }
        markdown << "## Methods\n\n";
        for (const auto& method : input.methods) {
            appendMethodSection(markdown, method);
        }
        markdown << "## Validation Records\n\n";
        markdown << "| Case | Status | Reference | Measured | Abs Error | Notes |\n";
        markdown << "| --- | --- | --- | --- | --- | --- |\n";
        for (const auto& record : input.records) {
            markdown << "| " << record.caseName << " | " << validationStatusLabel(record.status) << " | "
                     << record.referenceValue << ' ' << record.units << " | " << record.measuredValue << ' ' << record.units
                     << " | " << record.absoluteError << " | " << record.notes << " |\n";
        }

        nlohmann::json root;
        root["title"] = input.title;
        root["generated"] = timestamp;
        root["subject"] = input.subject;
        root["summary"] = input.summary;
        root["approximation_warning"] = input.approximationWarning;
        root["output_stem"] = input.outputStem;
        root["methods"] = nlohmann::json::array();
        for (const auto& method : input.methods) {
            root["methods"].push_back({{"label", method.label}, {"stamp", toJson(method.stamp)}});
        }
        root["records"] = nlohmann::json::array();
        for (const auto& record : input.records) {
            root["records"].push_back(toJson(record));
        }

        std::ofstream markdownStream(markdownPath, std::ios::binary | std::ios::trunc);
        std::ofstream jsonStream(jsonPath, std::ios::binary | std::ios::trunc);
        if (!markdownStream.is_open() || !jsonStream.is_open()) {
            output.message = "Unable to open validation report output file.";
            return output;
        }

        markdownStream << markdown.str();
        jsonStream << root.dump(2);

        output.ok = true;
        output.markdownPath = markdownPath;
        output.jsonPath = jsonPath;
        output.message = "Validation report written successfully.";
        return output;
    } catch (const std::exception& exception) {
        output.message = exception.what();
        return output;
    }
}

} // namespace quantum::validation
