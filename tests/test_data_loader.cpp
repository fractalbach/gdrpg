#include <catch2/catch_test_macros.hpp>
#include <gdrpg/data_loader.hpp>
#include <gdrpg/registry.hpp>

#include <filesystem>

using namespace gdrpg;

static std::filesystem::path data_dir() {
#ifdef GDRPG_DATA_DIR
    return GDRPG_DATA_DIR;
#else
    return "data";
#endif
}

TEST_CASE("DataLoader loads all JSON files", "[data]") {
    Registry reg;
    std::string error;
    const auto dir = data_dir();
    REQUIRE(std::filesystem::exists(dir / "creatures.json"));
    REQUIRE(DataLoader::load_directory(reg, dir, &error));
    REQUIRE(error.empty());

    REQUIRE(reg.creature("warrior") != nullptr);
    REQUIRE(reg.creature("mage") != nullptr);
    REQUIRE(reg.creature("goblin") != nullptr);
    REQUIRE(reg.creature("dragon") != nullptr);

    REQUIRE(reg.ability("fireball") != nullptr);
    REQUIRE(reg.ability("ice_blast") != nullptr);
    REQUIRE(reg.ability("fireball")->effects.size() >= 3);

    REQUIRE(reg.status("burn") != nullptr);
    REQUIRE(reg.status("chill") != nullptr);
    REQUIRE(reg.status("stun") != nullptr);

    REQUIRE(reg.persistent("magma_ball") != nullptr);
    REQUIRE(reg.persistent("ice_shard") != nullptr);

    REQUIRE(reg.item("flame_staff") != nullptr);
    REQUIRE_FALSE(reg.item("flame_staff")->granted_abilities.empty());
}

TEST_CASE("Registry make_creature equips starting gear", "[data]") {
    Registry reg;
    REQUIRE(DataLoader::load_directory(reg, data_dir()));
    Creature c = reg.make_creature("warrior", 1, Team::Player);
    REQUIRE(c.name() == "Warrior");
    REQUIRE(c.find_ability("slash") != nullptr);
    // Iron sword should boost attack above base 28
    REQUIRE(c.stats().get(StatType::Attack) > 28.f);
}
