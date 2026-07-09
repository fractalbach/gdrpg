#include "gdrpg_creature.hpp"

using namespace godot;

void GDRPG_Creature::set_internal(gdrpg::Creature* creature) {
    creature_ = creature;
}

int GDRPG_Creature::get_id() const {
    return creature_ ? static_cast<int>(creature_->id()) : 0;
}

String GDRPG_Creature::get_creature_name() const {
    return creature_ ? String(creature_->name().c_str()) : String();
}

float GDRPG_Creature::get_hp() const {
    return creature_ ? creature_->hp() : 0.f;
}

float GDRPG_Creature::get_max_hp() const {
    return creature_ ? creature_->max_hp() : 0.f;
}

float GDRPG_Creature::get_mana() const {
    return creature_ ? creature_->mana() : 0.f;
}

float GDRPG_Creature::get_max_mana() const {
    return creature_ ? creature_->max_mana() : 0.f;
}

bool GDRPG_Creature::is_alive() const {
    return creature_ && creature_->is_alive();
}

int GDRPG_Creature::get_team() const {
    return creature_ ? static_cast<int>(creature_->team()) : 0;
}

void GDRPG_Creature::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_id"), &GDRPG_Creature::get_id);
    ClassDB::bind_method(D_METHOD("get_creature_name"), &GDRPG_Creature::get_creature_name);
    ClassDB::bind_method(D_METHOD("get_hp"), &GDRPG_Creature::get_hp);
    ClassDB::bind_method(D_METHOD("get_max_hp"), &GDRPG_Creature::get_max_hp);
    ClassDB::bind_method(D_METHOD("get_mana"), &GDRPG_Creature::get_mana);
    ClassDB::bind_method(D_METHOD("get_max_mana"), &GDRPG_Creature::get_max_mana);
    ClassDB::bind_method(D_METHOD("is_alive"), &GDRPG_Creature::is_alive);
    ClassDB::bind_method(D_METHOD("get_team"), &GDRPG_Creature::get_team);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "id"), "", "get_id");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "creature_name"), "", "get_creature_name");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "hp"), "", "get_hp");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_hp"), "", "get_max_hp");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "mana"), "", "get_mana");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_mana"), "", "get_max_mana");
}
