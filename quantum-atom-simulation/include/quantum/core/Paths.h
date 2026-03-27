#pragma once

#include <filesystem>
#include <string>

namespace quantum::core {

class Paths {
public:
    explicit Paths(std::filesystem::path projectRoot);

    [[nodiscard]] const std::filesystem::path& projectRoot() const;
    [[nodiscard]] std::filesystem::path asset(const std::string& relative) const;
    [[nodiscard]] std::filesystem::path docs(const std::string& relative) const;
    [[nodiscard]] std::filesystem::path report(const std::string& relative) const;

private:
    std::filesystem::path projectRoot_;
};

} // namespace quantum::core
