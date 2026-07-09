#include "gdrpg/stats.hpp"

#include <algorithm>
#include <cmath>

namespace gdrpg {

StatBlock::StatBlock(std::array<float, static_cast<size_t>(StatType::COUNT)> base) : base_(base) {}

float StatBlock::base(StatType s) const {
    return base_[static_cast<size_t>(s)];
}

void StatBlock::set_base(StatType s, float value) {
    base_[static_cast<size_t>(s)] = value;
}

void StatBlock::add_modifier(StatModifier mod) {
    modifiers_.push_back(std::move(mod));
}

void StatBlock::remove_modifiers_from(const std::string& source) {
    modifiers_.erase(std::remove_if(modifiers_.begin(), modifiers_.end(),
                                    [&](const StatModifier& m) { return m.source == source; }),
                     modifiers_.end());
}

void StatBlock::clear_temporary_modifiers() {
    modifiers_.erase(std::remove_if(modifiers_.begin(), modifiers_.end(),
                                    [](const StatModifier& m) { return !m.is_permanent(); }),
                     modifiers_.end());
}

int StatBlock::tick(float dt) {
    int removed = 0;
    for (auto& m : modifiers_) {
        if (!m.is_permanent()) {
            m.remaining -= dt;
            if (m.remaining < 0.f) {
                m.remaining = 0.f;
            }
        }
    }
    const auto before = modifiers_.size();
    modifiers_.erase(std::remove_if(modifiers_.begin(), modifiers_.end(),
                                    [](const StatModifier& m) { return m.is_expired(); }),
                     modifiers_.end());
    removed = static_cast<int>(before - modifiers_.size());
    return removed;
}

float StatBlock::get(StatType s) const {
    const size_t idx = static_cast<size_t>(s);
    float flat = base_[idx];
    float mult = 1.f;
    for (const auto& m : modifiers_) {
        if (m.stat != s) {
            continue;
        }
        if (m.op == ModifierOp::Flat) {
            flat += m.amount;
        } else {
            mult *= (1.f + m.amount);
        }
    }
    return flat * mult;
}

} // namespace gdrpg
