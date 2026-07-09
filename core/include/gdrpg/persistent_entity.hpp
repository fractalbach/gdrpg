#pragma once

#include "gdrpg/types.hpp"
#include "gdrpg/vec3.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace gdrpg {

class CombatContext;
class Creature;
class Effect;

/// Template for persistent battlefield entities (MagmaBall, IceShard, BurningGround).
struct PersistentEntityData {
    PersistentId id;
    std::string name;
    float lifetime = 5.f;
    float tick_interval = 1.f;
    float initial_delay = 0.f;
    TargetType retarget = TargetType::SingleEnemy;
    bool follow_owner = false;
    bool destroy_on_owner_death = true;
    std::vector<std::shared_ptr<Effect>> on_tick_effects;
    std::vector<std::shared_ptr<Effect>> on_expire_effects;
};

/// Runtime persistent entity instance managed by Battle.
struct PersistentEntity {
    EntityInstanceId instance_id = kInvalidEntityInstanceId;
    PersistentId template_id;
    std::string name;
    CreatureId owner_id = kInvalidCreatureId;
    CreatureId locked_target_id = kInvalidCreatureId;
    float remaining = 0.f;
    float tick_timer = 0.f;
    float initial_delay = 0.f;
    bool pending_destroy = false;
    std::optional<Vec3> position;
    TargetType retarget = TargetType::SingleEnemy;
    bool destroy_on_owner_death = true;
    std::vector<std::shared_ptr<Effect>> on_tick_effects;
    std::vector<std::shared_ptr<Effect>> on_expire_effects;

    bool is_expired() const { return remaining <= 0.f || pending_destroy; }
};

} // namespace gdrpg
