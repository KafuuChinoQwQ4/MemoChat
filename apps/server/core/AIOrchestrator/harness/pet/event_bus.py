from __future__ import annotations

import asyncio
import time
import uuid
from collections import defaultdict
from typing import AsyncIterator

from .contracts import (
    PetAudio,
    PetControlEvent,
    PetGaze,
    PetLipSync,
    PetPrivacy,
    PetSafety,
    PetSpeech,
    PetText,
    PetVision,
)


class PetEventBus:
    """Ordered per-session control event queues with synthetic heartbeat events."""

    def __init__(self) -> None:
        self._queues: dict[str, asyncio.Queue[PetControlEvent]] = defaultdict(asyncio.Queue)
        self._seq: dict[str, int] = defaultdict(int)

    async def publish(self, session_id: str, **kwargs) -> PetControlEvent:
        event = self.make_event(session_id, **kwargs)
        await self._queues[session_id].put(event)
        return event

    async def emit(self, event: PetControlEvent) -> PetControlEvent:
        self._seq[event.session_id] += 1
        event.seq = self._seq[event.session_id]
        await self._queues[event.session_id].put(event)
        return event

    async def heartbeat(self, session_id: str) -> PetControlEvent:
        return self.make_event(
            session_id,
            turn_id=f"petturn-heartbeat-{uuid.uuid4().hex}",
            phase="idle",
            emotion="neutral",
            intensity=0.28,
            motion="idle",
            expression="neutral",
            lip_sync=0.0,
            speech_text="",
            action_name="heartbeat",
        )

    def make_event(
        self,
        session_id: str,
        turn_id: str = "",
        phase: str = "idle",
        emotion: str = "neutral",
        intensity: float = 0.35,
        motion: str = "idle",
        expression: str = "neutral",
        lip_sync: float = 0.0,
        speech_text: str = "",
        audio_state: str | None = None,
        audio_sample_rate: int = 0,
        audio_duration_ms: int = 0,
        audio_chunk_ref: str | None = None,
        audio_url: str | None = None,
        audio_phoneme: str | None = None,
        audio_viseme: str | None = None,
        privacy_retention: str | None = None,
        gaze: PetGaze | None = None,
        safety: PetSafety | None = None,
        action_name: str = "",
        vision: PetVision | None = None,
        privacy: PetPrivacy | None = None,
        debug: dict | None = None,
    ) -> PetControlEvent:
        self._seq[session_id] += 1
        rms = _clamp_float(lip_sync, 0.0, 1.0, 0.0)
        return PetControlEvent(
            session_id=session_id,
            seq=self._seq[session_id],
            timestamp_ms=_now_ms(),
            turn_id=turn_id or f"petturn-{uuid.uuid4().hex}",
            phase=phase,
            emotion=emotion,
            intensity=_clamp_float(intensity, 0.0, 1.0, 0.0),
            motion=motion,
            expression=expression,
            gaze=gaze or PetGaze(),
            lip_sync=PetLipSync(value=rms),
            speech=PetSpeech(text_delta=speech_text, audio_url=audio_url, audio_chunk_ref=audio_chunk_ref),
            audio=PetAudio(
                state=audio_state or _audio_state_for_phase(phase),
                rms=rms,
                sample_rate=_non_negative_int(audio_sample_rate),
                duration_ms=_non_negative_int(audio_duration_ms),
                chunk_ref=audio_chunk_ref,
                url=audio_url,
                phoneme=audio_phoneme,
                viseme=audio_viseme,
            ),
            text=PetText(delta=speech_text, final=phase in {"idle", "interrupted", "error"}),
            vision=vision or PetVision(),
            privacy=privacy or PetPrivacy(
                camera_used=bool(safety and safety.camera_used),
                retention=str(privacy_retention or "none"),
            ),
            action={"name": action_name, "args": {}},
            safety=safety or PetSafety(),
            debug=debug or {},
        )

    async def stream(self, session_id: str, heartbeat_sec: float = 15.0) -> AsyncIterator[PetControlEvent]:
        queue = self._queues[session_id]
        while True:
            try:
                yield await asyncio.wait_for(queue.get(), timeout=heartbeat_sec)
            except asyncio.TimeoutError:
                yield self.make_event(
                    session_id,
                    turn_id=f"petturn-heartbeat-{uuid.uuid4().hex}",
                    phase="idle",
                    emotion="neutral",
                    intensity=0.28,
                    motion="idle",
                    expression="neutral",
                    lip_sync=0.0,
                    speech_text="",
                    action_name="heartbeat",
                )

    def clear_pending(self, session_id: str) -> int:
        queue = self._queues[session_id]
        cleared = 0
        while True:
            try:
                queue.get_nowait()
                cleared += 1
            except asyncio.QueueEmpty:
                return cleared


def _now_ms() -> int:
    return int(time.time() * 1000)


def _clamp_float(value, minimum: float, maximum: float, fallback: float) -> float:
    try:
        number = float(value)
    except (TypeError, ValueError):
        return fallback
    return max(minimum, min(maximum, number))


def _non_negative_int(value) -> int:
    try:
        number = int(value)
    except (TypeError, ValueError):
        return 0
    return max(0, number)


def _audio_state_for_phase(phase: str) -> str:
    if phase == "speaking":
        return "playing"
    if phase == "listening":
        return "listening"
    if phase == "interrupted":
        return "stopped"
    if phase == "error":
        return "error"
    return "idle"
