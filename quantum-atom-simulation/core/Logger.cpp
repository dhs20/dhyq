#include "quantum/core/Logger.h"

namespace quantum::core {

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
    entries_.push_back({level, message});
    if (entries_.size() > 512) {
        entries_.erase(entries_.begin(), entries_.begin() + static_cast<std::ptrdiff_t>(entries_.size() - 512));
    }
}

} // namespace quantum::core
