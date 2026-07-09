extends Node
## Minimal Godot example driving the gdrpg GDExtension.
## Build the extension first (see README), then open this project in Godot 4.3+.

const TEAM_PLAYER := 0
const TEAM_ENEMY := 1

@onready var log_label: RichTextLabel = $Log

var battle: GDRPG_Battle
var ai_timer := 0.0
var started := false

func _ready() -> void:
	battle = GDRPG_Battle.new()
	# Point at the repo data/ folder (sibling of godot_example/)
	var data_path := ProjectSettings.globalize_path("res://").path_join("..").path_join("data")
	if not battle.load_data(data_path):
		_append("[color=red]Failed to load data from %s[/color]" % data_path)
		return

	battle.start(42)
	var warrior := battle.add_creature("warrior", TEAM_PLAYER)
	var mage := battle.add_creature("mage", TEAM_PLAYER)
	var goblin := battle.add_creature("goblin", TEAM_ENEMY)
	var shaman := battle.add_creature("goblin_shaman", TEAM_ENEMY)

	_append("[b]Battle started[/b]")
	_append("Warrior id=%d, Mage id=%d, Goblin id=%d, Shaman id=%d" % [warrior, mage, goblin, shaman])

	battle.use_ability(mage, "fireball", goblin)
	started = true

func _process(delta: float) -> void:
	if not started or battle == null or battle.is_finished():
		return

	ai_timer += delta
	if ai_timer >= 1.5:
		ai_timer = 0.0
		battle.run_ai(TEAM_ENEMY)
		battle.run_ai(TEAM_PLAYER)

	battle.tick(delta)
	_refresh_log()

	if battle.is_finished():
		_append("\n[b]Result: %d[/b] (1=PlayerWin, 2=EnemyWin, 3=Draw)" % battle.get_result())
		started = false

func _refresh_log() -> void:
	log_label.clear()
	for line in battle.get_log_lines():
		log_label.append_text(str(line) + "\n")

func _append(text: String) -> void:
	log_label.append_text(text + "\n")
