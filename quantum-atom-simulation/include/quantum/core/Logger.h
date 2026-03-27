#pragma once

#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

namespace quantum::core {

enum class LogLevel {
    Info,
    Warning,
    Error
};

struct LogEntry {
    LogLevel level;
    std::string timestamp;
    std::string message;
};

class Logger {
public:
    void setOutputFile(const std::filesystem::path& path);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);

    [[nodiscard]] std::vector<LogEntry> entries() const;
    void clear();

private:
    void push(LogLevel level, const std::string& message);

    mutable std::mutex mutex_;
    std::vector<LogEntry> entries_;
    std::filesystem::path outputPath_;
};

} // namespace quantum::core
