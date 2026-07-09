#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <gdrpg/creature.hpp>
#include <gdrpg/item.hpp>
#include <gdrpg/registry.hpp>

using namespace gdrpg;
using Catch::Approx;

static StatBlock make_basic_stats() {
    StatBlock s;
    s.set_base(StatType::MaxHP, 100.f);
    s.set_base(StatType::MaxMana, 50.f);
    s.set_base(StatType::Attack, 10.f);
    s.set_base(StatType::Defense, 10.f);
    s.set_base(StatType::SpellPower, 10.f);
    s.set_base(StatType::Speed, 100.f);
    s.set_base(StatType::CritChance, 0.f);
    s.set_base(StatType::CritMultiplier, 1.5f);
    return s;
}

TEST_CASE("Creature damage and overkill", "[creature]") {
    Creature c(1, "Test", Team::Player, make_basic_stats());
    REQUIRE(c.hp() == 100.f);
    float dealt = c.take_damage(1000.f, DamageType::True);
    REQUIRE(dealt == 100.f); // overkill clamped to remaining HP
    REQUIRE_FALSE(c.is_alive());
    REQUIRE(c.hp() == 0.f);
}

TEST_CASE("Creature defense mitigation", "[creature]") {
    Creature c(1, "Tank", Team::Player, make_basic_stats());
    // defense 10 => multiplier 100/110
    float dealt = c.take_damage(110.f, DamageType::Physical);
    REQUIRE(dealt == Approx(100.f).margin(0.01f));
}

TEST_CASE("Creature mana affordability", "[creature]") {
    Creature c(1, "Mage", Team::Player, make_basic_stats());
    REQUIRE(c.can_afford(50.f));
    REQUIRE_FALSE(c.can_afford(51.f));
    c.spend_mana(30.f);
    REQUIRE(c.mana() == 20.f);
}

TEST_CASE("Creature equip grants stats and abilities", "[creature]") {
    Creature c(1, "Hero", Team::Player, make_basic_stats());
    ItemData sword;
    sword.id = "iron_sword";
    sword.name = "Iron Sword";
    sword.slot = EquipmentSlot::Weapon;
    StatModifier atk;
    atk.stat = StatType::Attack;
    atk.op = ModifierOp::Flat;
    atk.amount = 12.f;
    sword.stat_mods.push_back(atk);
    sword.granted_abilities.push_back("slash");

    ItemInstance inst;
    inst.item_id = sword.id;
    c.equip(EquipmentSlot::Weapon, inst, sword);

    REQUIRE(c.stats().get(StatType::Attack) == 22.f);
    REQUIRE(c.find_ability("slash") != nullptr);

    c.unequip(EquipmentSlot::Weapon, &sword);
    REQUIRE(c.stats().get(StatType::Attack) == 10.f);
}
