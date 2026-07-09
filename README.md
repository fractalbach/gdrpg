# gdrpg

Modular, data-driven **C++ RPG combat simulation core** with an optional Godot 4 GDExtension.

The core is completely independent of Godot — build and run the demo and tests with only a C++20 toolchain and CMake. The same library powers a thin GDExtension for Godot 4.3+.

## Features

- Real-time tick-based combat (`Battle::tick(dt)`)
- Data-driven creatures, abilities, items, status effects, and persistent entities (JSON)
- Ability effect chains: damage → status → spawn MagmaBalls / IceShards / BurningGround
- Equipment that modifies stats and grants abilities (including mid-combat swaps)
- Status stacking, stun/silence, DoTs, summons
- Standalone demo with rich combat logging
- Catch2 unit/integration tests
- Thin Godot wrappers (`GDRPG_Battle`, `GDRPG_Creature`)

## Architecture

```
core/        Pure C++ static library (gdrpg_core) — no Godot
demo/        Standalone combat demo executable
tests/       Catch2 tests
extension/   GDExtension (core + godot-cpp)
data/        JSON game data
docs/        Design notes
godot_example/  Minimal Godot project
```

See [docs/design.md](docs/design.md) for details.

## Requirements

- CMake 3.20+
- C++20 compiler (MSVC 2022, GCC 12+, Clang 15+)
- Ninja (recommended) or Visual Studio generator
- Network access on first configure (FetchContent: nlohmann/json, Catch2)
- Optional: Godot 4.3+ and godot-cpp (only for the extension)

## Build (core + demo + tests)

```bash
cmake --preset default
cmake --build --preset default
ctest --preset default
```

Or manually:

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

On Windows with Visual Studio:

```bash
cmake --preset vs2022
cmake --build --preset vs2022
```

### Run the demo

```bash
./build/default/demo/gdrpg_demo
# or pass an explicit data path:
./build/default/demo/gdrpg_demo ./data
```

The demo runs three scripted fights (goblin skirmish, dragon boss, equipment swap) and prints a detailed combat log showing damage, status ticks, and persistent entity chains.

## Build GDExtension

```bash
cmake --preset extension
cmake --build --preset extension
```

This fetches godot-cpp (Godot 4.3 stable tags by default) and writes the shared library into `godot_example/bin/`.

Open `godot_example/` in Godot 4.3+. The sample scene loads `../data`, starts a battle, casts Fireball, and ticks with simple AI.

### GDScript usage

```gdscript
var battle := GDRPG_Battle.new()
battle.load_data("res://../data")  # or an absolute path to data/
battle.start(42)

var mage := battle.add_creature("mage", 0)      # Team.Player = 0
var goblin := battle.add_creature("goblin", 1)  # Team.Enemy = 1

battle.use_ability(mage, "fireball", goblin)

func _process(delta: float) -> void:
    battle.run_ai(1)
    battle.tick(delta)
    if battle.is_finished():
        print(battle.get_log_dump())
```

## Data format (sketch)

Abilities are ordered effect lists:

```json
{
  "id": "fireball",
  "cooldown": 5.0,
  "mana_cost": 35,
  "targeting": "single_enemy",
  "effects": [
    { "type": "damage", "base": 40, "coeff": 1.2, "spell": true, "damage_type": "fire" },
    { "type": "apply_status", "status": "burn", "duration": 5.0 },
    { "type": "spawn_persistent", "persistent": "magma_ball", "count": 3, "delay_between": 0.25 }
  ]
}
```

Effect types: `damage`, `heal`, `apply_status`, `spawn_persistent`, `stat_mod`, `summon`, `dispel`.

## CMake options

| Option | Default | Description |
|--------|---------|-------------|
| `GDRPG_BUILD_DEMO` | ON | Standalone demo |
| `GDRPG_BUILD_TESTS` | ON | Catch2 tests |
| `GDRPG_BUILD_EXTENSION` | OFF | Godot GDExtension |
| `GDRPG_DATA_DIR` | `./data` | JSON data path baked into targets |

## License

MIT — see [LICENSE](LICENSE).
