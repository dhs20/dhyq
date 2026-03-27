#include "quantum/core/Paths.h"

namespace quantum::core {

Paths::Paths(std::filesystem::path projectRoot)
    : projectRoot_(std::move(projectRoot)) {}

const std::filesystem::path& Paths::projectRoot() const {
    return projectRoot_;
}

std::filesystem::path Paths::asset(const std::string& relative) const {
    return projectRoot_ / "assets" / relative;
}

std::filesystem::path Paths::docs(const std::string& relative) const {
    return projectRoot_ / "docs" / relative;
}

std::filesystem::path Paths::report(const std::string& relative) const {
    return projectRoot_ / "docs" / "reports" / relative;
}

} // namespace quantum::core
