from __future__ import annotations

import uuid
from dataclasses import asdict, dataclass, field, is_dataclass
from typing import Any

SCHEMA_VERSION = 1


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
class PetAnimation:
    expression: str = "neutral"
    motion_group: str = "body"
    motion: str = "idle"
    priority: int = 1
    loop: bool = False
    fade_in_ms: int = 160
    fade_out_ms: int = 180
    parameters: dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


@dataclass
class PetAudio:
    state: str = "idle"
    rms: float = 0.0
    sample_rate: int = 0
    duration_ms: int = 0
    chunk_ref: str | None = None
    url: str | None = None
    phoneme: str | None = None
    viseme: str | None = None

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


@dataclass
class PetText:
    delta: str = ""
    final: bool = False
    language: str = "zh-CN"
    display: str = ""
    translation: str = ""

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


@dataclass
class PetVision:
    enabled: bool = False
    mode: str = "none"
    face_present: bool = False
    attention: str = ""
    expression: str = ""
    confidence: float = 0.0
    pose: dict[str, Any] = field(default_factory=dict)
    head_pose: dict[str, Any] = field(default_factory=dict)
    blendshapes: dict[str, Any] = field(default_factory=dict)
    scene: dict[str, Any] = field(default_factory=dict)
    objects: list[Any] = field(default_factory=list)
    gesture: str = ""
    source: str = ""
    frame: dict[str, Any] = field(default_factory=dict)
    client_frame: dict[str, Any] = field(default_factory=dict)
    frame_mime: str = ""
    captured_at_ms: int = 0
    changed_fields: list[str] = field(default_factory=list)
    stable_for_ms: int = 0
    state_hash: str = ""
    semantic_event: str = ""

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


@dataclass
class PetPrivacy:
    camera_used: bool = False
    mic_used: bool = False
    cloud_vision_used: bool = False
    raw_frame_sent: bool = False
    raw_audio_recorded: bool = False
    retention: str = "none"

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


@dataclass
class PetControlEvent:
    session_id: str
    seq: int
    timestamp_ms: int
    schema_version: int = SCHEMA_VERSION
    event_id: str = field(default_factory=lambda: f"petevt-{uuid.uuid4().hex}")
    turn_id: str = ""
    phase: str = "idle"
    emotion: str = "neutral"
    intensity: float = 0.35
    motion: str = "idle"
    expression: str = "neutral"
    animation: PetAnimation = field(default_factory=PetAnimation)
    audio: PetAudio = field(default_factory=PetAudio)
    text: PetText = field(default_factory=PetText)
    vision: PetVision = field(default_factory=PetVision)
    privacy: PetPrivacy = field(default_factory=PetPrivacy)
    debug: dict[str, Any] = field(default_factory=dict)
    gaze: PetGaze = field(default_factory=PetGaze)
    lip_sync: PetLipSync = field(default_factory=PetLipSync)
    speech: PetSpeech = field(default_factory=PetSpeech)
    action: dict[str, Any] = field(default_factory=lambda: {"name": "", "args": {}})
    safety: PetSafety = field(default_factory=PetSafety)
    event_type: str = "pet.control"

    def to_dict(self) -> dict[str, Any]:
        animation = {**asdict(PetAnimation()), **_plain_dict(self.animation)}
        audio = {**asdict(PetAudio()), **_plain_dict(self.audio)}
        text = {**asdict(PetText()), **_plain_dict(self.text)}
        vision = _sanitize_vision_payload({**asdict(PetVision()), **_plain_dict(self.vision)})
        privacy = {**asdict(PetPrivacy()), **_plain_dict(self.privacy)}

        motion = _preferred_text(self.motion, animation.get("motion"), "idle")
        expression = _preferred_text(self.expression, animation.get("expression"), "neutral")
        animation["motion"] = motion
        animation["expression"] = expression

        gaze = {**asdict(PetGaze()), **_plain_dict(self.gaze)}
        lip_sync = {**asdict(PetLipSync()), **_plain_dict(self.lip_sync)}
        speech = {**asdict(PetSpeech()), **_plain_dict(self.speech)}
        safety = {**asdict(PetSafety()), **_plain_dict(self.safety)}
        action = {"name": "", "args": {}, **_plain_dict(self.action)}

        audio["rms"] = _preferred_float(lip_sync.get("value"), audio.get("rms"), 0.0)
        audio["chunk_ref"] = speech.get("audio_chunk_ref") or audio.get("chunk_ref")
        audio["url"] = speech.get("audio_url") or audio.get("url")
        lip_sync["value"] = audio["rms"]

        text["delta"] = _preferred_text(speech.get("text_delta"), text.get("delta"), "")
        text["display"] = _preferred_text(text.get("display"), text.get("delta"), text["delta"])
        text["translation"] = str(text.get("translation") or speech.get("translation") or "")
        speech["text_delta"] = text["delta"]
        speech["audio_chunk_ref"] = audio.get("chunk_ref")
        speech["audio_url"] = audio.get("url")

        vision["mode"] = str(vision.get("mode") or safety.get("vision_detail") or "none")
        privacy["camera_used"] = _json_bool(privacy.get("camera_used", False)) or _json_bool(
            safety.get("camera_used", False)
        )
        privacy["mic_used"] = _json_bool(privacy.get("mic_used", False))
        privacy["cloud_vision_used"] = _explicit_true(privacy.get("cloud_vision_used", False))
        privacy["raw_frame_sent"] = _explicit_true(privacy.get("raw_frame_sent", False))
        privacy["raw_audio_recorded"] = _explicit_true(privacy.get("raw_audio_recorded", False))
        privacy["retention"] = str(privacy.get("retention") or "none")
        safety["camera_used"] = privacy["camera_used"]
        safety["vision_detail"] = vision["mode"]

        event_id = self.event_id or f"petevt-{uuid.uuid4().hex}"
        data = {
            "type": self.event_type,
            "schema_version": SCHEMA_VERSION,
            "event_id": event_id,
            "session_id": self.session_id,
            "turn_id": self.turn_id or event_id,
            "seq": self.seq,
            "timestamp_ms": self.timestamp_ms,
            "phase": _normalize_phase(self.phase),
            "emotion": self.emotion,
            "intensity": _clamp_float(self.intensity, 0.0, 1.0, 0.35),
            "animation": animation,
            "audio": audio,
            "text": text,
            "vision": vision,
            "privacy": privacy,
            "debug": _plain_dict(self.debug),
            "motion": motion,
            "expression": expression,
            "gaze": gaze,
            "lip_sync": lip_sync,
            "speech": speech,
            "action": action,
            "safety": safety,
        }
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
            audio=_sanitize_audio(data.get("audio")),
            vision=_sanitize_vision(data.get("vision")),
            privacy=_sanitize_observation_privacy(data.get("privacy")),
        )

    def to_dict(self) -> dict[str, Any]:
        data = {
            "type": self.event_type,
            "session_id": self.session_id,
            "audio": _sanitize_audio(self.audio),
            "vision": _sanitize_vision(self.vision),
            "privacy": _sanitize_observation_privacy(self.privacy),
        }
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


def _plain_dict(value: Any) -> dict[str, Any]:
    if isinstance(value, dict):
        return dict(value)
    if is_dataclass(value) and not isinstance(value, type):
        return asdict(value)
    return {}


def _preferred_text(primary: Any, secondary: Any, fallback: str) -> str:
    primary_text = str(primary) if primary is not None else ""
    secondary_text = str(secondary) if secondary is not None else ""
    if primary_text and primary_text != fallback:
        return primary_text
    if secondary_text:
        return secondary_text
    return primary_text or fallback


def _preferred_float(primary: Any, secondary: Any, fallback: float) -> float:
    primary_value = _clamp_float(primary, 0.0, 1.0, fallback)
    secondary_value = _clamp_float(secondary, 0.0, 1.0, fallback)
    if primary_value != fallback:
        return primary_value
    return secondary_value


def _clamp_float(value: Any, minimum: float, maximum: float, fallback: float) -> float:
    try:
        number = float(value)
    except (TypeError, ValueError):
        return fallback
    return max(minimum, min(maximum, number))


def _json_bool(value: Any) -> bool:
    if isinstance(value, bool):
        return value
    if isinstance(value, (int, float)):
        return value != 0
    if isinstance(value, str):
        return value.strip().lower() in {"1", "true", "yes", "on"}
    return False


def _explicit_true(value: Any) -> bool:
    return value is True


def _normalize_phase(value: Any) -> str:
    phase = str(value or "idle").strip().lower()
    if phase in {"idle", "listening", "thinking", "speaking", "audio_ready", "interrupted", "error"}:
        return phase
    return "idle"


def _sanitize_audio(value: Any) -> dict[str, Any]:
    if not isinstance(value, dict) or not value:
        return {}
    audio = _dict_or_empty(value)
    sanitized = dict(audio)
    sanitized["vad"] = str(audio.get("vad") or "")
    sanitized["rms"] = _clamp_float(audio.get("rms"), 0.0, 1.0, 0.0)
    sanitized["interrupt"] = _json_bool(audio.get("interrupt", False))
    return sanitized


def _sanitize_vision(value: Any) -> dict[str, Any]:
    if not isinstance(value, dict) or not value:
        return {}
    return _sanitize_vision_payload(value)


def _sanitize_vision_payload(value: Any) -> dict[str, Any]:
    vision = _dict_or_empty(value)
    sanitized = dict(vision)
    enabled = _json_bool(vision.get("enabled", False))
    pose = _dict_or_empty(vision.get("pose"))
    face_present = _json_bool(vision.get("face_present", False)) if enabled else False

    sanitized["enabled"] = enabled
    sanitized["mode"] = str(vision.get("mode") or ("landmarks_only" if enabled else "none"))
    sanitized["face_present"] = face_present
    sanitized["confidence"] = _vision_confidence(vision, pose, enabled, face_present)

    if enabled:
        sanitized["attention"] = str(vision.get("attention") or "")
        sanitized["expression"] = str(vision.get("expression") or "")
        sanitized["pose"] = pose
        sanitized["head_pose"] = _dict_or_empty(vision.get("head_pose"))
        sanitized["blendshapes"] = _dict_or_empty(vision.get("blendshapes"))
        sanitized["scene"] = _dict_or_empty(vision.get("scene"))
        sanitized["objects"] = _list_or_empty(vision.get("objects"))
        sanitized["gesture"] = str(vision.get("gesture") or "")
        sanitized["source"] = str(vision.get("source") or "")
        sanitized["frame"] = _dict_or_empty(vision.get("frame"))
        sanitized["client_frame"] = _dict_or_empty(vision.get("client_frame"))
        sanitized["frame_mime"] = str(vision.get("frame_mime") or "")
        sanitized["captured_at_ms"] = _non_negative_int(vision.get("captured_at_ms"))
        sanitized["changed_fields"] = _list_or_empty(vision.get("changed_fields"))
        sanitized["stable_for_ms"] = _non_negative_int(vision.get("stable_for_ms"))
        sanitized["state_hash"] = str(vision.get("state_hash") or "")
        sanitized["semantic_event"] = str(vision.get("semantic_event") or "")
    else:
        sanitized["attention"] = ""
        sanitized["expression"] = ""
        sanitized["pose"] = {}
        sanitized["head_pose"] = {}
        sanitized["blendshapes"] = {}
        sanitized["scene"] = {}
        sanitized["objects"] = []
        sanitized["gesture"] = ""
        sanitized["source"] = ""
        sanitized["frame"] = {}
        sanitized["client_frame"] = {}
        sanitized["frame_mime"] = ""
        sanitized["captured_at_ms"] = 0
        sanitized["changed_fields"] = []
        sanitized["stable_for_ms"] = 0
        sanitized["state_hash"] = ""
        sanitized["semantic_event"] = ""

    return sanitized


def _vision_confidence(vision: dict[str, Any], pose: dict[str, Any], enabled: bool, face_present: bool) -> float:
    if not enabled:
        return 0.0
    fallback = 0.0
    if "face_confidence" in pose:
        fallback = _clamp_float(pose.get("face_confidence"), 0.0, 1.0, fallback)
    return _clamp_float(vision.get("confidence"), 0.0, 1.0, fallback)


def _list_or_empty(value: Any) -> list[Any]:
    if isinstance(value, list):
        return list(value)
    if isinstance(value, tuple):
        return list(value)
    return []


def _non_negative_int(value: Any) -> int:
    try:
        number = int(value)
    except (TypeError, ValueError):
        return 0
    return max(0, number)


def _sanitize_observation_privacy(value: Any) -> dict[str, Any]:
    privacy = _dict_or_empty(value)
    sanitized = dict(privacy)
    sanitized["cloud_vision_used"] = _explicit_true(privacy.get("cloud_vision_used", False))
    sanitized["raw_frame_sent"] = _explicit_true(privacy.get("raw_frame_sent", False))
    sanitized["raw_audio_recorded"] = _explicit_true(privacy.get("raw_audio_recorded", False))
    sanitized["retention"] = str(privacy.get("retention") or "none")
    return sanitized
