#include "gdrpg_battle.hpp"

#include <cstdio>

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

GDRPG_Battle::GDRPG_Battle() = default;

bool GDRPG_Battle::load_data(const String& directory) {
    registry_.clear();
    std::string error;
    const std::string path = directory.utf8().get_data();
    if (!gdrpg::DataLoader::load_directory(registry_, path, &error)) {
        UtilityFunctions::printerr(String("gdrpg load_data failed: ") + String(error.c_str()));
        data_loaded_ = false;
        return false;
    }
    data_loaded_ = true;
    return true;
}

void GDRPG_Battle::start(int seed) {
    if (!data_loaded_) {
        UtilityFunctions::printerr("GDRPG_Battle: call load_data() before start()");
        return;
    }
    battle_ = std::make_unique<gdrpg::Battle>(registry_, static_cast<std::uint64_t>(seed));
}

int GDRPG_Battle::add_creature(const String& template_id, int team) {
    if (!battle_) {
        return 0;
    }
    const auto tid = std::string(template_id.utf8().get_data());
    return static_cast<int>(
        battle_->add_creature(tid, static_cast<gdrpg::Team>(team)));
}

bool GDRPG_Battle::use_ability(int caster_id, const String& ability_id, int target_id) {
    if (!battle_) {
        return false;
    }
    const auto aid = std::string(ability_id.utf8().get_data());
    auto result = battle_->use_ability(static_cast<gdrpg::CreatureId>(caster_id), aid,
                                       static_cast<gdrpg::CreatureId>(target_id));
    return result.success;
}

void GDRPG_Battle::tick(float dt) {
    if (battle_) {
        battle_->tick(dt);
    }
}

void GDRPG_Battle::run_ai(int team) {
    if (battle_) {
        battle_->run_simple_ai(static_cast<gdrpg::Team>(team));
    }
}

float GDRPG_Battle::get_time() const {
    return battle_ ? battle_->time() : 0.f;
}

int GDRPG_Battle::get_result() const {
    return battle_ ? static_cast<int>(battle_->result()) : 0;
}

bool GDRPG_Battle::is_finished() const {
    return battle_ && battle_->is_finished();
}

Ref<GDRPG_Creature> GDRPG_Battle::get_creature(int id) const {
    Ref<GDRPG_Creature> ref;
    if (!battle_) {
        return ref;
    }
    gdrpg::Creature* c = battle_->get_creature(static_cast<gdrpg::CreatureId>(id));
    if (!c) {
        return ref;
    }
    ref.instantiate();
    ref->set_internal(c);
    return ref;
}

PackedStringArray GDRPG_Battle::get_log_lines() const {
    PackedStringArray lines;
    if (!battle_) {
        return lines;
    }
    for (const auto& e : battle_->log().entries()) {
        if (e.level < gdrpg::LogLevel::Combat) {
            continue;
        }
        char buf[32];
        std::snprintf(buf, sizeof(buf), "[%.2f] ", e.time);
        lines.push_back(String(buf) + String(e.message.c_str()));
    }
    return lines;
}

String GDRPG_Battle::get_log_dump() const {
    if (!battle_) {
        return String();
    }
    return String(battle_->log().dump(gdrpg::LogLevel::Combat).c_str());
}

int GDRPG_Battle::get_creature_count() const {
    return battle_ ? static_cast<int>(battle_->creatures_storage().size()) : 0;
}

void GDRPG_Battle::_bind_methods() {
    ClassDB::bind_method(D_METHOD("load_data", "directory"), &GDRPG_Battle::load_data);
    ClassDB::bind_method(D_METHOD("start", "seed"), &GDRPG_Battle::start, DEFVAL(42));
    ClassDB::bind_method(D_METHOD("add_creature", "template_id", "team"),
                         &GDRPG_Battle::add_creature);
    ClassDB::bind_method(D_METHOD("use_ability", "caster_id", "ability_id", "target_id"),
                         &GDRPG_Battle::use_ability, DEFVAL(0));
    ClassDB::bind_method(D_METHOD("tick", "dt"), &GDRPG_Battle::tick);
    ClassDB::bind_method(D_METHOD("run_ai", "team"), &GDRPG_Battle::run_ai);
    ClassDB::bind_method(D_METHOD("get_time"), &GDRPG_Battle::get_time);
    ClassDB::bind_method(D_METHOD("get_result"), &GDRPG_Battle::get_result);
    ClassDB::bind_method(D_METHOD("is_finished"), &GDRPG_Battle::is_finished);
    ClassDB::bind_method(D_METHOD("get_creature", "id"), &GDRPG_Battle::get_creature);
    ClassDB::bind_method(D_METHOD("get_log_lines"), &GDRPG_Battle::get_log_lines);
    ClassDB::bind_method(D_METHOD("get_log_dump"), &GDRPG_Battle::get_log_dump);
    ClassDB::bind_method(D_METHOD("get_creature_count"), &GDRPG_Battle::get_creature_count);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "time"), "", "get_time");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "result"), "", "get_result");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "finished"), "", "is_finished");
}
