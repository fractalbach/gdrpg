#pragma once

#include "gdrpg/stats.hpp"
#include "gdrpg/types.hpp"

#include <memory>
#include <string>
#include <vector>

namespace gdrpg {

class CombatContext;
class Creature;
class Effect;

enum class StackRule : std::uint8_t {
    RefreshDuration = 0, // same status: refresh timer, keep stacks
    AddStacks,           // increase stacks up to max, refresh duration
    Independent,         // allow multiple instances
    IgnoreIfPresent,     // do nothing if already present
};

/// Immutable status effect template from JSON.
struct StatusEffectData {
    StatusId id;
    std::string name;
    std::string description;
    float default_duration = 5.f;
    float tick_interval = 1.f;
    int max_stacks = 1;
    StackRule stack_rule = StackRule::RefreshDuration;
    bool is_stun = false;
    bool is_silence = false;
    bool is_buff = false;
    float speed_multiplier = 1.f; // e.g. chill = 0.7
    std::vector<StatModifier> stat_mods;
    std::vector<std::shared_ptr<Effect>> on_tick_effects;
    std::vector<std::shared_ptr<Effect>> on_apply_effects;
};

/// Runtime instance of a status on a creature.
struct StatusInstance {
    StatusId status_id;
    float remaining = 0.f;
    float tick_timer = 0.f;
    int stacks = 1;
    CreatureId source_id = kInvalidCreatureId;
    EntityInstanceId instance_id = kInvalidEntityInstanceId;

    bool is_expired() const { return remaining <= 0.f; }
};

} // namespace gdrpg
