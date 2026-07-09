#pragma once

#include <cstdint>
#include <string>

namespace gdrpg {

using CreatureId = std::uint32_t;
using AbilityId = std::string;
using ItemId = std::string;
using StatusId = std::string;
using PersistentId = std::string;
using EntityInstanceId = std::uint64_t;

constexpr CreatureId kInvalidCreatureId = 0;
constexpr EntityInstanceId kInvalidEntityInstanceId = 0;

enum class Team : std::uint8_t {
    Player = 0,
    Enemy = 1,
    Neutral = 2,
};

enum class TargetType : std::uint8_t {
    Self = 0,
    SingleAlly,
    SingleEnemy,
    AllAllies,
    AllEnemies,
    All,
    RandomEnemy,
    RandomAlly,
};

enum class DamageType : std::uint8_t {
    Physical = 0,
    Fire,
    Ice,
    Lightning,
    Poison,
    Arcane,
    True, // ignores mitigation
};

enum class StatType : std::uint8_t {
    MaxHP = 0,
    MaxMana,
    Attack,
    Defense,
    SpellPower,
    Speed,
    CritChance,
    CritMultiplier,
    COUNT
};

enum class EquipmentSlot : std::uint8_t {
    Weapon = 0,
    Offhand,
    Head,
    Chest,
    Legs,
    Boots,
    Accessory1,
    Accessory2,
    COUNT
};

enum class ModifierOp : std::uint8_t {
    Flat = 0,          // value += amount
    Multiplicative = 1 // value *= (1 + amount)
};

inline const char* to_string(Team t) {
    switch (t) {
        case Team::Player: return "Player";
        case Team::Enemy: return "Enemy";
        case Team::Neutral: return "Neutral";
    }
    return "Unknown";
}

inline const char* to_string(DamageType t) {
    switch (t) {
        case DamageType::Physical: return "Physical";
        case DamageType::Fire: return "Fire";
        case DamageType::Ice: return "Ice";
        case DamageType::Lightning: return "Lightning";
        case DamageType::Poison: return "Poison";
        case DamageType::Arcane: return "Arcane";
        case DamageType::True: return "True";
    }
    return "Unknown";
}

inline const char* to_string(StatType s) {
    switch (s) {
        case StatType::MaxHP: return "max_hp";
        case StatType::MaxMana: return "max_mana";
        case StatType::Attack: return "attack";
        case StatType::Defense: return "defense";
        case StatType::SpellPower: return "spell_power";
        case StatType::Speed: return "speed";
        case StatType::CritChance: return "crit_chance";
        case StatType::CritMultiplier: return "crit_multiplier";
        case StatType::COUNT: break;
    }
    return "unknown";
}

inline const char* to_string(EquipmentSlot s) {
    switch (s) {
        case EquipmentSlot::Weapon: return "weapon";
        case EquipmentSlot::Offhand: return "offhand";
        case EquipmentSlot::Head: return "head";
        case EquipmentSlot::Chest: return "chest";
        case EquipmentSlot::Legs: return "legs";
        case EquipmentSlot::Boots: return "boots";
        case EquipmentSlot::Accessory1: return "accessory1";
        case EquipmentSlot::Accessory2: return "accessory2";
        case EquipmentSlot::COUNT: break;
    }
    return "unknown";
}

} // namespace gdrpg
