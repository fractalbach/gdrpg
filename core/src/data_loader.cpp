#include "gdrpg/data_loader.hpp"
#include "gdrpg/effect.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

namespace gdrpg {

using json = nlohmann::json;

namespace {

StatType parse_stat(const std::string& s) {
    if (s == "max_hp") return StatType::MaxHP;
    if (s == "max_mana") return StatType::MaxMana;
    if (s == "attack") return StatType::Attack;
    if (s == "defense") return StatType::Defense;
    if (s == "spell_power") return StatType::SpellPower;
    if (s == "speed") return StatType::Speed;
    if (s == "crit_chance") return StatType::CritChance;
    if (s == "crit_multiplier") return StatType::CritMultiplier;
    return StatType::Attack;
}

DamageType parse_damage_type(const std::string& s) {
    if (s == "physical") return DamageType::Physical;
    if (s == "fire") return DamageType::Fire;
    if (s == "ice") return DamageType::Ice;
    if (s == "lightning") return DamageType::Lightning;
    if (s == "poison") return DamageType::Poison;
    if (s == "arcane") return DamageType::Arcane;
    if (s == "true") return DamageType::True;
    return DamageType::Physical;
}

TargetType parse_target(const std::string& s) {
    if (s == "self") return TargetType::Self;
    if (s == "single_ally") return TargetType::SingleAlly;
    if (s == "single_enemy") return TargetType::SingleEnemy;
    if (s == "all_allies") return TargetType::AllAllies;
    if (s == "all_enemies") return TargetType::AllEnemies;
    if (s == "all") return TargetType::All;
    if (s == "random_enemy") return TargetType::RandomEnemy;
    if (s == "random_ally") return TargetType::RandomAlly;
    return TargetType::SingleEnemy;
}

EquipmentSlot parse_slot(const std::string& s) {
    if (s == "weapon") return EquipmentSlot::Weapon;
    if (s == "offhand") return EquipmentSlot::Offhand;
    if (s == "head") return EquipmentSlot::Head;
    if (s == "chest") return EquipmentSlot::Chest;
    if (s == "legs") return EquipmentSlot::Legs;
    if (s == "boots") return EquipmentSlot::Boots;
    if (s == "accessory1") return EquipmentSlot::Accessory1;
    if (s == "accessory2") return EquipmentSlot::Accessory2;
    return EquipmentSlot::Weapon;
}

ModifierOp parse_op(const std::string& s) {
    if (s == "mult" || s == "multiplicative") return ModifierOp::Multiplicative;
    return ModifierOp::Flat;
}

StackRule parse_stack_rule(const std::string& s) {
    if (s == "add_stacks") return StackRule::AddStacks;
    if (s == "independent") return StackRule::Independent;
    if (s == "ignore_if_present") return StackRule::IgnoreIfPresent;
    return StackRule::RefreshDuration;
}

Team parse_team(const std::string& s) {
    if (s == "player") return Team::Player;
    if (s == "enemy") return Team::Enemy;
    return Team::Neutral;
}

StatModifier parse_stat_mod(const json& j) {
    StatModifier m;
    m.stat = parse_stat(j.value("stat", "attack"));
    m.op = parse_op(j.value("op", "flat"));
    m.amount = j.value("amount", 0.f);
    m.remaining = j.value("duration", -1.f);
    m.source = j.value("source", "");
    return m;
}

std::shared_ptr<Effect> parse_effect(const json& j) {
    const std::string type = j.value("type", "");
    if (type == "damage") {
        auto e = std::make_shared<DamageEffect>();
        e->base_amount = j.value("base", 0.f);
        e->power_coefficient = j.value("coeff", 1.f);
        e->use_spell_power = j.value("spell", false);
        e->damage_type = parse_damage_type(j.value("damage_type", "physical"));
        e->can_crit = j.value("can_crit", true);
        return e;
    }
    if (type == "heal") {
        auto e = std::make_shared<HealEffect>();
        e->base_amount = j.value("base", 0.f);
        e->power_coefficient = j.value("coeff", 0.5f);
        e->use_spell_power = j.value("spell", true);
        return e;
    }
    if (type == "apply_status") {
        auto e = std::make_shared<ApplyStatusEffect>();
        e->status_id = j.value("status", "");
        e->duration_override = j.value("duration", -1.f);
        e->stacks = j.value("stacks", 1);
        return e;
    }
    if (type == "spawn_persistent") {
        auto e = std::make_shared<SpawnPersistentEffect>();
        e->template_id = j.value("persistent", "");
        e->count = j.value("count", 1);
        e->delay_between = j.value("delay_between", 0.f);
        return e;
    }
    if (type == "stat_mod") {
        auto e = std::make_shared<StatModEffect>();
        e->stat = parse_stat(j.value("stat", "attack"));
        e->op = parse_op(j.value("op", "flat"));
        e->amount = j.value("amount", 0.f);
        e->duration = j.value("duration", 5.f);
        e->source_tag = j.value("source", "effect");
        return e;
    }
    if (type == "summon") {
        auto e = std::make_shared<SummonMinionEffect>();
        e->creature_template_id = j.value("creature", "");
        e->lifetime = j.value("lifetime", 30.f);
        e->count = j.value("count", 1);
        return e;
    }
    if (type == "dispel") {
        auto e = std::make_shared<DispelEffect>();
        e->dispel_buffs = j.value("buffs", false);
        e->dispel_debuffs = j.value("debuffs", true);
        e->max_count = j.value("max", 1);
        return e;
    }
    return nullptr;
}

std::vector<std::shared_ptr<Effect>> parse_effects(const json& j) {
    std::vector<std::shared_ptr<Effect>> out;
    if (!j.is_array()) {
        return out;
    }
    for (const auto& item : j) {
        if (auto e = parse_effect(item)) {
            out.push_back(std::move(e));
        }
    }
    return out;
}

bool read_json_file(const std::filesystem::path& path, json& out, std::string* error_out) {
    std::ifstream in(path);
    if (!in) {
        if (error_out) {
            *error_out = "Failed to open " + path.string();
        }
        return false;
    }
    try {
        in >> out;
    } catch (const std::exception& ex) {
        if (error_out) {
            *error_out = std::string("JSON parse error in ") + path.string() + ": " + ex.what();
        }
        return false;
    }
    return true;
}

} // namespace

bool DataLoader::load_abilities(Registry& registry, const std::filesystem::path& path,
                                std::string* error_out) {
    json root;
    if (!read_json_file(path, root, error_out)) {
        return false;
    }
    if (!root.contains("abilities") || !root["abilities"].is_array()) {
        if (error_out) {
            *error_out = "abilities.json missing 'abilities' array";
        }
        return false;
    }
    for (const auto& j : root["abilities"]) {
        AbilityData a;
        a.id = j.value("id", "");
        a.name = j.value("name", a.id);
        a.description = j.value("description", "");
        a.cooldown = j.value("cooldown", 0.f);
        a.mana_cost = j.value("mana_cost", 0.f);
        a.cast_time = j.value("cast_time", 0.f);
        a.targeting = parse_target(j.value("targeting", "single_enemy"));
        a.primary_damage_type = parse_damage_type(j.value("damage_type", "physical"));
        a.power_coefficient = j.value("power_coefficient", 1.f);
        a.uses_spell_power = j.value("uses_spell_power", false);
        if (j.contains("effects")) {
            a.effects = parse_effects(j["effects"]);
        }
        if (!a.id.empty()) {
            registry.register_ability(std::move(a));
        }
    }
    return true;
}

bool DataLoader::load_creatures(Registry& registry, const std::filesystem::path& path,
                                std::string* error_out) {
    json root;
    if (!read_json_file(path, root, error_out)) {
        return false;
    }
    if (!root.contains("creatures") || !root["creatures"].is_array()) {
        if (error_out) {
            *error_out = "creatures.json missing 'creatures' array";
        }
        return false;
    }
    for (const auto& j : root["creatures"]) {
        CreatureTemplate c;
        c.id = j.value("id", "");
        c.name = j.value("name", c.id);
        c.default_team = parse_team(j.value("team", "neutral"));

        StatBlock stats;
        const auto& s = j.value("stats", json::object());
        stats.set_base(StatType::MaxHP, s.value("max_hp", 100.f));
        stats.set_base(StatType::MaxMana, s.value("max_mana", 50.f));
        stats.set_base(StatType::Attack, s.value("attack", 10.f));
        stats.set_base(StatType::Defense, s.value("defense", 5.f));
        stats.set_base(StatType::SpellPower, s.value("spell_power", 10.f));
        stats.set_base(StatType::Speed, s.value("speed", 100.f));
        stats.set_base(StatType::CritChance, s.value("crit_chance", 0.05f));
        stats.set_base(StatType::CritMultiplier, s.value("crit_multiplier", 1.5f));
        c.base_stats = stats;

        if (j.contains("abilities") && j["abilities"].is_array()) {
            for (const auto& ab : j["abilities"]) {
                c.ability_ids.push_back(ab.get<std::string>());
            }
        }
        if (j.contains("equipment") && j["equipment"].is_array()) {
            for (const auto& it : j["equipment"]) {
                c.starting_equipment.push_back(it.get<std::string>());
            }
        }
        if (!c.id.empty()) {
            registry.register_creature(std::move(c));
        }
    }
    return true;
}

bool DataLoader::load_items(Registry& registry, const std::filesystem::path& path,
                            std::string* error_out) {
    json root;
    if (!read_json_file(path, root, error_out)) {
        return false;
    }
    if (!root.contains("items") || !root["items"].is_array()) {
        if (error_out) {
            *error_out = "items.json missing 'items' array";
        }
        return false;
    }
    for (const auto& j : root["items"]) {
        ItemData item;
        item.id = j.value("id", "");
        item.name = j.value("name", item.id);
        item.description = j.value("description", "");
        item.slot = parse_slot(j.value("slot", "weapon"));
        if (j.contains("stat_mods") && j["stat_mods"].is_array()) {
            for (const auto& m : j["stat_mods"]) {
                item.stat_mods.push_back(parse_stat_mod(m));
            }
        }
        if (j.contains("granted_abilities") && j["granted_abilities"].is_array()) {
            for (const auto& ab : j["granted_abilities"]) {
                item.granted_abilities.push_back(ab.get<std::string>());
            }
        }
        if (!item.id.empty()) {
            registry.register_item(std::move(item));
        }
    }
    return true;
}

bool DataLoader::load_status_effects(Registry& registry, const std::filesystem::path& path,
                                     std::string* error_out) {
    json root;
    if (!read_json_file(path, root, error_out)) {
        return false;
    }
    if (!root.contains("status_effects") || !root["status_effects"].is_array()) {
        if (error_out) {
            *error_out = "status_effects.json missing 'status_effects' array";
        }
        return false;
    }
    for (const auto& j : root["status_effects"]) {
        StatusEffectData st;
        st.id = j.value("id", "");
        st.name = j.value("name", st.id);
        st.description = j.value("description", "");
        st.default_duration = j.value("duration", 5.f);
        st.tick_interval = j.value("tick_interval", 1.f);
        st.max_stacks = j.value("max_stacks", 1);
        st.stack_rule = parse_stack_rule(j.value("stack_rule", "refresh"));
        st.is_stun = j.value("stun", false);
        st.is_silence = j.value("silence", false);
        st.is_buff = j.value("buff", false);
        st.speed_multiplier = j.value("speed_multiplier", 1.f);
        if (j.contains("stat_mods") && j["stat_mods"].is_array()) {
            for (const auto& m : j["stat_mods"]) {
                st.stat_mods.push_back(parse_stat_mod(m));
            }
        }
        if (j.contains("on_tick")) {
            st.on_tick_effects = parse_effects(j["on_tick"]);
        }
        if (j.contains("on_apply")) {
            st.on_apply_effects = parse_effects(j["on_apply"]);
        }
        if (!st.id.empty()) {
            registry.register_status(std::move(st));
        }
    }
    return true;
}

bool DataLoader::load_persistent_entities(Registry& registry, const std::filesystem::path& path,
                                          std::string* error_out) {
    json root;
    if (!read_json_file(path, root, error_out)) {
        return false;
    }
    if (!root.contains("persistent_entities") || !root["persistent_entities"].is_array()) {
        if (error_out) {
            *error_out = "persistent_entities.json missing 'persistent_entities' array";
        }
        return false;
    }
    for (const auto& j : root["persistent_entities"]) {
        PersistentEntityData p;
        p.id = j.value("id", "");
        p.name = j.value("name", p.id);
        p.lifetime = j.value("lifetime", 5.f);
        p.tick_interval = j.value("tick_interval", 1.f);
        p.initial_delay = j.value("initial_delay", 0.f);
        p.retarget = parse_target(j.value("retarget", "single_enemy"));
        p.follow_owner = j.value("follow_owner", false);
        p.destroy_on_owner_death = j.value("destroy_on_owner_death", true);
        if (j.contains("on_tick")) {
            p.on_tick_effects = parse_effects(j["on_tick"]);
        }
        if (j.contains("on_expire")) {
            p.on_expire_effects = parse_effects(j["on_expire"]);
        }
        if (!p.id.empty()) {
            registry.register_persistent(std::move(p));
        }
    }
    return true;
}

bool DataLoader::load_directory(Registry& registry, const std::filesystem::path& dir,
                                std::string* error_out) {
    auto try_load = [&](auto fn, const char* filename, bool required) -> bool {
        const auto path = dir / filename;
        if (!std::filesystem::exists(path)) {
            if (required) {
                if (error_out) {
                    *error_out = std::string("Missing required data file: ") + path.string();
                }
                return false;
            }
            return true;
        }
        return fn(registry, path, error_out);
    };

    // Statuses & persistents before abilities (abilities reference them by id; effects are
    // resolved at runtime via registry so order is mostly for clarity).
    if (!try_load(load_status_effects, "status_effects.json", true)) return false;
    if (!try_load(load_persistent_entities, "persistent_entities.json", true)) return false;
    if (!try_load(load_items, "items.json", true)) return false;
    if (!try_load(load_abilities, "abilities.json", true)) return false;
    if (!try_load(load_creatures, "creatures.json", true)) return false;
    return true;
}

} // namespace gdrpg
