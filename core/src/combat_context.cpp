#include "gdrpg/combat_context.hpp"
#include "gdrpg/battle.hpp"
#include "gdrpg/combat_log.hpp"
#include "gdrpg/effect.hpp"
#include "gdrpg/registry.hpp"

#include <algorithm>

namespace gdrpg {

CombatContext::CombatContext(Battle& battle, Registry& registry, CombatLog& log, std::mt19937& rng)
    : battle_(battle), registry_(registry), log_(log), rng_(rng) {}

float CombatContext::time() const {
    return battle_.time();
}

EntityInstanceId CombatContext::next_instance_id() {
    return battle_.next_instance_id();
}

Creature* CombatContext::find_creature(CreatureId id) {
    return battle_.get_creature(id);
}

const Creature* CombatContext::find_creature(CreatureId id) const {
    return battle_.get_creature(id);
}

std::vector<Creature*> CombatContext::living_enemies_of(const Creature& source) {
    std::vector<Creature*> out;
    for (Creature* c : battle_.creatures()) {
        if (c && c->is_alive() && c->team() != source.team() && c->team() != Team::Neutral) {
            out.push_back(c);
        }
    }
    // Also treat Neutral as non-ally for enemy listing when source is Player/Enemy
    if (out.empty()) {
        for (Creature* c : battle_.creatures()) {
            if (c && c->is_alive() && c->id() != source.id() && c->team() != source.team()) {
                out.push_back(c);
            }
        }
    }
    return out;
}

std::vector<Creature*> CombatContext::living_allies_of(const Creature& source, bool include_self) {
    std::vector<Creature*> out;
    for (Creature* c : battle_.creatures()) {
        if (!c || !c->is_alive()) {
            continue;
        }
        if (c->team() != source.team()) {
            continue;
        }
        if (!include_self && c->id() == source.id()) {
            continue;
        }
        out.push_back(c);
    }
    return out;
}

Creature* CombatContext::random_enemy(const Creature& source) {
    auto enemies = living_enemies_of(source);
    if (enemies.empty()) {
        return nullptr;
    }
    std::uniform_int_distribution<size_t> dist(0, enemies.size() - 1);
    return enemies[dist(rng_)];
}

Creature* CombatContext::random_ally(const Creature& source, bool include_self) {
    auto allies = living_allies_of(source, include_self);
    if (allies.empty()) {
        return nullptr;
    }
    std::uniform_int_distribution<size_t> dist(0, allies.size() - 1);
    return allies[dist(rng_)];
}

std::vector<Creature*> CombatContext::resolve_targets(const Creature& source, TargetType targeting,
                                                      Creature* primary) {
    std::vector<Creature*> out;
    switch (targeting) {
        case TargetType::Self:
            out.push_back(const_cast<Creature*>(&source));
            break;
        case TargetType::SingleAlly:
            if (primary && primary->is_alive() && primary->team() == source.team()) {
                out.push_back(primary);
            } else if (auto* a = random_ally(source, true)) {
                out.push_back(a);
            }
            break;
        case TargetType::SingleEnemy:
            if (primary && primary->is_alive() && primary->team() != source.team()) {
                out.push_back(primary);
            } else if (auto* e = random_enemy(source)) {
                out.push_back(e);
            }
            break;
        case TargetType::AllAllies:
            out = living_allies_of(source, true);
            break;
        case TargetType::AllEnemies:
            out = living_enemies_of(source);
            break;
        case TargetType::All:
            for (Creature* c : battle_.creatures()) {
                if (c && c->is_alive()) {
                    out.push_back(c);
                }
            }
            break;
        case TargetType::RandomEnemy:
            if (auto* e = random_enemy(source)) {
                out.push_back(e);
            }
            break;
        case TargetType::RandomAlly:
            if (auto* a = random_ally(source, true)) {
                out.push_back(a);
            }
            break;
    }
    return out;
}

void CombatContext::apply_damage(Creature& source, Creature& target, float amount, DamageType type,
                                 bool is_crit) {
    if (!target.is_alive()) {
        return;
    }
    const float dealt = target.take_damage(amount, type);
    log_.combat_fmt(source.name(), " deals ", static_cast<int>(dealt), " ", to_string(type),
                    is_crit ? " CRIT" : "", " to ", target.name(), " (",
                    static_cast<int>(target.hp()), "/", static_cast<int>(target.max_hp()), " HP)");
    if (!target.is_alive()) {
        log_.important(target.name() + " has been defeated!");
    }
}

void CombatContext::apply_heal(Creature& source, Creature& target, float amount) {
    if (!target.is_alive()) {
        return;
    }
    const float before = target.hp();
    target.heal(amount);
    const float healed = target.hp() - before;
    log_.combat_fmt(source.name(), " heals ", target.name(), " for ", static_cast<int>(healed),
                    " (", static_cast<int>(target.hp()), "/", static_cast<int>(target.max_hp()),
                    " HP)");
}

void CombatContext::apply_status(Creature& source, Creature& target, const StatusId& status_id,
                                 float duration_override, int stacks) {
    const auto* data = registry_.status(status_id);
    if (!data || !target.is_alive()) {
        return;
    }

    const float duration = duration_override >= 0.f ? duration_override : data->default_duration;

    auto find_existing = [&]() -> StatusInstance* {
        for (auto& s : target.statuses()) {
            if (s.status_id == status_id && !s.is_expired()) {
                return &s;
            }
        }
        return nullptr;
    };

    StatusInstance* existing = find_existing();
    switch (data->stack_rule) {
        case StackRule::IgnoreIfPresent:
            if (existing) {
                return;
            }
            break;
        case StackRule::RefreshDuration:
            if (existing) {
                existing->remaining = duration;
                existing->source_id = source.id();
                log_.combat_fmt(target.name(), " refreshes ", data->name, " (", duration, "s)");
                return;
            }
            break;
        case StackRule::AddStacks:
            if (existing) {
                existing->stacks = std::min(data->max_stacks, existing->stacks + stacks);
                existing->remaining = duration;
                existing->source_id = source.id();
                log_.combat_fmt(target.name(), " stacks ", data->name, " to ", existing->stacks);
                return;
            }
            break;
        case StackRule::Independent:
            break;
    }

    StatusInstance inst;
    inst.status_id = status_id;
    inst.remaining = duration;
    inst.tick_timer = data->tick_interval;
    inst.stacks = std::min(data->max_stacks, stacks);
    inst.source_id = source.id();
    inst.instance_id = next_instance_id();
    target.statuses().push_back(inst);

    for (auto mod : data->stat_mods) {
        mod.source = "status:" + status_id;
        mod.remaining = duration;
        mod.source_instance = inst.instance_id;
        // Scale flat mods by stacks lightly
        if (mod.op == ModifierOp::Flat) {
            mod.amount *= static_cast<float>(inst.stacks);
        }
        target.stats().add_modifier(mod);
    }

    if (data->speed_multiplier != 1.f) {
        StatModifier speed_mod;
        speed_mod.stat = StatType::Speed;
        speed_mod.op = ModifierOp::Multiplicative;
        speed_mod.amount = data->speed_multiplier - 1.f;
        speed_mod.remaining = duration;
        speed_mod.source = "status:" + status_id;
        speed_mod.source_instance = inst.instance_id;
        target.stats().add_modifier(speed_mod);
    }

    target.refresh_crowd_control_flags(registry_);

    for (const auto& fx : data->on_apply_effects) {
        if (fx) {
            fx->apply(*this, source, &target);
        }
    }

    log_.combat_fmt(target.name(), " afflicted with ", data->name, " x", inst.stacks, " (",
                    duration, "s)");
}

PersistentEntity& CombatContext::spawn_persistent(const PersistentId& template_id, Creature& owner,
                                                  Creature* locked_target) {
    const auto* data = registry_.persistent(template_id);
    PersistentEntity ent;
    ent.instance_id = next_instance_id();
    ent.template_id = template_id;
    ent.owner_id = owner.id();
    if (locked_target) {
        ent.locked_target_id = locked_target->id();
    }
    if (data) {
        ent.name = data->name;
        ent.remaining = data->lifetime;
        ent.tick_timer = data->initial_delay > 0.f ? data->initial_delay : data->tick_interval;
        ent.initial_delay = data->initial_delay;
        ent.retarget = data->retarget;
        ent.destroy_on_owner_death = data->destroy_on_owner_death;
        for (const auto& fx : data->on_tick_effects) {
            if (fx) {
                ent.on_tick_effects.push_back(fx->clone());
            }
        }
        for (const auto& fx : data->on_expire_effects) {
            if (fx) {
                ent.on_expire_effects.push_back(fx->clone());
            }
        }
    } else {
        ent.name = template_id;
        ent.remaining = 5.f;
        ent.tick_timer = 1.f;
    }
    if (owner.position()) {
        ent.position = owner.position();
    }

    battle_.persistents().push_back(std::move(ent));
    auto& ref = battle_.persistents().back();
    log_.combat_fmt(owner.name(), " spawns ", ref.name, " [", ref.instance_id, "]");
    return ref;
}

Creature* CombatContext::summon_creature(const std::string& template_id, Team team,
                                         Creature& summoner, float lifetime) {
    const CreatureId summoner_id = summoner.id();
    const std::string summoner_name = summoner.name();
    const std::optional<Vec3> pos = summoner.position();

    CreatureId id = battle_.add_creature(template_id, team, pos);
    Creature* c = battle_.get_creature(id);
    if (!c) {
        return nullptr;
    }
    c->lifetime() = lifetime;
    c->set_summoner(summoner_id);
    log_.combat_fmt(summoner_name, " summons ", c->name(), " for ", lifetime, "s");
    return c;
}

} // namespace gdrpg
