#pragma once

#include "gdrpg/ability.hpp"
#include "gdrpg/creature.hpp"
#include "gdrpg/item.hpp"
#include "gdrpg/persistent_entity.hpp"
#include "gdrpg/status_effect.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace gdrpg {

/// Central factory/registry for all data-driven templates.
class Registry {
public:
    void clear();

    void register_ability(AbilityData data);
    void register_creature(CreatureTemplate data);
    void register_item(ItemData data);
    void register_status(StatusEffectData data);
    void register_persistent(PersistentEntityData data);

    const AbilityData* ability(const AbilityId& id) const;
    const CreatureTemplate* creature(const std::string& id) const;
    const ItemData* item(const ItemId& id) const;
    const StatusEffectData* status(const StatusId& id) const;
    const PersistentEntityData* persistent(const PersistentId& id) const;

    const std::unordered_map<AbilityId, AbilityData>& abilities() const { return abilities_; }
    const std::unordered_map<std::string, CreatureTemplate>& creatures() const { return creatures_; }
    const std::unordered_map<ItemId, ItemData>& items() const { return items_; }
    const std::unordered_map<StatusId, StatusEffectData>& statuses() const { return statuses_; }
    const std::unordered_map<PersistentId, PersistentEntityData>& persistents() const {
        return persistents_;
    }

    /// Create a creature instance from a template (does not assign battle id).
    Creature make_creature(const std::string& template_id, CreatureId id, Team team) const;

private:
    std::unordered_map<AbilityId, AbilityData> abilities_;
    std::unordered_map<std::string, CreatureTemplate> creatures_;
    std::unordered_map<ItemId, ItemData> items_;
    std::unordered_map<StatusId, StatusEffectData> statuses_;
    std::unordered_map<PersistentId, PersistentEntityData> persistents_;
};

} // namespace gdrpg
