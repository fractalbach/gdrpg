#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <gdrpg/battle.hpp>
#include <gdrpg/data_loader.hpp>
#include <gdrpg/effect.hpp>
#include <gdrpg/registry.hpp>

using namespace gdrpg;
using Catch::Approx;

static void register_minimal(Registry& reg) {
    StatusEffectData burn;
    burn.id = "burn";
    burn.name = "Burn";
    burn.default_duration = 3.f;
    burn.tick_interval = 1.f;
    burn.max_stacks = 3;
    burn.stack_rule = StackRule::AddStacks;
    auto dot = std::make_shared<DamageEffect>();
    dot->base_amount = 10.f;
    dot->power_coefficient = 0.f;
    dot->damage_type = DamageType::Fire;
    dot->can_crit = false;
    burn.on_tick_effects.push_back(dot);
    reg.register_status(burn);

    StatusEffectData stun;
    stun.id = "stun";
    stun.name = "Stun";
    stun.default_duration = 2.f;
    stun.is_stun = true;
    reg.register_status(stun);

    PersistentEntityData magma;
    magma.id = "magma_ball";
    magma.name = "Magma Ball";
    magma.lifetime = 3.f;
    magma.tick_interval = 1.f;
    magma.initial_delay = 0.5f;
    magma.retarget = TargetType::SingleEnemy;
    auto tick_dmg = std::make_shared<DamageEffect>();
    tick_dmg->base_amount = 5.f;
    tick_dmg->power_coefficient = 0.f;
    tick_dmg->damage_type = DamageType::Fire;
    tick_dmg->can_crit = false;
    magma.on_tick_effects.push_back(tick_dmg);
    reg.register_persistent(magma);

    AbilityData fireball;
    fireball.id = "fireball";
    fireball.name = "Fireball";
    fireball.cooldown = 5.f;
    fireball.mana_cost = 20.f;
    fireball.targeting = TargetType::SingleEnemy;
    auto dmg = std::make_shared<DamageEffect>();
    dmg->base_amount = 30.f;
    dmg->power_coefficient = 0.f;
    dmg->damage_type = DamageType::Fire;
    dmg->can_crit = false;
    fireball.effects.push_back(dmg);
    auto apply = std::make_shared<ApplyStatusEffect>();
    apply->status_id = "burn";
    fireball.effects.push_back(apply);
    auto spawn = std::make_shared<SpawnPersistentEffect>();
    spawn->template_id = "magma_ball";
    spawn->count = 2;
    fireball.effects.push_back(spawn);
    reg.register_ability(fireball);

    AbilityData slash;
    slash.id = "slash";
    slash.name = "Slash";
    slash.cooldown = 1.f;
    slash.mana_cost = 0.f;
    slash.targeting = TargetType::SingleEnemy;
    auto sd = std::make_shared<DamageEffect>();
    sd->base_amount = 15.f;
    sd->power_coefficient = 0.f;
    sd->can_crit = false;
    slash.effects.push_back(sd);
    reg.register_ability(slash);

    CreatureTemplate hero;
    hero.id = "hero";
    hero.name = "Hero";
    hero.base_stats.set_base(StatType::MaxHP, 200.f);
    hero.base_stats.set_base(StatType::MaxMana, 100.f);
    hero.base_stats.set_base(StatType::Attack, 20.f);
    hero.base_stats.set_base(StatType::Defense, 5.f);
    hero.base_stats.set_base(StatType::SpellPower, 20.f);
    hero.base_stats.set_base(StatType::Speed, 100.f);
    hero.base_stats.set_base(StatType::CritChance, 0.f);
    hero.base_stats.set_base(StatType::CritMultiplier, 1.5f);
    hero.ability_ids = {"fireball", "slash"};
    reg.register_creature(hero);

    CreatureTemplate goblin;
    goblin.id = "goblin";
    goblin.name = "Goblin";
    goblin.base_stats.set_base(StatType::MaxHP, 80.f);
    goblin.base_stats.set_base(StatType::MaxMana, 10.f);
    goblin.base_stats.set_base(StatType::Attack, 10.f);
    goblin.base_stats.set_base(StatType::Defense, 0.f);
    goblin.base_stats.set_base(StatType::SpellPower, 0.f);
    goblin.base_stats.set_base(StatType::Speed, 100.f);
    goblin.base_stats.set_base(StatType::CritChance, 0.f);
    goblin.base_stats.set_base(StatType::CritMultiplier, 1.5f);
    goblin.ability_ids = {"slash"};
    reg.register_creature(goblin);
}

TEST_CASE("Battle fireball spawns persistents and applies burn", "[battle]") {
    Registry reg;
    register_minimal(reg);
    Battle battle(reg, 1);
    auto hero = battle.add_creature("hero", Team::Player);
    auto goblin = battle.add_creature("goblin", Team::Enemy);

    auto result = battle.use_ability(hero, "fireball", goblin);
    REQUIRE(result.success);
    REQUIRE(battle.persistents().size() == 2);

    Creature* g = battle.get_creature(goblin);
    REQUIRE(g != nullptr);
    REQUIRE(g->hp() < 80.f);
    REQUIRE_FALSE(g->statuses().empty());
    REQUIRE(g->statuses().front().status_id == "burn");
}

TEST_CASE("Battle stun prevents actions", "[battle]") {
    Registry reg;
    register_minimal(reg);
    Battle battle(reg, 1);
    auto hero = battle.add_creature("hero", Team::Player);
    auto goblin = battle.add_creature("goblin", Team::Enemy);

    auto ctx = battle.make_context();
    Creature* h = battle.get_creature(hero);
    Creature* g = battle.get_creature(goblin);
    ctx.apply_status(*h, *g, "stun", 2.f, 1);
    REQUIRE(g->is_stunned());

    auto result = battle.use_ability(goblin, "slash", hero);
    REQUIRE_FALSE(result.success);
    REQUIRE(result.reason.find("stun") != std::string::npos);
}

TEST_CASE("Battle win condition", "[battle]") {
    Registry reg;
    register_minimal(reg);
    Battle battle(reg, 1);
    auto hero = battle.add_creature("hero", Team::Player);
    auto goblin = battle.add_creature("goblin", Team::Enemy);

    Creature* g = battle.get_creature(goblin);
    g->take_damage(1000.f, DamageType::True);
    battle.check_win_conditions();
    REQUIRE(battle.result() == BattleResult::PlayerVictory);
}

TEST_CASE("Persistent entities tick damage over time", "[battle]") {
    Registry reg;
    register_minimal(reg);
    Battle battle(reg, 1);
    auto hero = battle.add_creature("hero", Team::Player);
    auto goblin = battle.add_creature("goblin", Team::Enemy);

    battle.use_ability(hero, "fireball", goblin);
    float hp_after_cast = battle.get_creature(goblin)->hp();

    for (int i = 0; i < 20; ++i) {
        battle.tick(0.25f);
    }

    float hp_later = battle.get_creature(goblin)->hp();
    // Burn DoT and/or magma balls should deal additional damage
    REQUIRE(hp_later < hp_after_cast);
}

TEST_CASE("Resource cost blocks cast", "[battle]") {
    Registry reg;
    register_minimal(reg);
    Battle battle(reg, 1);
    auto hero = battle.add_creature("hero", Team::Player);
    auto goblin = battle.add_creature("goblin", Team::Enemy);
    battle.get_creature(hero)->set_mana(5.f);
    auto result = battle.use_ability(hero, "fireball", goblin);
    REQUIRE_FALSE(result.success);
    REQUIRE(result.reason.find("mana") != std::string::npos);
}
