# gdrpg Architecture

## Overview

**gdrpg** is a modular, data-driven RPG combat simulation core written in modern C++20.
The same library powers:

1. A **standalone demo** for balancing and mechanics testing
2. **Unit/integration tests** (Catch2)
3. A thin **Godot 4 GDExtension** for a future 3D game

Godot is an optional consumer. The core never includes Godot headers.

## Hybrid Option 1 Layout

```
core/        → static library `gdrpg_core` (pure C++)
demo/        → executable linking only core
tests/       → Catch2 tests linking only core
extension/   → shared library GDExtension (core + godot-cpp)
data/        → JSON templates (creatures, abilities, items, statuses, persistents)
```

## Simulation Model

Combat is **real-time and tick-based**. Call `Battle::tick(dt)` each frame (or at a fixed
step in the demo). Abilities are activated explicitly via `Battle::use_ability`.

### Key types

| Type | Role |
|------|------|
| `Registry` | Factory for all JSON templates |
| `DataLoader` | Parses JSON into the registry |
| `Battle` | Owns creatures, persistents, time, win conditions, combat log |
| `CombatContext` | Passed into effect resolution; targeting, damage, spawn helpers |
| `Creature` | Stats, HP/mana, abilities, equipment, statuses, optional `Vec3` |
| `AbilityData` / `AbilityInstance` | Immutable template + runtime cooldown |
| `Effect` | Polymorphic: Damage, Heal, ApplyStatus, SpawnPersistent, StatMod, Summon, Dispel |
| `PersistentEntity` | MagmaBall / IceShard / BurningGround — lifetime + tick interval |
| `StatBlock` / `StatModifier` | Flat + multiplicative modifiers with sources and durations |

### Ability resolution chain

```
use_ability(caster, id, target)
  → validate (alive, stun/silence, cooldown, mana)
  → spend mana, start cooldown
  → for each Effect in AbilityData:
       → Spawn/Summon: apply once
       → otherwise: apply to resolved target(s)
  → effects may spawn PersistentEntities or apply Statuses
  → tick(dt) advances cooldowns, DoTs, persistents, summon lifetimes
```

### Targeting (v1)

Abstract enums: `Self`, `SingleEnemy`, `AllEnemies`, `RandomEnemy`, etc.
Position (`Vec3`) is stored but not yet used for range checks — reserved for spatial combat.

### Status stacking

Configured per status via `stack_rule`:

- `refresh` — refresh duration
- `add_stacks` — increase stacks up to `max_stacks`
- `independent` — multiple instances
- `ignore_if_present` — no-op if already applied

### Edge cases handled

- **Death mid-chain**: remaining effects aborted if caster dies; damage skipped on dead targets
- **Overkill**: HP clamped to 0; actual damage returned is HP lost
- **Resource costs**: mana checked before cast
- **Owner death**: persistents with `destroy_on_owner_death` dissipate
- **Equipment swap mid-combat**: `Battle::equip_item` / `unequip_item` updates mods and granted abilities
- **Stun/silence**: blocks `use_ability`

## Extending the system

### New ability (data only)

Add an entry to `data/abilities.json` referencing existing effect types and status/persistent ids.

### New effect type

1. Subclass `gdrpg::Effect` in `effect.hpp` / `effect.cpp`
2. Parse it in `data_loader.cpp` (`parse_effect`)
3. Use `"type": "your_type"` in JSON

### New persistent entity

Add to `data/persistent_entities.json` with `on_tick` / `on_expire` effect lists.

## GDExtension

Thin `RefCounted` wrappers:

- `GDRPG_Battle` — load data, start battle, tick, use abilities, read log
- `GDRPG_Creature` — read-only view of a participant

The extension links `gdrpg_core` and `godot-cpp`. Core remains Godot-free.

## Future spatial work

`Vec3` and optional `Creature::position` / `PersistentEntity::position` are stubs.
Planned: range checks, ground AoE radii, projectile travel time, 3D Godot node sync.
