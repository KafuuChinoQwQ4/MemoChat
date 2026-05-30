from __future__ import annotations

import json
from types import SimpleNamespace
from typing import Any, Callable

from harness.games.contracts import GameAction, GameEvent, GameParticipant, GameState

EventFactory = Callable[
    [GameState, str, str, str, str, dict[str, Any] | None],
    GameEvent,
]


class GameHostService:
    def __init__(self, agent_service, event_factory: EventFactory, logger):
        self._agent_service = agent_service
        self._event_factory = event_factory
        self._logger = logger

    def config(self, state: GameState) -> dict[str, Any]:
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

    def line_for_state(self, state: GameState, reason: str = "") -> str:
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
        if phase == "chat":
            return "测试对话进行中。"
        if phase == "reveal":
            return "进入揭晓阶段。请整理关键证据并给出最终判断。"
        if phase == "ended":
            winner = state.state.get("winner", "")
            return f"游戏结束。胜利方：{winner}。" if winner else "游戏结束。"
        return f"当前阶段：{phase or '未知'}。请继续推进。"

    def append_event(self, state: GameState, content: str, reason: str = "") -> None:
        host = self.config(state)
        if not host.get("enabled"):
            return
        display_name = host.get("display_name") or "酒馆主持人"
        line = str(content or self.line_for_state(state, reason)).strip()
        if not line:
            return
        state.events.append(
            self._event_factory(
                state,
                "host",
                f"{display_name}: {line}",
                "",
                "",
                {"host": host, "reason": reason},
            )
        )

    async def append_tick_event(
        self, state: GameState, actor: GameParticipant, action: GameAction, status: str
    ) -> None:
        host = self.config(state)
        if not host.get("enabled"):
            return
        content = ""
        if host.get("model_type") and host.get("model_name"):
            content = await self.ask_for_line(state, host, actor, action, status)
        if not content:
            content = self.line_for_state(state, "tick")
        self.append_event(state, content, reason="tick")

    async def ask_for_line(
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
            self._logger.warning("game.host.run_failed", room_id=state.room.room_id, error=str(exc))
            return ""
        return str(getattr(result, "content", "") or "").strip().replace("\n", " ")[:180]
