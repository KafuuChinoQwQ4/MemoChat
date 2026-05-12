from __future__ import annotations

import uuid
from dataclasses import dataclass
from typing import AsyncIterator

from .contracts import PetControlEvent, PetObservation, PetSession
from .event_bus import PetEventBus
from .policy import PetPolicy
from .providers import PetProviderError, PetProviderRouter
from .session_store import PetSessionStore


@dataclass(frozen=True)
class PetRuntimeConfig:
    enabled: bool = True
    deterministic: bool = True
    live2d_native_enabled: bool = False
    live2d_sdk_root: str = ""
    asset_root: str = ""
    cloud_vision_enabled: bool = False
    voice_clone_enabled: bool = False


class PetRuntime:
    """Provider-neutral in-memory pet runtime facade used by the API router."""

    def __init__(self, config: PetRuntimeConfig | object | None = None) -> None:
        self._config = _runtime_config_from(config)
        self._sessions = PetSessionStore()
        self._observations: dict[str, PetObservation] = {}
        self._events = PetEventBus()
        self._policy = PetPolicy()
        self._providers = PetProviderRouter(deterministic=self._config.deterministic)

    async def create_session(
        self,
        uid: int = 0,
        profile_id: str = "default",
        persona: str = "memo-pet",
        provider: str = "scripted",
    ) -> PetSession:
        if not self._config.enabled:
            raise RuntimeError("desktop pet is disabled by configuration")
        if not self._config.deterministic:
            raise RuntimeError("desktop pet deterministic runtime is disabled and no provider adapter is configured")

        session = self._sessions.create(
            uid=uid,
            profile_id=profile_id,
            persona=persona,
            provider=provider,
        )
        await self._emit(session.session_id, turn_id=_turn_id(), **self._policy.appear())
        return session

    async def submit_input(
        self,
        session_id: str,
        content: str,
        model_type: str = "",
        model_name: str = "",
    ) -> list[PetControlEvent]:
        session = self._sessions.touch(session_id, status="active")
        text = content.strip()
        if not text:
            raise ValueError("content cannot be empty")

        turn_id = _turn_id()
        events = [await self._emit(session_id, turn_id=turn_id, **self._policy.input_started())]
        prompt = self._policy.build_prompt(
            session,
            text,
            model_type=model_type,
            model_name=model_name,
            observation=self._observations.get(session_id),
        )

        try:
            chunks = await self._providers.generate(session, prompt)
        except PetProviderError as exc:
            events.append(await self._emit(session_id, turn_id=turn_id, **self._policy.provider_error(exc)))
            return events

        for chunk in chunks:
            events.append(await self._emit(session_id, turn_id=turn_id, **self._policy.provider_chunk(chunk)))

        events.append(await self._emit(session_id, turn_id=turn_id, **self._policy.input_finished()))
        return events

    async def update_observation(self, observation: PetObservation) -> PetControlEvent:
        self._sessions.touch(observation.session_id, status="active")
        self._observations[observation.session_id] = observation
        return await self._emit(
            observation.session_id,
            turn_id=f"petturn-observe-{uuid.uuid4().hex}",
            **self._policy.observation(observation),
        )

    async def interrupt(self, session_id: str) -> PetControlEvent:
        session = self._sessions.touch(session_id, status="interrupted")
        voice_payload: dict = {}
        try:
            voice_result = await self._providers.interrupt_voice(session)
            voice_payload = voice_result.to_dict()
        except PetProviderError as exc:
            voice_payload = {
                "state": "error",
                "provider": exc.provider,
                "recoverable": exc.recoverable,
                "message": str(exc),
                "retention": "none",
            }

        self._events.clear_pending(session_id)
        event_args = self._policy.interrupted()
        event_args["audio_state"] = str(voice_payload.get("state") or "interrupted")
        event_args["audio_sample_rate"] = int(voice_payload.get("sample_rate") or 0)
        event_args["audio_duration_ms"] = int(voice_payload.get("duration_ms") or 0)
        event_args["audio_chunk_ref"] = voice_payload.get("chunk_ref")
        event_args["audio_url"] = voice_payload.get("url")
        event_args["audio_phoneme"] = voice_payload.get("phoneme")
        event_args["audio_viseme"] = voice_payload.get("viseme")
        event_args["privacy_retention"] = str(voice_payload.get("retention") or "none")
        event_args["debug"] = {
            "voice": {
                "provider": voice_payload.get("provider") or "",
                "voice": voice_payload.get("voice") or "",
                "state": voice_payload.get("state") or "interrupted",
                "duration_ms": voice_payload.get("duration_ms") or 0,
                "retention": voice_payload.get("retention") or "none",
                "metadata": voice_payload.get("metadata") or {},
            }
        }
        return await self._emit(
            session_id,
            turn_id=f"petturn-interrupt-{uuid.uuid4().hex}",
            **event_args,
        )

    async def stream(self, session_id: str, heartbeat_sec: float = 15.0) -> AsyncIterator[PetControlEvent]:
        self._sessions.require(session_id)
        async for event in self._events.stream(session_id, heartbeat_sec=heartbeat_sec):
            yield event

    def get_session(self, session_id: str) -> PetSession | None:
        return self._sessions.get(session_id)

    def list_sessions(self, uid: int = 0) -> list[PetSession]:
        return self._sessions.list(uid)

    @property
    def config(self) -> PetRuntimeConfig:
        return self._config

    async def _emit(self, session_id: str, **kwargs) -> PetControlEvent:
        return await self._events.publish(session_id, **kwargs)


def _turn_id() -> str:
    return f"petturn-{uuid.uuid4().hex}"


def _runtime_config_from(config: PetRuntimeConfig | object | None) -> PetRuntimeConfig:
    if config is None:
        return PetRuntimeConfig()
    if isinstance(config, PetRuntimeConfig):
        return config
    return PetRuntimeConfig(
        enabled=bool(getattr(config, "enabled", True)),
        deterministic=bool(getattr(config, "deterministic", True)),
        live2d_native_enabled=bool(getattr(config, "live2d_native_enabled", False)),
        live2d_sdk_root=str(getattr(config, "live2d_sdk_root", "") or ""),
        asset_root=str(getattr(config, "asset_root", "") or ""),
        cloud_vision_enabled=bool(getattr(config, "cloud_vision_enabled", False)),
        voice_clone_enabled=bool(getattr(config, "voice_clone_enabled", False)),
    )
