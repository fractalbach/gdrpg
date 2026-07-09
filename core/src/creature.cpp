#include "gdrpg/creature.hpp"
#include "gdrpg/registry.hpp"

#include <algorithm>
#include <cmath>

namespace gdrpg {

Creature::Creature(CreatureId id, std::string name, Team team, StatBlock stats)
    : id_(id), name_(std::move(name)), team_(team), stats_(std::move(stats)) {
    hp_ = stats_.get(StatType::MaxHP);
    mana_ = stats_.get(StatType::MaxMana);
}

void Creature::set_hp(float v) {
    hp_ = std::clamp(v, 0.f, max_hp());
    if (hp_ <= 0.f) {
        alive_ = false;
    }
}

void Creature::set_mana(float v) {
    mana_ = std::clamp(v, 0.f, max_mana());
}

void Creature::heal(float amount) {
    if (!is_alive() || amount <= 0.f) {
        return;
    }
    set_hp(hp_ + amount);
}

float Creature::take_damage(float amount, DamageType type) {
    if (!is_alive() || amount <= 0.f) {
        return 0.f;
    }

    float mitigated = amount;
    if (type != DamageType::True) {
        const float defense = stats_.get(StatType::Defense);
        // Simple mitigation: damage * (100 / (100 + defense))
        mitigated = amount * (100.f / (100.f + std::max(0.f, defense)));
    }

    const float before = hp_;
    set_hp(hp_ - mitigated);
    return before - hp_; // actual HP lost (handles overkill)
}

void Creature::spend_mana(float amount) {
    set_mana(mana_ - amount);
}

bool Creature::can_afford(float mana_cost) const {
    return mana_ >= mana_cost;
}

void Creature::add_ability(const AbilityId& ability_id) {
    if (find_ability(ability_id)) {
        return;
    }
    AbilityInstance inst;
    inst.ability_id = ability_id;
    abilities_.push_back(std::move(inst));
}

AbilityInstance* Creature::find_ability(const AbilityId& ability_id) {
    for (auto& a : abilities_) {
        if (a.ability_id == ability_id) {
            return &a;
        }
    }
    return nullptr;
}

const AbilityInstance* Creature::find_ability(const AbilityId& ability_id) const {
    for (const auto& a : abilities_) {
        if (a.ability_id == ability_id) {
            return &a;
        }
    }
    return nullptr;
}

void Creature::equip(EquipmentSlot slot, ItemInstance item, const ItemData& data) {
    // Remove previous item mods if any
    if (equipment_[static_cast<size_t>(slot)].has_value()) {
        stats_.remove_modifiers_from("item:" + equipment_[static_cast<size_t>(slot)]->item_id);
    }

    equipment_[static_cast<size_t>(slot)] = item;

    for (auto mod : data.stat_mods) {
        mod.source = "item:" + data.id;
        mod.remaining = -1.f;
        stats_.add_modifier(mod);
    }

    for (const auto& ab : data.granted_abilities) {
        add_ability(ab);
    }

    // Clamp resources to new caps
    set_hp(std::min(hp_, max_hp()));
    set_mana(std::min(mana_, max_mana()));
}

std::optional<ItemInstance> Creature::unequip(EquipmentSlot slot, const ItemData* data) {
    auto& slot_ref = equipment_[static_cast<size_t>(slot)];
    if (!slot_ref) {
        return std::nullopt;
    }
    ItemInstance removed = *slot_ref;
    slot_ref.reset();

    if (data) {
        stats_.remove_modifiers_from("item:" + data->id);
        // Note: granted abilities remain known (learned); cooldown state kept.
    }

    set_hp(std::min(hp_, max_hp()));
    set_mana(std::min(mana_, max_mana()));
    return removed;
}

void Creature::refresh_crowd_control_flags(const Registry& registry) {
    stunned_ = false;
    silenced_ = false;
    for (const auto& st : statuses_) {
        if (st.is_expired()) {
            continue;
        }
        const auto* data = registry.status(st.status_id);
        if (!data) {
            continue;
        }
        if (data->is_stun) {
            stunned_ = true;
        }
        if (data->is_silence) {
            silenced_ = true;
        }
    }
}

void Creature::tick_cooldowns(float dt) {
    // Speed affects effective cooldown recovery slightly
    const float speed = stats_.get(StatType::Speed);
    const float rate = 1.f + (speed - 100.f) * 0.001f; // mild scaling around 100 baseline
    const float scaled = dt * std::max(0.1f, rate);
    for (auto& a : abilities_) {
        a.tick(scaled);
    }
}

void Creature::tick_lifetime(float dt) {
    if (lifetime_ >= 0.f) {
        lifetime_ -= dt;
        if (lifetime_ <= 0.f) {
            alive_ = false;
            hp_ = 0.f;
        }
    }
}

} // namespace gdrpg
