#include "gdrpg/effect.hpp"
#include "gdrpg/combat_context.hpp"
#include "gdrpg/combat_log.hpp"
#include "gdrpg/creature.hpp"
#include "gdrpg/registry.hpp"

#include <algorithm>
#include <cmath>
#include <random>

namespace gdrpg {

namespace {

float roll_power(Creature& source, float base, float coeff, bool spell) {
    const float power =
        spell ? source.stats().get(StatType::SpellPower) : source.stats().get(StatType::Attack);
    return base + power * coeff;
}

bool roll_crit(CombatContext& ctx, Creature& source) {
    const float chance = std::clamp(source.stats().get(StatType::CritChance), 0.f, 1.f);
    std::uniform_real_distribution<float> dist(0.f, 1.f);
    return dist(ctx.rng()) < chance;
}

} // namespace

std::shared_ptr<Effect> DamageEffect::clone() const {
    return std::make_shared<DamageEffect>(*this);
}

void DamageEffect::apply(CombatContext& ctx, Creature& source, Creature* primary_target) {
    if (!primary_target || !primary_target->is_alive() || !source.is_alive()) {
        return;
    }
    float amount = roll_power(source, base_amount, power_coefficient, use_spell_power);
    bool crit = false;
    if (can_crit && roll_crit(ctx, source)) {
        crit = true;
        const float mult = std::max(1.f, source.stats().get(StatType::CritMultiplier));
        amount *= mult;
    }
    ctx.apply_damage(source, *primary_target, amount, damage_type, crit);
}

std::shared_ptr<Effect> HealEffect::clone() const {
    return std::make_shared<HealEffect>(*this);
}

void HealEffect::apply(CombatContext& ctx, Creature& source, Creature* primary_target) {
    Creature* target = primary_target ? primary_target : &source;
    if (!target->is_alive()) {
        return;
    }
    const float amount = roll_power(source, base_amount, power_coefficient, use_spell_power);
    ctx.apply_heal(source, *target, amount);
}

std::shared_ptr<Effect> ApplyStatusEffect::clone() const {
    return std::make_shared<ApplyStatusEffect>(*this);
}

void ApplyStatusEffect::apply(CombatContext& ctx, Creature& source, Creature* primary_target) {
    if (!primary_target || !primary_target->is_alive()) {
        return;
    }
    ctx.apply_status(source, *primary_target, status_id, duration_override, stacks);
}

std::shared_ptr<Effect> SpawnPersistentEffect::clone() const {
    return std::make_shared<SpawnPersistentEffect>(*this);
}

void SpawnPersistentEffect::apply(CombatContext& ctx, Creature& source, Creature* primary_target) {
    if (!source.is_alive()) {
        // Owner died mid-chain — still allow spawn if template doesn't require living owner,
        // but default: skip.
        return;
    }
    for (int i = 0; i < count; ++i) {
        auto& ent = ctx.spawn_persistent(template_id, source, primary_target);
        if (delay_between > 0.f && i > 0) {
            ent.initial_delay += delay_between * static_cast<float>(i);
            ent.tick_timer = ent.initial_delay;
        }
    }
}

std::shared_ptr<Effect> StatModEffect::clone() const {
    return std::make_shared<StatModEffect>(*this);
}

void StatModEffect::apply(CombatContext& ctx, Creature& source, Creature* primary_target) {
    Creature* target = primary_target ? primary_target : &source;
    if (!target->is_alive()) {
        return;
    }
    StatModifier mod;
    mod.stat = stat;
    mod.op = op;
    mod.amount = amount;
    mod.remaining = duration;
    mod.source = source_tag;
    mod.source_instance = ctx.next_instance_id();
    target->stats().add_modifier(mod);
    ctx.log().combat_fmt(target->name(), " gains ", to_string(stat), " mod ", amount, " for ",
                         duration, "s");
}

std::shared_ptr<Effect> SummonMinionEffect::clone() const {
    return std::make_shared<SummonMinionEffect>(*this);
}

void SummonMinionEffect::apply(CombatContext& ctx, Creature& source, Creature* /*primary_target*/) {
    if (!source.is_alive()) {
        return;
    }
    for (int i = 0; i < count; ++i) {
        ctx.summon_creature(creature_template_id, source.team(), source, lifetime);
    }
}

std::shared_ptr<Effect> DispelEffect::clone() const {
    return std::make_shared<DispelEffect>(*this);
}

void DispelEffect::apply(CombatContext& ctx, Creature& source, Creature* primary_target) {
    Creature* target = primary_target ? primary_target : &source;
    if (!target->is_alive()) {
        return;
    }
    int removed = 0;
    auto& statuses = target->statuses();
    for (auto it = statuses.begin(); it != statuses.end() && removed < max_count;) {
        const auto* data = ctx.registry().status(it->status_id);
        if (!data) {
            ++it;
            continue;
        }
        const bool is_buff = data->is_buff;
        if ((is_buff && dispel_buffs) || (!is_buff && dispel_debuffs)) {
            target->stats().remove_modifiers_from("status:" + it->status_id);
            ctx.log().combat_fmt("Dispelled ", data->name, " from ", target->name());
            it = statuses.erase(it);
            ++removed;
        } else {
            ++it;
        }
    }
    target->refresh_crowd_control_flags(ctx.registry());
}

} // namespace gdrpg
