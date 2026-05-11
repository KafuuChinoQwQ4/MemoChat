from __future__ import annotations

import asyncio
import time
import uuid
from collections import defaultdict
from typing import AsyncIterator

from .contracts import PetControlEvent, PetGaze, PetLipSync, PetObservation, PetSafety, PetSession, PetSpeech


class PetRuntime:
    """Deterministic pet runtime used until provider-backed voice/LLM adapters arrive."""

    def __init__(self) -> None:
        self._sessions: dict[str, PetSession] = {}
        self._observations: dict[str, PetObservation] = {}
        self._queues: dict[str, asyncio.Queue[PetControlEvent]] = defaultdict(asyncio.Queue)
        self._seq: dict[str, int] = defaultdict(int)

    async def create_session(
        self,
        uid: int = 0,
        profile_id: str = "default",
        persona: str = "memo-pet",
        provider: str = "scripted",
    ) -> PetSession:
        now = _now_ms()
        session = PetSession(
            session_id=f"pet-{uuid.uuid4().hex}",
            uid=uid,
            profile_id=profile_id or "default",
            persona=persona or "memo-pet",
            provider=provider or "scripted",
            created_at_ms=now,
            updated_at_ms=now,
        )
        self._sessions[session.session_id] = session
        await self._emit(
            session.session_id,
            emotion="curious",
            intensity=0.48,
            motion="appear",
            expression="smile_soft",
            speech_text="我在。Live2D 控制通道已连接。",
            action_name="appear",
        )
        return session

    async def submit_input(
        self,
        session_id: str,
        content: str,
        model_type: str = "",
        model_name: str = "",
    ) -> list[PetControlEvent]:
        session = self._require_session(session_id)
        session.updated_at_ms = _now_ms()
        text = content.strip()
        if not text:
            raise ValueError("content cannot be empty")

        style_hint = f"{model_type}:{model_name}".strip(":")
        reply = f"收到：{text}"
        if style_hint:
            reply = f"{reply}（{style_hint} scripted）"

        events = [
            await self._emit(
                session_id,
                emotion="attentive",
                intensity=0.52,
                motion="listen",
                expression="focus",
                speech_text="",
                lip_sync=0.0,
                action_name="listen",
            )
        ]

        chunks = _chunk_text(reply, 8)
        for index, chunk in enumerate(chunks):
            events.append(
                await self._emit(
                    session_id,
                    emotion="cheerful" if index == len(chunks) - 1 else "speaking",
                    intensity=0.66,
                    motion="talk",
                    expression="smile_soft",
                    speech_text=chunk,
                    lip_sync=min(1.0, 0.25 + 0.08 * len(chunk)),
                    action_name="speak",
                )
            )

        events.append(
            await self._emit(
                session_id,
                emotion="neutral",
                intensity=0.35,
                motion="idle",
                expression="neutral",
                speech_text="",
                lip_sync=0.0,
                action_name="idle",
            )
        )
        return events

    async def update_observation(self, observation: PetObservation) -> PetControlEvent:
        session = self._require_session(observation.session_id)
        session.updated_at_ms = _now_ms()
        self._observations[observation.session_id] = observation

        vision = observation.vision
        audio = observation.audio
        expression = str(vision.get("expression") or "neutral")
        face_present = bool(vision.get("face_present", False))
        rms = _clamp_float(audio.get("rms"), 0.0, 1.0, 0.0)
        return await self._emit(
            observation.session_id,
            emotion=expression if face_present else "neutral",
            intensity=max(0.25, rms),
            motion="observe" if face_present else "idle",
            expression=_expression_for_observation(expression, face_present),
            lip_sync=rms,
            speech_text="",
            gaze=PetGaze(target="user_face" if face_present else "screen", x=0.5, y=0.5),
            safety=PetSafety(
                camera_used=bool(vision.get("enabled", False)),
                vision_detail=str(vision.get("mode") or "landmarks_only"),
            ),
            action_name="observe",
        )

    async def interrupt(self, session_id: str) -> PetControlEvent:
        session = self._require_session(session_id)
        session.updated_at_ms = _now_ms()
        return await self._emit(
            session_id,
            emotion="neutral",
            intensity=0.2,
            motion="stop",
            expression="neutral",
            lip_sync=0.0,
            speech_text="",
            action_name="interrupt",
        )

    async def stream(self, session_id: str, heartbeat_sec: float = 15.0) -> AsyncIterator[PetControlEvent]:
        self._require_session(session_id)
        queue = self._queues[session_id]
        while True:
            try:
                yield await asyncio.wait_for(queue.get(), timeout=heartbeat_sec)
            except asyncio.TimeoutError:
                yield await self._make_event(
                    session_id,
                    emotion="neutral",
                    intensity=0.28,
                    motion="idle",
                    expression="neutral",
                    lip_sync=0.0,
                    speech_text="",
                    action_name="heartbeat",
                )

    def get_session(self, session_id: str) -> PetSession | None:
        return self._sessions.get(session_id)

    def list_sessions(self, uid: int = 0) -> list[PetSession]:
        sessions = list(self._sessions.values())
        if uid > 0:
            sessions = [session for session in sessions if session.uid == uid]
        return sorted(sessions, key=lambda item: item.updated_at_ms, reverse=True)

    async def _emit(self, session_id: str, **kwargs) -> PetControlEvent:
        event = await self._make_event(session_id, **kwargs)
        await self._queues[session_id].put(event)
        return event

    async def _make_event(
        self,
        session_id: str,
        emotion: str,
        intensity: float,
        motion: str,
        expression: str,
        lip_sync: float = 0.0,
        speech_text: str = "",
        gaze: PetGaze | None = None,
        safety: PetSafety | None = None,
        action_name: str = "",
    ) -> PetControlEvent:
        self._seq[session_id] += 1
        return PetControlEvent(
            session_id=session_id,
            seq=self._seq[session_id],
            timestamp_ms=_now_ms(),
            emotion=emotion,
            intensity=_clamp_float(intensity, 0.0, 1.0, 0.0),
            motion=motion,
            expression=expression,
            gaze=gaze or PetGaze(),
            lip_sync=PetLipSync(value=_clamp_float(lip_sync, 0.0, 1.0, 0.0)),
            speech=PetSpeech(text_delta=speech_text),
            action={"name": action_name, "args": {}},
            safety=safety or PetSafety(),
        )

    def _require_session(self, session_id: str) -> PetSession:
        session = self._sessions.get(session_id)
        if session is None:
            raise KeyError(f"pet session not found: {session_id}")
        return session


def _now_ms() -> int:
    return int(time.time() * 1000)


def _chunk_text(text: str, size: int) -> list[str]:
    return [text[index:index + size] for index in range(0, len(text), size)] or [""]


def _clamp_float(value, minimum: float, maximum: float, fallback: float) -> float:
    try:
        number = float(value)
    except (TypeError, ValueError):
        return fallback
    return max(minimum, min(maximum, number))


def _expression_for_observation(expression: str, face_present: bool) -> str:
    if not face_present:
        return "neutral"
    normalized = expression.strip().lower()
    if normalized in {"smile", "happy", "cheerful"}:
        return "smile_soft"
    if normalized in {"surprised", "surprise"}:
        return "surprised"
    if normalized in {"sad", "tired"}:
        return "concerned"
    return "focus"
