#include <catch2/catch_test_macros.hpp>
#include <gdrpg/battle.hpp>
#include <gdrpg/effect.hpp>
#include <gdrpg/registry.hpp>

using namespace gdrpg;

static CreatureTemplate make_tmpl(const std::string& id, float hp) {
    CreatureTemplate t;
    t.id = id;
    t.name = id;
    t.base_stats.set_base(StatType::MaxHP, hp);
    t.base_stats.set_base(StatType::MaxMana, 100.f);
    t.base_stats.set_base(StatType::Attack, 20.f);
    t.base_stats.set_base(StatType::Defense, 0.f);
    t.base_stats.set_base(StatType::SpellPower, 20.f);
    t.base_stats.set_base(StatType::Speed, 100.f);
    t.base_stats.set_base(StatType::CritChance, 0.f);
    t.base_stats.set_base(StatType::CritMultiplier, 1.5f);
    return t;
}

TEST_CASE("Heal effect restores HP", "[effects]") {
    Registry reg;
    reg.register_creature(make_tmpl("a", 100.f));
    reg.register_creature(make_tmpl("b", 100.f));
    Battle battle(reg, 1);
    auto a = battle.add_creature("a", Team::Player);
    battle.get_creature(a)->take_damage(50.f, DamageType::True);

    auto ctx = battle.make_context();
    HealEffect heal;
    heal.base_amount = 30.f;
    heal.power_coefficient = 0.f;
    heal.apply(ctx, *battle.get_creature(a), battle.get_creature(a));
    REQUIRE(battle.get_creature(a)->hp() == 80.f);
}

TEST_CASE("Status stacking add_stacks", "[effects]") {
    Registry reg;
    StatusEffectData poison;
    poison.id = "poison";
    poison.name = "Poison";
    poison.default_duration = 5.f;
    poison.max_stacks = 5;
    poison.stack_rule = StackRule::AddStacks;
    reg.register_status(poison);
    reg.register_creature(make_tmpl("a", 100.f));
    reg.register_creature(make_tmpl("b", 100.f));

    Battle battle(reg, 1);
    auto a = battle.add_creature("a", Team::Player);
    auto b = battle.add_creature("b", Team::Enemy);
    auto ctx = battle.make_context();
    ctx.apply_status(*battle.get_creature(a), *battle.get_creature(b), "poison", 5.f, 1);
    ctx.apply_status(*battle.get_creature(a), *battle.get_creature(b), "poison", 5.f, 2);
    REQUIRE(battle.get_creature(b)->statuses().size() == 1);
    REQUIRE(battle.get_creature(b)->statuses().front().stacks == 3);
}

TEST_CASE("Summon minion creates temporary creature", "[effects]") {
    Registry reg;
    reg.register_creature(make_tmpl("summoner", 100.f));
    auto imp = make_tmpl("imp", 40.f);
    reg.register_creature(imp);

    Battle battle(reg, 1);
    auto s = battle.add_creature("summoner", Team::Player);
    auto ctx = battle.make_context();
    Creature* summoned =
        ctx.summon_creature("imp", Team::Player, *battle.get_creature(s), 10.f);

    REQUIRE(summoned != nullptr);
    REQUIRE(battle.creatures_storage().size() == 2);
    REQUIRE(summoned->name() == "imp");
    REQUIRE(summoned->lifetime() == 10.f);
    REQUIRE(summoned->summoner_id() == s);
}

TEST_CASE("Death mid-chain skips remaining damage", "[effects]") {
    Registry reg;
    AbilityData nuke;
    nuke.id = "nuke";
    nuke.name = "Nuke";
    nuke.mana_cost = 0.f;
    nuke.targeting = TargetType::SingleEnemy;
    for (int i = 0; i < 3; ++i) {
        auto d = std::make_shared<DamageEffect>();
        d->base_amount = 100.f;
        d->power_coefficient = 0.f;
        d->can_crit = false;
        d->damage_type = DamageType::True;
        nuke.effects.push_back(d);
    }
    reg.register_ability(nuke);

    auto hero = make_tmpl("hero", 200.f);
    hero.ability_ids = {"nuke"};
    reg.register_creature(hero);
    reg.register_creature(make_tmpl("rat", 50.f));

    Battle battle(reg, 1);
    auto h = battle.add_creature("hero", Team::Player);
    auto r = battle.add_creature("rat", Team::Enemy);
    auto result = battle.use_ability(h, "nuke", r);
    REQUIRE(result.success);
    REQUIRE_FALSE(battle.get_creature(r)->is_alive());
    // Should not go negative
    REQUIRE(battle.get_creature(r)->hp() == 0.f);
}
