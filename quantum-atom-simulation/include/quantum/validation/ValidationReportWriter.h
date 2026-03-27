#pragma once

#include "quantum/meta/MethodMetadata.h"

#include <filesystem>
#include <string>
#include <vector>

namespace quantum::validation {

struct NamedMethodStamp {
    std::string label;
    quantum::meta::MethodStamp stamp;
};

struct ValidationReportInput {
    std::string title;
    std::string subject;
    std::string summary;
    std::string approximationWarning;
    std::string outputStem;
    std::vector<NamedMethodStamp> methods;
    std::vector<quantum::meta::ValidationRecord> records;
};

struct ValidationReportOutput {
    bool ok = false;
    std::filesystem::path markdownPath;
    std::filesystem::path jsonPath;
    std::string message;
};

class ValidationReportWriter {
public:
    [[nodiscard]] ValidationReportOutput write(const ValidationReportInput& input,
                                               const std::filesystem::path& markdownPath) const;
};

} // namespace quantum::validation
