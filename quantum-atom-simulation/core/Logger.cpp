#include "quantum/core/Logger.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace quantum::core {
namespace {

std::string currentTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &time);
#else
    localtime_r(&time, &localTime);
#endif

    std::ostringstream stream;
    stream << std::put_time(&localTime, "%H:%M:%S");
    return stream.str();
}

const char* levelTag(LogLevel level) {
    switch (level) {
    case LogLevel::Info:
        return "INFO";
    case LogLevel::Warning:
        return "WARN";
    case LogLevel::Error:
        return "ERROR";
    default:
        return "LOG";
    }
}

} // namespace

void Logger::setOutputFile(const std::filesystem::path& path) {
    std::scoped_lock lock(mutex_);
    outputPath_ = path;
    if (outputPath_.empty()) {
        return;
    }

    std::filesystem::create_directories(outputPath_.parent_path());
    std::ofstream stream(outputPath_, std::ios::out | std::ios::trunc | std::ios::binary);
    if (stream.is_open()) {
        constexpr unsigned char kUtf8Bom[] = {0xEF, 0xBB, 0xBF};
        stream.write(reinterpret_cast<const char*>(kUtf8Bom), sizeof(kUtf8Bom));
    }
}

void Logger::info(const std::string& message) {
    push(LogLevel::Info, message);
}

void Logger::warn(const std::string& message) {
    push(LogLevel::Warning, message);
}

void Logger::error(const std::string& message) {
    push(LogLevel::Error, message);
}

std::vector<LogEntry> Logger::entries() const {
    std::scoped_lock lock(mutex_);
    return entries_;
}

void Logger::clear() {
    std::scoped_lock lock(mutex_);
    entries_.clear();
}

void Logger::push(LogLevel level, const std::string& message) {
    std::scoped_lock lock(mutex_);
    const std::string timestamp = currentTimestamp();
    entries_.push_back({level, timestamp, message});
    if (entries_.size() > 512) {
        entries_.erase(entries_.begin(), entries_.begin() + static_cast<std::ptrdiff_t>(entries_.size() - 512));
    }

    if (!outputPath_.empty()) {
        std::ofstream stream(outputPath_, std::ios::out | std::ios::app);
        if (stream.is_open()) {
            stream << '[' << timestamp << "][" << levelTag(level) << "] " << message << '\n';
        }
    }
}

} // namespace quantum::core
