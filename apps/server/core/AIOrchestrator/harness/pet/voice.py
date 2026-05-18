from __future__ import annotations

import hashlib
import math
import os
import struct
import tempfile
import time
import uuid
import wave
from dataclasses import asdict, dataclass, field
from pathlib import Path
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

    def __init__(self, output_dir: str | Path | None = None) -> None:
        self._active_turns: set[tuple[str, str]] = set()
        self._output_dir = Path(
            output_dir
            or os.environ.get("MEMOCHAT_PET_SOVITS_OUTPUT_DIR")
            or Path(tempfile.gettempdir()).joinpath("memochat-pet-voice-cache")
        )

    async def synthesize(self, request: VoiceSynthesisRequest) -> VoiceSynthesisResult:
        self._active_turns.add((request.session_id, request.turn_id))
        text = request.text.strip()
        result = _deterministic_result(
            request,
            text=text,
            output_dir=self._output_dir,
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
                    output_dir=self._output_dir,
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


class GPTSoVITSVoiceProvider:
    name = "gpt-sovits"

    def __init__(
        self,
        base_url: str = "",
        reference_audio: str = "",
        prompt_text: str = "",
        prompt_language: str = "zh",
        text_language: str = "zh",
        output_dir: str = "/app/.data/pet-voice-cache",
        timeout_sec: float = 60.0,
    ) -> None:
        self._base_url = str(base_url or "").strip().rstrip("/")
        self._reference_audio = str(reference_audio or "").strip()
        self._prompt_text = str(prompt_text or "").strip() or _prompt_text_from_reference(self._reference_audio)
        self._prompt_language = str(prompt_language or "zh").strip() or "zh"
        self._text_language = str(text_language or "zh").strip() or "zh"
        self._output_dir = Path(output_dir or "/app/.data/pet-voice-cache")
        self._timeout_sec = max(1.0, float(timeout_sec or 60.0))

    async def diagnostics(self, probe_endpoint: bool = False) -> dict[str, Any]:
        result: dict[str, Any] = {
            "provider": self.name,
            "configured": bool(self._base_url),
            "base_url": self._base_url,
            "reference_audio": self._reference_audio,
            "reference_audio_configured": bool(self._reference_audio),
            "reference_audio_exists": Path(self._reference_audio).is_file() if self._reference_audio else False,
            "prompt_text_configured": bool(self._prompt_text),
            "prompt_language": self._prompt_language,
            "text_language": self._text_language,
            "output_dir": str(self._output_dir),
            "output_dir_writable": _directory_writable(self._output_dir),
            "endpoint_probe_tested": bool(probe_endpoint),
            "endpoint_reachable": False,
            "ready": False,
            "status": "not_configured",
            "message": "",
        }
        if not self._base_url:
            result["message"] = "GPT-SoVITS base URL is not configured."
            return result
        if not self._reference_audio:
            result["status"] = "missing_reference_audio"
            result["message"] = "GPT-SoVITS reference audio is not configured."
            return result
        if not result["reference_audio_exists"]:
            result["status"] = "reference_audio_not_found"
            result["message"] = "Configured GPT-SoVITS reference audio is not visible to this runtime."
            return result
        result["status"] = "configured"
        result["message"] = "GPT-SoVITS configuration is present; endpoint probe was not requested."
        if not probe_endpoint:
            result["ready"] = True
            return result

        try:
            import httpx
        except ImportError:
            result["status"] = "httpx_missing"
            result["message"] = "httpx is required to probe GPT-SoVITS."
            return result

        try:
            async with httpx.AsyncClient(timeout=min(self._timeout_sec, 5.0)) as client:
                response = await client.get(self._base_url)
            result["endpoint_status_code"] = int(response.status_code)
            result["endpoint_reachable"] = response.status_code < 500
            result["ready"] = bool(result["endpoint_reachable"])
            result["status"] = "ready" if result["ready"] else "endpoint_unhealthy"
            result["message"] = (
                "GPT-SoVITS endpoint is reachable."
                if result["ready"]
                else f"GPT-SoVITS endpoint returned HTTP {response.status_code}."
            )
            return result
        except Exception as exc:
            result["status"] = "endpoint_unreachable"
            result["message"] = f"GPT-SoVITS endpoint probe failed: {exc}"
            return result

    async def synthesize(self, request: VoiceSynthesisRequest) -> VoiceSynthesisResult:
        if not self._base_url:
            raise VoiceProviderUnavailable("GPT-SoVITS base URL is not configured", provider=self.name)
        text = request.text.strip()
        if not text:
            return VoiceSynthesisResult(
                text="",
                state="text-only",
                provider=self.name,
                voice=request.voice or self.name,
                retention="none",
                metadata={"empty_text": True},
            )

        try:
            import httpx
        except ImportError as exc:  # pragma: no cover - exercised only on lean images.
            raise VoiceProviderUnavailable("httpx is required for GPT-SoVITS synthesis", provider=self.name) from exc

        metadata = dict(request.metadata or {})
        reference_audio = str(metadata.get("ref_audio_path") or self._reference_audio).strip()
        if not reference_audio:
            raise VoiceProviderUnavailable("GPT-SoVITS reference audio is not configured", provider=self.name)

        media_type = _media_type(metadata.get("media_type"))
        payload = {
            "text": text,
            "text_lang": _sovits_language(metadata.get("text_lang") or request.language or self._text_language or "zh"),
            "ref_audio_path": reference_audio,
            "prompt_text": str(metadata.get("prompt_text") or self._prompt_text),
            "prompt_lang": _sovits_language(metadata.get("prompt_lang") or self._prompt_language or "zh"),
            "text_split_method": str(metadata.get("text_split_method") or "cut5"),
            "batch_size": _positive_int(metadata.get("batch_size"), 1, maximum=8),
            "speed_factor": _positive_float(metadata.get("speed_factor"), 1.0, minimum=0.5, maximum=2.0),
            "top_k": _positive_int(metadata.get("top_k"), 15, maximum=100),
            "top_p": _positive_float(metadata.get("top_p"), 1.0, minimum=0.05, maximum=1.0),
            "temperature": _positive_float(metadata.get("temperature"), 1.0, minimum=0.05, maximum=2.0),
            "media_type": media_type,
            "streaming_mode": bool(metadata.get("streaming_mode", False)),
        }

        try:
            async with httpx.AsyncClient(timeout=self._timeout_sec) as client:
                response = await client.post(f"{self._base_url}/tts", json=payload)
                response.raise_for_status()
        except Exception as exc:
            raise VoiceProviderError(f"GPT-SoVITS synthesis failed: {exc}", provider=self.name) from exc

        content_type = response.headers.get("content-type", "")
        if "application/json" in content_type.lower():
            data = response.json()
            url = str(data.get("url") or data.get("audio_url") or data.get("path") or "")
            if not url:
                raise VoiceProviderError(f"GPT-SoVITS returned JSON without audio URL: {data}", provider=self.name)
            result_metadata = dict(metadata)
            result_metadata.update(
                {
                    "audio_persisted": False,
                    "remote_json": True,
                    "text_language": payload["text_lang"],
                    "reference_audio": reference_audio,
                }
            )
            return VoiceSynthesisResult(
                text=text,
                state="ready",
                sample_rate=_non_negative_int(data.get("sample_rate")),
                duration_ms=_non_negative_int(data.get("duration_ms")),
                rms=_scripted_rms(text),
                chunk_ref=str(data.get("chunk_ref") or _gpt_sovits_chunk_ref(request, text)),
                url=url,
                provider=self.name,
                voice=request.voice or self.name,
                retention="ephemeral",
                metadata=result_metadata,
            )

        audio_bytes = bytes(response.content or b"")
        if not audio_bytes:
            raise VoiceProviderError("GPT-SoVITS returned an empty audio response", provider=self.name)

        file_name = f"{_gpt_sovits_chunk_ref(request, text)}.{media_type}"
        try:
            self._output_dir.mkdir(parents=True, exist_ok=True)
            output_path = self._output_dir / file_name
            output_path.write_bytes(audio_bytes)
        except Exception as exc:
            raise VoiceProviderError(f"failed to persist GPT-SoVITS audio: {exc}", provider=self.name) from exc

        sample_rate, duration_ms = _wav_diagnostics(output_path) if media_type == "wav" else (0, 0)
        result_metadata = dict(metadata)
        result_metadata.update(
            {
                "audio_persisted": True,
                "media_type": media_type,
                "bytes": len(audio_bytes),
                "text_language": payload["text_lang"],
                "reference_audio": reference_audio,
                "created_at_ms": int(time.time() * 1000),
            }
        )
        return VoiceSynthesisResult(
            text=text,
            state="ready",
            sample_rate=sample_rate,
            duration_ms=duration_ms or _estimated_voice_duration_ms(text),
            rms=_scripted_rms(text),
            chunk_ref=output_path.stem,
            url=f"/audio/{file_name}",
            phoneme=None,
            viseme=_scripted_viseme(text),
            provider=self.name,
            voice=request.voice or self.name,
            retention="ephemeral",
            metadata=result_metadata,
        )

    async def stream(self, request: VoiceSynthesisRequest) -> AsyncIterator[VoiceSynthesisResult]:
        yield await self.synthesize(request)

    async def interrupt(self, request: VoiceInterruptRequest) -> VoiceSynthesisResult:
        return VoiceSynthesisResult(
            text="",
            state="interrupted",
            provider=self.name,
            voice=request.voice or self.name,
            retention="none",
            metadata={"reason": request.reason, "turn_id": request.turn_id},
        )


class VoiceProviderRouter:
    def __init__(self, deterministic: bool = True, deterministic_output_dir: str | Path | None = None) -> None:
        self._providers: dict[str, VoiceProvider] = {}
        if deterministic:
            provider = DeterministicVoiceProvider(output_dir=deterministic_output_dir)
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


def _media_type(value: Any) -> str:
    text = str(value or "wav").strip().lower().lstrip(".")
    if text in {"wav", "mp3", "ogg", "aac"}:
        return text
    return "wav"


def _sovits_language(value: Any) -> str:
    text = str(value or "zh").strip().lower().replace("_", "-")
    aliases = {
        "zh-cn": "zh",
        "zh-hans": "zh",
        "zh": "zh",
        "ja-jp": "ja",
        "jp": "ja",
        "ja": "ja",
        "en-us": "en",
        "en-gb": "en",
        "en": "en",
        "ko-kr": "ko",
        "ko": "ko",
    }
    return aliases.get(text, text or "zh")


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


def _positive_int(value: Any, fallback: int, maximum: int) -> int:
    try:
        number = int(value)
    except (TypeError, ValueError):
        number = fallback
    return max(1, min(maximum, number))


def _positive_float(value: Any, fallback: float, minimum: float, maximum: float) -> float:
    try:
        number = float(value)
    except (TypeError, ValueError):
        number = fallback
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


def _estimated_voice_duration_ms(text: str) -> int:
    return max(240, min(60000, 90 * len(text))) if text else 0


def _deterministic_result(
    request: VoiceSynthesisRequest,
    text: str,
    output_dir: Path,
    metadata: dict[str, Any] | None = None,
) -> VoiceSynthesisResult:
    digest = hashlib.sha1(
        f"{request.session_id}:{request.turn_id}:{request.voice}:{text}".encode("utf-8")
    ).hexdigest()[:16]
    duration_ms = _estimated_voice_duration_ms(text)
    return VoiceSynthesisResult(
        text=text,
        state="text-only",
        sample_rate=0,
        duration_ms=duration_ms,
        rms=_scripted_rms(text),
        chunk_ref=None,
        url=None,
        phoneme=None,
        viseme=_scripted_viseme(text),
        provider=DeterministicVoiceProvider.name,
        voice=request.voice or "deterministic",
        retention="none",
        metadata=metadata or {},
    )


def _write_scripted_wav(path: Path, text: str, duration_ms: int, sample_rate: int) -> None:
    # A compact generated tone keeps the default pet voice audible even without GPT-SoVITS.
    frames = max(1, int(sample_rate * max(duration_ms, 180) / 1000))
    frequency = 540.0 + (sum(ord(char) for char in text) % 180)
    amplitude = 9000
    with wave.open(str(path), "wb") as handle:
        handle.setnchannels(1)
        handle.setsampwidth(2)
        handle.setframerate(sample_rate)
        for index in range(frames):
            envelope = min(1.0, index / max(1, int(sample_rate * 0.025)))
            tail = min(1.0, (frames - index) / max(1, int(sample_rate * 0.035)))
            value = int(amplitude * min(envelope, tail) * math.sin(2.0 * math.pi * frequency * index / sample_rate))
            handle.writeframesraw(struct.pack("<h", value))


def _gpt_sovits_chunk_ref(request: VoiceSynthesisRequest, text: str) -> str:
    digest = hashlib.sha1(
        f"{request.session_id}:{request.turn_id}:{request.voice}:{text}:{uuid.uuid4().hex}".encode("utf-8")
    ).hexdigest()[:20]
    return f"gpt-sovits-{digest}"


def _prompt_text_from_reference(reference_audio: str) -> str:
    if not reference_audio:
        return ""
    path = Path(reference_audio)
    for candidate in (path.with_suffix(".ja.txt"), path.with_suffix(".zh.txt"), path.with_suffix(".txt")):
        try:
            if candidate.is_file():
                return " ".join(candidate.read_text(encoding="utf-8").split())
        except OSError:
            continue
    return ""


def _wav_diagnostics(path: Path) -> tuple[int, int]:
    try:
        import wave

        with wave.open(str(path), "rb") as handle:
            sample_rate = int(handle.getframerate() or 0)
            frames = int(handle.getnframes() or 0)
            duration_ms = int((frames / sample_rate) * 1000) if sample_rate > 0 else 0
            return sample_rate, max(0, duration_ms)
    except Exception:
        return 0, 0


def _stream_chunk_size(request: VoiceSynthesisRequest) -> int:
    value = request.metadata.get("chunk_size") if isinstance(request.metadata, dict) else None
    try:
        size = int(value)
    except (TypeError, ValueError):
        size = 48
    return max(1, min(240, size))


def _chunk_text(text: str, size: int) -> list[str]:
    return [text[index:index + size] for index in range(0, len(text), size)] or [""]


def _directory_writable(path: Path) -> bool:
    try:
        path.mkdir(parents=True, exist_ok=True)
        probe = path / ".write-test"
        probe.write_text("ok", encoding="utf-8")
        probe.unlink(missing_ok=True)
        return True
    except Exception:
        return False
