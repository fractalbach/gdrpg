#pragma once

#include "gdrpg_creature.hpp"

#include <gdrpg/gdrpg.hpp>

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

#include <memory>
#include <string>

namespace godot {

class GDRPG_Battle : public RefCounted {
    GDCLASS(GDRPG_Battle, RefCounted)

public:
    GDRPG_Battle();
    ~GDRPG_Battle() override = default;

    /// Load JSON data from a directory (creatures.json, abilities.json, ...).
    bool load_data(const String& directory);

    /// Start a new battle with the given RNG seed.
    void start(int seed = 42);

    int add_creature(const String& template_id, int team);
    bool use_ability(int caster_id, const String& ability_id, int target_id = 0);
    void tick(float dt);
    void run_ai(int team);

    float get_time() const;
    int get_result() const;
    bool is_finished() const;

    Ref<GDRPG_Creature> get_creature(int id) const;
    PackedStringArray get_log_lines() const;
    String get_log_dump() const;

    int get_creature_count() const;

protected:
    static void _bind_methods();

private:
    gdrpg::Registry registry_;
    std::unique_ptr<gdrpg::Battle> battle_;
    bool data_loaded_ = false;
};

} // namespace godot
