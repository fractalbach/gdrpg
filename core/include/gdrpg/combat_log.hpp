#pragma once

#include "gdrpg/types.hpp"

#include <chrono>
#include <sstream>
#include <string>
#include <vector>

namespace gdrpg {

enum class LogLevel : std::uint8_t {
    Debug = 0,
    Info,
    Combat,
    Important,
};

struct CombatLogEntry {
    float time = 0.f;
    LogLevel level = LogLevel::Info;
    std::string message;
};

class CombatLog {
public:
    void set_time(float t) { current_time_ = t; }
    float time() const { return current_time_; }

    void clear() { entries_.clear(); }
    void log(LogLevel level, std::string message);

    void debug(std::string msg) { log(LogLevel::Debug, std::move(msg)); }
    void info(std::string msg) { log(LogLevel::Info, std::move(msg)); }
    void combat(std::string msg) { log(LogLevel::Combat, std::move(msg)); }
    void important(std::string msg) { log(LogLevel::Important, std::move(msg)); }

    const std::vector<CombatLogEntry>& entries() const { return entries_; }

    /// Format all entries at or above min_level as a multi-line string.
    std::string dump(LogLevel min_level = LogLevel::Info) const;

    template <typename... Args>
    void combat_fmt(Args&&... args) {
        std::ostringstream oss;
        (oss << ... << std::forward<Args>(args));
        combat(oss.str());
    }

private:
    float current_time_ = 0.f;
    std::vector<CombatLogEntry> entries_;
};

inline const char* to_string(LogLevel l) {
    switch (l) {
        case LogLevel::Debug: return "DBG";
        case LogLevel::Info: return "INF";
        case LogLevel::Combat: return "CBT";
        case LogLevel::Important: return "!!!";
    }
    return "???";
}

} // namespace gdrpg
