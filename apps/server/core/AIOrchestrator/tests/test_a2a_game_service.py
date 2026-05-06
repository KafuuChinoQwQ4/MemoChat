from __future__ import annotations

import json
import unittest
import sys
from pathlib import Path
from types import SimpleNamespace

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from harness.games.service import A2AGameService
from harness.games.contracts import GameState, GameTemplate


class FakeAgentService:
    def __init__(self, outputs: list[str] | None = None):
        self.outputs = list(outputs or [])
        self.calls: list[SimpleNamespace] = []

    async def run_turn(self, request):
        self.calls.append(request)
        content = self.outputs.pop(0) if self.outputs else '{"action_type":"skip","content":"我先跳过。"}'
        return SimpleNamespace(content=content, trace_id=f"trace-{len(self.calls)}")


class FakeGameStateStore:
    def __init__(self):
        self.snapshots: dict[str, dict] = {}
        self.templates: dict[str, GameTemplate] = {}
        self.started = False

    async def startup(self):
        self.started = True

    async def load_rooms(self):
        return [GameState.from_snapshot(snapshot) for snapshot in self.snapshots.values()]

    async def save_room(self, state):
        self.snapshots[state.room.room_id] = state.to_snapshot()

    async def delete_room(self, room_id, deleted_at):
        self.snapshots.pop(room_id, None)

    async def list_templates(self, uid):
        return [
            template
            for template in sorted(self.templates.values(), key=lambda item: item.updated_at, reverse=True)
            if uid <= 0 or template.uid == uid
        ]

    async def save_template(self, template):
        existing = self.templates.get(template.template_id)
        if existing is None or existing.updated_at <= template.updated_at:
            self.templates[template.template_id] = template

    async def get_template(self, template_id, uid=0):
        template = self.templates.get(template_id)
        if template is not None and (uid <= 0 or template.uid == uid):
            return template
        return None

    async def delete_template(self, template_id, uid, deleted_at):
        template = self.templates.get(template_id)
        if template is not None and template.uid == uid:
            self.templates.pop(template_id, None)


class A2AGameServiceTests(unittest.IsolatedAsyncioTestCase):
    def _locked_config(self, state: GameState) -> dict:
        config = state.room.config
        self.assertIsInstance(config, dict)
        candidates: list[tuple[str, dict]] = []
        for key, value in config.items():
            if isinstance(value, dict) and ("lock" in key or "fixed" in key or "init" in key):
                candidates.append((key, value))
        if any(key in config for key in ("locked", "ruleset_id", "agent_count", "agent_preset_pool", "preset_pool", "agent_presets")):
            candidates.append(("room.config", config))

        for name, candidate in candidates:
            if "locked" in candidate:
                self.assertTrue(candidate["locked"], f"{name}.locked should be true")
            if self._lock_value(candidate, "ruleset_id", "ruleset") is not None and self._lock_agent_pool(candidate, required=False) is not None:
                return candidate

        self.fail(f"room.config does not expose a locked initialization contract: {config!r}")

    def _lock_value(self, lock: dict, *keys: str):
        for key in keys:
            if key in lock:
                return lock[key]
        return None

    def _lock_agent_pool(self, lock: dict, required: bool = True) -> list[dict] | None:
        for key in ("agent_preset_pool", "preset_pool", "agent_pool", "agent_presets", "locked_agents", "agents"):
            value = lock.get(key)
            if isinstance(value, list):
                return value
        if required:
            self.fail(f"locked config does not include an agent preset pool: {lock!r}")
        return None

    def _canonical(self, value) -> str:
        return json.dumps(value, ensure_ascii=False, sort_keys=True)

    def _shuffle_marker(self, state: GameState) -> str:
        for container in (state.state, state.room.metadata, state.room.config):
            if not isinstance(container, dict):
                continue
            for key, value in container.items():
                lowered = str(key).lower()
                if "shuffle" in lowered or "identity_marker" in lowered or "preset_binding_marker" in lowered:
                    return self._canonical(value)
        for event in reversed(state.events):
            event_type = event.event_type.lower()
            if "shuffle" not in event_type and event.event_type not in {"game_started", "game_restarted"}:
                continue
            for key, value in event.payload.items():
                lowered = str(key).lower()
                if "shuffle" in lowered or "identity_marker" in lowered or "preset_binding_marker" in lowered:
                    return self._canonical(value)
        self.fail(
            "game state does not record a shuffle marker in state, room metadata/config, "
            "or lifecycle event payloads"
        )

    def _force_ended(self, state: GameState) -> None:
        state.room.status = "ended"
        state.room.phase = "ended"
        state.state["phase"] = "ended"
        state.state["winner"] = "test"

    async def test_create_room_supports_custom_ai_count(self):
        service = A2AGameService(FakeAgentService())

        state = service.create_room(
            uid=1001,
            title="今晚狼人杀",
            ruleset_id="werewolf.basic",
            max_players=8,
            agent_count=5,
        )

        self.assertEqual(state.room.title, "今晚狼人杀")
        self.assertEqual(len(state.participants), 6)
        self.assertEqual(len([p for p in state.participants if p.kind == "agent"]), 5)
        self.assertEqual(state.available_actions, ["start"])

    async def test_create_room_locks_ruleset_agent_count_and_preset_pool(self):
        service = A2AGameService(FakeAgentService())
        agent_presets = [
            {"display_name": "侦探林澈", "role_key": "detective", "persona": "追问细节", "skill_name": "writer"},
            {"display_name": "管家周叔", "role_key": "witness", "persona": "谨慎回忆", "skill_name": "writer"},
        ]

        state = service.create_room(
            uid=1001,
            title="古宅锁定局",
            ruleset_id="script_murder.basic",
            max_players=5,
            agent_count=0,
            agents=agent_presets,
            config={"scene": "旧宅书房"},
        )
        agent_presets[0]["display_name"] = "mutated-after-create"

        lock = self._locked_config(state)
        pool = self._lock_agent_pool(lock)

        self.assertEqual(state.room.ruleset_id, "script_murder.basic")
        self.assertEqual(state.room.max_players, 5)
        self.assertEqual(self._lock_value(lock, "ruleset_id", "ruleset"), "script_murder.basic")
        self.assertEqual(int(self._lock_value(lock, "agent_count", "agents_count")), 2)
        self.assertEqual({item["display_name"] for item in pool}, {"侦探林澈", "管家周叔"})
        self.assertEqual({item["role_key"] for item in pool}, {"detective", "witness"})
        self.assertNotIn("mutated-after-create", {item["display_name"] for item in pool})
        self.assertEqual(state.room.config["scene"], "旧宅书房")

    async def test_add_agents_rejects_locked_room_after_creation(self):
        service = A2AGameService(FakeAgentService())
        state = service.create_room(uid=1001, title="locked", max_players=4, agent_count=1)

        before_count = len(state.participants)
        with self.assertRaises(ValueError) as raised:
            service.add_agents(state.room.room_id, agent_count=1)

        self.assertIn("lock", str(raised.exception).lower())
        self.assertEqual(len(service.get_state(state.room.room_id, uid=1001).participants), before_count)

    async def test_create_room_rejects_initial_agents_over_capacity(self):
        service = A2AGameService(FakeAgentService())

        with self.assertRaises(ValueError):
            service.create_room(uid=1001, title="too many", max_players=2, agent_count=2)

    async def test_role_presets_are_exposed_for_builtin_rulesets(self):
        service = A2AGameService(FakeAgentService())

        werewolf = service.list_role_presets("werewolf.basic")
        script = service.list_role_presets("script_murder.basic")

        self.assertIn("werewolf", {item["role_key"] for item in werewolf})
        self.assertIn("villager", {item["role_key"] for item in werewolf})
        self.assertIn("detective", {item["role_key"] for item in script})

    async def test_template_presets_are_exposed_and_filter_by_ruleset(self):
        service = A2AGameService(FakeAgentService())

        all_presets = service.list_template_presets()
        werewolf = service.list_template_presets("werewolf.basic")
        script = service.list_template_presets("script_murder.basic")

        self.assertGreaterEqual(len(all_presets), 3)
        self.assertTrue(all(item.metadata["builtin"] for item in all_presets))
        self.assertTrue(all(item.uid == 0 for item in all_presets))
        self.assertTrue(all(item.template_id.startswith("preset.") for item in all_presets))
        self.assertTrue(all(item.ruleset_id == "werewolf.basic" for item in werewolf))
        self.assertTrue(all(item.ruleset_id == "script_murder.basic" for item in script))
        self.assertIn("preset.werewolf.6p.classic", {item.template_id for item in werewolf})
        self.assertIn("preset.script_murder.5p.mansion", {item.template_id for item in script})

    async def test_template_presets_reject_unknown_ruleset(self):
        service = A2AGameService(FakeAgentService())

        with self.assertRaises(ValueError):
            service.list_template_presets("unknown.ruleset")

    async def test_clone_template_preset_saves_user_owned_template(self):
        store = FakeGameStateStore()
        service = A2AGameService(FakeAgentService(), store=store)

        template = await service.clone_template_preset(
            "preset.werewolf.6p.classic",
            uid=42,
            title="我的 6 人局",
        )

        self.assertNotEqual(template.template_id, "preset.werewolf.6p.classic")
        self.assertEqual(template.uid, 42)
        self.assertEqual(template.title, "我的 6 人局")
        self.assertEqual(template.ruleset_id, "werewolf.basic")
        self.assertEqual(template.max_players, 6)
        self.assertEqual(template.metadata["source"], "builtin_preset")
        self.assertEqual(template.metadata["preset_id"], "preset.werewolf.6p.classic")
        self.assertEqual(template.metadata["cloned_from_preset"], "preset.werewolf.6p.classic")
        self.assertFalse(template.metadata["builtin"])
        self.assertIn(template.template_id, store.templates)

    async def test_clone_template_preset_uses_preset_title_and_keeps_preset_immutable(self):
        store = FakeGameStateStore()
        service = A2AGameService(FakeAgentService(), store=store)
        before = service.list_template_presets("script_murder.basic")[0]

        template = await service.clone_template_preset(
            "preset.script_murder.5p.mansion",
            uid=7,
            title="",
        )
        template.agents[0]["display_name"] = "changed"
        after = service.list_template_presets("script_murder.basic")[0]

        self.assertEqual(template.title, before.title)
        self.assertEqual(template.uid, 7)
        self.assertEqual(after.agents[0]["display_name"], before.agents[0]["display_name"])
        self.assertEqual(after.metadata["builtin"], True)

    async def test_clone_template_preset_rejects_unknown_preset(self):
        service = A2AGameService(FakeAgentService())

        with self.assertRaises(ValueError):
            await service.clone_template_preset("preset.missing", uid=1)

    async def test_requested_agent_config_survives_randomized_role_assignment_on_start(self):
        fake = FakeAgentService(['{"action_type":"night_kill","target_participant_id":"","content":"夜晚行动。"}'])
        service = A2AGameService(fake)
        state = service.create_room(
            uid=1,
            title="randomized identity",
            agent_count=0,
            agents=[{"display_name": "自定义狼人", "role_key": "werewolf", "persona": "潜伏"}],
        )

        state = service.start_room(state.room.room_id, uid=1)
        agent = next(p for p in state.participants if p.kind == "agent")
        human = next(p for p in state.participants if p.kind == "human")

        self.assertEqual({agent.role_key, human.role_key}, {"werewolf", "villager"})
        self.assertEqual(agent.private_state["role_key"], agent.role_key)
        self.assertEqual(human.private_state["role_key"], human.role_key)
        self.assertEqual(agent.persona, "潜伏")

    async def test_host_events_are_added_for_create_and_start(self):
        service = A2AGameService(FakeAgentService())
        state = service.create_room(
            uid=1,
            title="hosted",
            agent_count=1,
            config={
                "host": {
                    "enabled": True,
                    "display_name": "法官",
                    "persona": "你是清晰的主持人。",
                }
            },
        )

        self.assertTrue(any(event.event_type == "host" and "法官" in event.content for event in state.events))

        state = service.start_room(state.room.room_id, uid=1)
        host_events = [event for event in state.events if event.event_type == "host"]

        self.assertGreaterEqual(len(host_events), 2)
        self.assertEqual(host_events[-1].payload["reason"], "start")
        self.assertIn("法官", host_events[-1].content)

    async def test_host_tick_can_use_model_summary_without_becoming_participant(self):
        fake = FakeAgentService(
            [
                '{"action_type":"skip","content":"我先跳过。"}',
                "主持总结：夜晚行动已结算，请进入讨论。",
            ]
        )
        service = A2AGameService(fake)
        state = service.create_room(
            uid=1,
            title="host tick",
            agent_count=2,
            config={
                "host": {
                    "enabled": True,
                    "display_name": "主持人",
                    "persona": "你是克制的法官。",
                    "model_type": "fake",
                    "model_name": "host-model",
                    "skill_name": "writer",
                }
            },
        )
        state.participants = [participant for participant in state.participants if participant.kind == "agent"]
        state = service.start_room(state.room.room_id, uid=0)

        tick = await service.tick_room(state.room.room_id, uid=1)
        host_events = [event for event in tick["state"].events if event.event_type == "host"]

        self.assertEqual(len([p for p in tick["state"].participants if p.kind == "agent"]), 2)
        self.assertTrue(any("主持总结" in event.content for event in host_events))
        self.assertEqual(fake.calls[-1].metadata["source"], "a2a_game_host")
        self.assertEqual(fake.calls[-1].model_name, "host-model")

    async def test_werewolf_start_and_agent_tick_apply_action(self):
        fake = FakeAgentService(['{"action_type":"night_kill","target_participant_id":"","content":"夜晚行动。"}'])
        service = A2AGameService(fake)
        state = service.create_room(uid=1, title="werewolf", agent_count=2)
        room_id = state.room.room_id

        state = service.start_room(room_id, uid=1)
        wolf = next(p for p in state.participants if p.role_key == "werewolf")

        tick = await service.tick_room(room_id, uid=1)

        if wolf.kind == "human":
            self.assertEqual(tick["status"], "waiting_human")
        else:
            self.assertEqual(tick["status"], "acted")
        self.assertEqual(tick["actor"]["participant_id"], wolf.participant_id)
        self.assertEqual(tick["graph"]["name"], "a2a_game_tick")
        self.assertIn(tick["graph"]["backend"], {"langgraph", "fallback"})
        self.assertEqual(tick["state"].graph["name"], "a2a_game_tick")

    async def test_restart_rejects_lobby_and_running_rooms(self):
        service = A2AGameService(FakeAgentService())
        state = service.create_room(uid=1, title="restart guard", agent_count=2)

        with self.assertRaises(ValueError):
            service.restart_room(state.room.room_id, uid=1)

        running = service.start_room(state.room.room_id, uid=1)
        self.assertEqual(running.room.status, "running")
        with self.assertRaises(ValueError):
            service.restart_room(state.room.room_id, uid=1)

    async def test_restart_preserves_room_fixed_config_and_resets_next_game(self):
        service = A2AGameService(FakeAgentService())
        state = service.create_room(
            uid=1,
            title="restart lifecycle",
            ruleset_id="werewolf.basic",
            max_players=4,
            agent_count=0,
            agents=[
                {"display_name": "潜伏狼人", "role_key": "werewolf", "persona": "低调"},
                {"display_name": "逻辑村民", "role_key": "villager", "persona": "推理"},
                {"display_name": "谨慎村民", "role_key": "villager", "persona": "观察"},
            ],
        )
        room_id = state.room.room_id
        started = service.start_room(room_id, uid=1)
        started_marker = self._shuffle_marker(started)
        old_lock = self._canonical(self._locked_config(started))
        old_count = len(started.participants)
        old_game_index = int(started.state.get("game_index", 0))
        self._force_ended(started)

        restarted = service.restart_room(room_id, uid=1)
        new_marker = self._shuffle_marker(restarted)

        self.assertEqual(restarted.room.room_id, room_id)
        self.assertEqual(restarted.room.ruleset_id, "werewolf.basic")
        self.assertEqual(restarted.room.max_players, 4)
        self.assertEqual(self._canonical(self._locked_config(restarted)), old_lock)
        self.assertEqual(len(restarted.participants), old_count)
        self.assertEqual(int(restarted.state["game_index"]), old_game_index + 1)
        self.assertEqual(restarted.room.status, "running")
        self.assertEqual(restarted.room.phase, "night")
        self.assertEqual(restarted.state["phase"], "night")
        self.assertTrue(all(participant.status == "alive" for participant in restarted.participants))
        self.assertTrue(all(participant.private_state.get("role_key") == participant.role_key for participant in restarted.participants))
        self.assertNotEqual(new_marker, started_marker)

    async def test_persistent_snapshot_round_trip_preserves_locked_config_and_restarts(self):
        store = FakeGameStateStore()
        service = A2AGameService(FakeAgentService(), store=store)
        await service.startup()
        state = service.create_room(
            uid=7,
            title="persistent lifecycle",
            agent_count=0,
            agents=[
                {"display_name": "冷静狼人", "role_key": "werewolf", "persona": "潜伏"},
                {"display_name": "逻辑村民", "role_key": "villager", "persona": "推理"},
            ],
        )
        room_id = state.room.room_id
        started = service.start_room(room_id, uid=7)
        old_lock = self._canonical(self._locked_config(started))
        old_marker = self._shuffle_marker(started)
        old_game_index = int(started.state.get("game_index", 0))
        old_count = len(started.participants)
        self._force_ended(started)
        await store.save_room(started)
        await service.shutdown()

        restored = A2AGameService(FakeAgentService(), store=store)
        await restored.startup()
        loaded = restored.get_state(room_id, uid=7)
        self.assertEqual(self._canonical(self._locked_config(loaded)), old_lock)
        self.assertEqual(len(loaded.participants), old_count)

        restarted = restored.restart_room(room_id, uid=7)
        self.assertEqual(restarted.room.room_id, room_id)
        self.assertEqual(self._canonical(self._locked_config(restarted)), old_lock)
        self.assertEqual(len(restarted.participants), old_count)
        self.assertEqual(int(restarted.state["game_index"]), old_game_index + 1)
        self.assertEqual(restarted.room.status, "running")
        self.assertNotEqual(self._shuffle_marker(restarted), old_marker)

    async def test_list_rooms_refreshes_from_persistent_store(self):
        store = FakeGameStateStore()
        service = A2AGameService(FakeAgentService(), store=store)
        state = service.create_room(
            uid=11,
            title="persistent list room",
            agent_count=0,
            agents=[{"display_name": "列表狼人", "role_key": "werewolf", "persona": "潜伏"}],
        )
        room_id = state.room.room_id
        await service.shutdown()

        restored = A2AGameService(FakeAgentService(), store=store)
        rooms = await restored.list_rooms(uid=11)

        self.assertIn(room_id, {room.room_id for room in rooms})

    async def test_agent_tick_falls_back_to_skip_for_invalid_output(self):
        fake = FakeAgentService(["not json"])
        service = A2AGameService(fake)
        state = service.create_room(uid=1, title="fallback", agent_count=2)
        state.participants = [participant for participant in state.participants if participant.kind == "agent"]
        room_id = state.room.room_id
        state = service.start_room(room_id, uid=0)
        agent = next(p for p in state.participants if p.role_key == "werewolf")

        tick = await service.tick_room(room_id, uid=1)
        event_types = [event.event_type for event in tick["state"].events]

        self.assertEqual(tick["status"], "acted")
        self.assertEqual(tick["action"]["action_type"], "skip")
        self.assertEqual(fake.calls[0].skill_name, agent.skill_name)
        self.assertIn("agent_error", event_types)
        self.assertIn("skip", event_types)
        self.assertEqual(tick["graph"]["name"], "a2a_game_tick")

    async def test_auto_tick_stops_when_waiting_for_human(self):
        service = A2AGameService(FakeAgentService())
        state = service.create_room(uid=1, title="auto human", agent_count=0)
        state = service.start_room(state.room.room_id, uid=1)

        result = await service.auto_tick_room(state.room.room_id, uid=1, max_steps=5)

        self.assertTrue(result["auto"])
        self.assertEqual(result["status"], "waiting_human")
        self.assertEqual(result["stop_reason"], "waiting_human")
        self.assertEqual(result["steps"], 1)
        self.assertEqual(len(result["step_results"]), 1)
        self.assertIn("state", result)

    async def test_auto_tick_runs_multiple_agent_steps_until_limit(self):
        fake = FakeAgentService(
            [
                '{"action_type":"speak","content":"我先复盘时间线。"}',
                '{"action_type":"speak","content":"我补充一个线索。"}',
            ]
        )
        service = A2AGameService(fake)
        state = service.create_room(
            uid=1,
            title="auto agents",
            ruleset_id="script_murder.basic",
            max_players=3,
            agents=[
                {"display_name": "侦探A", "role_key": "detective"},
                {"display_name": "嫌疑人B", "role_key": "suspect"},
            ],
        )
        state.participants = [participant for participant in state.participants if participant.kind == "agent"]
        state = service.start_room(state.room.room_id, uid=0)

        result = await service.auto_tick_room(state.room.room_id, uid=0, max_steps=2)

        self.assertTrue(result["auto"])
        self.assertEqual(result["steps"], 2)
        self.assertEqual(len(result["step_results"]), 2)
        self.assertEqual(len(fake.calls), 2)
        self.assertEqual(result["stop_reason"], "max_steps")

    async def test_human_join_and_public_view_hides_other_private_roles(self):
        service = A2AGameService(FakeAgentService())
        state = service.create_room(uid=1, title="roles", agent_count=1)
        state = service.join_room(state.room.room_id, uid=2, display_name="二号玩家")
        state = service.start_room(state.room.room_id, uid=1)

        view = state.to_dict(viewer_uid=2)
        self_item = next(p for p in view["participants"] if p["uid"] == 2)
        other_items = [p for p in view["participants"] if p["uid"] != 2]

        self.assertNotEqual(self_item["role_key"], "")
        self.assertTrue(all(p["role_key"] == "" for p in other_items))

    async def test_human_action_requires_matching_uid(self):
        service = A2AGameService(FakeAgentService())
        state = service.create_room(uid=1, title="ownership", agent_count=1)
        state = service.start_room(state.room.room_id, uid=1)
        human = next(p for p in state.participants if p.kind == "human")

        with self.assertRaises(ValueError):
            service.submit_action(
                room_id=state.room.room_id,
                participant_id=human.participant_id,
                action_type="skip",
                uid=999,
            )

    async def test_script_murder_ruleset_supports_clues_and_accusations(self):
        service = A2AGameService(FakeAgentService())
        state = service.create_room(
            uid=1,
            title="古宅疑云",
            ruleset_id="script_murder.basic",
            agent_count=1,
        )
        room_id = state.room.room_id
        human = next(p for p in state.participants if p.kind == "human")
        agent = next(p for p in state.participants if p.kind == "agent")

        state = service.start_room(room_id, uid=1)
        self.assertEqual(state.room.phase, "free_talk")

        state = service.submit_action(
            room_id=room_id,
            participant_id=human.participant_id,
            action_type="search_clue",
            content="抽屉里有一张被撕开的票据。",
            uid=1,
        )
        state = service.submit_action(
            room_id=room_id,
            participant_id=human.participant_id,
            action_type="accuse",
            target_participant_id=agent.participant_id,
            content="我怀疑这个时间线有问题。",
            uid=1,
        )

        self.assertEqual(state.state["clues"][0]["content"], "抽屉里有一张被撕开的票据。")
        self.assertEqual(state.state["accusations"][human.participant_id], agent.participant_id)

    async def test_persistent_store_restores_room_snapshot(self):
        store = FakeGameStateStore()
        service = A2AGameService(FakeAgentService(), store=store)
        await service.startup()

        state = service.create_room(
            uid=7,
            title="persistent room",
            agent_count=0,
            agents=[{"display_name": "持久狼人", "role_key": "werewolf", "persona": "潜伏"}],
        )
        room_id = state.room.room_id
        await service.shutdown()

        restored = A2AGameService(FakeAgentService(), store=store)
        await restored.startup()
        loaded = restored.get_state(room_id, uid=7)

        self.assertEqual(loaded.room.title, "persistent room")
        self.assertEqual(len(loaded.participants), 2)
        agent = next(p for p in loaded.participants if p.kind == "agent")
        self.assertEqual(agent.display_name, "持久狼人")
        self.assertEqual(agent.persona, "潜伏")
        self.assertTrue(any(event.event_type == "room_created" for event in loaded.events))

        started = restored.start_room(room_id, uid=7)
        await restored.shutdown()
        self.assertEqual(store.snapshots[room_id]["room"]["phase"], started.room.phase)

    async def test_save_template_persists_contract_fields(self):
        store = FakeGameStateStore()
        service = A2AGameService(FakeAgentService(), store=store)

        template = await service.save_template(
            uid=42,
            title="午夜会议",
            description="自定义狼人杀模板",
            ruleset_id="werewolf.basic",
            max_players=12,
            agents=[
                {
                    "display_name": "冷静狼人",
                    "role_key": "werewolf",
                    "persona": "低调发言",
                    "skill_name": "writer",
                    "metadata": {"seat": 1},
                }
            ],
            config={"night_seconds": 30},
            metadata={"source": "unit"},
        )

        self.assertTrue(template.template_id)
        self.assertEqual(template.uid, 42)
        self.assertEqual(template.title, "午夜会议")
        self.assertEqual(template.description, "自定义狼人杀模板")
        self.assertEqual(template.ruleset_id, "werewolf.basic")
        self.assertEqual(template.max_players, 12)
        self.assertEqual(template.agents[0]["display_name"], "冷静狼人")
        self.assertEqual(template.config["night_seconds"], 30)
        self.assertEqual(template.metadata["source"], "unit")
        self.assertGreater(template.created_at, 0)
        self.assertGreaterEqual(template.updated_at, template.created_at)
        self.assertIn(template.template_id, store.templates)

    async def test_saved_template_reloads_from_store(self):
        store = FakeGameStateStore()
        service = A2AGameService(FakeAgentService(), store=store)
        template = await service.save_template(
            uid=7,
            title="可重载模板",
            ruleset_id="script_murder.basic",
            max_players=6,
            agents=[{"display_name": "侦探", "role_key": "detective"}],
            config={"scene": "古宅"},
        )

        restored = A2AGameService(FakeAgentService(), store=store)
        templates = await restored.list_templates(uid=7)

        self.assertEqual([item.template_id for item in templates], [template.template_id])
        loaded = templates[0]
        self.assertEqual(loaded.title, "可重载模板")
        self.assertEqual(loaded.ruleset_id, "script_murder.basic")
        self.assertEqual(loaded.agents[0]["role_key"], "detective")
        self.assertEqual(loaded.config["scene"], "古宅")

    async def test_create_room_from_template_reuses_agents_config_and_ruleset(self):
        store = FakeGameStateStore()
        service = A2AGameService(FakeAgentService(), store=store)
        template = await service.save_template(
            uid=5,
            title="模板标题",
            description="",
            ruleset_id="script_murder.basic",
            max_players=5,
            agents=[
                {"display_name": "嫌疑人A", "role_key": "suspect", "persona": "紧张"},
                {"display_name": "目击者B", "role_key": "witness", "persona": "谨慎"},
            ],
            config={"scene": "书房"},
            metadata={"pinned": True},
        )

        state = await service.create_room_from_template(template.template_id, uid=5, title="")
        agents = [participant for participant in state.participants if participant.kind == "agent"]

        self.assertEqual(state.room.title, "模板标题")
        self.assertEqual(state.room.ruleset_id, "script_murder.basic")
        self.assertEqual(state.room.max_players, 5)
        self.assertEqual(state.room.config["scene"], "书房")
        self.assertEqual(state.room.metadata["template_id"], template.template_id)
        self.assertEqual([agent.display_name for agent in agents], ["嫌疑人A", "目击者B"])
        self.assertTrue(all(agent.role_key for agent in agents))
        self.assertTrue(any(agent.role_key in {"suspect", "witness"} for agent in agents))


if __name__ == "__main__":
    unittest.main()
