#pragma once

#include "gdrpg/types.hpp"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace gdrpg {

/// A single flat or multiplicative modifier applied to a stat.
struct StatModifier {
    StatType stat = StatType::Attack;
    ModifierOp op = ModifierOp::Flat;
    float amount = 0.f;
    float remaining = -1.f; // < 0 means permanent / until removed
    std::string source;     // ability id, item id, status id, etc.
    EntityInstanceId source_instance = kInvalidEntityInstanceId;

    bool is_expired() const { return !is_permanent() && remaining <= 0.f; }
    bool is_permanent() const { return remaining < 0.f; }
};

/// Base stats plus layered modifiers (flat then multiplicative).
class StatBlock {
public:
    StatBlock() = default;
    explicit StatBlock(std::array<float, static_cast<size_t>(StatType::COUNT)> base);

    float base(StatType s) const;
    void set_base(StatType s, float value);

    void add_modifier(StatModifier mod);
    void remove_modifiers_from(const std::string& source);
    void clear_temporary_modifiers();

    /// Tick timed modifiers; returns number removed.
    int tick(float dt);

    /// Effective value: (base + sum flat) * product(1 + mult)
    float get(StatType s) const;

    const std::vector<StatModifier>& modifiers() const { return modifiers_; }

private:
    std::array<float, static_cast<size_t>(StatType::COUNT)> base_{};
    std::vector<StatModifier> modifiers_;
};

} // namespace gdrpg
