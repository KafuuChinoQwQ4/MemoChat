from __future__ import annotations

from dataclasses import dataclass, field
from typing import AsyncIterator, Protocol

from ..contracts import PetSession
from ..persona import PetPromptContext
from ..voice import (
    VoiceInterruptRequest,
    VoiceProviderError,
    VoiceProviderRouter,
    VoiceSynthesisRequest,
    VoiceSynthesisResult,
)


class PetProviderError(RuntimeError):
    def __init__(self, message: str, provider: str = "", recoverable: bool = True) -> None:
        super().__init__(message)
        self.provider = provider
        self.recoverable = recoverable


class PetProviderUnavailable(PetProviderError):
    pass


@dataclass(frozen=True)
class ProviderChunk:
    text: str = ""
    emotion: str = "speaking"
    intensity: float = 0.66
    final: bool = False
    voice: VoiceSynthesisResult | None = None
    metadata: dict = field(default_factory=dict)


class PetTextProvider(Protocol):
    name: str

    async def generate(self, prompt: PetPromptContext) -> list[ProviderChunk]: ...


class PetProviderRouter:
    def __init__(self, deterministic: bool = True, voice_router: VoiceProviderRouter | None = None) -> None:
        self._providers: dict[str, PetTextProvider] = {}
        self._voice = voice_router or VoiceProviderRouter(deterministic=deterministic)
        if deterministic:
            from .deterministic import DeterministicPetProvider

            deterministic_provider = DeterministicPetProvider(self._voice)
            self.register("scripted", deterministic_provider)
            self.register("deterministic", deterministic_provider)

    def register(self, name: str, provider: PetTextProvider) -> None:
        normalized = _normalize_provider_name(name)
        if normalized:
            self._providers[normalized] = provider

    async def generate(
        self,
        session: PetSession,
        prompt: PetPromptContext,
        runtime_metadata: dict | None = None,
    ) -> list[ProviderChunk]:
        provider_name = _normalize_provider_name(session.provider or "scripted")
        provider = self._providers.get(provider_name)
        if provider is None:
            raise PetProviderUnavailable(
                f"pet provider is not configured: {provider_name or 'scripted'}",
                provider=provider_name,
            )
        try:
            if runtime_metadata:
                prompt = PetPromptContext(
                    session_id=prompt.session_id,
                    uid=prompt.uid,
                    profile_id=prompt.profile_id,
                    persona=prompt.persona,
                    provider=prompt.provider,
                    user_text=prompt.user_text,
                    model_type=prompt.model_type,
                    model_name=prompt.model_name,
                    observation_summary=prompt.observation_summary,
                    runtime_metadata=dict(runtime_metadata),
                    memory_snippets=prompt.memory_snippets,
                    safety_notes=prompt.safety_notes,
                )
            return await provider.generate(prompt)
        except PetProviderError:
            raise
        except Exception as exc:
            raise PetProviderError(str(exc), provider=provider_name) from exc

    async def synthesize_voice(
        self,
        session: PetSession,
        turn_id: str,
        text: str,
        voice: str = "deterministic",
        language: str = "zh-CN",
    ) -> VoiceSynthesisResult:
        try:
            return await self._voice.synthesize(
                VoiceSynthesisRequest(
                    session_id=session.session_id,
                    turn_id=turn_id,
                    text=text,
                    provider=session.provider or "scripted",
                    voice=voice,
                    language=language,
                )
            )
        except VoiceProviderError as exc:
            raise PetProviderError(str(exc), provider=exc.provider, recoverable=exc.recoverable) from exc

    async def stream_voice(
        self,
        session: PetSession,
        turn_id: str,
        text: str,
        voice: str = "deterministic",
        language: str = "zh-CN",
        metadata: dict | None = None,
    ) -> AsyncIterator[VoiceSynthesisResult]:
        try:
            async for result in self._voice.stream(
                VoiceSynthesisRequest(
                    session_id=session.session_id,
                    turn_id=turn_id,
                    text=text,
                    provider=session.provider or "scripted",
                    voice=voice,
                    language=language,
                    metadata=metadata or {},
                )
            ):
                yield result
        except VoiceProviderError as exc:
            raise PetProviderError(str(exc), provider=exc.provider, recoverable=exc.recoverable) from exc

    async def interrupt_voice(
        self,
        session: PetSession,
        turn_id: str = "",
        voice: str = "deterministic",
        reason: str = "user_interrupt",
    ) -> VoiceSynthesisResult:
        try:
            return await self._voice.interrupt(
                VoiceInterruptRequest(
                    session_id=session.session_id,
                    turn_id=turn_id,
                    provider=session.provider or "scripted",
                    voice=voice,
                    reason=reason,
                )
            )
        except VoiceProviderError as exc:
            raise PetProviderError(str(exc), provider=exc.provider, recoverable=exc.recoverable) from exc


def _normalize_provider_name(value: str) -> str:
    return str(value or "").strip().lower()
