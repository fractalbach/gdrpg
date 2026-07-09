#include <gdrpg/gdrpg.hpp>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>

namespace fs = std::filesystem;

namespace {

std::string find_data_dir(int argc, char** argv) {
    if (argc > 1) {
        return argv[1];
    }
#ifdef GDRPG_DATA_DIR
    if (fs::exists(GDRPG_DATA_DIR)) {
        return GDRPG_DATA_DIR;
    }
#endif
#ifdef GDRPG_DEFAULT_DATA_DIR
    if (fs::exists(GDRPG_DEFAULT_DATA_DIR)) {
        return GDRPG_DEFAULT_DATA_DIR;
    }
#endif
    // Walk up from cwd looking for data/
    fs::path p = fs::current_path();
    for (int i = 0; i < 6; ++i) {
        if (fs::exists(p / "data" / "creatures.json")) {
            return (p / "data").string();
        }
        if (!p.has_parent_path() || p.parent_path() == p) {
            break;
        }
        p = p.parent_path();
    }
    return "data";
}

void print_banner(const std::string& title) {
    std::cout << "\n============================================================\n";
    std::cout << "  " << title << "\n";
    std::cout << "============================================================\n";
}

void print_roster(const gdrpg::Battle& battle) {
    std::cout << "\n-- Roster --\n";
    for (const auto& c : battle.creatures_storage()) {
        std::cout << "  [" << c.id() << "] " << c.name() << " (" << gdrpg::to_string(c.team())
                  << ") HP " << static_cast<int>(c.hp()) << "/" << static_cast<int>(c.max_hp())
                  << " MP " << static_cast<int>(c.mana()) << "/" << static_cast<int>(c.max_mana())
                  << (c.is_alive() ? "" : " [DEAD]") << "\n";
    }
}

void run_scripted_fight(gdrpg::Registry& registry, const std::string& title,
                        const std::vector<std::pair<std::string, gdrpg::Team>>& roster,
                        const std::vector<std::tuple<float, gdrpg::CreatureId, std::string,
                                                     gdrpg::CreatureId>>& script,
                        float max_time, float dt, std::uint64_t seed) {
    print_banner(title);
    gdrpg::Battle battle(registry, seed);

    std::vector<gdrpg::CreatureId> ids;
    ids.reserve(roster.size());
    for (const auto& [tmpl, team] : roster) {
        ids.push_back(battle.add_creature(tmpl, team));
    }
    print_roster(battle);

    size_t script_idx = 0;
    float ai_timer = 0.f;
    const float ai_interval = 1.5f;

    while (!battle.is_finished() && battle.time() < max_time) {
        // Fire scripted player actions at scheduled times
        while (script_idx < script.size()) {
            const auto& [at, caster_slot, ability, target_slot] = script[script_idx];
            if (battle.time() + 0.0001f < at) {
                break;
            }
            const gdrpg::CreatureId caster =
                caster_slot < ids.size() ? ids[static_cast<size_t>(caster_slot)] : caster_slot;
            const gdrpg::CreatureId target =
                target_slot < ids.size() ? ids[static_cast<size_t>(target_slot)] : target_slot;
            auto result = battle.use_ability(caster, ability, target);
            if (!result.success) {
                battle.log().info("Scripted cast failed: " + result.reason);
            }
            ++script_idx;
        }

        ai_timer += dt;
        if (ai_timer >= ai_interval) {
            ai_timer = 0.f;
            battle.run_simple_ai(gdrpg::Team::Enemy);
            // Player side AI for summons / fillers when not scripted
            battle.run_simple_ai(gdrpg::Team::Player);
        }

        battle.tick(dt);
    }

    std::cout << "\n" << battle.log().dump(gdrpg::LogLevel::Combat);
    print_roster(battle);
    std::cout << "\nResult: " << gdrpg::to_string(battle.result()) << " at t=" << battle.time()
              << "s\n";
}

void fight_goblin_skirmish(gdrpg::Registry& registry) {
    // ids assigned in order: 0 warrior, 1 mage, 2 goblin, 3 goblin, 4 shaman
    // Script uses slot indices into the roster vector.
    using T = gdrpg::Team;
    std::vector<std::pair<std::string, T>> roster = {
        {"warrior", T::Player}, {"mage", T::Player},     {"goblin", T::Enemy},
        {"goblin", T::Enemy},   {"goblin_shaman", T::Enemy},
    };

    // tuple: time, caster_slot, ability, target_slot
    std::vector<std::tuple<float, gdrpg::CreatureId, std::string, gdrpg::CreatureId>> script = {
        {0.1f, 0, "slash", 2},
        {0.5f, 1, "fireball", 2},
        {2.0f, 0, "power_strike", 3},
        {2.5f, 1, "ice_blast", 4},
        {5.5f, 1, "flame_wave", 2},
        {6.0f, 0, "battle_shout", 0},
        {8.0f, 1, "fireball", 4},
    };

    run_scripted_fight(registry, "Fight 1: Goblin Skirmish (Fireball + Ice Blast chains)", roster,
                       script, 30.f, 0.1f, 42);
}

void fight_dragon_boss(gdrpg::Registry& registry) {
    using T = gdrpg::Team;
    std::vector<std::pair<std::string, T>> roster = {
        {"warrior", T::Player},
        {"mage", T::Player},
        {"summoner", T::Player},
        {"dragon", T::Enemy},
    };

    std::vector<std::tuple<float, gdrpg::CreatureId, std::string, gdrpg::CreatureId>> script = {
        {0.2f, 2, "summon_imp", 2},
        {0.5f, 1, "shield", 1},
        {1.0f, 0, "battle_shout", 0},
        {1.5f, 1, "fireball", 3},
        {3.0f, 0, "power_strike", 3},
        {4.0f, 1, "ice_blast", 3},
        {6.0f, 2, "arcane_bolt", 3},
        {7.0f, 1, "flame_wave", 3},
        {10.0f, 1, "heal", 0},
        {12.0f, 1, "fireball", 3},
        {15.0f, 2, "summon_imp", 2},
        {16.0f, 0, "slash", 3},
        {18.0f, 1, "ice_blast", 3},
    };

    run_scripted_fight(registry, "Fight 2: Ancient Dragon (summons + persistent chains)", roster,
                       script, 60.f, 0.1f, 99);
}

void fight_equipment_swap(gdrpg::Registry& registry) {
    print_banner("Fight 3: Mid-combat equipment swap");
    gdrpg::Battle battle(registry, 7);
    auto mage = battle.add_creature("mage", gdrpg::Team::Player);
    auto goblin = battle.add_creature("goblin", gdrpg::Team::Enemy);
    auto goblin2 = battle.add_creature("goblin", gdrpg::Team::Enemy);

    print_roster(battle);

    battle.use_ability(mage, "arcane_bolt", goblin);
    battle.tick(0.5f);

    battle.log().important("Mage swaps to Frost Wand mid-combat");
    battle.equip_item(mage, "frost_wand");

    battle.use_ability(mage, "ice_blast", goblin2);

    float t = 0.f;
    while (!battle.is_finished() && t < 12.f) {
        battle.run_simple_ai(gdrpg::Team::Enemy);
        battle.run_simple_ai(gdrpg::Team::Player);
        battle.tick(0.25f);
        t += 0.25f;
    }

    std::cout << "\n" << battle.log().dump(gdrpg::LogLevel::Combat);
    print_roster(battle);
    std::cout << "\nResult: " << gdrpg::to_string(battle.result()) << "\n";
}

} // namespace

int main(int argc, char** argv) {
    const std::string data_dir = find_data_dir(argc, argv);
    std::cout << "gdrpg demo — loading data from: " << data_dir << "\n";

    gdrpg::Registry registry;
    std::string error;
    if (!gdrpg::DataLoader::load_directory(registry, data_dir, &error)) {
        std::cerr << "Failed to load data: " << error << "\n";
        return 1;
    }

    std::cout << "Loaded " << registry.creatures().size() << " creatures, "
              << registry.abilities().size() << " abilities, " << registry.items().size()
              << " items, " << registry.statuses().size() << " statuses, "
              << registry.persistents().size() << " persistent templates.\n";

    fight_goblin_skirmish(registry);
    fight_dragon_boss(registry);
    fight_equipment_swap(registry);

    print_banner("All demo fights complete");
    return 0;
}
