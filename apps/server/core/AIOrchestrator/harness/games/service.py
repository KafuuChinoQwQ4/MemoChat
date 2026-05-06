from __future__ import annotations

import asyncio
import copy
import json
import random
import uuid
from types import SimpleNamespace
from typing import Any

from harness.games.contracts import GameAction, GameEvent, GameParticipant, GameRoom, GameState, GameTemplate
from harness.games.participants import GameParticipantFactory, now_ms
from harness.games.rules import GameRuleEngine, builtin_rulesets
from harness.games.store import GameStateStore, NoopGameStateStore

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

try:
    from langgraph.graph import END, StateGraph
except Exception:
    END = None
    StateGraph = None

logger = structlog.get_logger()


ROLE_PRESETS: dict[str, list[dict[str, Any]]] = {
    "werewolf.basic": [
        {
            "role_key": "werewolf",
            "display_name": "狼人",
            "description": "夜晚选择目标，白天隐藏身份并带偏投票。",
            "persona": "你是狼人。发言克制，保护自己，制造合理怀疑。",
            "strategy": "deceptive",
            "min_count": 1,
        },
        {
            "role_key": "villager",
            "display_name": "村民",
            "description": "通过发言和投票找出狼人。",
            "persona": "你是村民。基于时间线和发言矛盾做判断。",
            "strategy": "deductive",
            "min_count": 1,
        },
    ],
    "script_murder.basic": [
        {
            "role_key": "detective",
            "display_name": "侦探",
            "description": "主持调查，梳理线索和嫌疑链。",
            "persona": "你是侦探。追问细节，整理证词矛盾。",
            "strategy": "investigative",
            "min_count": 1,
        },
        {
            "role_key": "suspect",
            "display_name": "嫌疑人",
            "description": "提供自洽证词，也可能隐瞒关键事实。",
            "persona": "你是嫌疑人。保持角色设定，避免直接暴露秘密。",
            "strategy": "roleplay",
            "min_count": 1,
        },
        {
            "role_key": "witness",
            "display_name": "目击者",
            "description": "掌握局部线索，推动讨论。",
            "persona": "你是目击者。只透露你能证明的片段。",
            "strategy": "observant",
            "min_count": 1,
        },
        {
            "role_key": "culprit",
            "display_name": "真凶",
            "description": "隐藏动机，误导调查方向。",
            "persona": "你是真凶。用细节伪装自己，但不要跳出剧本。",
            "strategy": "deceptive",
            "min_count": 0,
        },
    ],
}


TEMPLATE_PRESETS: list[GameTemplate] = [
    GameTemplate(
        template_id="preset.werewolf.6p.classic",
        uid=0,
        title="狼人杀 6 人经典局",
        description="1 名真人玩家搭配 5 名 AI 玩家，适合快速体验发言、夜晚行动和投票闭环。",
        ruleset_id="werewolf.basic",
        max_players=6,
        agents=[
            {
                "display_name": "冷静狼人",
                "role_key": "werewolf",
                "persona": "你是狼人。白天保持冷静，制造合理怀疑，夜晚果断选择目标。",
                "skill_name": "writer",
                "strategy": "deceptive",
            },
            {
                "display_name": "逻辑村民",
                "role_key": "villager",
                "persona": "你是村民。用时间线、票型和发言矛盾推进判断。",
                "skill_name": "writer",
                "strategy": "deductive",
            },
            {
                "display_name": "谨慎村民",
                "role_key": "villager",
                "persona": "你是村民。少量发言但关注异常细节。",
                "skill_name": "writer",
                "strategy": "observant",
            },
            {
                "display_name": "强势村民",
                "role_key": "villager",
                "persona": "你是村民。主动归纳焦点，推动大家表态。",
                "skill_name": "writer",
                "strategy": "assertive",
            },
            {
                "display_name": "摇摆村民",
                "role_key": "villager",
                "persona": "你是村民。容易被说服，但会追问关键漏洞。",
                "skill_name": "writer",
                "strategy": "balanced",
            },
        ],
        config={"night_seconds": 30, "day_seconds": 90, "vote_seconds": 45},
        metadata={"builtin": True, "preset_id": "preset.werewolf.6p.classic"},
        created_at=0,
        updated_at=0,
    ),
    GameTemplate(
        template_id="preset.werewolf.8p.argument",
        uid=0,
        title="狼人杀 8 人推理局",
        description="更偏重白天辩论的 8 人局，包含两名狼人和多种村民发言风格。",
        ruleset_id="werewolf.basic",
        max_players=8,
        agents=[
            {
                "display_name": "潜伏狼人",
                "role_key": "werewolf",
                "persona": "你是狼人。避免过早冲锋，优先攻击逻辑链薄弱的人。",
                "skill_name": "writer",
                "strategy": "deceptive",
            },
            {
                "display_name": "冲锋狼人",
                "role_key": "werewolf",
                "persona": "你是狼人。积极发言带节奏，但保留退路。",
                "skill_name": "writer",
                "strategy": "aggressive",
            },
            {
                "display_name": "票型分析师",
                "role_key": "villager",
                "persona": "你是村民。重点分析投票、跟票和弃票动机。",
                "skill_name": "writer",
                "strategy": "deductive",
            },
            {
                "display_name": "时间线记录员",
                "role_key": "villager",
                "persona": "你是村民。记录每轮发言顺序和立场变化。",
                "skill_name": "writer",
                "strategy": "observant",
            },
            {
                "display_name": "质询者",
                "role_key": "villager",
                "persona": "你是村民。用短问题逼迫可疑玩家补充细节。",
                "skill_name": "writer",
                "strategy": "assertive",
            },
            {
                "display_name": "保守村民",
                "role_key": "villager",
                "persona": "你是村民。倾向等待证据，但会指出明显矛盾。",
                "skill_name": "writer",
                "strategy": "balanced",
            },
            {
                "display_name": "直觉村民",
                "role_key": "villager",
                "persona": "你是村民。会说出直觉判断，并愿意被证据修正。",
                "skill_name": "writer",
                "strategy": "intuitive",
            },
        ],
        config={"night_seconds": 35, "day_seconds": 120, "vote_seconds": 60},
        metadata={"builtin": True, "preset_id": "preset.werewolf.8p.argument"},
        created_at=0,
        updated_at=0,
    ),
    GameTemplate(
        template_id="preset.script_murder.5p.mansion",
        uid=0,
        title="剧本杀 5 人古宅疑云",
        description="1 名真人玩家与 4 名 AI 角色围绕古宅线索、证词矛盾和最终指认真凶。",
        ruleset_id="script_murder.basic",
        max_players=5,
        agents=[
            {
                "display_name": "侦探林澈",
                "role_key": "detective",
                "persona": "你是侦探。冷静追问细节，整理每个人的时间线。",
                "skill_name": "writer",
                "strategy": "investigative",
            },
            {
                "display_name": "继承人顾遥",
                "role_key": "suspect",
                "persona": "你是继承人。你有隐瞒的财产纠纷，但不想被误认为凶手。",
                "skill_name": "writer",
                "strategy": "roleplay",
            },
            {
                "display_name": "管家周叔",
                "role_key": "witness",
                "persona": "你是管家。你熟悉古宅动线，只透露亲眼确认的事实。",
                "skill_name": "writer",
                "strategy": "observant",
            },
            {
                "display_name": "医生许岚",
                "role_key": "culprit",
                "persona": "你是真凶。用专业知识掩饰破绽，避免直接撒太离谱的谎。",
                "skill_name": "writer",
                "strategy": "deceptive",
            },
        ],
        config={
            "scene": "暴雨夜的旧宅书房",
            "opening_clue": "书桌抽屉里有一张被撕开的火车票。",
            "discussion_seconds": 180,
        },
        metadata={"builtin": True, "preset_id": "preset.script_murder.5p.mansion"},
        created_at=0,
        updated_at=0,
    ),
]


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
        self._tick_graph = self._build_tick_graph()

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
        return [
            {
                "ruleset_id": ruleset_id,
                "display_name": "狼人杀" if ruleset_id == "werewolf.basic" else "剧本杀",
                "description": (
                    "Deterministic Werewolf social deduction MVP."
                    if ruleset_id == "werewolf.basic"
                    else "Script murder roleplay MVP with clues and accusations."
                ),
            }
            for ruleset_id in sorted(self._rulesets)
        ]

    def list_role_presets(self, ruleset_id: str = "") -> list[dict[str, Any]]:
        ruleset_id = (ruleset_id or "werewolf.basic").strip()
        self._engine(ruleset_id)
        return [dict(item) for item in ROLE_PRESETS.get(ruleset_id, [])]

    def list_template_presets(self, ruleset_id: str = "") -> list[GameTemplate]:
        ruleset_id = (ruleset_id or "").strip()
        if ruleset_id:
            self._engine(ruleset_id)
        return [
            GameTemplate.from_dict(copy.deepcopy(preset.to_dict()))
            for preset in TEMPLATE_PRESETS
            if not ruleset_id or preset.ruleset_id == ruleset_id
        ]

    async def clone_template_preset(self, preset_id: str, uid: int, title: str = "") -> GameTemplate:
        preset_id = (preset_id or "").strip()
        preset = next((item for item in TEMPLATE_PRESETS if item.template_id == preset_id), None)
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

    async def create_room_from_template(self, template_id: str, uid: int, title: str = "") -> GameState:
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
    ) -> GameState:
        engine = self._engine(ruleset_id)
        now = now_ms()
        agent_configs = self._participants.agent_configs(agent_count, agents)
        room_config = self._locked_room_config(
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
        participants = [
            self._participants.create_human(
                room.room_id,
                uid=uid,
            )
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
        self._append_host_event(state, "房间已创建。请确认角色与模型配置，准备好后开始游戏。", reason="create_room")
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
        if self._is_locked_room(state):
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

    def start_room(self, room_id: str, uid: int = 0) -> GameState:
        state = self._state(room_id)
        self._ensure_game_markers(state)
        starter = self._participant_for_uid_or_first(state, uid)
        events = self._engine(state.room.ruleset_id).apply_action(
            state,
            GameAction(participant_id=starter.participant_id, action_type="start"),
        )
        state.events.extend(events)
        self._append_host_event(state, self._host_line_for_state(state, "start"), reason="start")
        self._touch(state)
        self._refresh_view(state, uid)
        self._schedule_save(state)
        return state

    def restart_room(self, room_id: str, uid: int = 0) -> GameState:
        previous = self._state(room_id)
        if previous.room.status != "ended" and previous.room.phase != "ended":
            raise ValueError("game room can only restart after it has ended")

        locked_config = self._locked_config(previous)
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

        participants = self._reset_participants_for_restart(previous.participants, locked_config, shuffle_marker)
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
        self._append_host_event(restarted, self._host_line_for_state(restarted, "restart"), reason="restart")
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
        self._append_host_event(state, self._host_line_for_state(state, action.action_type), reason=action.action_type)
        self._touch(state)
        self._refresh_view(state, uid)
        self._schedule_save(state)
        return state

    async def tick_room(self, room_id: str, uid: int = 0) -> dict[str, Any]:
        context: dict[str, Any] = {"room_id": room_id, "uid": uid, "graph_steps": []}
        if self._tick_graph is not None:
            try:
                context = await self._tick_graph.ainvoke(context)
            except Exception as exc:
                context["graph_error"] = f"{type(exc).__name__}: {exc}"
                context = await self._run_tick_fallback_graph(context)
        else:
            context = await self._run_tick_fallback_graph(context)

        state = context["state"]
        engine = context["engine"]
        actor_id = context["actor_id"]
        graph_metadata = self._graph_metadata(context)
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
        ai_action, trace_id, raw_content, parsed = await self._ask_agent_for_action(state, actor, allowed)
        if not parsed or ai_action.action_type not in allowed:
            fallback = "skip" if "skip" in allowed else (allowed[0] if allowed else "")
            state.events.append(
                self._event(
                    state,
                    "agent_error",
                    f"{actor.display_name} 返回了不可用行动，已降级为 {fallback or 'idle'}。",
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

        await self._append_host_tick_event(state, actor, ai_action, tick_status)
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

    def _build_tick_graph(self):
        if StateGraph is None or END is None:
            return None
        graph = StateGraph(dict)
        graph.add_node("load_state", self._graph_load_state)
        graph.add_node("select_actor", self._graph_select_actor)
        graph.add_edge("load_state", "select_actor")
        graph.add_edge("select_actor", END)
        graph.set_entry_point("load_state")
        return graph.compile()

    async def _run_tick_fallback_graph(self, context: dict[str, Any]) -> dict[str, Any]:
        context = await self._graph_load_state(context)
        return await self._graph_select_actor(context)

    async def _graph_load_state(self, context: dict[str, Any]) -> dict[str, Any]:
        state = self._state(context["room_id"])
        return {**context, "state": state, "engine": self._engine(state.room.ruleset_id), "graph_steps": [*context.get("graph_steps", []), "load_state"]}

    async def _graph_select_actor(self, context: dict[str, Any]) -> dict[str, Any]:
        engine = context["engine"]
        actor_id = engine.next_pending_actor(context["state"])
        return {**context, "actor_id": actor_id, "graph_steps": [*context.get("graph_steps", []), "select_actor"]}

    def _graph_metadata(self, context: dict[str, Any]) -> dict[str, Any]:
        backend = "langgraph" if self._tick_graph is not None and not context.get("graph_error") else "fallback"
        return {
            "name": "a2a_game_tick",
            "backend": backend,
            "available": self._tick_graph is not None,
            "nodes": list(context.get("graph_steps", [])),
            "actor_id": context.get("actor_id", ""),
            "error": context.get("graph_error", ""),
        }

    async def _ask_agent_for_action(self, state: GameState, actor: GameParticipant, allowed: list[str]) -> tuple[GameAction, str, str, bool]:
        target_hint = self._first_alive_target(state, actor.participant_id)
        prompt = {
            "task": "You are an AI participant in a social deduction game. Return only JSON.",
            "json_schema": {
                "action_type": allowed,
                "content": "short in-character public message",
                "target_participant_id": "optional participant id",
                "payload": "object",
            },
            "ruleset_id": state.room.ruleset_id,
            "room": state.room.to_dict(),
            "self": actor.to_dict(),
            "allowed_actions": allowed,
            "suggested_target_participant_id": target_hint,
            "public_participants": [p.to_dict(include_private=False) for p in state.participants],
            "public_timeline": [event.to_dict() for event in state.events if event.visibility == "public"][-12:],
            "persona": actor.persona,
        }
        request = SimpleNamespace(
            uid=actor.uid,
            session_id=f"game-{state.room.room_id}-{actor.participant_id}",
            content=json.dumps(prompt, ensure_ascii=False),
            model_type=actor.model_type,
            model_name=actor.model_name,
            deployment_preference="any",
            skill_name=actor.skill_name or "writer",
            feature_type="",
            target_lang="",
            requested_tools=[],
            tool_arguments={},
            metadata={"source": "a2a_game", "room_id": state.room.room_id, "participant_id": actor.participant_id, "max_tokens": 512, "temperature": 0.6},
        )
        try:
            result = await self._agent_service.run_turn(request)
            raw = result.content or ""
            trace_id = result.trace_id
            payload = self._extract_json(raw)
        except Exception as exc:
            raw = f"{type(exc).__name__}: {exc}"
            trace_id = ""
            payload = {}

        return (
            GameAction(
                participant_id=actor.participant_id,
                action_type=str(payload.get("action_type") or "skip"),
                content=str(payload.get("content") or ""),
                target_participant_id=str(payload.get("target_participant_id") or target_hint or ""),
                payload=payload.get("payload") if isinstance(payload.get("payload"), dict) else {},
            ),
            trace_id,
            raw,
            bool(payload),
        )

    def _extract_json(self, content: str) -> dict[str, Any]:
        text = (content or "").strip()
        if not text:
            return {}
        try:
            data = json.loads(text)
        except json.JSONDecodeError:
            start = text.find("{")
            end = text.rfind("}")
            if start < 0 or end <= start:
                return {}
            try:
                data = json.loads(text[start : end + 1])
            except json.JSONDecodeError:
                return {}
        return data if isinstance(data, dict) else {}

    def _locked_room_config(
        self,
        config: dict[str, Any] | None,
        ruleset_id: str,
        max_players: int,
        agent_configs: list[Any],
    ) -> dict[str, Any]:
        room_config = copy.deepcopy(config or {})
        agent_preset_pool = [config_item.to_dict() for config_item in agent_configs]
        room_config["locked"] = True
        room_config["locked_config"] = {
            "locked": True,
            "ruleset_id": ruleset_id,
            "max_players": max(1, int(max_players or 12)),
            "agent_count": len(agent_preset_pool),
            "agent_preset_pool": agent_preset_pool,
        }
        return room_config

    def _is_locked_room(self, state: GameState) -> bool:
        if not isinstance(state.room.config, dict):
            return False
        locked_config = state.room.config.get("locked_config")
        return bool(state.room.config.get("locked") or isinstance(locked_config, dict) and locked_config.get("locked", True))

    def _locked_config(self, state: GameState) -> dict[str, Any]:
        raw = state.room.config.get("locked_config") if isinstance(state.room.config, dict) else None
        if isinstance(raw, dict):
            return copy.deepcopy(raw)
        agent_preset_pool = [
            self._agent_preset_from_participant(participant)
            for participant in state.participants
            if participant.kind == "agent"
        ]
        return {
            "locked": False,
            "ruleset_id": state.room.ruleset_id,
            "max_players": state.room.max_players,
            "agent_count": len(agent_preset_pool),
            "agent_preset_pool": agent_preset_pool,
        }

    def _agent_preset_from_participant(self, participant: GameParticipant) -> dict[str, Any]:
        metadata = dict(participant.metadata or {})
        strategy = str(metadata.pop("strategy", "balanced") or "balanced")
        metadata.pop("preset_binding_index", None)
        metadata.pop("preset_shuffle_marker", None)
        return {
            "display_name": participant.display_name,
            "model_type": participant.model_type,
            "model_name": participant.model_name,
            "persona": participant.persona,
            "skill_name": participant.skill_name,
            "strategy": strategy,
            "role_key": participant.role_key,
            "metadata": metadata,
        }

    def _reset_participants_for_restart(
        self,
        participants: list[GameParticipant],
        locked_config: dict[str, Any],
        shuffle_marker: str,
    ) -> list[GameParticipant]:
        agent_preset_pool = [
            dict(item)
            for item in locked_config.get("agent_preset_pool", [])
            if isinstance(item, dict)
        ]
        agents = [participant for participant in participants if participant.kind == "agent"]
        if len(agent_preset_pool) != len(agents):
            raise ValueError("locked agent preset pool does not match room participants")

        rng = random.Random(shuffle_marker)
        shuffled_presets = copy.deepcopy(agent_preset_pool)
        rng.shuffle(shuffled_presets)

        for participant in participants:
            participant.status = "alive"
            participant.role_key = ""
            participant.private_state = {}
            if participant.kind != "agent":
                continue
            preset = shuffled_presets.pop(0)
            self._apply_agent_preset(participant, preset, shuffle_marker)
        return participants

    def _apply_agent_preset(self, participant: GameParticipant, preset: dict[str, Any], shuffle_marker: str) -> None:
        participant.display_name = str(preset.get("display_name") or preset.get("name") or participant.display_name or "AI玩家")
        participant.model_type = str(preset.get("model_type") or "")
        participant.model_name = str(preset.get("model_name") or "")
        participant.persona = str(preset.get("persona") or "")
        participant.skill_name = str(preset.get("skill_name") or "writer")
        participant.role_key = str(preset.get("role_key") or "")
        metadata = preset.get("metadata") if isinstance(preset.get("metadata"), dict) else {}
        participant.metadata = {
            "strategy": str(preset.get("strategy") or "balanced"),
            **metadata,
            "preset_shuffle_marker": shuffle_marker,
        }

    def _ensure_game_markers(self, state: GameState) -> None:
        if not state.state.get("game_index"):
            state.state["game_index"] = 1
        marker = str(state.state.get("shuffle_marker") or state.room.metadata.get("shuffle_marker") or "")
        if not marker:
            marker = uuid.uuid4().hex
            state.state["shuffle_marker"] = marker
        state.room.metadata = {**state.room.metadata, "shuffle_marker": marker}

    def _host_config(self, state: GameState) -> dict[str, Any]:
        raw = state.room.config.get("host") if isinstance(state.room.config, dict) else {}
        if not isinstance(raw, dict):
            return {"enabled": False}
        enabled = bool(raw.get("enabled", False))
        return {
            "enabled": enabled,
            "display_name": str(raw.get("display_name") or "酒馆主持人"),
            "persona": str(raw.get("persona") or "你是游戏主持人。用简短、清晰、克制的中文主持当前阶段。"),
            "model_type": str(raw.get("model_type") or ""),
            "model_name": str(raw.get("model_name") or ""),
            "skill_name": str(raw.get("skill_name") or "writer"),
        }

    def _host_line_for_state(self, state: GameState, reason: str = "") -> str:
        phase = state.room.phase
        if phase == "lobby":
            return "请确认座位与角色设定，准备好后开始游戏。"
        if phase == "night":
            return "天黑请闭眼。夜晚行动开始，相关角色请依次行动。"
        if phase == "day_discussion":
            return "天亮了。请根据昨夜结果和发言线索展开讨论。"
        if phase == "vote":
            return "讨论结束，进入投票。请给出你的怀疑对象。"
        if phase == "free_talk":
            return "自由讨论开始。请围绕线索、动机和时间线发言。"
        if phase == "reveal":
            return "进入揭晓阶段。请整理关键证据并给出最终判断。"
        if phase == "ended":
            winner = state.state.get("winner", "")
            return f"游戏结束。胜利方：{winner}。" if winner else "游戏结束。"
        return f"当前阶段：{phase or '未知'}。请继续推进。"

    def _append_host_event(self, state: GameState, content: str, reason: str = "") -> None:
        host = self._host_config(state)
        if not host.get("enabled"):
            return
        display_name = host.get("display_name") or "酒馆主持人"
        line = str(content or self._host_line_for_state(state, reason)).strip()
        if not line:
            return
        state.events.append(
            self._event(
                state,
                "host",
                f"{display_name}: {line}",
                payload={"host": host, "reason": reason},
            )
        )

    async def _append_host_tick_event(self, state: GameState, actor: GameParticipant, action: GameAction, status: str) -> None:
        host = self._host_config(state)
        if not host.get("enabled"):
            return
        content = ""
        if host.get("model_type") and host.get("model_name"):
            content = await self._ask_host_for_line(state, host, actor, action, status)
        if not content:
            content = self._host_line_for_state(state, "tick")
        self._append_host_event(state, content, reason="tick")

    async def _ask_host_for_line(
        self,
        state: GameState,
        host: dict[str, Any],
        actor: GameParticipant,
        action: GameAction,
        status: str,
    ) -> str:
        prompt = {
            "task": "你是游戏主持人。请只输出一句中文主持词，60字以内，不泄露隐藏身份。",
            "ruleset_id": state.room.ruleset_id,
            "phase": state.room.phase,
            "round": state.room.round_index,
            "status": status,
            "actor": actor.to_dict(include_private=False),
            "action": action.to_dict(),
            "public_timeline": [event.to_dict() for event in state.events if event.visibility == "public"][-10:],
            "persona": host.get("persona", ""),
        }
        request = SimpleNamespace(
            uid=state.room.owner_uid,
            session_id=f"game-host-{state.room.room_id}",
            content=json.dumps(prompt, ensure_ascii=False),
            model_type=str(host.get("model_type") or ""),
            model_name=str(host.get("model_name") or ""),
            deployment_preference="any",
            skill_name=str(host.get("skill_name") or "writer"),
            feature_type="",
            target_lang="",
            requested_tools=[],
            tool_arguments={},
            metadata={"source": "a2a_game_host", "room_id": state.room.room_id, "max_tokens": 160, "temperature": 0.4},
        )
        try:
            result = await self._agent_service.run_turn(request)
        except Exception as exc:
            logger.warning("game.host.run_failed", room_id=state.room.room_id, error=str(exc))
            return ""
        return str(getattr(result, "content", "") or "").strip().replace("\n", " ")[:180]

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
