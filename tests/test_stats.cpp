#include <catch2/catch_test_macros.hpp>
#include <gdrpg/stats.hpp>

using namespace gdrpg;

TEST_CASE("StatBlock base values", "[stats]") {
    StatBlock s;
    s.set_base(StatType::Attack, 20.f);
    s.set_base(StatType::Defense, 10.f);
    REQUIRE(s.get(StatType::Attack) == 20.f);
    REQUIRE(s.get(StatType::Defense) == 10.f);
}

TEST_CASE("StatBlock flat and multiplicative modifiers", "[stats]") {
    StatBlock s;
    s.set_base(StatType::Attack, 100.f);

    StatModifier flat;
    flat.stat = StatType::Attack;
    flat.op = ModifierOp::Flat;
    flat.amount = 20.f;
    flat.source = "buff";
    s.add_modifier(flat);

    StatModifier mult;
    mult.stat = StatType::Attack;
    mult.op = ModifierOp::Multiplicative;
    mult.amount = 0.5f; // * 1.5
    mult.source = "aura";
    s.add_modifier(mult);

    // (100 + 20) * 1.5 = 180
    REQUIRE(s.get(StatType::Attack) == 180.f);
}

TEST_CASE("StatBlock timed modifiers expire", "[stats]") {
    StatBlock s;
    s.set_base(StatType::Speed, 100.f);

    StatModifier m;
    m.stat = StatType::Speed;
    m.op = ModifierOp::Flat;
    m.amount = 50.f;
    m.remaining = 2.f;
    m.source = "haste";
    s.add_modifier(m);

    REQUIRE(s.get(StatType::Speed) == 150.f);
    s.tick(1.5f);
    REQUIRE(s.get(StatType::Speed) == 150.f);
    int removed = s.tick(1.0f);
    REQUIRE(removed == 1);
    REQUIRE(s.get(StatType::Speed) == 100.f);
}

TEST_CASE("StatBlock remove by source", "[stats]") {
    StatBlock s;
    s.set_base(StatType::MaxHP, 100.f);
    StatModifier m;
    m.stat = StatType::MaxHP;
    m.op = ModifierOp::Flat;
    m.amount = 40.f;
    m.source = "item:leather";
    s.add_modifier(m);
    REQUIRE(s.get(StatType::MaxHP) == 140.f);
    s.remove_modifiers_from("item:leather");
    REQUIRE(s.get(StatType::MaxHP) == 100.f);
}
