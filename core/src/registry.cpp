#include "gdrpg/registry.hpp"

namespace gdrpg {

void Registry::clear() {
    abilities_.clear();
    creatures_.clear();
    items_.clear();
    statuses_.clear();
    persistents_.clear();
}

void Registry::register_ability(AbilityData data) {
    abilities_[data.id] = std::move(data);
}

void Registry::register_creature(CreatureTemplate data) {
    creatures_[data.id] = std::move(data);
}

void Registry::register_item(ItemData data) {
    items_[data.id] = std::move(data);
}

void Registry::register_status(StatusEffectData data) {
    statuses_[data.id] = std::move(data);
}

void Registry::register_persistent(PersistentEntityData data) {
    persistents_[data.id] = std::move(data);
}

const AbilityData* Registry::ability(const AbilityId& id) const {
    auto it = abilities_.find(id);
    return it == abilities_.end() ? nullptr : &it->second;
}

const CreatureTemplate* Registry::creature(const std::string& id) const {
    auto it = creatures_.find(id);
    return it == creatures_.end() ? nullptr : &it->second;
}

const ItemData* Registry::item(const ItemId& id) const {
    auto it = items_.find(id);
    return it == items_.end() ? nullptr : &it->second;
}

const StatusEffectData* Registry::status(const StatusId& id) const {
    auto it = statuses_.find(id);
    return it == statuses_.end() ? nullptr : &it->second;
}

const PersistentEntityData* Registry::persistent(const PersistentId& id) const {
    auto it = persistents_.find(id);
    return it == persistents_.end() ? nullptr : &it->second;
}

Creature Registry::make_creature(const std::string& template_id, CreatureId id, Team team) const {
    const auto* tmpl = creature(template_id);
    if (!tmpl) {
        StatBlock empty;
        empty.set_base(StatType::MaxHP, 1.f);
        return Creature(id, "Missing:" + template_id, team, empty);
    }

    Creature c(id, tmpl->name, team, tmpl->base_stats);
    for (const auto& ab : tmpl->ability_ids) {
        c.add_ability(ab);
    }

    // Equip starting gear
    for (const auto& item_id : tmpl->starting_equipment) {
        const auto* item = this->item(item_id);
        if (!item) {
            continue;
        }
        ItemInstance inst;
        inst.item_id = item_id;
        inst.instance_id = 0;
        c.equip(item->slot, inst, *item);
    }

    // Start at full resources after equipment mods apply
    c.set_hp(c.max_hp());
    c.set_mana(c.max_mana());

    return c;
}

} // namespace gdrpg
