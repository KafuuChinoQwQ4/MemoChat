from __future__ import annotations

import random
import uuid
from abc import ABC, abstractmethod
from typing import Any

from harness.games.contracts import GameAction, GameEvent, GameParticipant, GameRoom, GameState


def _event(room: GameRoom, event_type: str, content: str = "", actor: str = "", target: str = "", payload: dict[str, Any] | None = None) -> GameEvent:
    return GameEvent(
        event_id=uuid.uuid4().hex,
        room_id=room.room_id,
        event_type=event_type,
        content=content,
        actor_participant_id=actor,
        target_participant_id=target,
        phase=room.phase,
        round_index=room.round_index,
        payload=dict(payload or {}),
    )


class GameRuleEngine(ABC):
    @abstractmethod
    def ruleset_id(self) -> str:
        raise NotImplementedError

    @abstractmethod
    def create_initial_state(self, room: GameRoom, participants: list[GameParticipant]) -> GameState:
        raise NotImplementedError

    @abstractmethod
    def available_actions(self, state: GameState, participant_id: str) -> list[str]:
        raise NotImplementedError

    @abstractmethod
    def apply_action(self, state: GameState, action: GameAction) -> list[GameEvent]:
        raise NotImplementedError

    @abstractmethod
    def next_pending_actor(self, state: GameState) -> str:
        raise NotImplementedError


class WerewolfRuleEngine(GameRuleEngine):
    def ruleset_id(self) -> str:
        return "werewolf.basic"

    def create_initial_state(self, room: GameRoom, participants: list[GameParticipant]) -> GameState:
        room.phase = "lobby"
        room.status = "lobby"
        return GameState(
            room=room,
            participants=participants,
            state={
                "ruleset": self.ruleset_id(),
                "phase": "lobby",
                "round": 0,
                "votes": {},
                "night_target": "",
                "day_speakers": [],
                "moderator_note": "Minimal Werewolf: start, night kill, day discussion, vote, victory.",
            },
        )

    def available_actions(self, state: GameState, participant_id: str) -> list[str]:
        participant = _participant(state, participant_id)
        if participant is None or participant.status != "alive":
            return []
        if state.room.phase == "lobby":
            return ["start"]
        if state.room.phase == "night":
            return ["night_kill", "skip"] if participant.role_key == "werewolf" else ["skip"]
        if state.room.phase == "day_discussion":
            return ["speak", "skip"]
        if state.room.phase == "vote":
            return ["vote", "skip"]
        return []

    def apply_action(self, state: GameState, action: GameAction) -> list[GameEvent]:
        allowed = self.available_actions(state, action.participant_id)
        if action.action_type not in allowed:
            raise ValueError(f"action {action.action_type} is not available")

        room = state.room
        actor = _participant(state, action.participant_id)
        if actor is None:
            raise ValueError("participant not found")

        if action.action_type == "start":
            self._assign_roles(state)
            room.status = "running"
            room.phase = "night"
            room.round_index = 1
            state.state.update({"phase": "night", "round": 1, "votes": {}, "night_target": "", "day_speakers": []})
            return [_event(room, "game_started", "游戏开始，进入夜晚。", actor.participant_id)]

        if action.action_type == "night_kill":
            target = _participant(state, action.target_participant_id)
            if target is None or target.status != "alive":
                raise ValueError("night target is not alive")
            target.status = "dead"
            room.phase = "day_discussion"
            state.state["phase"] = room.phase
            state.state["night_target"] = target.participant_id
            events = [_event(room, "night_kill", f"{target.display_name} 在夜晚出局。", actor.participant_id, target.participant_id)]
            events.extend(self._maybe_finish(state))
            return events

        if action.action_type == "speak":
            speakers = state.state.setdefault("day_speakers", [])
            if actor.participant_id not in speakers:
                speakers.append(actor.participant_id)
            if len(speakers) >= len(_alive(state)):
                room.phase = "vote"
                state.state["phase"] = room.phase
            return [_event(room, "speak", action.content or "我先观察。", actor.participant_id)]

        if action.action_type == "vote":
            target = _participant(state, action.target_participant_id)
            if target is None or target.status != "alive":
                raise ValueError("vote target is not alive")
            votes = state.state.setdefault("votes", {})
            votes[actor.participant_id] = target.participant_id
            events = [_event(room, "vote", f"{actor.display_name} 投票给 {target.display_name}。", actor.participant_id, target.participant_id)]
            if len(votes) >= len(_alive(state)):
                eliminated_id = _majority_target(votes)
                eliminated = _participant(state, eliminated_id)
                if eliminated is not None:
                    eliminated.status = "dead"
                    events.append(_event(room, "vote_result", f"{eliminated.display_name} 被投票出局。", "", eliminated.participant_id))
                room.phase = "night"
                room.round_index += 1
                state.state.update({"phase": "night", "round": room.round_index, "votes": {}, "night_target": "", "day_speakers": []})
                events.extend(self._maybe_finish(state))
            return events

        if action.action_type == "skip":
            if room.phase == "night" and actor.role_key == "werewolf":
                room.phase = "day_discussion"
                state.state["phase"] = room.phase
            return [_event(room, "skip", f"{actor.display_name} 跳过行动。", actor.participant_id)]

        raise ValueError(f"unsupported action {action.action_type}")

    def next_pending_actor(self, state: GameState) -> str:
        if state.room.phase == "lobby":
            return state.participants[0].participant_id if state.participants else ""
        alive = _alive(state)
        if state.room.phase == "night":
            wolf = next((p for p in alive if p.role_key == "werewolf"), None)
            return wolf.participant_id if wolf else ""
        if state.room.phase == "day_discussion":
            speakers = set(state.state.get("day_speakers", []))
            pending = next((p for p in alive if p.participant_id not in speakers), None)
            return pending.participant_id if pending else ""
        if state.room.phase == "vote":
            votes = state.state.get("votes", {})
            pending = next((p for p in alive if p.participant_id not in votes), None)
            return pending.participant_id if pending else ""
        return ""

    def _assign_roles(self, state: GameState) -> None:
        participants = list(state.participants)
        if not participants:
            return
        requested_wolf_count = len([participant for participant in participants if participant.role_key == "werewolf"])
        wolf_count = int(state.room.config.get("wolf_count", requested_wolf_count or 1) or 1)
        wolf_count = max(1, min(wolf_count, len(participants)))
        role_pool = ["werewolf"] * wolf_count + ["villager"] * (len(participants) - wolf_count)
        _role_rng(state, "werewolf").shuffle(role_pool)
        for participant, role_key in zip(participants, role_pool):
            participant.role_key = role_key
            participant.private_state["role_key"] = participant.role_key
        state.state["role_shuffle_marker"] = _shuffle_marker(state)

    def _maybe_finish(self, state: GameState) -> list[GameEvent]:
        alive = _alive(state)
        wolves = [p for p in alive if p.role_key == "werewolf"]
        villagers = [p for p in alive if p.role_key != "werewolf"]
        winner = ""
        if not wolves:
            winner = "villagers"
        elif len(wolves) >= len(villagers):
            winner = "werewolves"
        if not winner:
            return []
        state.room.phase = "ended"
        state.room.status = "ended"
        state.state["phase"] = "ended"
        state.state["winner"] = winner
        return [_event(state.room, "game_ended", f"游戏结束，{winner} 获胜。", payload={"winner": winner})]


class ScriptMurderRuleEngine(GameRuleEngine):
    def ruleset_id(self) -> str:
        return "script_murder.basic"

    def create_initial_state(self, room: GameRoom, participants: list[GameParticipant]) -> GameState:
        room.phase = "intro"
        room.status = "lobby"
        base_roles = ["detective", "suspect", "witness", "victim_friend", "culprit"]
        requested_roles = [participant.role_key for participant in participants if participant.role_key]
        roles = requested_roles + [base_roles[index % len(base_roles)] for index in range(len(participants))]
        roles = roles[: len(participants)]
        rng = random.Random(f"{room.room_id}:{room.metadata.get('shuffle_marker', '')}:script_murder")
        rng.shuffle(roles)
        for participant, role_key in zip(participants, roles):
            participant.role_key = role_key
            participant.private_state["role_key"] = participant.role_key
        return GameState(
            room=room,
            participants=participants,
            state={
                "ruleset": self.ruleset_id(),
                "phase": "intro",
                "round": 0,
                "spoken": [],
                "clues": [],
                "accusations": {},
            },
        )

    def available_actions(self, state: GameState, participant_id: str) -> list[str]:
        participant = _participant(state, participant_id)
        if participant is None:
            return []
        if state.room.phase == "intro":
            return ["start", "speak"]
        if state.room.phase == "free_talk":
            return ["speak", "search_clue", "accuse", "skip"]
        if state.room.phase == "reveal":
            return ["speak"]
        return []

    def apply_action(self, state: GameState, action: GameAction) -> list[GameEvent]:
        if action.action_type not in self.available_actions(state, action.participant_id):
            raise ValueError(f"action {action.action_type} is not available")
        room = state.room
        actor = _participant(state, action.participant_id)
        if actor is None:
            raise ValueError("participant not found")
        if action.action_type == "start":
            room.status = "running"
            room.phase = "free_talk"
            room.round_index = 1
            state.state.update({"phase": "free_talk", "round": 1})
            return [_event(room, "game_started", "剧本杀开场，进入自由讨论。", actor.participant_id)]
        if action.action_type == "search_clue":
            clue = action.content or f"{actor.display_name} 发现了一条线索。"
            state.state.setdefault("clues", []).append({"by": actor.participant_id, "content": clue})
            return [_event(room, "clue", clue, actor.participant_id)]
        if action.action_type == "accuse":
            state.state.setdefault("accusations", {})[actor.participant_id] = action.target_participant_id
            return [_event(room, "accuse", action.content or "我提出一个嫌疑人。", actor.participant_id, action.target_participant_id)]
        if action.action_type == "skip":
            return [_event(room, "skip", f"{actor.display_name} 暂不行动。", actor.participant_id)]
        spoken = state.state.setdefault("spoken", [])
        if actor.participant_id not in spoken:
            spoken.append(actor.participant_id)
        return [_event(room, "speak", action.content or "我补充一点信息。", actor.participant_id)]

    def next_pending_actor(self, state: GameState) -> str:
        if not state.participants:
            return ""
        spoken = set(state.state.get("spoken", []))
        pending = next((p for p in state.participants if p.participant_id not in spoken), None)
        return pending.participant_id if pending else state.participants[0].participant_id


def builtin_rulesets() -> dict[str, GameRuleEngine]:
    engines: list[GameRuleEngine] = [WerewolfRuleEngine(), ScriptMurderRuleEngine()]
    return {engine.ruleset_id(): engine for engine in engines}


def _participant(state: GameState, participant_id: str) -> GameParticipant | None:
    return next((p for p in state.participants if p.participant_id == participant_id), None)


def _alive(state: GameState) -> list[GameParticipant]:
    return [p for p in state.participants if p.status == "alive"]


def _majority_target(votes: dict[str, str]) -> str:
    counts: dict[str, int] = {}
    for target in votes.values():
        counts[target] = counts.get(target, 0) + 1
    return sorted(counts.items(), key=lambda item: (-item[1], item[0]))[0][0] if counts else ""


def _shuffle_marker(state: GameState) -> str:
    return str(state.state.get("shuffle_marker") or state.room.metadata.get("shuffle_marker") or "")


def _role_rng(state: GameState, purpose: str) -> random.Random:
    return random.Random(f"{state.room.room_id}:{state.state.get('game_index', 0)}:{_shuffle_marker(state)}:{purpose}")
