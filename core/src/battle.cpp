#include "gdrpg/battle.hpp"
#include "gdrpg/effect.hpp"

#include <algorithm>
#include <cmath>

namespace gdrpg {

Battle::Battle(Registry& registry, std::uint64_t seed)
    : registry_(registry), rng_(static_cast<std::mt19937::result_type>(seed)) {}

void Battle::rebuild_ptrs() {
    creature_ptrs_.clear();
    creature_ptrs_.reserve(creatures_.size());
    for (auto& c : creatures_) {
        creature_ptrs_.push_back(&c);
    }
}

CombatContext Battle::make_context() {
    return CombatContext(*this, registry_, log_, rng_);
}

CreatureId Battle::add_creature(const std::string& template_id, Team team,
                                std::optional<Vec3> position) {
    const CreatureId id = next_creature_id_++;
    Creature c = registry_.make_creature(template_id, id, team);
    c.position() = position;
    creatures_.push_back(std::move(c));
    rebuild_ptrs();
    log_.info("Added " + creatures_.back().name() + " (" + to_string(team) + ") id=" +
              std::to_string(id));
    return id;
}

CreatureId Battle::add_creature(Creature creature) {
    if (creature.id() == kInvalidCreatureId) {
        // Assign id via a temporary — Creature id is set at construction, so rebuild
        const CreatureId id = next_creature_id_++;
        Creature c(id, creature.name(), creature.team(), creature.stats());
        c.set_hp(creature.hp());
        c.set_mana(creature.mana());
        for (const auto& a : creature.abilities()) {
            c.add_ability(a.ability_id);
        }
        c.position() = creature.position();
        c.lifetime() = creature.lifetime();
        creatures_.push_back(std::move(c));
    } else {
        next_creature_id_ = std::max(next_creature_id_, creature.id() + 1);
        creatures_.push_back(std::move(creature));
    }
    rebuild_ptrs();
    return creatures_.back().id();
}

Creature* Battle::get_creature(CreatureId id) {
    for (auto& c : creatures_) {
        if (c.id() == id) {
            return &c;
        }
    }
    return nullptr;
}

const Creature* Battle::get_creature(CreatureId id) const {
    for (const auto& c : creatures_) {
        if (c.id() == id) {
            return &c;
        }
    }
    return nullptr;
}

namespace {

bool is_aoe_target(TargetType t) {
    return t == TargetType::AllEnemies || t == TargetType::AllAllies || t == TargetType::All;
}

bool is_global_effect(const Effect& fx) {
    const auto& n = fx.type_name();
    return n == "SpawnPersistent" || n == "SummonMinion";
}

} // namespace

void Battle::resolve_ability_effects(Creature& caster, const AbilityData& ability,
                                     Creature* primary) {
    auto ctx = make_context();
    auto targets = ctx.resolve_targets(caster, ability.targeting, primary);

    if (targets.empty() && ability.targeting != TargetType::Self) {
        log_.combat(caster.name() + " finds no valid targets for " + ability.name);
        return;
    }

    const bool aoe = is_aoe_target(ability.targeting);

    for (const auto& fx : ability.effects) {
        if (!fx) {
            continue;
        }
        if (!caster.is_alive()) {
            log_.combat("Caster died mid-chain; aborting remaining effects of " + ability.name);
            break;
        }

        // Spawns/summons fire once per cast, not once per AoE target.
        if (is_global_effect(*fx)) {
            Creature* t = targets.empty() ? primary : targets.front();
            fx->apply(ctx, caster, t);
            continue;
        }

        if (aoe) {
            for (Creature* t : targets) {
                if (!caster.is_alive()) {
                    break;
                }
                if (!t || !t->is_alive()) {
                    continue;
                }
                fx->apply(ctx, caster, t);
            }
        } else {
            Creature* t = targets.empty() ? primary : targets.front();
            if (t && !t->is_alive() && fx->type_name() == "Damage") {
                continue;
            }
            fx->apply(ctx, caster, t);
        }
    }
}

AbilityUseResult Battle::use_ability(CreatureId caster_id, const AbilityId& ability_id,
                                     CreatureId primary_target_id) {
    AbilityUseResult result;
    if (is_finished()) {
        result.reason = "Battle already finished";
        return result;
    }

    Creature* caster = get_creature(caster_id);
    if (!caster || !caster->is_alive()) {
        result.reason = "Caster is dead or missing";
        return result;
    }
    if (caster->is_stunned()) {
        result.reason = "Caster is stunned";
        log_.combat(caster->name() + " is stunned and cannot act");
        return result;
    }
    if (caster->is_silenced()) {
        result.reason = "Caster is silenced";
        log_.combat(caster->name() + " is silenced");
        return result;
    }

    const AbilityData* data = registry_.ability(ability_id);
    if (!data) {
        result.reason = "Unknown ability: " + ability_id;
        return result;
    }

    AbilityInstance* inst = caster->find_ability(ability_id);
    if (!inst) {
        result.reason = "Caster does not know ability";
        return result;
    }
    if (!inst->is_ready()) {
        result.reason = "Ability on cooldown";
        return result;
    }
    if (!caster->can_afford(data->mana_cost)) {
        result.reason = "Not enough mana";
        return result;
    }

    Creature* primary = nullptr;
    if (primary_target_id != kInvalidCreatureId) {
        primary = get_creature(primary_target_id);
    }

    caster->spend_mana(data->mana_cost);
    inst->start_cooldown(data->cooldown);

    if (primary) {
        log_.combat_fmt(caster->name(), " uses ", data->name, " on ", primary->name());
    } else {
        log_.combat_fmt(caster->name(), " uses ", data->name);
    }

    resolve_ability_effects(*caster, *data, primary);

    check_win_conditions();
    result.success = true;
    return result;
}

bool Battle::equip_item(CreatureId creature_id, const ItemId& item_id) {
    Creature* c = get_creature(creature_id);
    const ItemData* data = registry_.item(item_id);
    if (!c || !data) {
        return false;
    }
    ItemInstance inst;
    inst.item_id = item_id;
    inst.instance_id = next_instance_id();
    c->equip(data->slot, inst, *data);
    log_.info(c->name() + " equips " + data->name);
    return true;
}

bool Battle::unequip_item(CreatureId creature_id, EquipmentSlot slot) {
    Creature* c = get_creature(creature_id);
    if (!c) {
        return false;
    }
    auto& eq = c->equipment()[static_cast<size_t>(slot)];
    if (!eq) {
        return false;
    }
    const ItemData* data = registry_.item(eq->item_id);
    auto removed = c->unequip(slot, data);
    if (removed && data) {
        log_.info(c->name() + " unequips " + data->name);
    }
    return removed.has_value();
}

void Battle::tick_statuses(float dt) {
    auto ctx = make_context();
    for (auto& creature : creatures_) {
        if (!creature.is_alive()) {
            continue;
        }
        for (auto& st : creature.statuses()) {
            if (st.is_expired()) {
                continue;
            }
            const auto* data = registry_.status(st.status_id);
            if (!data) {
                st.remaining = 0.f;
                continue;
            }

            st.remaining -= dt;
            st.tick_timer -= dt;

            while (st.tick_timer <= 0.f && !st.is_expired() && creature.is_alive()) {
                st.tick_timer += data->tick_interval;
                Creature* source = get_creature(st.source_id);
                Creature& src_ref = source ? *source : creature;
                for (const auto& fx : data->on_tick_effects) {
                    if (!fx) {
                        continue;
                    }
                    // Scale DoT by stacks
                    if (auto* dmg = dynamic_cast<DamageEffect*>(fx.get())) {
                        DamageEffect scaled = *dmg;
                        scaled.base_amount *= static_cast<float>(st.stacks);
                        scaled.apply(ctx, src_ref, &creature);
                    } else {
                        fx->apply(ctx, src_ref, &creature);
                    }
                }
                if (data->tick_interval <= 0.f) {
                    break; // avoid infinite loop
                }
            }
        }

        // Remove expired statuses and their mods
        auto& statuses = creature.statuses();
        for (auto it = statuses.begin(); it != statuses.end();) {
            if (it->is_expired()) {
                creature.stats().remove_modifiers_from("status:" + it->status_id);
                const auto* data = registry_.status(it->status_id);
                if (data) {
                    log_.debug(creature.name() + " loses " + data->name);
                }
                it = statuses.erase(it);
            } else {
                ++it;
            }
        }
        creature.refresh_crowd_control_flags(registry_);
        creature.stats().tick(dt);
    }
}

void Battle::tick_persistents(float dt) {
    auto ctx = make_context();
    for (auto& ent : persistents_) {
        if (ent.is_expired()) {
            continue;
        }

        // Owner death handling
        if (ent.destroy_on_owner_death) {
            Creature* owner = get_creature(ent.owner_id);
            if (!owner || !owner->is_alive()) {
                ent.pending_destroy = true;
                log_.combat(ent.name + " dissipates (owner dead)");
                continue;
            }
        }

        ent.remaining -= dt;
        ent.tick_timer -= dt;

        const auto* data = registry_.persistent(ent.template_id);
        const float interval = data ? data->tick_interval : 1.f;

        while (ent.tick_timer <= 0.f && !ent.is_expired()) {
            ent.tick_timer += interval;

            Creature* owner = get_creature(ent.owner_id);
            if (!owner) {
                ent.pending_destroy = true;
                break;
            }

            std::vector<Creature*> tick_targets;
            if (!is_aoe_target(ent.retarget) && ent.locked_target_id != kInvalidCreatureId) {
                Creature* locked = get_creature(ent.locked_target_id);
                if (locked && locked->is_alive()) {
                    tick_targets.push_back(locked);
                } else {
                    tick_targets = ctx.resolve_targets(*owner, ent.retarget, nullptr);
                    ent.locked_target_id =
                        tick_targets.empty() ? kInvalidCreatureId : tick_targets.front()->id();
                }
            } else {
                tick_targets = ctx.resolve_targets(*owner, ent.retarget, nullptr);
            }

            if (tick_targets.empty()) {
                continue;
            }

            for (Creature* target : tick_targets) {
                if (!target || !target->is_alive()) {
                    continue;
                }
                for (const auto& fx : ent.on_tick_effects) {
                    if (!fx) {
                        continue;
                    }
                    if (!owner->is_alive() && ent.destroy_on_owner_death) {
                        break;
                    }
                    fx->apply(ctx, *owner, target);
                }
            }

            if (interval <= 0.f) {
                break;
            }
        }

        if (ent.remaining <= 0.f) {
            Creature* owner = get_creature(ent.owner_id);
            if (owner) {
                for (const auto& fx : ent.on_expire_effects) {
                    if (!fx) {
                        continue;
                    }
                    Creature* target = get_creature(ent.locked_target_id);
                    fx->apply(ctx, *owner, target);
                }
            }
            log_.combat(ent.name + " expires");
        }
    }
}

void Battle::tick_creatures(float dt) {
    for (auto& c : creatures_) {
        if (!c.is_alive()) {
            continue;
        }
        c.tick_cooldowns(dt);
        c.tick_lifetime(dt);
        // Passive mana regen
        c.set_mana(c.mana() + 2.f * dt);
    }
}

void Battle::cleanup_dead_and_expired() {
    persistents_.erase(std::remove_if(persistents_.begin(), persistents_.end(),
                                      [](const PersistentEntity& e) { return e.is_expired(); }),
                       persistents_.end());
}

void Battle::check_win_conditions() {
    bool player_alive = false;
    bool enemy_alive = false;
    for (const auto& c : creatures_) {
        if (!c.is_alive()) {
            continue;
        }
        // Ignore temporary summons for win check? Keep them — they can win fights.
        if (c.team() == Team::Player) {
            player_alive = true;
        } else if (c.team() == Team::Enemy) {
            enemy_alive = true;
        }
    }
    if (!player_alive && !enemy_alive) {
        result_ = BattleResult::Draw;
        log_.important("Battle ends in a draw!");
    } else if (!player_alive) {
        result_ = BattleResult::EnemyVictory;
        log_.important("Enemy victory!");
    } else if (!enemy_alive) {
        result_ = BattleResult::PlayerVictory;
        log_.important("Player victory!");
    }
}

void Battle::tick(float dt) {
    if (is_finished() || dt <= 0.f) {
        return;
    }
    time_ += dt;
    log_.set_time(time_);

    tick_creatures(dt);
    tick_statuses(dt);
    tick_persistents(dt);
    cleanup_dead_and_expired();
    check_win_conditions();
}

void Battle::run_simple_ai(Team team) {
    if (is_finished()) {
        return;
    }
    auto ctx = make_context();
    for (auto& c : creatures_) {
        if (!c.is_alive() || c.team() != team || c.is_stunned()) {
            continue;
        }
        for (auto& ab : c.abilities()) {
            if (!ab.is_ready()) {
                continue;
            }
            const AbilityData* data = registry_.ability(ab.ability_id);
            if (!data || !c.can_afford(data->mana_cost)) {
                continue;
            }
            Creature* target = nullptr;
            if (data->targeting == TargetType::Self || data->targeting == TargetType::AllAllies ||
                data->targeting == TargetType::SingleAlly ||
                data->targeting == TargetType::RandomAlly) {
                target = &c;
            } else {
                target = ctx.random_enemy(c);
            }
            use_ability(c.id(), ab.ability_id, target ? target->id() : kInvalidCreatureId);
            break; // one ability per AI pulse
        }
    }
}

} // namespace gdrpg
