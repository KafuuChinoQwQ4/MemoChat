from __future__ import annotations

from dataclasses import asdict, dataclass, field
from typing import Any


@dataclass
class PetGaze:
    target: str = "user_face"
    x: float = 0.5
    y: float = 0.5

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


@dataclass
class PetLipSync:
    source: str = "scripted"
    value: float = 0.0

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


@dataclass
class PetSpeech:
    text_delta: str = ""
    audio_url: str | None = None
    audio_chunk_ref: str | None = None

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


@dataclass
class PetSafety:
    camera_used: bool = False
    vision_detail: str = "none"

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


@dataclass
class PetControlEvent:
    session_id: str
    seq: int
    timestamp_ms: int
    emotion: str = "neutral"
    intensity: float = 0.35
    motion: str = "idle"
    expression: str = "neutral"
    gaze: PetGaze = field(default_factory=PetGaze)
    lip_sync: PetLipSync = field(default_factory=PetLipSync)
    speech: PetSpeech = field(default_factory=PetSpeech)
    action: dict[str, Any] = field(default_factory=lambda: {"name": "", "args": {}})
    safety: PetSafety = field(default_factory=PetSafety)
    event_type: str = "pet.control"

    def to_dict(self) -> dict[str, Any]:
        data = asdict(self)
        data["type"] = data.pop("event_type")
        return data


@dataclass
class PetObservation:
    session_id: str
    audio: dict[str, Any] = field(default_factory=dict)
    vision: dict[str, Any] = field(default_factory=dict)
    privacy: dict[str, Any] = field(default_factory=dict)
    event_type: str = "pet.observation"

    @classmethod
    def from_dict(cls, data: dict[str, Any]) -> "PetObservation":
        return cls(
            session_id=str(data.get("session_id") or ""),
            audio=_dict_or_empty(data.get("audio")),
            vision=_dict_or_empty(data.get("vision")),
            privacy=_dict_or_empty(data.get("privacy")),
        )

    def to_dict(self) -> dict[str, Any]:
        data = asdict(self)
        data["type"] = data.pop("event_type")
        return data


@dataclass
class PetSession:
    session_id: str
    uid: int = 0
    profile_id: str = "default"
    persona: str = "memo-pet"
    provider: str = "scripted"
    created_at_ms: int = 0
    updated_at_ms: int = 0
    status: str = "active"

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


def _dict_or_empty(value: Any) -> dict[str, Any]:
    return dict(value) if isinstance(value, dict) else {}
