from __future__ import annotations

import asyncio
import copy
import uuid
from typing import Any

from harness.games.agent_actions import AgentActionService
from harness.games.contracts import GameAction, GameEvent, GameParticipant, GameRoom, GameState, GameTemplate
from harness.games.host import GameHostService
from harness.games.participants import GameParticipantFactory, now_ms
from harness.games.presets import find_template_preset, list_role_presets, list_template_presets
from harness.games.room_lifecycle import (
    build_locked_room_config,
    ensure_game_markers,
    get_locked_config,
    is_locked_room,
    reset_participants_for_restart,
    sync_locked_agent_preset,
)
from harness.games.rules import GameRuleEngine, builtin_rulesets
from harness.games.store import GameStateStore, NoopGameStateStore
from harness.games.tick_graph import GameTickGraph

try:
    import structlog
except ModuleNotFoundError:
    class _FallbackLogger:
        def info(self, *args, **kwargs):
            return None

        def warning(self, *args, **kwargs):
            return None

    class _StructlogFallback:
        def get_logger(self):
            return _FallbackLogger()

    structlog = _StructlogFallback()

logger = structlog.get_logger()


class A2AGameService:
    def __init__(
        self,
        agent_service,
        rulesets: dict[str, GameRuleEngine] | None = None,
        store: GameStateStore | None = None,
    ):
        self._agent_service = agent_service
        self._rulesets = rulesets or builtin_rulesets()
        self._store = store or NoopGameStateStore()
        self._rooms: dict[str, GameState] = {}
        self._templates: dict[str, GameTemplate] = {}
        self._participants = GameParticipantFactory()
        self._save_tasks: set[asyncio.Task] = set()
        self._host = GameHostService(agent_service, self._event, logger)
        self._agent_actions = AgentActionService(agent_service, self._participant_public_name, self._first_alive_target)
        self._tick_graph = GameTickGraph(self._state, self._engine)

    async def startup(self) -> None:
        try:
            await self._store.startup()
            rooms = await self._store.load_rooms()
        except Exception as exc:
            logger.warning("game.persistence.startup_failed", error=str(exc))
            return
        loaded = 0
        for state in rooms:
            if state.room.ruleset_id not in self._rulesets:
                logger.warning("game.persistence.unknown_ruleset_skipped", room_id=state.room.room_id, ruleset_id=state.room.ruleset_id)
                continue
            self._refresh_view(state)
            self._rooms[state.room.room_id] = state
            loaded += 1
        logger.info("game.persistence.rooms_loaded", count=loaded)

    async def shutdown(self) -> None:
        await self._flush_save_tasks()

    async def _flush_save_tasks(self) -> None:
        if not self._save_tasks:
            return
        await asyncio.gather(*list(self._save_tasks), return_exceptions=True)

    async def _sync_rooms_from_store(self) -> None:
        try:
            rooms = await self._store.load_rooms()
        except Exception as exc:
            logger.warning("game.persistence.refresh_failed", error=str(exc))
            return
        for state in rooms:
            if state.room.ruleset_id not in self._rulesets:
                continue
            cached = self._rooms.get(state.room.room_id)
            if cached is not None and cached.room.updated_at > state.room.updated_at:
                continue
            self._refresh_view(state)
            self._rooms[state.room.room_id] = state

    async def ensure_room_loaded(self, room_id: str) -> GameState:
        room_id = (room_id or "").strip()
        if not room_id:
            raise ValueError("game room not found")
        cached = self._rooms.get(room_id)
        if cached is not None:
            return cached
        await self._sync_rooms_from_store()
        cached = self._rooms.get(room_id)
        if cached is None:
            raise ValueError(f"game room not found: {room_id}")
        return cached

    def list_rulesets(self) -> list[dict[str, Any]]:
        labels = {
            "multi_ai_chat.test": (
                "多 AI 对话测试",
                "Temporary user + multiple AI group chat for basic runtime testing.",
            ),
            "werewolf.basic": (
                "狼人杀",
                "Deterministic Werewolf social deduction MVP.",
            ),
            "script_murder.basic": (
                "剧本杀",
                "Script murder roleplay MVP with clues and accusations.",
            ),
        }
        return [
            {
                "ruleset_id": ruleset_id,
                "display_name": labels.get(ruleset_id, (ruleset_id, ""))[0],
                "description": labels.get(ruleset_id, (ruleset_id, ""))[1],
            }
            for ruleset_id in sorted(self._rulesets)
        ]

    def list_role_presets(self, ruleset_id: str = "") -> list[dict[str, Any]]:
        ruleset_id = (ruleset_id or "werewolf.basic").strip()
        self._engine(ruleset_id)
        return list_role_presets(ruleset_id)

    def list_template_presets(self, ruleset_id: str = "") -> list[GameTemplate]:
        ruleset_id = (ruleset_id or "").strip()
        if ruleset_id:
            self._engine(ruleset_id)
        return list_template_presets(ruleset_id)

    async def clone_template_preset(self, preset_id: str, uid: int, title: str = "") -> GameTemplate:
        preset_id = (preset_id or "").strip()
        preset = find_template_preset(preset_id)
        if preset is None:
            raise ValueError(f"game template preset not found: {preset_id}")
        metadata = {
            **copy.deepcopy(preset.metadata),
            "builtin": False,
            "source": "builtin_preset",
            "preset_id": preset.template_id,
            "cloned_from_preset": preset.template_id,
        }
        return await self.save_template(
            uid=uid,
            title=(title or "").strip() or preset.title,
            description=preset.description,
            ruleset_id=preset.ruleset_id,
            max_players=preset.max_players,
            agents=copy.deepcopy(preset.agents),
            config=copy.deepcopy(preset.config),
            metadata=metadata,
        )

    async def list_templates(self, uid: int) -> list[GameTemplate]:
        try:
            templates = await self._store.list_templates(uid)
        except Exception as exc:
            logger.warning("game.template.list_failed", uid=uid, error=str(exc))
            templates = []
        merged: dict[str, GameTemplate] = {template.template_id: template for template in templates}
        for template in self._templates.values():
            if uid <= 0 or template.uid == uid:
                merged[template.template_id] = template
        return sorted(merged.values(), key=lambda template: template.updated_at, reverse=True)

    async def save_template(
        self,
        uid: int,
        title: str,
        description: str = "",
        ruleset_id: str = "werewolf.basic",
        max_players: int = 12,
        agents: list[dict[str, Any]] | None = None,
        config: dict[str, Any] | None = None,
        metadata: dict[str, Any] | None = None,
        template_id: str = "",
    ) -> GameTemplate:
        ruleset_id = (ruleset_id or "werewolf.basic").strip()
        self._engine(ruleset_id)
        template_id = (template_id or "").strip() or uuid.uuid4().hex
        now = now_ms()
        existing = await self._get_template(template_id, 0)
        if existing is not None and existing.uid != int(uid or 0):
            raise ValueError("template_id belongs to another uid")
        created_at = existing.created_at if existing is not None and existing.created_at else now
        template = GameTemplate(
            template_id=template_id,
            uid=int(uid or 0),
            title=(title or "").strip() or "A2A Game Template",
            description=str(description or ""),
            ruleset_id=ruleset_id,
            max_players=max(1, int(max_players or 12)),
            agents=[agent.to_dict() for agent in self._participants.agent_configs(0, agents)],
            config=dict(config or {}),
            metadata=dict(metadata or {}),
            created_at=created_at,
            updated_at=max(now, existing.updated_at if existing is not None else 0),
        )
        await self._store.save_template(template)
        self._templates[template.template_id] = template
        return template

    async def delete_template(self, template_id: str, uid: int) -> bool:
        template_id = (template_id or "").strip()
        if not template_id:
            raise ValueError("template_id is required")
        template = await self._get_template(template_id, uid)
        if template is None:
            return False
        deleted_at = now_ms()
        await self._store.delete_template(template_id, uid, deleted_at)
        self._templates.pop(template_id, None)
        return True

    async def create_room_from_template(self, template_id: str, uid: int, title: str = "", display_name: str = "") -> GameState:
        template = await self._get_template(template_id, uid)
        if template is None:
            raise ValueError(f"game template not found: {template_id}")
        state = self.create_room(
            uid=uid,
            title=(title or "").strip() or template.title,
            ruleset_id=template.ruleset_id,
            max_players=template.max_players,
            agent_count=0,
            agents=[dict(agent) for agent in template.agents],
            config=dict(template.config),
            display_name=display_name,
        )
        state.room.metadata = {
            **state.room.metadata,
            "template_id": template.template_id,
            "template_title": template.title,
        }
        self._touch(state)
        self._refresh_view(state, uid)
        self._schedule_save(state)
        return state

    def create_room(
        self,
        uid: int,
        title: str,
        ruleset_id: str = "werewolf.basic",
        max_players: int = 12,
        agent_count: int = 0,
        agents: list[dict[str, Any]] | None = None,
        config: dict[str, Any] | None = None,
        display_name: str = "",
    ) -> GameState:
        engine = self._engine(ruleset_id)
        now = now_ms()
        agent_configs = self._participants.agent_configs(agent_count, agents)
        room_config = build_locked_room_config(
            config,
            ruleset_id=ruleset_id,
            max_players=max(1, int(max_players or 12)),
            agent_configs=agent_configs,
        )
        shuffle_marker = uuid.uuid4().hex
        room = GameRoom(
            room_id=uuid.uuid4().hex,
            owner_uid=uid,
            title=(title or "").strip() or "A2A Game",
            ruleset_id=ruleset_id,
            max_players=max(1, int(max_players or 12)),
            config=room_config,
            created_at=now,
            updated_at=now,
            metadata={"shuffle_marker": shuffle_marker},
        )
        human = self._participants.create_human(
            room.room_id,
            uid=uid,
            display_name=display_name,
        )
        human_role_key = str(room_config.get("human_role_key") or "").strip()
        if human_role_key:
            human.role_key = human_role_key
            human.private_state["role_key"] = human_role_key
        participants = [
            human
        ]
        for config_item in agent_configs:
            if len(participants) >= room.max_players:
                raise ValueError("initial participants exceed max_players")
            participants.append(self._participants.create_agent(room.room_id, config_item))

        state = engine.create_initial_state(room, participants)
        state.state["game_index"] = 1
        state.state["shuffle_marker"] = shuffle_marker
        state.events.append(self._event(state, "room_created", f"{room.title} 已创建。"))
        state.events.extend(
            self._event(state, "participant_joined", f"{participant.display_name} 加入房间。", participant.participant_id)
            for participant in participants
        )
        create_line = (
            "测试对话房间已创建。可以直接在右侧对话区发送消息。"
            if ruleset_id == "multi_ai_chat.test"
            else "房间已创建。请确认角色与模型配置，准备好后开始游戏。"
        )
        self._host.append_event(state, create_line, reason="create_room")
        self._refresh_view(state, uid)
        self._rooms[room.room_id] = state
        self._schedule_save(state)
        return state

    async def list_rooms(self, uid: int = 0) -> list[GameRoom]:
        await self._sync_rooms_from_store()
        rooms = [state.room for state in self._rooms.values()]
        if uid > 0:
            rooms = [
                room
                for room in rooms
                if room.owner_uid == uid or any(p.uid == uid for p in self._rooms[room.room_id].participants)
            ]
        return sorted(rooms, key=lambda room: room.updated_at, reverse=True)

    async def delete_room(self, room_id: str, uid: int = 0) -> bool:
        room_id = (room_id or "").strip()
        if not room_id:
            raise ValueError("room_id is required")
        try:
            state = await self.ensure_room_loaded(room_id)
        except ValueError:
            return False
        if uid > 0 and state.room.owner_uid != uid and not any(p.uid == uid for p in state.participants):
            return False
        await self._flush_save_tasks()
        deleted_at = max(now_ms(), int(state.room.updated_at or 0) + 1)
        await self._store.delete_room(room_id, deleted_at)
        self._rooms.pop(room_id, None)
        return True

    def get_state(self, room_id: str, uid: int = 0) -> GameState:
        state = self._state(room_id)
        self._refresh_view(state, uid)
        return state

    def join_room(self, room_id: str, uid: int, display_name: str = "") -> GameState:
        state = self._state(room_id)
        existing = next((p for p in state.participants if p.uid == uid and p.kind == "human"), None)
        if existing is None:
            self._ensure_capacity(state)
            participant = self._participants.create_human(
                room_id,
                uid=uid,
                display_name=display_name,
            )
            state.participants.append(participant)
            state.events.append(self._event(state, "participant_joined", f"{participant.display_name} 加入房间。", participant.participant_id))
        self._touch(state)
        self._refresh_view(state, uid)
        self._schedule_save(state)
        return state

    def add_agents(self, room_id: str, agent_count: int = 1, agents: list[dict[str, Any]] | None = None) -> GameState:
        state = self._state(room_id)
        if is_locked_room(state):
            raise ValueError("game room config is locked; agents cannot be added")
        for config_item in self._participants.agent_configs(agent_count, agents):
            self._ensure_capacity(state)
            participant = self._participants.create_agent(room_id, config_item)
            state.participants.append(participant)
            state.events.append(self._event(state, "participant_joined", f"{participant.display_name} 加入房间。", participant.participant_id))
        self._touch(state)
        self._refresh_view(state)
        self._schedule_save(state)
        return state

    def update_participant(
        self,
        room_id: str,
        participant_id: str,
        uid: int = 0,
        display_name: str = "",
        persona: str = "",
        strategy: str = "",
        skill_name: str = "",
    ) -> GameState:
        state = self._state(room_id)
        participant = self._participant(state, participant_id)
        if participant is None:
            raise ValueError("participant not found")
        if participant.kind != "agent":
            raise ValueError("only agent participants can be edited")
        if uid > 0 and state.room.owner_uid != uid and not any(p.uid == uid for p in state.participants):
            raise ValueError("uid cannot edit this room")

        next_display_name = (display_name or "").strip()
        if next_display_name:
            participant.display_name = next_display_name
        participant.persona = str(persona or "")
        if skill_name.strip():
            participant.skill_name = skill_name.strip()
        metadata = dict(participant.metadata or {})
        if strategy.strip():
            metadata["strategy"] = strategy.strip()
        participant.metadata = metadata
        sync_locked_agent_preset(state, participant)
        self._touch(state)
        self._refresh_view(state, uid)
        self._schedule_save(state)
        return state

    def start_room(self, room_id: str, uid: int = 0) -> GameState:
        state = self._state(room_id)
        if state.room.ruleset_id == "multi_ai_chat.test":
            state.room.status = "running"
            state.room.phase = "chat"
            state.state["phase"] = "chat"
            self._host.append_event(state, "测试对话已就绪。", reason="start")
            self._touch(state)
            self._refresh_view(state, uid)
            self._schedule_save(state)
            return state
        ensure_game_markers(state)
        starter = self._participant_for_uid_or_first(state, uid)
        events = self._engine(state.room.ruleset_id).apply_action(
            state,
            GameAction(participant_id=starter.participant_id, action_type="start"),
        )
        state.events.extend(events)
        self._host.append_event(state, self._host.line_for_state(state, "start"), reason="start")
        self._touch(state)
        self._refresh_view(state, uid)
        self._schedule_save(state)
        return state

    def restart_room(self, room_id: str, uid: int = 0) -> GameState:
        previous = self._state(room_id)
        if previous.room.status != "ended" and previous.room.phase != "ended":
            raise ValueError("game room can only restart after it has ended")

        locked_config = get_locked_config(previous)
        locked_ruleset = str(locked_config.get("ruleset_id") or previous.room.ruleset_id)
        locked_max_players = int(locked_config.get("max_players") or previous.room.max_players)
        locked_agent_count = int(
            locked_config.get("agent_count")
            or len(locked_config.get("agent_preset_pool", []))
            or 0
        )
        current_agent_count = len([participant for participant in previous.participants if participant.kind == "agent"])
        if locked_agent_count != current_agent_count:
            raise ValueError("locked agent count does not match room participants")

        room = previous.room
        room.ruleset_id = locked_ruleset
        room.max_players = locked_max_players
        room.status = "lobby"
        room.phase = "lobby"
        room.round_index = 0
        room.ended_at = 0

        game_index = max(1, int(previous.state.get("game_index") or 1)) + 1
        shuffle_marker = uuid.uuid4().hex
        room.metadata = {**room.metadata, "shuffle_marker": shuffle_marker}

        participants = reset_participants_for_restart(previous.participants, locked_config, shuffle_marker)
        restarted = self._engine(room.ruleset_id).create_initial_state(room, participants)
        restarted.events = list(previous.events)
        restarted.graph = {}
        restarted.state["game_index"] = game_index
        restarted.state["shuffle_marker"] = shuffle_marker
        restarted.events.append(
            self._event(
                restarted,
                "game_restarted",
                f"第 {game_index} 局重新开始，身份与 AI 预设已重新洗牌。",
                payload={
                    "game_index": game_index,
                    "shuffle_marker": shuffle_marker,
                    "agent_count": locked_agent_count,
                },
            )
        )

        self._rooms[room.room_id] = restarted
        starter = self._participant_for_uid_or_first(restarted, uid)
        restarted.events.extend(
            self._engine(room.ruleset_id).apply_action(
                restarted,
                GameAction(participant_id=starter.participant_id, action_type="start"),
            )
        )
        self._host.append_event(restarted, self._host.line_for_state(restarted, "restart"), reason="restart")
        self._touch(restarted)
        self._refresh_view(restarted, uid)
        self._schedule_save(restarted)
        return restarted

    def submit_action(
        self,
        room_id: str,
        participant_id: str,
        action_type: str,
        content: str = "",
        target_participant_id: str = "",
        payload: dict[str, Any] | None = None,
        uid: int = 0,
    ) -> GameState:
        state = self._state(room_id)
        actor = self._participant(state, participant_id)
        if actor is None:
            raise ValueError("participant not found")
        if actor.kind == "human" and uid > 0 and actor.uid != uid:
            raise ValueError("uid does not match human participant")
        action = GameAction(
            participant_id=participant_id,
            action_type=action_type,
            content=content,
            target_participant_id=target_participant_id,
            payload=dict(payload or {}),
        )
        state.events.extend(self._engine(state.room.ruleset_id).apply_action(state, action))
        self._host.append_event(state, self._host.line_for_state(state, action.action_type), reason=action.action_type)
        self._touch(state)
        self._refresh_view(state, uid)
        self._schedule_save(state)
        return state

    async def tick_room(self, room_id: str, uid: int = 0) -> dict[str, Any]:
        context: dict[str, Any] = {"room_id": room_id, "uid": uid, "graph_steps": []}
        context = await self._tick_graph.run(context)
        state = context["state"]
        engine = context["engine"]
        actor_id = context["actor_id"]
        graph_metadata = self._tick_graph.metadata(context)
        state.graph = graph_metadata
        self._refresh_view(state, uid)
        if not actor_id:
            self._schedule_save(state)
            return {"status": "idle", "message": "no pending actor", "state": state, "graph": graph_metadata}
        actor = self._participant(state, actor_id)
        if actor is None:
            self._schedule_save(state)
            return {"status": "idle", "message": "pending actor missing", "state": state, "graph": graph_metadata}
        if actor.kind == "human":
            self._schedule_save(state)
            return {"status": "waiting_human", "message": f"waiting for {actor.display_name}", "actor": actor.to_dict(), "state": state, "graph": graph_metadata}

        allowed = engine.available_actions(state, actor.participant_id)
        ai_action, trace_id, raw_content, parsed = await self._agent_actions.ask_for_action(state, actor, allowed)
        if not parsed or ai_action.action_type not in allowed:
            fallback = "skip" if "skip" in allowed else (allowed[0] if allowed else "")
            state.events.append(
                self._event(
                    state,
                    "agent_error",
                    self._agent_actions.action_error_message(actor, raw_content, fallback),
                    actor.participant_id,
                    payload={
                        "raw": raw_content[:1000],
                        "trace_id": trace_id,
                        "requested_action": ai_action.action_type,
                        "parse_ok": parsed,
                    },
                )
            )
            if not fallback:
                self._touch(state)
                self._refresh_view(state, uid)
                self._schedule_save(state)
                return {"status": "agent_error", "message": "no fallback action", "actor": actor.to_dict(), "state": state, "graph": graph_metadata}
            ai_action.action_type = fallback
            ai_action.content = ai_action.content or "我暂时跳过。"
            ai_action.target_participant_id = ""

        try:
            state.events.extend(engine.apply_action(state, ai_action))
            tick_status = "acted"
            message = "agent action applied"
        except Exception as exc:
            state.events.append(
                self._event(
                    state,
                    "agent_error",
                    f"{actor.display_name} 行动失败: {type(exc).__name__}",
                    actor.participant_id,
                    payload={"error": str(exc), "raw": raw_content[:1000], "trace_id": trace_id},
                )
            )
            tick_status = "agent_error"
            message = str(exc)

        await self._host.append_tick_event(state, actor, ai_action, tick_status)
        self._touch(state)
        self._refresh_view(state, uid)
        self._schedule_save(state)
        return {
            "status": tick_status,
            "message": message,
            "actor": actor.to_dict(include_private=False),
            "action": ai_action.to_dict(),
            "trace_id": trace_id,
            "graph": graph_metadata,
            "state": state,
        }

    async def auto_tick_room(self, room_id: str, uid: int = 0, max_steps: int = 8) -> dict[str, Any]:
        max_steps = max(1, min(int(max_steps or 8), 32))
        step_results: list[dict[str, Any]] = []
        final_tick: dict[str, Any] | None = None
        stop_reason = "max_steps"

        for _ in range(max_steps):
            tick = await self.tick_room(room_id, uid)
            final_tick = tick
            state = tick["state"]
            status = str(tick.get("status") or "")
            step_results.append(
                {
                    "status": status,
                    "message": tick.get("message", ""),
                    "actor": tick.get("actor", {}),
                    "action": tick.get("action", {}),
                    "trace_id": tick.get("trace_id", ""),
                    "phase": state.room.phase,
                    "round_index": state.room.round_index,
                }
            )
            if state.room.status == "ended" or state.room.phase == "ended":
                stop_reason = "ended"
                break
            if status in {"waiting_human", "idle", "agent_error"}:
                stop_reason = status
                break

        if final_tick is None:
            state = self.get_state(room_id, uid)
            final_tick = {"status": "idle", "message": "no auto steps", "state": state, "graph": state.graph}
            stop_reason = "idle"

        final_tick = dict(final_tick)
        final_tick["auto"] = True
        final_tick["steps"] = len(step_results)
        final_tick["stop_reason"] = stop_reason
        final_tick["step_results"] = step_results
        return final_tick

    def _participant_public_name(self, state: GameState, participant_id: str) -> str:
        participant = self._participant(state, participant_id)
        return participant.display_name if participant is not None else participant_id

    def _event(self, state: GameState, event_type: str, content: str = "", actor: str = "", target: str = "", payload: dict[str, Any] | None = None) -> GameEvent:
        return GameEvent(
            event_id=uuid.uuid4().hex,
            room_id=state.room.room_id,
            event_type=event_type,
            content=content,
            actor_participant_id=actor,
            target_participant_id=target,
            phase=state.room.phase,
            round_index=state.room.round_index,
            payload=dict(payload or {}),
            created_at=now_ms(),
        )

    def _refresh_view(self, state: GameState, uid: int = 0) -> None:
        engine = self._engine(state.room.ruleset_id)
        state.pending_actor_id = engine.next_pending_actor(state)
        viewer = self._participant_for_uid(state, uid)
        if viewer is not None:
            state.available_actions = engine.available_actions(state, viewer.participant_id)
        elif state.pending_actor_id:
            state.available_actions = engine.available_actions(state, state.pending_actor_id)
        else:
            state.available_actions = []

    def _touch(self, state: GameState) -> None:
        now = now_ms()
        state.room.updated_at = now
        if state.room.status == "ended" and not state.room.ended_at:
            state.room.ended_at = now
        for participant in state.participants:
            participant.updated_at = now

    def _schedule_save(self, state: GameState) -> None:
        try:
            loop = asyncio.get_running_loop()
        except RuntimeError:
            return
        task = loop.create_task(self._save_state(state))
        self._save_tasks.add(task)
        task.add_done_callback(self._save_tasks.discard)

    async def _save_state(self, state: GameState) -> None:
        try:
            await self._store.save_room(state)
        except Exception as exc:
            logger.warning("game.persistence.save_failed", room_id=state.room.room_id, error=str(exc))

    async def _get_template(self, template_id: str, uid: int = 0) -> GameTemplate | None:
        template_id = (template_id or "").strip()
        if not template_id:
            return None
        cached = self._templates.get(template_id)
        if cached is not None and (uid <= 0 or cached.uid == uid):
            return cached
        try:
            template = await self._store.get_template(template_id, uid)
        except Exception as exc:
            logger.warning("game.template.get_failed", template_id=template_id, uid=uid, error=str(exc))
            return None
        if template is not None:
            self._templates[template.template_id] = template
        return template

    def _engine(self, ruleset_id: str) -> GameRuleEngine:
        engine = self._rulesets.get((ruleset_id or "").strip())
        if engine is None:
            raise ValueError(f"unknown ruleset: {ruleset_id}")
        return engine

    def _state(self, room_id: str) -> GameState:
        state = self._rooms.get((room_id or "").strip())
        if state is None:
            raise ValueError(f"game room not found: {room_id}")
        return state

    def _ensure_capacity(self, state: GameState) -> None:
        if len(state.participants) >= state.room.max_players:
            raise ValueError("game room is full")

    def _participant(self, state: GameState, participant_id: str) -> GameParticipant | None:
        return next((p for p in state.participants if p.participant_id == participant_id), None)

    def _participant_for_uid(self, state: GameState, uid: int) -> GameParticipant | None:
        return next((p for p in state.participants if uid > 0 and p.uid == uid), None)

    def _participant_for_uid_or_first(self, state: GameState, uid: int) -> GameParticipant:
        participant = self._participant_for_uid(state, uid) or (state.participants[0] if state.participants else None)
        if participant is None:
            raise ValueError("room has no participants")
        return participant

    def _first_alive_target(self, state: GameState, actor_id: str) -> str:
        target = next((p for p in state.participants if p.participant_id != actor_id and p.status == "alive"), None)
        return target.participant_id if target else ""
