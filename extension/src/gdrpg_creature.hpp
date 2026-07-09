#pragma once

#include <gdrpg/gdrpg.hpp>

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

#include <memory>

namespace godot {

class GDRPG_Creature : public RefCounted {
    GDCLASS(GDRPG_Creature, RefCounted)

public:
    void set_internal(gdrpg::Creature* creature);
    gdrpg::Creature* get_internal() const { return creature_; }

    int get_id() const;
    String get_creature_name() const;
    float get_hp() const;
    float get_max_hp() const;
    float get_mana() const;
    float get_max_mana() const;
    bool is_alive() const;
    int get_team() const;

protected:
    static void _bind_methods();

private:
    gdrpg::Creature* creature_ = nullptr; // non-owning; owned by Battle
};

} // namespace godot
