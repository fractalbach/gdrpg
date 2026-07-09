#pragma once

#include "gdrpg/ability.hpp"
#include "gdrpg/item.hpp"
#include "gdrpg/stats.hpp"
#include "gdrpg/status_effect.hpp"
#include "gdrpg/types.hpp"
#include "gdrpg/vec3.hpp"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace gdrpg {

struct CreatureTemplate {
    std::string id;
    std::string name;
    StatBlock base_stats;
    std::vector<AbilityId> ability_ids;
    std::vector<ItemId> starting_equipment; // item ids to equip on spawn
    Team default_team = Team::Neutral;
};

class Creature {
public:
    Creature() = default;
    Creature(CreatureId id, std::string name, Team team, StatBlock stats);

    CreatureId id() const { return id_; }
    const std::string& name() const { return name_; }
    Team team() const { return team_; }

    float hp() const { return hp_; }
    float mana() const { return mana_; }
    float max_hp() const { return stats_.get(StatType::MaxHP); }
    float max_mana() const { return stats_.get(StatType::MaxMana); }

    bool is_alive() const { return alive_ && hp_ > 0.f; }
    bool is_stunned() const { return stunned_; }
    bool is_silenced() const { return silenced_; }

    StatBlock& stats() { return stats_; }
    const StatBlock& stats() const { return stats_; }

    std::vector<AbilityInstance>& abilities() { return abilities_; }
    const std::vector<AbilityInstance>& abilities() const { return abilities_; }

    std::vector<StatusInstance>& statuses() { return statuses_; }
    const std::vector<StatusInstance>& statuses() const { return statuses_; }

    std::array<std::optional<ItemInstance>, static_cast<size_t>(EquipmentSlot::COUNT)>& equipment() {
        return equipment_;
    }
    const std::array<std::optional<ItemInstance>, static_cast<size_t>(EquipmentSlot::COUNT)>&
    equipment() const {
        return equipment_;
    }

    std::optional<Vec3>& position() { return position_; }
    const std::optional<Vec3>& position() const { return position_; }

    /// Temporary summon lifetime; <0 means permanent participant.
    float& lifetime() { return lifetime_; }
    float lifetime() const { return lifetime_; }

    CreatureId summoner_id() const { return summoner_id_; }
    void set_summoner(CreatureId id) { summoner_id_ = id; }

    void set_hp(float v);
    void set_mana(float v);
    void heal(float amount);
    /// Returns actual damage dealt (after mitigation / overkill clamp).
    float take_damage(float amount, DamageType type);

    void spend_mana(float amount);
    bool can_afford(float mana_cost) const;

    void add_ability(const AbilityId& ability_id);
    AbilityInstance* find_ability(const AbilityId& ability_id);
    const AbilityInstance* find_ability(const AbilityId& ability_id) const;

    void equip(EquipmentSlot slot, ItemInstance item, const ItemData& data);
    std::optional<ItemInstance> unequip(EquipmentSlot slot, const ItemData* data);

    void refresh_crowd_control_flags(const class Registry& registry);

    void tick_cooldowns(float dt);
    void tick_lifetime(float dt);

private:
    CreatureId id_ = kInvalidCreatureId;
    std::string name_;
    Team team_ = Team::Neutral;
    StatBlock stats_;
    float hp_ = 0.f;
    float mana_ = 0.f;
    bool alive_ = true;
    bool stunned_ = false;
    bool silenced_ = false;
    float lifetime_ = -1.f;
    CreatureId summoner_id_ = kInvalidCreatureId;
    std::vector<AbilityInstance> abilities_;
    std::vector<StatusInstance> statuses_;
    std::array<std::optional<ItemInstance>, static_cast<size_t>(EquipmentSlot::COUNT)> equipment_{};
    std::optional<Vec3> position_;
};

} // namespace gdrpg
