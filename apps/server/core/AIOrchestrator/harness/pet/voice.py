from __future__ import annotations

import hashlib
from dataclasses import asdict, dataclass, field
from typing import Any, AsyncIterator, Protocol


class VoiceProviderError(RuntimeError):
    def __init__(self, message: str, provider: str = "", recoverable: bool = True) -> None:
        super().__init__(message)
        self.provider = provider
        self.recoverable = recoverable


class VoiceProviderUnavailable(VoiceProviderError):
    pass


@dataclass(frozen=True)
class VoiceSynthesisRequest:
    session_id: str
    turn_id: str
    text: str
    provider: str = "scripted"
    voice: str = "deterministic"
    language: str = "zh-CN"
    metadata: dict[str, Any] = field(default_factory=dict)


@dataclass(frozen=True)
class VoiceInterruptRequest:
    session_id: str
    turn_id: str = ""
    provider: str = "scripted"
    voice: str = "deterministic"
    reason: str = "user_interrupt"
    metadata: dict[str, Any] = field(default_factory=dict)


@dataclass(frozen=True)
class VoiceSynthesisResult:
    text: str
    state: str = "text-only"
    sample_rate: int = 0
    duration_ms: int = 0
    rms: float = 0.0
    chunk_ref: str | None = None
    url: str | None = None
    phoneme: str | None = None
    viseme: str | None = None
    provider: str = ""
    voice: str = ""
    retention: str = "none"
    metadata: dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> dict[str, Any]:
        data = asdict(self)
        data["state"] = _normalize_state(data.get("state"))
        data["sample_rate"] = _non_negative_int(data.get("sample_rate"))
        data["duration_ms"] = _non_negative_int(data.get("duration_ms"))
        data["rms"] = _clamp_float(data.get("rms"), 0.0, 1.0, 0.0)
        data["retention"] = str(data.get("retention") or "none")
        return data


class VoiceProvider(Protocol):
    name: str

    async def synthesize(self, request: VoiceSynthesisRequest) -> VoiceSynthesisResult:
        ...

    async def stream(self, request: VoiceSynthesisRequest) -> AsyncIterator[VoiceSynthesisResult]:
        ...

    async def interrupt(self, request: VoiceInterruptRequest) -> VoiceSynthesisResult:
        ...


class DeterministicVoiceProvider:
    name = "scripted"

    def __init__(self) -> None:
        self._active_turns: set[tuple[str, str]] = set()

    async def synthesize(self, request: VoiceSynthesisRequest) -> VoiceSynthesisResult:
        self._active_turns.add((request.session_id, request.turn_id))
        text = request.text.strip()
        result = _deterministic_result(
            request,
            text=text,
            metadata={
                "deterministic": True,
                "language": request.language,
                "audio_persisted": False,
                "stream": False,
            },
        )
        self._active_turns.discard((request.session_id, request.turn_id))
        return result

    async def stream(self, request: VoiceSynthesisRequest) -> AsyncIterator[VoiceSynthesisResult]:
        self._active_turns.add((request.session_id, request.turn_id))
        chunks = _chunk_text(request.text.strip(), _stream_chunk_size(request))
        try:
            for index, chunk in enumerate(chunks):
                if (request.session_id, request.turn_id) not in self._active_turns:
                    break
                yield _deterministic_result(
                    request,
                    text=chunk,
                    metadata={
                        "deterministic": True,
                        "language": request.language,
                        "audio_persisted": False,
                        "stream": True,
                        "chunk_index": index,
                        "final": index == len(chunks) - 1,
                    },
                )
        finally:
            self._active_turns.discard((request.session_id, request.turn_id))

    async def interrupt(self, request: VoiceInterruptRequest) -> VoiceSynthesisResult:
        if request.turn_id:
            self._active_turns.discard((request.session_id, request.turn_id))
        else:
            self._active_turns = {
                item for item in self._active_turns if item[0] != request.session_id
            }
        return VoiceSynthesisResult(
            text="",
            state="interrupted",
            sample_rate=0,
            duration_ms=0,
            rms=0.0,
            chunk_ref=None,
            url=None,
            phoneme=None,
            viseme=None,
            provider=self.name,
            voice=request.voice or "deterministic",
            retention="none",
            metadata={
                "deterministic": True,
                "audio_persisted": False,
                "reason": request.reason,
                "turn_id": request.turn_id,
            },
        )


class VoiceProviderRouter:
    def __init__(self, deterministic: bool = True) -> None:
        self._providers: dict[str, VoiceProvider] = {}
        if deterministic:
            provider = DeterministicVoiceProvider()
            self.register("scripted", provider)
            self.register("deterministic", provider)

    def register(self, name: str, provider: VoiceProvider) -> None:
        normalized = _normalize_provider_name(name)
        if normalized:
            self._providers[normalized] = provider

    async def synthesize(self, request: VoiceSynthesisRequest) -> VoiceSynthesisResult:
        provider_name = _normalize_provider_name(request.provider or "scripted")
        provider = self._providers.get(provider_name)
        if provider is None:
            raise VoiceProviderUnavailable(
                f"voice provider is not configured: {provider_name or 'scripted'}",
                provider=provider_name,
            )
        try:
            return await provider.synthesize(request)
        except VoiceProviderError:
            raise
        except Exception as exc:
            raise VoiceProviderError(str(exc), provider=provider_name) from exc

    async def stream(self, request: VoiceSynthesisRequest) -> AsyncIterator[VoiceSynthesisResult]:
        provider_name = _normalize_provider_name(request.provider or "scripted")
        provider = self._providers.get(provider_name)
        if provider is None:
            raise VoiceProviderUnavailable(
                f"voice provider is not configured: {provider_name or 'scripted'}",
                provider=provider_name,
            )
        try:
            async for result in provider.stream(request):
                yield result
        except VoiceProviderError:
            raise
        except Exception as exc:
            raise VoiceProviderError(str(exc), provider=provider_name) from exc

    async def interrupt(self, request: VoiceInterruptRequest) -> VoiceSynthesisResult:
        provider_name = _normalize_provider_name(request.provider or "scripted")
        provider = self._providers.get(provider_name)
        if provider is None:
            raise VoiceProviderUnavailable(
                f"voice provider is not configured: {provider_name or 'scripted'}",
                provider=provider_name,
            )
        try:
            return await provider.interrupt(request)
        except VoiceProviderError:
            raise
        except Exception as exc:
            raise VoiceProviderError(str(exc), provider=provider_name) from exc


def _normalize_provider_name(value: str) -> str:
    return str(value or "").strip().lower()


def _normalize_state(value: Any) -> str:
    state = str(value or "text-only").strip().lower()
    if state in {"text-only", "ready", "interrupted", "error"}:
        return state
    return "text-only"


def _non_negative_int(value: Any) -> int:
    try:
        number = int(value)
    except (TypeError, ValueError):
        return 0
    return max(0, number)


def _clamp_float(value: Any, minimum: float, maximum: float, fallback: float) -> float:
    try:
        number = float(value)
    except (TypeError, ValueError):
        return fallback
    return max(minimum, min(maximum, number))


def _scripted_rms(text: str) -> float:
    if not text:
        return 0.0
    return min(1.0, 0.22 + 0.025 * len(text))


def _scripted_viseme(text: str) -> str | None:
    if not text:
        return None
    if any(char in text.lower() for char in ("a", "o", "啊", "哦")):
        return "open"
    return "neutral"


def _deterministic_result(
    request: VoiceSynthesisRequest,
    text: str,
    metadata: dict[str, Any] | None = None,
) -> VoiceSynthesisResult:
    digest = hashlib.sha1(
        f"{request.session_id}:{request.turn_id}:{request.voice}:{text}".encode("utf-8")
    ).hexdigest()[:16]
    duration_ms = max(240, min(12000, 90 * len(text))) if text else 0
    return VoiceSynthesisResult(
        text=text,
        state="text-only",
        sample_rate=0,
        duration_ms=duration_ms,
        rms=_scripted_rms(text),
        chunk_ref=f"deterministic-text-{digest}" if text else None,
        url=None,
        phoneme=None,
        viseme=_scripted_viseme(text),
        provider=DeterministicVoiceProvider.name,
        voice=request.voice or "deterministic",
        retention="none",
        metadata=metadata or {},
    )


def _stream_chunk_size(request: VoiceSynthesisRequest) -> int:
    value = request.metadata.get("chunk_size") if isinstance(request.metadata, dict) else None
    try:
        size = int(value)
    except (TypeError, ValueError):
        size = 12
    return max(1, min(120, size))


def _chunk_text(text: str, size: int) -> list[str]:
    return [text[index:index + size] for index in range(0, len(text), size)] or [""]
