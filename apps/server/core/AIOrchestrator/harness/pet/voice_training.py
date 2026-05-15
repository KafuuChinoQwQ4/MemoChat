from __future__ import annotations

import time
import uuid
from dataclasses import asdict, dataclass, field
import base64
import binascii
import json
import os
from pathlib import Path
import shutil
import subprocess
from typing import Any


@dataclass(frozen=True)
class VoiceTrainingRequest:
    uid: int = 0
    profile_id: str = "default"
    voice_name: str = "Kafuuchino-voice"
    language: str = "zh-CN"
    reference_audio_path: str = ""
    reference_audio_directory: str = ""
    reference_audio_file: str = ""
    consent_confirmed: bool = False
    consent_scope: str = "local_default_reference"
    source: str = "src-default"
    provider: str = "gpt-sovits"
    metadata: dict[str, Any] = field(default_factory=dict)

    @classmethod
    def from_dict(cls, data: dict[str, Any]) -> "VoiceTrainingRequest":
        return cls(
            uid=_non_negative_int(data.get("uid")),
            profile_id=_text(data.get("profile_id"), "default"),
            voice_name=_text(data.get("voice_name"), "Kafuuchino-voice"),
            language=_text(data.get("language"), "zh-CN"),
            reference_audio_path=_text(data.get("reference_audio_path"), ""),
            reference_audio_directory=_text(data.get("reference_audio_directory"), ""),
            reference_audio_file=_text(data.get("reference_audio_file"), ""),
            consent_confirmed=data.get("consent_confirmed") is True,
            consent_scope=_text(data.get("consent_scope"), "local_default_reference"),
            source=_text(data.get("source"), "src-default"),
            provider=_text(data.get("provider"), "gpt-sovits"),
            metadata=dict(data.get("metadata")) if isinstance(data.get("metadata"), dict) else {},
        )


@dataclass(frozen=True)
class VoiceTrainingJob:
    job_id: str
    uid: int
    profile_id: str
    voice_name: str
    language: str
    reference_audio_path: str
    reference_audio_directory: str
    reference_audio_file: str
    provider: str
    status: str
    stage: str
    progress: int
    consent_confirmed: bool
    consent_scope: str
    source: str
    artifact_root: str
    artifact_path: str
    manifest_path: str
    message: str
    created_at_ms: int
    updated_at_ms: int
    diagnostics: dict[str, Any] = field(default_factory=dict)
    metadata: dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> dict[str, Any]:
        data = asdict(self)
        data["progress"] = max(0, min(100, int(data.get("progress") or 0)))
        data["diagnostics"] = dict(data.get("diagnostics") or {})
        data["metadata"] = dict(data.get("metadata") or {})
        return data


class VoiceTrainingService:
    def __init__(self, voice_clone_enabled: bool = False, artifact_root: str = "") -> None:
        self._voice_clone_enabled = bool(voice_clone_enabled)
        self._artifact_root = artifact_root.strip() or "/app/.data/pet-voice-training"
        self._jobs: dict[str, VoiceTrainingJob] = {}
        self._load_jobs()

    def create_job(self, request: VoiceTrainingRequest) -> VoiceTrainingJob:
        if not request.consent_confirmed:
            raise ValueError("voice training requires explicit consent_confirmed=true")
        if not request.reference_audio_path.strip():
            raise ValueError("reference_audio_path cannot be empty")

        now = _now_ms()
        job_id = f"voice-train-{uuid.uuid4().hex}"
        artifact_path = str(Path(self._artifact_root).joinpath(job_id))
        manifest_path = str(Path(artifact_path).joinpath("manifest.json"))
        persisted_audio_path, persistence = _persist_reference_audio(Path(artifact_path), request)
        effective_reference_audio = persisted_audio_path or request.reference_audio_path
        audio_diagnostics = diagnose_reference_audio(effective_reference_audio)
        status, stage, progress, message = _initial_job_state(
            self._voice_clone_enabled,
            audio_diagnostics,
            bool(persisted_audio_path),
        )
        job = VoiceTrainingJob(
            job_id=job_id,
            uid=request.uid,
            profile_id=request.profile_id,
            voice_name=request.voice_name,
            language=_language_code(request.language),
            reference_audio_path=request.reference_audio_path,
            reference_audio_directory=request.reference_audio_directory,
            reference_audio_file=request.reference_audio_file,
            provider=request.provider,
            status=status,
            stage=stage,
            progress=progress,
            consent_confirmed=True,
            consent_scope=request.consent_scope,
            source=request.source,
            artifact_root=self._artifact_root,
            artifact_path=artifact_path,
            manifest_path=manifest_path,
            message=message,
            created_at_ms=now,
            updated_at_ms=now,
            diagnostics={
                "voice_clone_enabled": self._voice_clone_enabled,
                "audio_persisted": bool(persisted_audio_path),
                "training_started": False,
                "manifest_persisted": False,
                "reference_audio_original_path": request.reference_audio_path,
                "reference_audio_effective_path": effective_reference_audio,
                "reference_audio_runtime_path": persisted_audio_path,
                "gpt_sovits_reference_audio": persisted_audio_path or "",
                "gpt_sovits_zero_shot_ready": bool(audio_diagnostics["zero_shot_ready"]),
                "gpt_sovits_prompt_text_required": bool(audio_diagnostics["zero_shot_ready"]),
                **persistence,
                "reference_audio_exists": audio_diagnostics["exists"],
                "reference_audio_size_bytes": audio_diagnostics["size_bytes"],
                "reference_audio_extension": audio_diagnostics["extension"],
                "reference_audio_readable": audio_diagnostics["readable"],
                "reference_audio_codec": audio_diagnostics["codec"],
                "reference_audio_duration_sec": audio_diagnostics["duration_sec"],
                "reference_audio_sample_rate": audio_diagnostics["sample_rate"],
                "reference_audio_channels": audio_diagnostics["channels"],
                "reference_audio_material_status": audio_diagnostics["material_status"],
                "reference_audio_zero_shot_ready": audio_diagnostics["zero_shot_ready"],
                "reference_audio_few_shot_ready": audio_diagnostics["few_shot_ready"],
                "needs_more_audio_for_inference": audio_diagnostics["needs_more_audio_for_inference"],
                "needs_more_audio_for_training": audio_diagnostics["needs_more_audio_for_training"],
                "recommended_next_step": audio_diagnostics["recommended_next_step"],
            },
            metadata=_redacted_metadata(request.metadata),
        )
        job = self._persist_job(job)
        self._jobs[job_id] = job
        return job

    def get_job(self, job_id: str) -> VoiceTrainingJob:
        job = self._jobs.get(job_id)
        if job is None:
            raise KeyError(f"voice training job not found: {job_id}")
        return job

    def list_jobs(self, uid: int = 0) -> list[VoiceTrainingJob]:
        jobs = list(self._jobs.values())
        if uid > 0:
            jobs = [job for job in jobs if job.uid == uid]
        jobs.sort(key=lambda job: job.updated_at_ms, reverse=True)
        return jobs

    def latest_ready_reference_audio(self, uid: int = 0) -> str:
        for job in self.list_jobs(uid=uid):
            diagnostics = dict(job.diagnostics or {})
            if job.status != "ready":
                continue
            reference_audio = str(
                diagnostics.get("gpt_sovits_reference_audio")
                or diagnostics.get("reference_audio_runtime_path")
                or ""
            ).strip()
            if reference_audio and Path(reference_audio).is_file():
                return reference_audio
        return ""

    def _load_jobs(self) -> None:
        root = Path(self._artifact_root)
        if not root.exists() or not root.is_dir():
            return
        for manifest in root.glob("voice-train-*/manifest.json"):
            try:
                with manifest.open("r", encoding="utf-8") as handle:
                    data = json.load(handle)
                job = _job_from_dict(data)
                job = _normalize_loaded_job(job)
            except Exception:
                continue
            self._jobs[job.job_id] = job

    def _persist_job(self, job: VoiceTrainingJob) -> VoiceTrainingJob:
        data = job.to_dict()
        diagnostics = dict(data.get("diagnostics") or {})
        try:
            artifact_dir = Path(job.artifact_path)
            artifact_dir.mkdir(parents=True, exist_ok=True)
            manifest_path = Path(job.manifest_path)
            manifest_path.parent.mkdir(parents=True, exist_ok=True)
            diagnostics["manifest_persisted"] = True
            data["diagnostics"] = diagnostics
            temp_path = manifest_path.with_suffix(".json.tmp")
            with temp_path.open("w", encoding="utf-8") as handle:
                json.dump(data, handle, ensure_ascii=False, indent=2, sort_keys=True)
                handle.write("\n")
            os.replace(temp_path, manifest_path)
        except Exception as exc:
            diagnostics["manifest_persisted"] = False
            diagnostics["manifest_error"] = str(exc)
            data["diagnostics"] = diagnostics
        return _job_from_dict(data)


def _now_ms() -> int:
    return int(time.time() * 1000)


def _non_negative_int(value: Any) -> int:
    try:
        number = int(value)
    except (TypeError, ValueError):
        return 0
    return max(0, number)


def _text(value: Any, fallback: str) -> str:
    text = str(value).strip() if value is not None else ""
    return text or fallback


def _redacted_metadata(metadata: dict[str, Any]) -> dict[str, Any]:
    redacted = dict(metadata) if isinstance(metadata, dict) else {}
    for key in (
        "reference_audio_base64",
        "reference_audio_data",
        "reference_audio_bytes",
        "audio_base64",
    ):
        redacted.pop(key, None)
    return redacted


def _persist_reference_audio(artifact_dir: Path, request: VoiceTrainingRequest) -> tuple[str, dict[str, Any]]:
    metadata = request.metadata if isinstance(request.metadata, dict) else {}
    upload = _text(
        metadata.get("reference_audio_base64")
        or metadata.get("reference_audio_data")
        or metadata.get("audio_base64"),
        "",
    )
    file_name = _safe_audio_file_name(
        metadata.get("reference_audio_file_name")
        or metadata.get("file_name")
        or request.reference_audio_file
        or Path(request.reference_audio_path).name
        or "reference.wav"
    )
    target_path = artifact_dir.joinpath(file_name)
    diagnostics: dict[str, Any] = {
        "reference_audio_persist_source": "none",
        "reference_audio_persist_error": "",
    }

    try:
        artifact_dir.mkdir(parents=True, exist_ok=True)
    except Exception as exc:
        diagnostics["reference_audio_persist_error"] = f"artifact directory error: {exc}"
        return "", diagnostics

    if upload:
        try:
            audio_bytes = base64.b64decode(upload, validate=True)
            if not audio_bytes:
                raise ValueError("uploaded reference audio is empty")
            target_path.write_bytes(audio_bytes)
            diagnostics["reference_audio_persist_source"] = "client_upload"
            diagnostics["reference_audio_uploaded_size_bytes"] = len(audio_bytes)
            return str(target_path), diagnostics
        except (binascii.Error, ValueError, OSError) as exc:
            diagnostics["reference_audio_persist_error"] = f"client upload error: {exc}"
            return "", diagnostics

    source_path = Path(request.reference_audio_path)
    if not source_path.is_file():
        diagnostics["reference_audio_persist_error"] = "reference audio path is not visible to the AI runtime"
        return "", diagnostics

    try:
        shutil.copyfile(source_path, target_path)
        diagnostics["reference_audio_persist_source"] = "runtime_path_copy"
        diagnostics["reference_audio_uploaded_size_bytes"] = target_path.stat().st_size
        return str(target_path), diagnostics
    except OSError as exc:
        diagnostics["reference_audio_persist_error"] = f"runtime copy error: {exc}"
        return "", diagnostics


def _safe_audio_file_name(value: Any) -> str:
    name = Path(str(value or "reference.wav")).name.strip() or "reference.wav"
    stem = Path(name).stem.strip() or "reference"
    suffix = Path(name).suffix.lower()
    if suffix not in {".wav", ".mp3", ".flac", ".m4a", ".ogg", ".aac"}:
        suffix = ".wav"
    safe_stem = "".join(char if char.isalnum() or char in {"-", "_", "."} else "_" for char in stem)
    return f"reference-{safe_stem[:80]}{suffix}"


def _initial_job_state(
    voice_clone_enabled: bool,
    audio_diagnostics: dict[str, Any],
    persisted: bool,
) -> tuple[str, str, int, str]:
    if not persisted or not audio_diagnostics.get("exists"):
        return (
            "blocked",
            "reference_not_visible",
            5,
            "参考音频没有进入 AI 运行时，请重新选择本地文件或检查容器挂载。",
        )
    if voice_clone_enabled:
        return (
            "queued",
            "waiting_for_worker",
            20,
            "Voice clone training request queued for the configured worker.",
        )
    if not audio_diagnostics.get("readable"):
        return (
            "blocked",
            "reference_unreadable",
            10,
            str(audio_diagnostics.get("recommended_next_step") or "参考音频元数据不可读。"),
        )
    if audio_diagnostics.get("needs_more_audio_for_inference"):
        return (
            "blocked",
            "reference_too_short",
            15,
            str(audio_diagnostics.get("recommended_next_step") or "请提供至少 5 秒清晰参考音频。"),
        )
    if audio_diagnostics.get("needs_reference_clip"):
        return (
            "ready",
            "needs_reference_clip",
            60,
            "声音素材已保存；建议再剪出 5-15 秒参考片段并填写对应提示词以提升 GPT-SoVITS 相似度。",
        )
    return (
        "ready",
        "ready_for_gpt_sovits",
        70,
        "声音参考片段已保存，可用于 GPT-SoVITS 零样本合成；请确认 GPT-SoVITS API 已启动并配置。",
    )


def diagnose_reference_audio(path: str) -> dict[str, Any]:
    return _audio_diagnostics(path)


def _audio_diagnostics(path: str) -> dict[str, Any]:
    audio_path = Path(path)
    exists = audio_path.exists() and audio_path.is_file()
    diagnostics = {
        "exists": exists,
        "size_bytes": audio_path.stat().st_size if exists else 0,
        "extension": audio_path.suffix.lower().lstrip("."),
        "readable": False,
        "codec": "",
        "duration_sec": 0.0,
        "sample_rate": 0,
        "channels": 0,
    }
    if exists:
        diagnostics.update(_ffprobe_audio(audio_path))
        if not diagnostics["readable"] and diagnostics["extension"] == "wav":
            diagnostics.update(_wave_audio(audio_path))
    diagnostics.update(_material_readiness(diagnostics))
    return diagnostics


def _ffprobe_audio(path: Path) -> dict[str, Any]:
    try:
        completed = subprocess.run(
            [
                "ffprobe",
                "-v",
                "error",
                "-show_entries",
                "format=duration:stream=codec_type,codec_name,sample_rate,channels",
                "-of",
                "json",
                str(path),
            ],
            check=False,
            capture_output=True,
            text=True,
            timeout=5,
        )
    except (FileNotFoundError, subprocess.SubprocessError, OSError):
        return {}
    if completed.returncode != 0 or not completed.stdout.strip():
        return {}
    try:
        payload = json.loads(completed.stdout)
    except json.JSONDecodeError:
        return {}
    streams = payload.get("streams") if isinstance(payload.get("streams"), list) else []
    audio_stream = next((item for item in streams if item.get("codec_type") == "audio"), {})
    duration = 0.0
    try:
        duration = float((payload.get("format") or {}).get("duration") or 0.0)
    except (TypeError, ValueError):
        duration = 0.0
    return {
        "readable": bool(audio_stream),
        "codec": str(audio_stream.get("codec_name") or ""),
        "duration_sec": round(max(0.0, duration), 3),
        "sample_rate": _non_negative_int(audio_stream.get("sample_rate")),
        "channels": _non_negative_int(audio_stream.get("channels")),
    }


def _wave_audio(path: Path) -> dict[str, Any]:
    try:
        import wave

        with wave.open(str(path), "rb") as handle:
            sample_rate = int(handle.getframerate() or 0)
            frames = int(handle.getnframes() or 0)
            duration = (frames / sample_rate) if sample_rate > 0 else 0.0
            return {
                "readable": True,
                "codec": "pcm_s16le",
                "duration_sec": round(max(0.0, duration), 3),
                "sample_rate": sample_rate,
                "channels": int(handle.getnchannels() or 0),
            }
    except Exception:
        return {}


def _material_readiness(diagnostics: dict[str, Any]) -> dict[str, Any]:
    exists = bool(diagnostics.get("exists"))
    size_bytes = _non_negative_int(diagnostics.get("size_bytes"))
    readable = bool(diagnostics.get("readable"))
    duration_sec = float(diagnostics.get("duration_sec") or 0.0)
    zero_shot_ready = readable and duration_sec >= 5.0
    few_shot_ready = readable and duration_sec >= 60.0

    if not exists:
        status = "missing"
        next_step = "Provide a reference audio file path that is visible to the AIOrchestrator runtime."
    elif size_bytes <= 0:
        status = "empty"
        next_step = "Provide a non-empty clean voice recording."
    elif not readable:
        status = "unreadable_metadata"
        next_step = "Provide WAV/MP3/FLAC audio with readable metadata, or install ffprobe in the runtime image."
    elif duration_sec < 5.0:
        status = "too_short"
        next_step = "Provide at least 5 seconds for zero-shot inference, and preferably 60 seconds or more for fine-tuning."
    elif duration_sec < 60.0:
        status = "zero_shot_ready"
        next_step = "Enough for GPT-SoVITS zero-shot inference; provide 60 seconds or more for better few-shot fine-tuning."
    else:
        status = "few_shot_ready"
        next_step = "Enough duration for few-shot preparation; segment a clean 5-15 second reference clip and transcribe it before synthesis or fine-tuning."

    return {
        "minimum_zero_shot_sec": 5.0,
        "recommended_reference_clip_max_sec": 15.0,
        "recommended_few_shot_sec": 60.0,
        "zero_shot_ready": zero_shot_ready,
        "few_shot_ready": few_shot_ready,
        "direct_reference_ready": readable and 5.0 <= duration_sec <= 15.0,
        "needs_reference_clip": readable and duration_sec > 15.0,
        "needs_more_audio_for_inference": not zero_shot_ready,
        "needs_more_audio_for_training": not few_shot_ready,
        "material_status": status,
        "recommended_next_step": next_step,
    }


def _language_code(value: str) -> str:
    text = _text(value, "zh-CN")
    aliases = {
        "中文": "zh-CN",
        "zh": "zh-CN",
        "zh_cn": "zh-CN",
        "zh-cn": "zh-CN",
        "日语": "ja-JP",
        "ja": "ja-JP",
        "ja_jp": "ja-JP",
        "ja-jp": "ja-JP",
        "英语": "en-US",
        "en": "en-US",
        "en_us": "en-US",
        "en-us": "en-US",
        "韩语": "ko-KR",
        "ko": "ko-KR",
        "ko_kr": "ko-KR",
        "ko-kr": "ko-KR",
        "法语": "fr-FR",
        "fr": "fr-FR",
        "fr_fr": "fr-FR",
        "fr-fr": "fr-FR",
        "西班牙语": "es-ES",
        "es": "es-ES",
        "es_es": "es-ES",
        "es-es": "es-ES",
    }
    normalized = aliases.get(text.lower(), aliases.get(text, text))
    if normalized in {"zh-CN", "ja-JP", "en-US", "ko-KR", "fr-FR", "es-ES"}:
        return normalized
    return "zh-CN"


def _job_from_dict(data: dict[str, Any]) -> VoiceTrainingJob:
    diagnostics = data.get("diagnostics") if isinstance(data.get("diagnostics"), dict) else {}
    metadata = data.get("metadata") if isinstance(data.get("metadata"), dict) else {}
    return VoiceTrainingJob(
        job_id=_text(data.get("job_id"), f"voice-train-{uuid.uuid4().hex}"),
        uid=_non_negative_int(data.get("uid")),
        profile_id=_text(data.get("profile_id"), "default"),
        voice_name=_text(data.get("voice_name"), "Kafuuchino-voice"),
        language=_language_code(_text(data.get("language"), "zh-CN")),
        reference_audio_path=_text(data.get("reference_audio_path"), ""),
        reference_audio_directory=_text(data.get("reference_audio_directory"), ""),
        reference_audio_file=_text(data.get("reference_audio_file"), ""),
        provider=_text(data.get("provider"), "gpt-sovits"),
        status=_text(data.get("status"), "prepared"),
        stage=_text(data.get("stage"), "ready_for_worker"),
        progress=max(0, min(100, _non_negative_int(data.get("progress")))),
        consent_confirmed=data.get("consent_confirmed") is True,
        consent_scope=_text(data.get("consent_scope"), "local_default_reference"),
        source=_text(data.get("source"), "src-default"),
        artifact_root=_text(data.get("artifact_root"), "/app/.data/pet-voice-training"),
        artifact_path=_text(data.get("artifact_path"), ""),
        manifest_path=_text(data.get("manifest_path"), ""),
        message=_text(data.get("message"), "Voice training request prepared."),
        created_at_ms=_non_negative_int(data.get("created_at_ms")),
        updated_at_ms=_non_negative_int(data.get("updated_at_ms")),
        diagnostics=dict(diagnostics),
        metadata=dict(metadata),
    )


def _normalize_loaded_job(job: VoiceTrainingJob) -> VoiceTrainingJob:
    diagnostics = dict(job.diagnostics or {})
    if diagnostics.get("voice_clone_enabled") is False and job.stage == "ready_for_worker":
        reference_ready = bool(
            diagnostics.get("gpt_sovits_reference_audio")
            or diagnostics.get("reference_audio_runtime_path")
            or job.reference_audio_path
        ) and bool(diagnostics.get("reference_audio_exists", True))
        if reference_ready:
            progress = max(int(job.progress or 0), 70)
            message = str(job.message or "").strip()
            if not message or "worker" in message.lower():
                message = "声音参考已就绪，可直接用于 GPT-SoVITS 零样本合成。"
            return VoiceTrainingJob(
                job_id=job.job_id,
                uid=job.uid,
                profile_id=job.profile_id,
                voice_name=job.voice_name,
                language=job.language,
                reference_audio_path=job.reference_audio_path,
                reference_audio_directory=job.reference_audio_directory,
                reference_audio_file=job.reference_audio_file,
                provider=job.provider,
                status="ready",
                stage="ready_for_gpt_sovits",
                progress=progress,
                consent_confirmed=job.consent_confirmed,
                consent_scope=job.consent_scope,
                source=job.source,
                artifact_root=job.artifact_root,
                artifact_path=job.artifact_path,
                manifest_path=job.manifest_path,
                message=message,
                created_at_ms=job.created_at_ms,
                updated_at_ms=job.updated_at_ms,
                diagnostics=diagnostics,
                metadata=job.metadata,
            )
    return job
