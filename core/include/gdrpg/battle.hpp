#pragma once

#include "gdrpg/combat_context.hpp"
#include "gdrpg/combat_log.hpp"
#include "gdrpg/creature.hpp"
#include "gdrpg/persistent_entity.hpp"
#include "gdrpg/registry.hpp"
#include "gdrpg/types.hpp"

#include <functional>
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace gdrpg {

enum class BattleResult : std::uint8_t {
    Ongoing = 0,
    PlayerVictory,
    EnemyVictory,
    Draw,
};

struct AbilityUseResult {
    bool success = false;
    std::string reason;
};

/// Main combat simulator: time, participants, persistent entities, win conditions.
class Battle {
public:
    explicit Battle(Registry& registry, std::uint64_t seed = 42);

    Registry& registry() { return registry_; }
    const Registry& registry() const { return registry_; }
    CombatLog& log() { return log_; }
    const CombatLog& log() const { return log_; }

    float time() const { return time_; }
    BattleResult result() const { return result_; }
    bool is_finished() const { return result_ != BattleResult::Ongoing; }

    EntityInstanceId next_instance_id() { return next_instance_id_++; }

    /// Add a creature created from a template. Returns assigned id.
    CreatureId add_creature(const std::string& template_id, Team team,
                            std::optional<Vec3> position = std::nullopt);

    /// Add a pre-built creature (takes ownership of identity via assigned id).
    CreatureId add_creature(Creature creature);

    Creature* get_creature(CreatureId id);
    const Creature* get_creature(CreatureId id) const;

    std::vector<Creature*>& creatures() { return creature_ptrs_; }
    const std::vector<Creature>& creatures_storage() const { return creatures_; }

    std::vector<PersistentEntity>& persistents() { return persistents_; }
    const std::vector<PersistentEntity>& persistents() const { return persistents_; }

    /// Advance simulation by dt seconds.
    void tick(float dt);

    /// Attempt to use an ability. primary_target may be null for AoE/self.
    AbilityUseResult use_ability(CreatureId caster_id, const AbilityId& ability_id,
                                 CreatureId primary_target_id = kInvalidCreatureId);

    /// Equip an item mid-combat (handles stat mods / granted abilities).
    bool equip_item(CreatureId creature_id, const ItemId& item_id);
    bool unequip_item(CreatureId creature_id, EquipmentSlot slot);

    /// Simple AI: each living enemy uses first ready affordable ability on a random player.
    void run_simple_ai(Team team);

    void check_win_conditions();

    CombatContext make_context();

private:
    void tick_creatures(float dt);
    void tick_statuses(float dt);
    void tick_persistents(float dt);
    void cleanup_dead_and_expired();
    void resolve_ability_effects(Creature& caster, const AbilityData& ability, Creature* primary);

    Registry& registry_;
    CombatLog log_;
    std::mt19937 rng_;
    float time_ = 0.f;
    BattleResult result_ = BattleResult::Ongoing;
    CreatureId next_creature_id_ = 1;
    EntityInstanceId next_instance_id_ = 1;

    std::vector<Creature> creatures_;
    std::vector<Creature*> creature_ptrs_; // mirrors creatures_ for context helpers
    std::vector<PersistentEntity> persistents_;

    void rebuild_ptrs();
};

inline const char* to_string(BattleResult r) {
    switch (r) {
        case BattleResult::Ongoing: return "Ongoing";
        case BattleResult::PlayerVictory: return "PlayerVictory";
        case BattleResult::EnemyVictory: return "EnemyVictory";
        case BattleResult::Draw: return "Draw";
    }
    return "Unknown";
}

} // namespace gdrpg
