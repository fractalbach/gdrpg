#pragma once

#include "gdrpg/stats.hpp"
#include "gdrpg/types.hpp"

#include <optional>
#include <string>
#include <vector>

namespace gdrpg {

/// Immutable item template from JSON.
struct ItemData {
    ItemId id;
    std::string name;
    std::string description;
    EquipmentSlot slot = EquipmentSlot::Weapon;
    std::vector<StatModifier> stat_mods;
    std::vector<AbilityId> granted_abilities;
};

/// Runtime item instance (could later hold durability, enchantments, etc.).
struct ItemInstance {
    ItemId item_id;
    EntityInstanceId instance_id = kInvalidEntityInstanceId;
};

} // namespace gdrpg
