from __future__ import annotations

import time
import uuid
from typing import Any

from harness.games.contracts import GameAgentConfig, GameParticipant


def now_ms() -> int:
    return int(time.time() * 1000)


class GameParticipantFactory:
    def agent_configs(self, agent_count: int = 0, agents: list[dict[str, Any]] | None = None) -> list[GameAgentConfig]:
        configs: list[GameAgentConfig] = []
        for index, item in enumerate(agents or []):
            configs.append(
                GameAgentConfig(
                    display_name=str(item.get("display_name") or item.get("name") or f"AI玩家{index + 1}"),
                    model_type=str(item.get("model_type") or ""),
                    model_name=str(item.get("model_name") or ""),
                    persona=str(item.get("persona") or ""),
                    skill_name=str(item.get("skill_name") or "writer"),
                    strategy=str(item.get("strategy") or "balanced"),
                    role_key=str(item.get("role_key") or ""),
                    metadata=item.get("metadata") if isinstance(item.get("metadata"), dict) else {},
                )
            )
        for _ in range(max(0, int(agent_count or 0))):
            configs.append(
                GameAgentConfig(
                    display_name=f"AI玩家{len(configs) + 1}",
                    persona="保持角色扮演，简洁发言。",
                    skill_name="writer",
                )
            )
        return configs

    def create_human(self, room_id: str, uid: int = 0, display_name: str = "") -> GameParticipant:
        return self.create_participant(
            room_id,
            kind="human",
            uid=uid,
            display_name=(display_name or "").strip() or f"玩家{uid}",
        )

    def create_participant(self, room_id: str, kind: str, display_name: str, uid: int = 0) -> GameParticipant:
        now = now_ms()
        return GameParticipant(
            participant_id=uuid.uuid4().hex,
            room_id=room_id,
            kind=kind,
            uid=uid,
            display_name=display_name,
            created_at=now,
            updated_at=now,
        )

    def create_agent(self, room_id: str, config: GameAgentConfig) -> GameParticipant:
        participant = self.create_participant(room_id, kind="agent", display_name=config.display_name or "AI玩家")
        participant.model_type = config.model_type
        participant.model_name = config.model_name
        participant.persona = config.persona
        participant.skill_name = config.skill_name
        participant.role_key = config.role_key
        if config.role_key:
            participant.private_state["role_key"] = config.role_key
        participant.metadata = {"strategy": config.strategy, **config.metadata}
        return participant
