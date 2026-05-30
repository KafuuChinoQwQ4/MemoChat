from __future__ import annotations

import copy
import random
import uuid
from typing import Any

from harness.games.contracts import GameParticipant, GameState


def build_locked_room_config(
    config: dict[str, Any] | None,
    ruleset_id: str,
    max_players: int,
    agent_configs: list[Any],
) -> dict[str, Any]:
    room_config = copy.deepcopy(config or {})
    agent_preset_pool = [config_item.to_dict() for config_item in agent_configs]
    human_role_key = str(room_config.get("human_role_key") or "").strip()
    room_config["locked"] = True
    room_config["locked_config"] = {
        "locked": True,
        "ruleset_id": ruleset_id,
        "max_players": max(1, int(max_players or 12)),
        "human_role_key": human_role_key,
        "agent_count": len(agent_preset_pool),
        "agent_preset_pool": agent_preset_pool,
    }
    return room_config


def is_locked_room(state: GameState) -> bool:
    if not isinstance(state.room.config, dict):
        return False
    locked_config = state.room.config.get("locked_config")
    return bool(
        state.room.config.get("locked") or isinstance(locked_config, dict) and locked_config.get("locked", True)
    )


def get_locked_config(state: GameState) -> dict[str, Any]:
    raw = state.room.config.get("locked_config") if isinstance(state.room.config, dict) else None
    if isinstance(raw, dict):
        return copy.deepcopy(raw)
    agent_preset_pool = [
        _agent_preset_from_participant(participant) for participant in state.participants if participant.kind == "agent"
    ]
    human = next((participant for participant in state.participants if participant.kind == "human"), None)
    return {
        "locked": False,
        "ruleset_id": state.room.ruleset_id,
        "max_players": state.room.max_players,
        "human_role_key": human.role_key if human is not None else "",
        "agent_count": len(agent_preset_pool),
        "agent_preset_pool": agent_preset_pool,
    }


def sync_locked_agent_preset(state: GameState, participant: GameParticipant) -> None:
    if not isinstance(state.room.config, dict):
        return
    locked_config = state.room.config.get("locked_config")
    if not isinstance(locked_config, dict):
        return
    pool = locked_config.get("agent_preset_pool")
    if not isinstance(pool, list):
        return

    metadata = dict(participant.metadata or {})
    strategy = str(metadata.get("strategy") or "balanced")
    match = next(
        (
            item
            for item in pool
            if isinstance(item, dict)
            and (
                item.get("display_name") == participant.display_name
                or item.get("name") == participant.display_name
                or item.get("persona") == participant.persona
            )
        ),
        None,
    )
    if match is None:
        agents = [p for p in state.participants if p.kind == "agent"]
        try:
            match = pool[agents.index(participant)]
        except (ValueError, IndexError):
            match = None
    if not isinstance(match, dict):
        return
    match["display_name"] = participant.display_name
    match["persona"] = participant.persona
    match["skill_name"] = participant.skill_name
    match["strategy"] = strategy


def reset_participants_for_restart(
    participants: list[GameParticipant],
    locked_config: dict[str, Any],
    shuffle_marker: str,
) -> list[GameParticipant]:
    agent_preset_pool = [dict(item) for item in locked_config.get("agent_preset_pool", []) if isinstance(item, dict)]
    agents = [participant for participant in participants if participant.kind == "agent"]
    if len(agent_preset_pool) != len(agents):
        raise ValueError("locked agent preset pool does not match room participants")

    rng = random.Random(shuffle_marker)
    shuffled_presets = copy.deepcopy(agent_preset_pool)
    rng.shuffle(shuffled_presets)
    human_role_key = str(locked_config.get("human_role_key") or "").strip()

    for participant in participants:
        participant.status = "alive"
        participant.role_key = ""
        participant.private_state = {}
        if participant.kind == "human" and human_role_key:
            participant.role_key = human_role_key
            participant.private_state["role_key"] = human_role_key
        if participant.kind != "agent":
            continue
        preset = shuffled_presets.pop(0)
        _apply_agent_preset(participant, preset, shuffle_marker)
    return participants


def ensure_game_markers(state: GameState) -> None:
    if not state.state.get("game_index"):
        state.state["game_index"] = 1
    marker = str(state.state.get("shuffle_marker") or state.room.metadata.get("shuffle_marker") or "")
    if not marker:
        marker = uuid.uuid4().hex
        state.state["shuffle_marker"] = marker
    state.room.metadata = {**state.room.metadata, "shuffle_marker": marker}


def _agent_preset_from_participant(participant: GameParticipant) -> dict[str, Any]:
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


def _apply_agent_preset(participant: GameParticipant, preset: dict[str, Any], shuffle_marker: str) -> None:
    participant.display_name = str(
        preset.get("display_name") or preset.get("name") or participant.display_name or "AI玩家"
    )
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
