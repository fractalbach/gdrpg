#pragma once

#include "gdrpg/types.hpp"

#include <memory>
#include <string>
#include <vector>

namespace gdrpg {

class CombatContext;
class Creature;
class Effect;

/// Immutable ability template loaded from JSON.
struct AbilityData {
    AbilityId id;
    std::string name;
    std::string description;
    float cooldown = 0.f;
    float mana_cost = 0.f;
    float cast_time = 0.f; // 0 = instant
    TargetType targeting = TargetType::SingleEnemy;
    DamageType primary_damage_type = DamageType::Physical;
    float power_coefficient = 1.f; // scales with attack or spell_power
    bool uses_spell_power = false;

    /// Ordered list of effects applied on successful cast.
    std::vector<std::shared_ptr<Effect>> effects;
};

/// Runtime cooldown / cast state for one ability on a creature.
struct AbilityInstance {
    AbilityId ability_id;
    float cooldown_remaining = 0.f;
    int charges = 1;
    int max_charges = 1;

    bool is_ready() const { return cooldown_remaining <= 0.f && charges > 0; }

    void tick(float dt) {
        if (cooldown_remaining > 0.f) {
            cooldown_remaining -= dt;
            if (cooldown_remaining < 0.f) {
                cooldown_remaining = 0.f;
            }
        }
    }

    void start_cooldown(float cd) {
        cooldown_remaining = cd;
        if (max_charges > 1 && charges > 0) {
            --charges;
        }
    }
};

} // namespace gdrpg
