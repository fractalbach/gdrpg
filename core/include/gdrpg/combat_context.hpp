#pragma once

#include "gdrpg/ability.hpp"
#include "gdrpg/creature.hpp"
#include "gdrpg/persistent_entity.hpp"
#include "gdrpg/types.hpp"

#include <functional>
#include <random>
#include <vector>

namespace gdrpg {

class Battle;
class Registry;
class CombatLog;

/// Context passed into ability activation and effect resolution.
/// Provides access to the battle, registries, RNG, and helpers for targeting.
class CombatContext {
public:
    CombatContext(Battle& battle, Registry& registry, CombatLog& log, std::mt19937& rng);

    Battle& battle() { return battle_; }
    const Battle& battle() const { return battle_; }
    Registry& registry() { return registry_; }
    const Registry& registry() const { return registry_; }
    CombatLog& log() { return log_; }
    std::mt19937& rng() { return rng_; }

    float time() const;
    EntityInstanceId next_instance_id();

    Creature* find_creature(CreatureId id);
    const Creature* find_creature(CreatureId id) const;

    std::vector<Creature*> living_enemies_of(const Creature& source);
    std::vector<Creature*> living_allies_of(const Creature& source, bool include_self = false);
    Creature* random_enemy(const Creature& source);
    Creature* random_ally(const Creature& source, bool include_self = false);

    std::vector<Creature*> resolve_targets(const Creature& source, TargetType targeting,
                                           Creature* primary);

    void apply_damage(Creature& source, Creature& target, float amount, DamageType type,
                      bool is_crit = false);
    void apply_heal(Creature& source, Creature& target, float amount);

    void apply_status(Creature& source, Creature& target, const StatusId& status_id,
                      float duration_override = -1.f, int stacks = 1);

    PersistentEntity& spawn_persistent(const PersistentId& template_id, Creature& owner,
                                       Creature* locked_target = nullptr);

    Creature* summon_creature(const std::string& template_id, Team team, Creature& summoner,
                              float lifetime);

private:
    Battle& battle_;
    Registry& registry_;
    CombatLog& log_;
    std::mt19937& rng_;
};

} // namespace gdrpg
