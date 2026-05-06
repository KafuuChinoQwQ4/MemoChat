from __future__ import annotations

from dataclasses import asdict, dataclass, field
from typing import Any


@dataclass
class GameAgentConfig:
    display_name: str = ""
    model_type: str = ""
    model_name: str = ""
    persona: str = ""
    skill_name: str = ""
    strategy: str = "balanced"
    role_key: str = ""
    metadata: dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


@dataclass
class GameRoom:
    room_id: str
    owner_uid: int
    title: str
    ruleset_id: str
    status: str = "lobby"
    phase: str = "lobby"
    round_index: int = 0
    max_players: int = 12
    config: dict[str, Any] = field(default_factory=dict)
    created_at: int = 0
    updated_at: int = 0
    ended_at: int = 0
    metadata: dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


@dataclass
class GameTemplate:
    template_id: str
    uid: int
    title: str
    description: str = ""
    ruleset_id: str = "werewolf.basic"
    max_players: int = 12
    agents: list[dict[str, Any]] = field(default_factory=list)
    config: dict[str, Any] = field(default_factory=dict)
    metadata: dict[str, Any] = field(default_factory=dict)
    created_at: int = 0
    updated_at: int = 0

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)

    @classmethod
    def from_dict(cls, data: dict[str, Any]) -> "GameTemplate":
        return cls(**_known_fields(cls, data))


@dataclass
class GameParticipant:
    participant_id: str
    room_id: str
    kind: str
    display_name: str
    uid: int = 0
    model_type: str = ""
    model_name: str = ""
    persona: str = ""
    skill_name: str = ""
    role_key: str = ""
    status: str = "alive"
    private_state: dict[str, Any] = field(default_factory=dict)
    metadata: dict[str, Any] = field(default_factory=dict)
    created_at: int = 0
    updated_at: int = 0

    def to_dict(self, include_private: bool = True) -> dict[str, Any]:
        data = asdict(self)
        if not include_private:
            data["private_state"] = {}
            data["role_key"] = "" if self.kind != "self" else data["role_key"]
        return data


@dataclass
class GameAction:
    participant_id: str
    action_type: str
    content: str = ""
    target_participant_id: str = ""
    payload: dict[str, Any] = field(default_factory=dict)
    visibility: str = "public"

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


@dataclass
class GameEvent:
    event_id: str
    room_id: str
    event_type: str
    content: str = ""
    actor_participant_id: str = ""
    target_participant_id: str = ""
    phase: str = ""
    round_index: int = 0
    visibility: str = "public"
    payload: dict[str, Any] = field(default_factory=dict)
    created_at: int = 0

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


@dataclass
class GameState:
    room: GameRoom
    participants: list[GameParticipant] = field(default_factory=list)
    events: list[GameEvent] = field(default_factory=list)
    state: dict[str, Any] = field(default_factory=dict)
    available_actions: list[str] = field(default_factory=list)
    pending_actor_id: str = ""
    graph: dict[str, Any] = field(default_factory=dict)

    def to_dict(self, viewer_uid: int = 0) -> dict[str, Any]:
        visible_participants: list[dict[str, Any]] = []
        for participant in self.participants:
            is_self = participant.uid > 0 and participant.uid == viewer_uid
            item = participant.to_dict(include_private=is_self)
            if is_self:
                item["kind"] = "self"
            visible_participants.append(item)

        return {
            "room": self.room.to_dict(),
            "participants": visible_participants,
            "events": [
                event.to_dict()
                for event in self.events
                if event.visibility == "public"
                or event.actor_participant_id in {p.participant_id for p in self.participants if p.uid == viewer_uid}
            ],
            "state": dict(self.state),
            "available_actions": list(self.available_actions),
            "pending_actor_id": self.pending_actor_id,
            "graph": dict(self.graph),
        }

    def to_snapshot(self) -> dict[str, Any]:
        return {
            "room": self.room.to_dict(),
            "participants": [participant.to_dict(include_private=True) for participant in self.participants],
            "events": [event.to_dict() for event in self.events],
            "state": dict(self.state),
            "available_actions": list(self.available_actions),
            "pending_actor_id": self.pending_actor_id,
            "graph": dict(self.graph),
        }

    @classmethod
    def from_snapshot(cls, snapshot: dict[str, Any]) -> "GameState":
        room_data = snapshot.get("room") if isinstance(snapshot, dict) else {}
        if not isinstance(room_data, dict):
            raise ValueError("game state snapshot missing room")
        room = GameRoom(**_known_fields(GameRoom, room_data))

        participants: list[GameParticipant] = []
        for item in snapshot.get("participants", []):
            if isinstance(item, dict):
                participants.append(GameParticipant(**_known_fields(GameParticipant, item)))

        events: list[GameEvent] = []
        for item in snapshot.get("events", []):
            if isinstance(item, dict):
                events.append(GameEvent(**_known_fields(GameEvent, item)))

        state_data = snapshot.get("state", {})
        graph_data = snapshot.get("graph", {})
        available_actions = snapshot.get("available_actions", [])
        return cls(
            room=room,
            participants=participants,
            events=events,
            state=state_data if isinstance(state_data, dict) else {},
            available_actions=list(available_actions) if isinstance(available_actions, list) else [],
            pending_actor_id=str(snapshot.get("pending_actor_id") or ""),
            graph=graph_data if isinstance(graph_data, dict) else {},
        )


def _known_fields(model: type, data: dict[str, Any]) -> dict[str, Any]:
    names = getattr(model, "__dataclass_fields__", {})
    return {key: data[key] for key in names if key in data}
