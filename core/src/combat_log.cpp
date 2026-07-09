#include "gdrpg/combat_log.hpp"

#include <iomanip>
#include <sstream>

namespace gdrpg {

void CombatLog::log(LogLevel level, std::string message) {
    CombatLogEntry e;
    e.time = current_time_;
    e.level = level;
    e.message = std::move(message);
    entries_.push_back(std::move(e));
}

std::string CombatLog::dump(LogLevel min_level) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    for (const auto& e : entries_) {
        if (static_cast<int>(e.level) < static_cast<int>(min_level)) {
            continue;
        }
        oss << "[" << std::setw(7) << e.time << "] " << to_string(e.level) << " " << e.message
            << "\n";
    }
    return oss.str();
}

} // namespace gdrpg
