#pragma once

#include "gdrpg/types.hpp"

#include <memory>
#include <string>
#include <vector>

namespace gdrpg {

class CombatContext;
class Creature;

/// Base polymorphic effect applied during ability resolution or status ticks.
class Effect {
public:
    virtual ~Effect() = default;
    virtual std::string type_name() const = 0;
    virtual void apply(CombatContext& ctx, Creature& source, Creature* primary_target) = 0;
    virtual std::shared_ptr<Effect> clone() const = 0;
};

class DamageEffect : public Effect {
public:
    float base_amount = 0.f;
    float power_coefficient = 1.f;
    bool use_spell_power = false;
    DamageType damage_type = DamageType::Physical;
    bool can_crit = true;

    std::string type_name() const override { return "Damage"; }
    void apply(CombatContext& ctx, Creature& source, Creature* primary_target) override;
    std::shared_ptr<Effect> clone() const override;
};

class HealEffect : public Effect {
public:
    float base_amount = 0.f;
    float power_coefficient = 0.5f;
    bool use_spell_power = true;

    std::string type_name() const override { return "Heal"; }
    void apply(CombatContext& ctx, Creature& source, Creature* primary_target) override;
    std::shared_ptr<Effect> clone() const override;
};

class ApplyStatusEffect : public Effect {
public:
    StatusId status_id;
    float duration_override = -1.f; // <0 use template default
    int stacks = 1;

    std::string type_name() const override { return "ApplyStatus"; }
    void apply(CombatContext& ctx, Creature& source, Creature* primary_target) override;
    std::shared_ptr<Effect> clone() const override;
};

class SpawnPersistentEffect : public Effect {
public:
    PersistentId template_id;
    int count = 1;
    float delay_between = 0.f;

    std::string type_name() const override { return "SpawnPersistent"; }
    void apply(CombatContext& ctx, Creature& source, Creature* primary_target) override;
    std::shared_ptr<Effect> clone() const override;
};

class StatModEffect : public Effect {
public:
    StatType stat = StatType::Attack;
    ModifierOp op = ModifierOp::Flat;
    float amount = 0.f;
    float duration = 5.f;
    std::string source_tag = "effect";

    std::string type_name() const override { return "StatMod"; }
    void apply(CombatContext& ctx, Creature& source, Creature* primary_target) override;
    std::shared_ptr<Effect> clone() const override;
};

class SummonMinionEffect : public Effect {
public:
    std::string creature_template_id;
    float lifetime = 30.f;
    int count = 1;

    std::string type_name() const override { return "SummonMinion"; }
    void apply(CombatContext& ctx, Creature& source, Creature* primary_target) override;
    std::shared_ptr<Effect> clone() const override;
};

class DispelEffect : public Effect {
public:
    bool dispel_buffs = false;
    bool dispel_debuffs = true;
    int max_count = 1;

    std::string type_name() const override { return "Dispel"; }
    void apply(CombatContext& ctx, Creature& source, Creature* primary_target) override;
    std::shared_ptr<Effect> clone() const override;
};

} // namespace gdrpg
