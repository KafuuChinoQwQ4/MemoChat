from __future__ import annotations

import asyncio
import uuid
from dataclasses import dataclass
from pathlib import Path
from typing import AsyncIterator

from .contracts import PetControlEvent, PetObservation, PetSession
from .event_bus import PetEventBus
from .policy import PetPolicy
from .providers import PetProviderError, PetProviderRouter
from .session_store import PetSessionStore
from .vision import LocalVisionAnalyzer, VisionCaptureRequest, VisionSegmentRequest
from .voice import GPTSoVITSVoiceProvider, VoiceProviderError, VoiceProviderRouter, VoiceSynthesisRequest
from .voice_training import VoiceTrainingJob, VoiceTrainingRequest, VoiceTrainingService, diagnose_reference_audio


@dataclass(frozen=True)
class PetRuntimeConfig:
    enabled: bool = True
    deterministic: bool = True
    live2d_native_enabled: bool = False
    live2d_sdk_root: str = ""
    asset_root: str = ""
    cloud_vision_enabled: bool = False
    local_vision_enabled: bool = False
    vision_camera_index: int = 0
    vision_analyzer: str = "opencv"
    face_landmarker_model_path: str = ""
    object_detector_model_path: str = ""
    vision_retain_raw_frames: bool = False
    voice_clone_enabled: bool = False
    voice_provider: str = "scripted"
    voice_sovits_base_url: str = ""
    voice_sovits_reference_audio: str = ""
    voice_sovits_prompt_text: str = ""
    voice_sovits_prompt_language: str = "zh"
    voice_sovits_text_language: str = "zh"
    voice_sovits_output_dir: str = "/app/.data/pet-voice-cache"
    voice_sovits_timeout_sec: float = 180.0
    pet_text_timeout_sec: float = 25.0
    voice_training_enabled: bool = True
    voice_training_artifact_root: str = "/app/.data/pet-voice-training"


class PetRuntime:
    """Provider-neutral in-memory pet runtime facade used by the API router."""

    def __init__(self, config: PetRuntimeConfig | object | None = None) -> None:
        self._config = _runtime_config_from(config)
        self._sessions = PetSessionStore()
        self._observations: dict[str, PetObservation] = {}
        self._events = PetEventBus()
        self._policy = PetPolicy()
        self._voice_router = VoiceProviderRouter(
            deterministic=self._config.deterministic,
            deterministic_output_dir=self._config.voice_sovits_output_dir,
        )
        self._gpt_sovits_provider: GPTSoVITSVoiceProvider | None = None
        self._register_configured_voice_providers()
        self._providers = PetProviderRouter(
            deterministic=self._config.deterministic,
            voice_router=self._voice_router,
        )
        self._vision = LocalVisionAnalyzer(
            enabled=self._config.local_vision_enabled,
            default_camera_index=self._config.vision_camera_index,
            analyzer=self._config.vision_analyzer,
            face_landmarker_model_path=self._config.face_landmarker_model_path,
            object_detector_model_path=self._config.object_detector_model_path,
            retain_raw_frames=self._config.vision_retain_raw_frames,
        )
        self._voice_training = VoiceTrainingService(
            voice_clone_enabled=self._config.voice_clone_enabled,
            artifact_root=self._config.voice_training_artifact_root,
        )

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
        metadata: dict | None = None,
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

        runtime_metadata = self._voice_runtime_metadata()
        runtime_metadata["text_timeout_sec"] = self._config.pet_text_timeout_sec
        runtime_metadata.update(_request_runtime_metadata(metadata))
        visual_summary = _visual_summary_runtime_metadata(self._policy.visual_summary(session_id))
        if visual_summary:
            runtime_metadata["visual_summary"] = visual_summary

        try:
            chunks = await self._providers.generate(
                session,
                prompt,
                runtime_metadata=runtime_metadata,
            )
        except PetProviderError as exc:
            events.append(await self._emit(session_id, turn_id=turn_id, **self._policy.provider_error(exc)))
            return events

        for chunk in chunks:
            event_args = self._policy.provider_chunk(chunk)
            if bool(getattr(chunk, "metadata", {}).get("voice_deferred")):
                event_args["audio_state"] = "loading"
                event_args.setdefault("debug", {})["voice"] = {
                    "provider": str(chunk.metadata.get("voice_provider") or ""),
                    "voice": str(chunk.metadata.get("voice_name") or ""),
                    "state": "loading",
                    "duration_ms": 0,
                    "retention": "none",
                    "metadata": {
                        "async": True,
                        "language": str(chunk.metadata.get("voice_language") or chunk.metadata.get("language") or "zh-CN"),
                        "text_lang": str(chunk.metadata.get("text_lang") or ""),
                    },
                }
                self._schedule_provider_chunk_voice(session, turn_id, chunk, event_args)
            events.append(await self._emit(session_id, turn_id=turn_id, **event_args))

        events.append(await self._emit(session_id, turn_id=turn_id, **self._policy.input_finished()))
        return events

    async def update_observation(self, observation: PetObservation) -> PetControlEvent:
        session = self._sessions.touch(observation.session_id, status="active")
        self._observations[observation.session_id] = observation
        turn_id = f"petturn-observe-{uuid.uuid4().hex}"
        event_args = self._policy.observation(observation)
        self._schedule_visual_voice(session, turn_id, event_args, observation)
        return await self._emit(
            observation.session_id,
            turn_id=turn_id,
            **event_args,
        )

    async def capture_observation(
        self,
        request: VisionCaptureRequest,
    ) -> tuple[PetObservation, PetControlEvent]:
        self._sessions.touch(request.session_id, status="active")
        observation = self._vision.capture(request)
        event = await self.update_observation(observation)
        return observation, event

    async def capture_segment_observation(
        self,
        request: VisionSegmentRequest,
    ) -> tuple[PetObservation, PetControlEvent]:
        self._sessions.touch(request.session_id, status="active")
        observation = self._vision.capture_segment(request)
        event = await self.update_observation(observation)
        return observation, event

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

    def create_voice_training_job(self, request: VoiceTrainingRequest) -> VoiceTrainingJob:
        if not self._config.enabled:
            raise RuntimeError("desktop pet is disabled by configuration")
        if not self._config.voice_training_enabled:
            raise RuntimeError("desktop pet voice training is disabled by configuration")
        return self._voice_training.create_job(request)

    def get_voice_training_job(self, job_id: str) -> VoiceTrainingJob:
        return self._voice_training.get_job(job_id)

    def list_voice_training_jobs(self, uid: int = 0) -> list[VoiceTrainingJob]:
        return self._voice_training.list_jobs(uid)

    async def diagnostics(
        self,
        probe_voice_endpoint: bool = False,
        open_camera: bool = False,
    ) -> dict:
        voice = await self.voice_diagnostics(probe_endpoint=probe_voice_endpoint)
        vision = self.vision_diagnostics(open_camera=open_camera)
        ready = bool(voice.get("ready")) and bool(vision.get("ready"))
        return {
            "ready": ready,
            "voice": voice,
            "vision": vision,
            "needs_user_voice_material": bool(voice.get("needs_user_material")),
            "needs_camera_runtime": not bool(vision.get("ready")),
        }

    async def voice_diagnostics(self, probe_endpoint: bool = False) -> dict:
        reference_audio = self._configured_or_trained_reference_audio()
        prompt_text = self._configured_or_trained_prompt_text(reference_audio)
        audio = diagnose_reference_audio(reference_audio) if reference_audio else diagnose_reference_audio("")
        provider = GPTSoVITSVoiceProvider(
            base_url=self._config.voice_sovits_base_url,
            reference_audio=reference_audio,
            prompt_text=prompt_text,
            prompt_language=self._config.voice_sovits_prompt_language,
            text_language=self._config.voice_sovits_text_language,
            output_dir=self._config.voice_sovits_output_dir,
            timeout_sec=self._config.voice_sovits_timeout_sec,
        )
        provider_diagnostics = await provider.diagnostics(probe_endpoint=probe_endpoint)
        needs_material = bool(audio.get("needs_more_audio_for_inference")) or not bool(reference_audio)
        return {
            "ready": bool(provider_diagnostics.get("ready")) and bool(audio.get("zero_shot_ready")),
            "provider": provider_diagnostics,
            "reference_audio": audio,
            "needs_user_material": needs_material,
            "needs_reference_clip": bool(audio.get("needs_reference_clip")),
            "needs_prompt_text": bool(reference_audio) and bool(audio.get("zero_shot_ready")) and not bool(prompt_text.strip()),
            "needs_training_material": bool(audio.get("needs_more_audio_for_training")),
            "message": _voice_diagnostics_message(provider_diagnostics, audio, bool(reference_audio)),
        }

    def vision_diagnostics(
        self,
        camera_index: int | None = None,
        open_camera: bool = False,
    ) -> dict:
        return self._vision.diagnostics(camera_index=camera_index, open_camera=open_camera)

    @property
    def config(self) -> PetRuntimeConfig:
        return self._config

    async def _emit(self, session_id: str, **kwargs) -> PetControlEvent:
        return await self._events.publish(session_id, **kwargs)

    def _schedule_visual_voice(
        self,
        session: PetSession,
        turn_id: str,
        event_args: dict,
        observation: PetObservation,
    ) -> None:
        speech_text = str(event_args.get("speech_text") or "").strip()
        if not speech_text:
            return

        runtime_metadata = self._visual_voice_runtime_metadata(event_args, observation)
        provider = str(runtime_metadata.get("voice_provider") or "scripted")
        voice_name = str(runtime_metadata.get("voice_name") or provider or "deterministic")
        language = str(runtime_metadata.get("language") or "zh-CN")
        if _normalize_provider(provider) in {"", "scripted", "deterministic"}:
            event_args["audio_state"] = "error"
            event_args.setdefault("debug", {})["voice"] = self._visual_voice_error_payload(
                provider=provider or "scripted",
                voice_name=voice_name,
                language=language,
                message="visual speech requires a configured non-deterministic voice provider",
                recoverable=True,
            )
            return

        event_args["audio_state"] = "loading"
        event_args.setdefault("debug", {})["voice"] = {
            "provider": provider,
            "voice": voice_name,
            "state": "loading",
            "duration_ms": 0,
            "retention": "none",
            "metadata": {
                "language": language,
                "text_lang": runtime_metadata.get("text_lang") or _voice_text_language(language),
                "async": True,
            },
        }
        try:
            asyncio.create_task(
                self._emit_visual_voice_when_ready(
                    session=session,
                    turn_id=turn_id,
                    speech_text=speech_text,
                    provider=provider,
                    voice_name=voice_name,
                    language=language,
                    runtime_metadata=runtime_metadata,
                    event_args=event_args,
                )
            )
        except RuntimeError:
            event_args["audio_state"] = "error"
            event_args.setdefault("debug", {})["voice"] = self._visual_voice_error_payload(
                provider=provider,
                voice_name=voice_name,
                language=language,
                message="event loop is not available for async visual voice synthesis",
                recoverable=True,
            )

    async def _emit_visual_voice_when_ready(
        self,
        session: PetSession,
        turn_id: str,
        speech_text: str,
        provider: str,
        voice_name: str,
        language: str,
        runtime_metadata: dict,
        event_args: dict,
    ) -> None:
        try:
            voice = await self._voice_router.synthesize(
                VoiceSynthesisRequest(
                    session_id=session.session_id,
                    turn_id=turn_id,
                    text=speech_text,
                    provider=provider,
                    voice=voice_name,
                    language=language,
                    metadata=runtime_metadata,
                )
            )
            voice_payload = voice.to_dict()
            payload = self._visual_voice_ready_event_args(event_args, voice_payload, provider, voice_name)
        except VoiceProviderError as exc:
            payload = self._visual_voice_error_event_args(event_args, exc, provider, voice_name, language)
        await self._emit(session.session_id, turn_id=turn_id, **payload)

    def _schedule_provider_chunk_voice(
        self,
        session: PetSession,
        turn_id: str,
        chunk,
        event_args: dict,
    ) -> None:
        text = str(getattr(chunk, "text", "") or "").strip()
        if not text:
            return
        metadata = dict(getattr(chunk, "metadata", {}) or {})
        provider = str(metadata.get("voice_provider") or "scripted")
        voice_name = str(metadata.get("voice_name") or provider or "deterministic")
        language = str(metadata.get("voice_language") or metadata.get("language") or "zh-CN")
        if _normalize_provider(provider) in {"", "scripted", "deterministic"}:
            return
        try:
            asyncio.create_task(
                self._emit_provider_chunk_voice_when_ready(
                    session=session,
                    turn_id=turn_id,
                    speech_text=text,
                    provider=provider,
                    voice_name=voice_name,
                    language=language,
                    runtime_metadata=metadata,
                    event_args=event_args,
                )
            )
        except RuntimeError:
            event_args["audio_state"] = "error"
            event_args.setdefault("debug", {})["voice"] = self._visual_voice_error_payload(
                provider=provider,
                voice_name=voice_name,
                language=language,
                message="event loop is not available for async provider voice synthesis",
                recoverable=True,
            )

    async def _emit_provider_chunk_voice_when_ready(
        self,
        session: PetSession,
        turn_id: str,
        speech_text: str,
        provider: str,
        voice_name: str,
        language: str,
        runtime_metadata: dict,
        event_args: dict,
    ) -> None:
        try:
            voice = await self._voice_router.synthesize(
                VoiceSynthesisRequest(
                    session_id=session.session_id,
                    turn_id=turn_id,
                    text=speech_text,
                    provider=provider,
                    voice=voice_name,
                    language=language,
                    metadata=runtime_metadata,
                )
            )
            payload = self._visual_voice_ready_event_args(
                event_args,
                voice.to_dict(),
                provider,
                voice_name,
            )
        except VoiceProviderError as exc:
            payload = self._visual_voice_error_event_args(event_args, exc, provider, voice_name, language)
        await self._emit(session.session_id, turn_id=turn_id, **payload)

    def _visual_voice_runtime_metadata(self, event_args: dict, observation: PetObservation) -> dict:
        runtime_metadata = self._voice_runtime_metadata()
        runtime_metadata.update(_request_runtime_metadata(_observation_voice_metadata(observation)))
        language = str(
            event_args.get("speech_language")
            or runtime_metadata.get("reply_language")
            or runtime_metadata.get("language")
            or runtime_metadata.get("voice_language")
            or "zh-CN"
        ).strip() or "zh-CN"
        runtime_metadata.setdefault("reply_language", language)
        runtime_metadata.setdefault("language", language)
        runtime_metadata["voice_language"] = language
        runtime_metadata["text_lang"] = _voice_text_language(language)
        return runtime_metadata

    def _visual_voice_ready_event_args(
        self,
        base_args: dict,
        voice_payload: dict,
        provider: str,
        voice_name: str,
    ) -> dict:
        event_args = dict(base_args)
        event_args["debug"] = dict(base_args.get("debug") or {})
        event_args["phase"] = "audio_ready"
        event_args["speech_text"] = ""
        event_args["speech_translation"] = ""
        event_args["text_final"] = False
        event_args["audio_state"] = voice_payload.get("state") or "text-only"
        event_args["audio_sample_rate"] = voice_payload.get("sample_rate") or 0
        event_args["audio_duration_ms"] = voice_payload.get("duration_ms") or 0
        event_args["audio_chunk_ref"] = voice_payload.get("chunk_ref")
        event_args["audio_url"] = voice_payload.get("url")
        event_args["audio_phoneme"] = voice_payload.get("phoneme")
        event_args["audio_viseme"] = voice_payload.get("viseme")
        event_args["privacy_retention"] = voice_payload.get("retention") or "none"
        event_args["lip_sync"] = max(float(event_args.get("lip_sync") or 0.0), float(voice_payload.get("rms") or 0.0))
        event_args.setdefault("debug", {})["voice"] = {
            "provider": voice_payload.get("provider") or provider,
            "voice": voice_payload.get("voice") or voice_name,
            "state": voice_payload.get("state") or "text-only",
            "duration_ms": voice_payload.get("duration_ms") or 0,
            "retention": voice_payload.get("retention") or "none",
            "metadata": voice_payload.get("metadata") or {},
        }
        return event_args

    def _visual_voice_error_payload(
        self,
        provider: str,
        voice_name: str,
        language: str,
        message: str,
        recoverable: bool,
    ) -> dict:
        return {
            "provider": provider,
            "voice": voice_name,
            "state": "error",
            "duration_ms": 0,
            "retention": "none",
            "metadata": {
                "language": language,
                "text_lang": _voice_text_language(language),
                "audio_error": message,
                "audio_error_provider": provider,
                "audio_error_recoverable": bool(recoverable),
                "async": True,
            },
        }

    def _visual_voice_error_event_args(
        self,
        base_args: dict,
        error: VoiceProviderError,
        provider: str,
        voice_name: str,
        language: str,
    ) -> dict:
        event_args = dict(base_args)
        event_args["debug"] = dict(base_args.get("debug") or {})
        event_args["phase"] = "audio_ready"
        event_args["speech_text"] = ""
        event_args["speech_translation"] = ""
        event_args["text_final"] = False
        event_args["audio_state"] = "error"
        event_args.setdefault("debug", {})["voice"] = self._visual_voice_error_payload(
            provider=error.provider or provider,
            voice_name=voice_name,
            language=language,
            message=str(error),
            recoverable=bool(error.recoverable),
        )
        return event_args

    def audio_file_path(self, file_name: str):
        safe_name = Path(str(file_name or "")).name
        if not safe_name:
            raise KeyError("audio file name is empty")
        path = Path(self._config.voice_sovits_output_dir).joinpath(safe_name)
        root = Path(self._config.voice_sovits_output_dir).resolve()
        resolved = path.resolve()
        if root not in (resolved, *resolved.parents):
            raise KeyError("audio file path escapes voice cache")
        if not resolved.exists() or not resolved.is_file():
            raise KeyError(f"audio file not found: {safe_name}")
        return resolved

    def _register_configured_voice_providers(self) -> None:
        if _normalize_provider(self._config.voice_provider) == "gpt-sovits" or self._config.voice_sovits_base_url:
            provider = GPTSoVITSVoiceProvider(
                base_url=self._config.voice_sovits_base_url,
                reference_audio=self._config.voice_sovits_reference_audio,
                prompt_text=self._config.voice_sovits_prompt_text,
                prompt_language=self._config.voice_sovits_prompt_language,
                text_language=self._config.voice_sovits_text_language,
                output_dir=self._config.voice_sovits_output_dir,
                timeout_sec=self._config.voice_sovits_timeout_sec,
            )
            self._gpt_sovits_provider = provider
            self._voice_router.register("gpt-sovits", provider)
            self._voice_router.register("sovits", provider)
            self._voice_router.register("SoVITS", provider)

    def _voice_runtime_metadata(self) -> dict:
        provider = _normalize_provider(self._config.voice_provider)
        reference_audio = self._configured_or_trained_reference_audio()
        prompt_text = self._configured_or_trained_prompt_text(reference_audio)
        if provider in {"", "scripted", "deterministic"}:
            if not self._config.voice_sovits_base_url or not reference_audio:
                return {}
            provider = "gpt-sovits"
        if provider == "gpt-sovits" and not reference_audio:
            return {}
        return {
            "voice_provider": provider,
            "voice_name": provider,
            "voice_language": self._config.voice_sovits_text_language or "zh",
            "ref_audio_path": reference_audio,
            "prompt_text": prompt_text,
            "prompt_lang": self._config.voice_sovits_prompt_language,
            "text_lang": self._config.voice_sovits_text_language,
        }

    def _configured_or_trained_reference_audio(self) -> str:
        configured = self._config.voice_sovits_reference_audio.strip()
        if configured:
            return configured
        return self._voice_training.latest_ready_reference_audio()

    def _configured_or_trained_prompt_text(self, reference_audio: str) -> str:
        configured = self._config.voice_sovits_prompt_text.strip()
        if configured:
            return configured
        if not reference_audio:
            return ""
        return _prompt_text_from_reference_audio(reference_audio)

    async def _attach_visual_voice(
        self,
        session: PetSession,
        turn_id: str,
        event_args: dict,
        observation: PetObservation,
    ) -> None:
        speech_text = str(event_args.get("speech_text") or "").strip()
        if not speech_text:
            return

        runtime_metadata = self._voice_runtime_metadata()
        runtime_metadata.update(_request_runtime_metadata(_observation_voice_metadata(observation)))
        language = str(
            event_args.get("speech_language")
            or runtime_metadata.get("reply_language")
            or runtime_metadata.get("language")
            or runtime_metadata.get("voice_language")
            or "zh-CN"
        ).strip() or "zh-CN"
        runtime_metadata.setdefault("reply_language", language)
        runtime_metadata.setdefault("language", language)
        runtime_metadata["voice_language"] = language
        runtime_metadata["text_lang"] = _voice_text_language(language)
        provider = str(runtime_metadata.get("voice_provider") or "scripted")
        voice_name = str(runtime_metadata.get("voice_name") or provider or "deterministic")
        if _normalize_provider(provider) in {"", "scripted", "deterministic"}:
            event_args["audio_state"] = "error"
            event_args.setdefault("debug", {})["voice"] = {
                "provider": provider or "scripted",
                "voice": voice_name,
                "state": "error",
                "duration_ms": 0,
                "retention": "none",
                "metadata": {
                    "language": language,
                    "text_lang": _voice_text_language(language),
                    "audio_error": "visual speech requires a configured non-deterministic voice provider",
                    "audio_error_provider": provider or "scripted",
                    "audio_error_recoverable": True,
                },
            }
            return

        try:
            voice = await self._voice_router.synthesize(
                VoiceSynthesisRequest(
                    session_id=session.session_id,
                    turn_id=turn_id,
                    text=speech_text,
                    provider=provider,
                    voice=voice_name,
                    language=language,
                    metadata=runtime_metadata,
                )
            )
        except VoiceProviderError as exc:
            event_args["audio_state"] = "error"
            event_args.setdefault("debug", {})["voice"] = {
                "provider": exc.provider or provider,
                "voice": voice_name,
                "state": "error",
                "duration_ms": 0,
                "retention": "none",
                "metadata": {
                    "language": language,
                    "text_lang": _voice_text_language(language),
                    "audio_error": str(exc),
                    "audio_error_provider": exc.provider or provider,
                    "audio_error_recoverable": bool(exc.recoverable),
                },
            }
            return

        voice_payload = voice.to_dict()
        event_args["audio_state"] = voice_payload.get("state") or "text-only"
        event_args["audio_sample_rate"] = voice_payload.get("sample_rate") or 0
        event_args["audio_duration_ms"] = voice_payload.get("duration_ms") or 0
        event_args["audio_chunk_ref"] = voice_payload.get("chunk_ref")
        event_args["audio_url"] = voice_payload.get("url")
        event_args["audio_phoneme"] = voice_payload.get("phoneme")
        event_args["audio_viseme"] = voice_payload.get("viseme")
        event_args["privacy_retention"] = voice_payload.get("retention") or "none"
        event_args["lip_sync"] = max(float(event_args.get("lip_sync") or 0.0), float(voice_payload.get("rms") or 0.0))
        event_args.setdefault("debug", {})["voice"] = {
            "provider": voice_payload.get("provider") or provider,
            "voice": voice_payload.get("voice") or voice_name,
            "state": voice_payload.get("state") or "text-only",
            "duration_ms": voice_payload.get("duration_ms") or 0,
            "retention": voice_payload.get("retention") or "none",
            "metadata": voice_payload.get("metadata") or {},
        }


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
        local_vision_enabled=bool(getattr(config, "local_vision_enabled", False)),
        vision_camera_index=_non_negative_int(getattr(config, "vision_camera_index", 0)),
        vision_analyzer=str(getattr(config, "vision_analyzer", "opencv") or "opencv"),
        face_landmarker_model_path=str(getattr(config, "face_landmarker_model_path", "") or ""),
        object_detector_model_path=str(getattr(config, "object_detector_model_path", "") or ""),
        vision_retain_raw_frames=bool(getattr(config, "vision_retain_raw_frames", False)),
        voice_clone_enabled=bool(getattr(config, "voice_clone_enabled", False)),
        voice_provider=str(getattr(config, "voice_provider", "scripted") or "scripted"),
        voice_sovits_base_url=str(getattr(config, "voice_sovits_base_url", "") or ""),
        voice_sovits_reference_audio=str(getattr(config, "voice_sovits_reference_audio", "") or ""),
        voice_sovits_prompt_text=str(getattr(config, "voice_sovits_prompt_text", "") or ""),
        voice_sovits_prompt_language=str(getattr(config, "voice_sovits_prompt_language", "zh") or "zh"),
        voice_sovits_text_language=str(getattr(config, "voice_sovits_text_language", "zh") or "zh"),
        voice_sovits_output_dir=str(
            getattr(config, "voice_sovits_output_dir", "/app/.data/pet-voice-cache")
            or "/app/.data/pet-voice-cache"
        ),
        voice_sovits_timeout_sec=_positive_float(getattr(config, "voice_sovits_timeout_sec", 180.0), 180.0),
        pet_text_timeout_sec=_positive_float(getattr(config, "pet_text_timeout_sec", 25.0), 25.0, minimum=0.001),
        voice_training_enabled=bool(getattr(config, "voice_training_enabled", True)),
        voice_training_artifact_root=str(
            getattr(config, "voice_training_artifact_root", "/app/.data/pet-voice-training")
            or "/app/.data/pet-voice-training"
        ),
    )


def _normalize_provider(value: str) -> str:
    return str(value or "").strip().lower()


def _non_negative_int(value) -> int:
    try:
        number = int(value)
    except (TypeError, ValueError):
        return 0
    return max(0, number)


def _positive_float(value, fallback: float, minimum: float = 1.0) -> float:
    try:
        number = float(value)
    except (TypeError, ValueError):
        return fallback
    return max(minimum, number)


def _voice_diagnostics_message(provider: dict, audio: dict, reference_configured: bool) -> str:
    if not reference_configured:
        return "No GPT-SoVITS reference audio is configured; ask the user for a clean voice sample."
    if audio.get("needs_more_audio_for_inference"):
        return str(audio.get("recommended_next_step") or "Provide at least 5 seconds of clean reference audio.")
    if audio.get("needs_reference_clip"):
        return "Voice material is long enough, but GPT-SoVITS synthesis still needs a clean 5-15 second reference clip plus matching prompt text."
    if not provider.get("configured"):
        return "Reference audio is usable, but GPT-SoVITS base URL is not configured."
    if provider.get("status") == "endpoint_unreachable":
        return str(provider.get("message") or "GPT-SoVITS endpoint is unreachable.")
    if audio.get("needs_more_audio_for_training"):
        return str(audio.get("recommended_next_step") or "Provide more audio for better few-shot training.")
    return "Voice material and GPT-SoVITS configuration look ready for the next integration step."


def _prompt_text_from_reference_audio(reference_audio: str) -> str:
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


def _request_runtime_metadata(metadata: dict | None) -> dict:
    if not isinstance(metadata, dict):
        return {}
    result: dict = {}
    reply_language = _metadata_text(metadata, "reply_language", "language", "target_language")
    if reply_language:
        result["reply_language"] = reply_language
        result["language"] = reply_language
        result["voice_language"] = reply_language
        result["text_lang"] = _voice_text_language(reply_language)
    voice_language = _metadata_text(metadata, "voice_language", "voiceLanguage")
    if voice_language:
        result["voice_language"] = voice_language
    text_lang = _metadata_text(metadata, "text_lang", "textLanguage")
    if text_lang:
        result["text_lang"] = _voice_text_language(text_lang)
    speech_rules = _metadata_text(metadata, "speech_rules", "speechRules")
    if speech_rules:
        result["speech_rules"] = speech_rules
    voice_provider = _metadata_text(metadata, "voice_provider", "voiceProvider", "provider")
    if voice_provider:
        result["voice_provider"] = _normalize_provider(voice_provider) or voice_provider
    voice_name = _metadata_text(metadata, "voice_name", "voiceName")
    if voice_name:
        result["voice_name"] = voice_name
    reference_audio = _metadata_text(
        metadata,
        "ref_audio_path",
        "refAudioPath",
        "reference_audio_path",
        "referenceAudioPath",
        "voiceTrainingReferenceAudioPath",
    )
    if reference_audio:
        result["ref_audio_path"] = reference_audio
    prompt_text = _metadata_text(metadata, "prompt_text", "promptText")
    if prompt_text:
        result["prompt_text"] = prompt_text
    prompt_lang = _metadata_text(metadata, "prompt_lang", "promptLanguage")
    if prompt_lang:
        result["prompt_lang"] = _voice_text_language(prompt_lang)
    reference_source = _metadata_text(metadata, "reference_audio_source", "referenceAudioSource")
    if reference_source:
        result["reference_audio_source"] = reference_source
    if result.get("ref_audio_path") and not result.get("voice_provider"):
        result["voice_provider"] = "gpt-sovits"
    if result.get("voice_provider") and not result.get("voice_name"):
        result["voice_name"] = str(result["voice_provider"])
    return result


def _observation_voice_metadata(observation: PetObservation) -> dict:
    vision = observation.vision if isinstance(observation.vision, dict) else {}
    return _request_runtime_metadata(vision)


def _metadata_text(metadata: dict, *keys: str) -> str:
    for key in keys:
        value = str(metadata.get(key) or "").strip()
        if value:
            return value
    return ""


def _visual_summary_runtime_metadata(summary: dict | None) -> dict:
    if not isinstance(summary, dict) or not summary:
        return {}
    result = dict(summary)
    summary_text = str(
        result.get("summary_text")
        or result.get("cached_summary_text")
        or result.get("speech_text")
        or ""
    ).strip()
    if not summary_text:
        return {}
    result["summary_text"] = summary_text
    if not str(result.get("speech_text") or "").strip():
        result["speech_text"] = summary_text
    cached_summary_text = str(result.get("cached_summary_text") or summary_text).strip()
    if cached_summary_text:
        result["cached_summary_text"] = cached_summary_text
    return result


def _voice_text_language(language: str) -> str:
    normalized = str(language or "").strip().lower().replace("_", "-")
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
        "fr-fr": "fr",
        "fr": "fr",
        "es-es": "es",
        "es": "es",
    }
    return aliases.get(normalized, normalized or "zh")
