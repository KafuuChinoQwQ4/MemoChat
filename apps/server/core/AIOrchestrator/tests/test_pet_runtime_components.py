from __future__ import annotations

import json
import asyncio
import importlib
import inspect
import sys
import unittest
import tempfile
from unittest import mock
from pathlib import Path
import types

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from harness.pet import PetControlEvent


def _load_attr(module_name: str, attr_name: str):
    try:
        module = importlib.import_module(module_name)
    except ImportError as exc:
        raise unittest.SkipTest(f"{module_name} is not available yet") from exc
    try:
        return getattr(module, attr_name)
    except AttributeError as exc:
        raise unittest.SkipTest(f"{module_name}.{attr_name} is not available yet") from exc


class PetRuntimeComponentTests(unittest.IsolatedAsyncioTestCase):
    async def test_event_bus_sequences_are_per_session_and_heartbeats_are_not_queued(self):
        PetEventBus = _load_attr("harness.pet.event_bus", "PetEventBus")
        bus = PetEventBus()

        first = await bus.emit(
            PetControlEvent(session_id="first", seq=0, timestamp_ms=1, action={"name": "first"})
        )
        second = await bus.emit(
            PetControlEvent(session_id="second", seq=0, timestamp_ms=2, action={"name": "second"})
        )
        heartbeat = await bus.heartbeat("first")

        self.assertEqual(first.to_dict()["seq"], 1)
        self.assertEqual(second.to_dict()["seq"], 1)
        self.assertEqual(heartbeat.to_dict()["seq"], 2)
        self.assertEqual(heartbeat.to_dict()["action"]["name"], "heartbeat")

        stream = bus.stream("first", heartbeat_sec=0.01)
        queued_payload = (await stream.__anext__()).to_dict()
        next_payload = (await stream.__anext__()).to_dict()
        await stream.aclose()

        self.assertEqual(queued_payload["action"]["name"], "first")
        self.assertEqual(next_payload["action"]["name"], "heartbeat")
        self.assertGreater(next_payload["seq"], queued_payload["seq"])

    async def test_session_store_lists_updated_sessions_descending(self):
        PetSessionStore = _load_attr("harness.pet.session_store", "PetSessionStore")
        store = PetSessionStore()

        older = store.create_session(uid=7, profile_id="older", persona="memo-pet", provider="scripted")
        await asyncio.sleep(0.002)
        newer = store.create_session(uid=7, profile_id="newer", persona="memo-pet", provider="scripted")
        await asyncio.sleep(0.002)
        other = store.create_session(uid=99, profile_id="other", persona="memo-pet", provider="scripted")
        store.touch_session(older.session_id)

        self.assertEqual([item.session_id for item in store.list_sessions(uid=7)], [older.session_id, newer.session_id])
        self.assertEqual(store.list_sessions()[0].session_id, older.session_id)
        self.assertNotIn(other.session_id, [item.session_id for item in store.list_sessions(uid=7)])
        with self.assertRaises(KeyError):
            store.require_session("missing-session")

    def test_animation_mapper_normalizes_provider_emotions(self):
        AnimationMapper = _load_attr("harness.pet.animation", "AnimationMapper")
        mapper = AnimationMapper()

        cheerful = mapper.map_emotion("happy", phase="speaking")
        error = mapper.map_emotion("provider_error", phase="error")

        cheerful_payload = cheerful.to_dict() if hasattr(cheerful, "to_dict") else dict(cheerful)
        error_payload = error.to_dict() if hasattr(error, "to_dict") else dict(error)
        self.assertEqual(cheerful_payload["expression"], "smile_soft")
        self.assertIn(cheerful_payload["motion"], {"talk", "speak", "idle"})
        self.assertEqual(error_payload["expression"], "concerned")
        self.assertIn(error_payload["motion"], {"idle", "stop"})

    async def test_deterministic_provider_returns_repeatable_text_response(self):
        DeterministicProvider = _load_attr(
            "harness.pet.providers.deterministic", "DeterministicProvider"
        )
        provider = DeterministicProvider()

        first = await _maybe_await(provider.generate_text("hello", model_type="scripted", model_name="deterministic"))
        second = await _maybe_await(provider.generate_text("hello", model_type="scripted", model_name="deterministic"))

        self.assertEqual(first, second)
        self.assertNotIn("hello", _response_text(first))
        self.assertTrue(_response_text(first).strip())

    async def test_deterministic_provider_respects_selected_reply_language(self):
        DeterministicProvider = _load_attr(
            "harness.pet.providers.deterministic", "DeterministicProvider"
        )
        PetPromptContext = _load_attr("harness.pet.persona", "PetPromptContext")
        VoiceSynthesisResult = _load_attr("harness.pet.voice", "VoiceSynthesisResult")

        class RecordingVoiceRouter:
            def __init__(self):
                self.requests = []

            async def synthesize(self, request):
                self.requests.append(request)
                return VoiceSynthesisResult(
                    text=request.text,
                    state="ready",
                    chunk_ref="gpt-sovits-ja",
                    url="/audio/gpt-sovits-ja.wav",
                    provider=request.provider,
                    voice=request.voice,
                    retention="ephemeral",
                    metadata=dict(request.metadata),
                )

        voice = RecordingVoiceRouter()
        provider = DeterministicProvider(voice)

        chunks = await provider.generate(
            PetPromptContext(
                session_id="pet-session",
                uid=7,
                profile_id="default",
                persona="memo-pet",
                provider="scripted",
                user_text="你好",
                runtime_metadata={
                    "reply_language": "ja-JP",
                    "language": "ja-JP",
                    "voice_language": "ja-JP",
                    "voice_provider": "gpt-sovits",
                    "voice_name": "chino",
                    "text_lang": "ja",
                },
            )
        )

        self.assertEqual(len(voice.requests), 1)
        self.assertEqual(voice.requests[0].provider, "gpt-sovits")
        self.assertEqual(voice.requests[0].language, "ja-JP")
        self.assertEqual(voice.requests[0].metadata["text_lang"], "ja")
        self.assertEqual(len(chunks), 1)
        self.assertEqual(chunks[0].text, "こんにちは、ここにいます。")
        self.assertEqual(chunks[0].voice.to_dict()["text"], "こんにちは、ここにいます。")
        self.assertEqual(chunks[0].voice.to_dict()["metadata"]["language"], "ja-JP")
        self.assertEqual(chunks[0].voice.to_dict()["metadata"]["text_lang"], "ja")

    async def test_voice_router_deterministic_synthesis_stays_text_only(self):
        VoiceSynthesisRequest = _load_attr("harness.pet.voice", "VoiceSynthesisRequest")
        VoiceProviderRouter = _load_attr("harness.pet.voice", "VoiceProviderRouter")
        router = VoiceProviderRouter(deterministic=True)
        request = VoiceSynthesisRequest(
            session_id="pet-session",
            turn_id="pet-turn",
            text="hello voice",
            provider="scripted",
            voice="deterministic",
        )

        result = await _maybe_await(router.synthesize(request))
        payload = result.to_dict() if hasattr(result, "to_dict") else vars(result)

        self.assertEqual(payload["text"], "hello voice")
        self.assertEqual(payload["state"], "text-only")
        self.assertEqual(payload["sample_rate"], 0)
        self.assertGreaterEqual(payload["duration_ms"], 0)
        self.assertIsNone(payload["url"])
        self.assertIsNone(payload["chunk_ref"])
        self.assertEqual(payload["provider"], "scripted")
        self.assertEqual(payload["voice"], "deterministic")
        self.assertEqual(payload["retention"], "none")

    async def test_voice_router_exposes_stream_and_interrupt_contract(self):
        VoiceProviderRouter = _load_attr("harness.pet.voice", "VoiceProviderRouter")
        DeterministicVoiceProvider = _load_attr("harness.pet.voice", "DeterministicVoiceProvider")

        router = VoiceProviderRouter(deterministic=True)
        provider = DeterministicVoiceProvider()

        self.assertTrue(callable(getattr(router, "stream", None)))
        self.assertTrue(callable(getattr(router, "interrupt", None)))
        self.assertTrue(callable(getattr(provider, "stream", None)))
        self.assertTrue(callable(getattr(provider, "interrupt", None)))

    async def test_voice_router_deterministic_stream_yields_text_only_segments(self):
        VoiceSynthesisRequest = _load_attr("harness.pet.voice", "VoiceSynthesisRequest")
        VoiceProviderRouter = _load_attr("harness.pet.voice", "VoiceProviderRouter")
        router = VoiceProviderRouter(deterministic=True)
        request = VoiceSynthesisRequest(
            session_id="pet-session",
            turn_id="pet-turn-stream",
            text="hello stream",
            provider="scripted",
            voice="deterministic",
        )

        segments = await _collect_voice_stream(router.stream(request))

        self.assertGreaterEqual(len(segments), 1)
        joined_text = "".join(_voice_payload(segment)["text"] for segment in segments)
        self.assertEqual(joined_text, "hello stream")
        for segment in segments:
            payload = _voice_payload(segment)
            self.assertTrue(payload["text"])
            self.assertEqual(payload["state"], "text-only")
            self.assertEqual(payload["sample_rate"], 0)
            self.assertIsNone(payload["url"])
            self.assertEqual(payload["retention"], "none")
            self.assertFalse(payload["metadata"].get("audio_persisted", True))

    async def test_voice_router_interrupt_is_text_only_and_idempotent(self):
        VoiceInterruptRequest = _load_attr("harness.pet.voice", "VoiceInterruptRequest")
        VoiceProviderRouter = _load_attr("harness.pet.voice", "VoiceProviderRouter")
        router = VoiceProviderRouter(deterministic=True)
        request = VoiceInterruptRequest(
            session_id="pet-session",
            turn_id="pet-turn-interrupt",
            provider="scripted",
            voice="deterministic",
            reason="user_interrupt",
        )

        first = _voice_payload(await _maybe_await(router.interrupt(request)))
        second = _voice_payload(await _maybe_await(router.interrupt(request)))

        for payload in (first, second):
            self.assertIn(payload["state"], {"text-only", "interrupted"})
            self.assertIsNone(payload["url"])
            self.assertIsNone(payload["chunk_ref"])
            self.assertEqual(payload["sample_rate"], 0)
            self.assertEqual(payload["duration_ms"], 0)
            self.assertEqual(payload["retention"], "none")
            self.assertFalse(payload["metadata"].get("audio_persisted", True))
            self.assertEqual(payload["metadata"].get("reason"), "user_interrupt")
            self.assertEqual(payload["metadata"].get("turn_id"), "pet-turn-interrupt")
        self.assertEqual(first["text"], second["text"])

    async def test_voice_router_unavailable_provider_falls_back_to_text_only_or_error(self):
        VoiceSynthesisRequest = _load_attr("harness.pet.voice", "VoiceSynthesisRequest")
        VoiceProviderRouter = _load_attr("harness.pet.voice", "VoiceProviderRouter")
        router = VoiceProviderRouter(deterministic=True)
        request = VoiceSynthesisRequest(
            session_id="pet-session",
            turn_id="pet-turn",
            text="fallback voice",
            provider="missing-provider",
            voice="deterministic",
        )

        try:
            result = await _maybe_await(router.synthesize(request))
        except Exception as exc:
            self.assertIn("provider", str(exc).lower())
            return

        payload = result.to_dict() if hasattr(result, "to_dict") else vars(result)
        self.assertEqual(payload["text"], "fallback voice")
        self.assertIn(payload["state"], {"text-only", "error"})
        self.assertIsNone(payload["url"])
        self.assertIsNone(payload["chunk_ref"])
        self.assertEqual(payload["retention"], "none")

    async def test_gpt_sovits_voice_provider_requires_base_url_and_reference_audio(self):
        GPTSoVITSVoiceProvider = _load_attr("harness.pet.voice", "GPTSoVITSVoiceProvider")
        VoiceSynthesisRequest = _load_attr("harness.pet.voice", "VoiceSynthesisRequest")
        provider = GPTSoVITSVoiceProvider()

        with self.assertRaisesRegex(Exception, "base URL"):
            await _maybe_await(
                provider.synthesize(
                    VoiceSynthesisRequest(
                        session_id="pet-session",
                        turn_id="pet-turn",
                        text="你好",
                        provider="gpt-sovits",
                        voice="gpt-sovits",
                    )
                )
            )

    async def test_gpt_sovits_voice_provider_diagnostics_reports_missing_reference(self):
        GPTSoVITSVoiceProvider = _load_attr("harness.pet.voice", "GPTSoVITSVoiceProvider")
        provider = GPTSoVITSVoiceProvider(base_url="http://127.0.0.1:9880")

        payload = await provider.diagnostics(probe_endpoint=False)

        self.assertFalse(payload["ready"])
        self.assertEqual(payload["status"], "missing_reference_audio")
        self.assertTrue(payload["configured"])
        self.assertFalse(payload["endpoint_probe_tested"])

    def test_gpt_sovits_language_aliases_match_api_values(self):
        voice_module = importlib.import_module("harness.pet.voice")

        self.assertEqual(voice_module._sovits_language("zh-CN"), "zh")
        self.assertEqual(voice_module._sovits_language("ja-JP"), "ja")
        self.assertEqual(voice_module._sovits_language("en-US"), "en")

    async def test_deterministic_pet_provider_synthesizes_external_voice_once_per_reply(self):
        DeterministicProvider = _load_attr(
            "harness.pet.providers.deterministic", "DeterministicProvider"
        )
        PetPromptContext = _load_attr("harness.pet.persona", "PetPromptContext")
        VoiceSynthesisResult = _load_attr("harness.pet.voice", "VoiceSynthesisResult")

        class CountingVoiceRouter:
            def __init__(self):
                self.requests = []

            async def synthesize(self, request):
                self.requests.append(request)
                return VoiceSynthesisResult(
                    text=request.text,
                    state="ready",
                    chunk_ref="gpt-sovits-test",
                    url="/audio/gpt-sovits-test.wav",
                    provider=request.provider,
                    voice=request.voice,
                    retention="ephemeral",
                )

        voice = CountingVoiceRouter()
        provider = DeterministicProvider(voice)
        chunks = await provider.generate(
            PetPromptContext(
                session_id="pet-session",
                uid=7,
                profile_id="default",
                persona="memo-pet",
                provider="scripted",
                user_text="你好，完整合成",
                runtime_metadata={"voice_provider": "gpt-sovits", "voice_name": "chino"},
            )
        )

        self.assertEqual(len(voice.requests), 1)
        self.assertEqual(len(chunks), 1)
        self.assertEqual(chunks[0].voice.to_dict()["provider"], "gpt-sovits")
        self.assertEqual(chunks[0].voice.to_dict()["url"], "/audio/gpt-sovits-test.wav")

    async def test_deterministic_pet_provider_keeps_text_when_voice_synthesis_fails(self):
        DeterministicProvider = _load_attr(
            "harness.pet.providers.deterministic", "DeterministicProvider"
        )
        PetPromptContext = _load_attr("harness.pet.persona", "PetPromptContext")
        VoiceProviderError = _load_attr("harness.pet.voice", "VoiceProviderError")

        class FailingVoiceRouter:
            async def synthesize(self, request):
                raise VoiceProviderError("voice backend unavailable", provider="gpt-sovits")

        provider = DeterministicProvider(FailingVoiceRouter())
        chunks = await provider.generate(
            PetPromptContext(
                session_id="pet-session",
                uid=7,
                profile_id="default",
                persona="memo-pet",
                provider="scripted",
                user_text="你好，桌宠",
                runtime_metadata={"voice_provider": "gpt-sovits", "voice_name": "chino"},
            )
        )

        self.assertEqual(len(chunks), 1)
        payload = _voice_payload(chunks[0].voice)
        self.assertEqual(chunks[0].text, "你好，我在这里。")
        self.assertEqual(payload["state"], "error")
        self.assertIsNone(payload["url"])
        self.assertIn("audio_error", payload["metadata"])

    async def test_local_vision_analyzer_disabled_is_recoverable(self):
        LocalVisionAnalyzer = _load_attr("harness.pet.vision", "LocalVisionAnalyzer")
        VisionCaptureRequest = _load_attr("harness.pet.vision", "VisionCaptureRequest")
        analyzer = LocalVisionAnalyzer(enabled=False)

        with self.assertRaisesRegex(Exception, "disabled"):
            analyzer.capture(VisionCaptureRequest(session_id="pet-session"))

    async def test_local_vision_analyzer_diagnostics_can_probe_one_frame(self):
        LocalVisionAnalyzer = _load_attr("harness.pet.vision", "LocalVisionAnalyzer")

        class FakeFrame:
            shape = (720, 1280, 3)

        class FakeCapture:
            def __init__(self, camera_index):
                self.camera_index = camera_index

            def isOpened(self):
                return True

            def read(self):
                return True, FakeFrame()

            def release(self):
                pass

        fake_cv2 = types.SimpleNamespace(VideoCapture=FakeCapture, __version__="4.test")
        analyzer = LocalVisionAnalyzer(enabled=True, default_camera_index=1, analyzer="opencv")

        with mock.patch.dict(sys.modules, {"cv2": fake_cv2}):
            payload = analyzer.diagnostics(open_camera=True)

        self.assertTrue(payload["ready"])
        self.assertEqual(payload["status"], "ready")
        self.assertEqual(payload["camera_index"], 1)
        self.assertEqual(payload["frame"], {"width": 1280, "height": 720})

    async def test_local_vision_analyzer_uses_configured_default_camera_index(self):
        LocalVisionAnalyzer = _load_attr("harness.pet.vision", "LocalVisionAnalyzer")
        VisionCaptureRequest = _load_attr("harness.pet.vision", "VisionCaptureRequest")
        captured_indices = []

        class FakeFrame:
            shape = (480, 640, 3)

        class FakeCapture:
            def __init__(self, camera_index):
                captured_indices.append(camera_index)

            def isOpened(self):
                return True

            def read(self):
                return True, FakeFrame()

            def release(self):
                pass

        fake_cv2 = types.SimpleNamespace(
            VideoCapture=FakeCapture,
            COLOR_BGR2GRAY=0,
            cvtColor=lambda frame, mode: types.SimpleNamespace(mean=lambda: 128.0),
            data=types.SimpleNamespace(haarcascades=""),
            CascadeClassifier=lambda path: types.SimpleNamespace(detectMultiScale=lambda *args, **kwargs: []),
        )
        analyzer = LocalVisionAnalyzer(enabled=True, default_camera_index=2, analyzer="opencv")

        with mock.patch.dict(sys.modules, {"cv2": fake_cv2}):
            observation = analyzer.capture(VisionCaptureRequest(session_id="pet-session"))

        payload = observation.to_dict()
        self.assertEqual(captured_indices, [2])
        self.assertEqual(payload["vision"]["camera_index"], 2)
        self.assertEqual(payload["vision"]["frame"], {"width": 640, "height": 480})
        self.assertEqual(payload["vision"]["mode"], "opencv_face_detection")
        self.assertTrue(payload["privacy"]["camera_used"])
        self.assertFalse(payload["privacy"]["raw_frame_sent"])
        self.assertEqual(payload["privacy"]["retention"], "none")

    async def test_vision_capture_request_accepts_uploaded_frame_payload(self):
        VisionCaptureRequest = _load_attr("harness.pet.vision", "VisionCaptureRequest")

        request = VisionCaptureRequest.from_dict(
            {
                "session_id": "pet-session",
                "camera_index": 3,
                "frame_base64": "AQIDBA==",
                "frame_mime": "image/png",
                "frame_width": 2,
                "frame_height": 1,
                "metadata": {"client": "qml-pet"},
            }
        )

        self.assertEqual(request.session_id, "pet-session")
        self.assertEqual(request.camera_index, 3)
        self.assertEqual(request.frame_base64, "AQIDBA==")
        self.assertEqual(request.frame_mime, "image/png")
        self.assertEqual(request.frame_width, 2)
        self.assertEqual(request.frame_height, 1)
        self.assertEqual(request.metadata["client"], "qml-pet")

    async def test_local_vision_analyzer_decodes_uploaded_frame_without_camera(self):
        LocalVisionAnalyzer = _load_attr("harness.pet.vision", "LocalVisionAnalyzer")
        VisionCaptureRequest = _load_attr("harness.pet.vision", "VisionCaptureRequest")
        calls = {"video_capture": 0, "imdecode": 0, "frombuffer": 0}

        class FakeFrame:
            shape = (1, 2, 3)
            size = 6

        class FakeCapture:
            def __init__(self, camera_index):
                calls["video_capture"] += 1

            def isOpened(self):
                return False

        class FakeCascade:
            def detectMultiScale(self, *args, **kwargs):
                return []

        def fake_frombuffer(data, dtype):
            calls["frombuffer"] += 1
            self.assertEqual(data, b"\x01\x02\x03\x04")
            return {"decoded": data, "dtype": dtype}

        def fake_imdecode(buffer, flags):
            calls["imdecode"] += 1
            self.assertEqual(buffer["decoded"], b"\x01\x02\x03\x04")
            return FakeFrame()

        fake_np = types.SimpleNamespace(uint8="uint8", frombuffer=fake_frombuffer)
        fake_cv2 = types.SimpleNamespace(
            VideoCapture=FakeCapture,
            IMREAD_COLOR=1,
            imdecode=fake_imdecode,
            COLOR_BGR2GRAY=0,
            cvtColor=lambda frame, mode: types.SimpleNamespace(mean=lambda: 196.0),
            data=types.SimpleNamespace(haarcascades=""),
            CascadeClassifier=lambda path: FakeCascade(),
        )
        analyzer = LocalVisionAnalyzer(enabled=True, default_camera_index=2, analyzer="opencv")

        with mock.patch.dict(sys.modules, {"cv2": fake_cv2, "numpy": fake_np}):
            observation = analyzer.capture(
                VisionCaptureRequest.from_dict(
                    {
                        "session_id": "pet-session",
                        "camera_index": 2,
                        "frame_base64": "AQIDBA==",
                        "frame_mime": "image/png",
                        "frame_width": 2,
                        "frame_height": 1,
                        "metadata": {"source": "qml"},
                    }
                )
            )

        payload = observation.to_dict()
        self.assertEqual(calls, {"video_capture": 0, "imdecode": 1, "frombuffer": 1})
        self.assertEqual(payload["vision"]["frame"], {"width": 2, "height": 1})
        self.assertEqual(payload["vision"].get("source"), "uploaded_frame")
        self.assertFalse(payload["privacy"]["raw_frame_sent"])
        self.assertEqual(payload["privacy"]["retention"], "none")

    async def test_voice_training_service_requires_consent_and_prepares_runtime_reference_without_sidecar(self):
        VoiceTrainingRequest = _load_attr("harness.pet.voice_training", "VoiceTrainingRequest")
        VoiceTrainingService = _load_attr("harness.pet.voice_training", "VoiceTrainingService")
        temp_dir = tempfile.TemporaryDirectory()
        self.addCleanup(temp_dir.cleanup)
        source_dir = Path(temp_dir.name) / "source"
        source_dir.mkdir()
        audio_path = source_dir / "Kafuuchino-voice.wav"
        _write_wav(audio_path, duration_sec=8.0)
        service = VoiceTrainingService(voice_clone_enabled=False, artifact_root=temp_dir.name)

        with self.assertRaisesRegex(ValueError, "consent"):
            service.create_job(VoiceTrainingRequest(reference_audio_path="voice.mp3", consent_confirmed=False))

        job = service.create_job(
            VoiceTrainingRequest(
                uid=7,
                profile_id="default",
                voice_name="Kafuuchino-voice",
                language="中文",
                reference_audio_path=str(audio_path),
                reference_audio_directory="src/KafuuChino/香风智乃voice",
                reference_audio_file="Kafuuchino-voice.wav",
                consent_confirmed=True,
            )
        )
        payload = job.to_dict()

        self.assertTrue(payload["job_id"].startswith("voice-train-"))
        self.assertEqual(payload["status"], "ready")
        self.assertEqual(payload["stage"], "ready_for_gpt_sovits")
        self.assertEqual(payload["progress"], 70)
        self.assertFalse(payload["diagnostics"]["voice_clone_enabled"])
        self.assertFalse(payload["diagnostics"]["training_started"])
        self.assertTrue(payload["diagnostics"]["audio_persisted"])
        self.assertTrue(payload["diagnostics"]["manifest_persisted"])
        self.assertTrue(payload["diagnostics"]["reference_audio_exists"])
        self.assertEqual(payload["diagnostics"]["reference_audio_extension"], "wav")
        self.assertGreater(payload["diagnostics"]["reference_audio_size_bytes"], 0)
        self.assertEqual(payload["diagnostics"]["reference_audio_persist_source"], "runtime_path_copy")
        self.assertTrue(payload["diagnostics"]["reference_audio_runtime_path"])
        self.assertEqual(
            payload["diagnostics"]["reference_audio_runtime_path"],
            payload["diagnostics"]["gpt_sovits_reference_audio"],
        )
        self.assertTrue(Path(payload["diagnostics"]["reference_audio_runtime_path"]).exists())
        self.assertEqual(service.latest_ready_reference_audio(uid=7), payload["diagnostics"]["reference_audio_runtime_path"])
        self.assertEqual(payload["language"], "zh-CN")
        self.assertTrue(Path(payload["manifest_path"]).exists())
        self.assertEqual(service.get_job(payload["job_id"]).job_id, payload["job_id"])
        self.assertEqual([item.job_id for item in service.list_jobs(uid=7)], [payload["job_id"]])

        restored = VoiceTrainingService(voice_clone_enabled=False, artifact_root=temp_dir.name)
        self.assertEqual(restored.get_job(payload["job_id"]).manifest_path, payload["manifest_path"])

    async def test_voice_training_service_normalizes_loaded_ready_for_worker_jobs(self):
        VoiceTrainingService = _load_attr("harness.pet.voice_training", "VoiceTrainingService")
        temp_dir = tempfile.TemporaryDirectory()
        self.addCleanup(temp_dir.cleanup)
        artifact_root = Path(temp_dir.name)
        audio_path = artifact_root / "reference.wav"
        _write_wav(audio_path, duration_sec=8.0)
        manifest_dir = artifact_root / "voice-train-old-ready"
        manifest_dir.mkdir()
        manifest_path = manifest_dir / "manifest.json"
        manifest = {
            "job_id": "voice-train-old-ready",
            "uid": 7,
            "profile_id": "default",
            "voice_name": "Kafuuchino-voice",
            "language": "zh-CN",
            "reference_audio_path": str(audio_path),
            "reference_audio_directory": "src/KafuuChino/香风智乃voice",
            "reference_audio_file": "Kafuuchino-voice.wav",
            "provider": "gpt-sovits",
            "status": "prepared",
            "stage": "ready_for_worker",
            "progress": 10,
            "consent_confirmed": True,
            "consent_scope": "local_default_reference",
            "source": "src-default",
            "artifact_root": str(artifact_root),
            "artifact_path": str(manifest_dir),
            "manifest_path": str(manifest_path),
            "message": "Voice training request prepared for worker",
            "created_at_ms": 1,
            "updated_at_ms": 2,
            "diagnostics": {
                "voice_clone_enabled": False,
                "reference_audio_exists": True,
                "gpt_sovits_reference_audio": str(audio_path),
                "reference_audio_runtime_path": str(audio_path),
            },
            "metadata": {},
        }
        manifest_path.write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8")

        service = VoiceTrainingService(voice_clone_enabled=False, artifact_root=temp_dir.name)
        payload = service.get_job("voice-train-old-ready").to_dict()

        self.assertEqual(payload["status"], "ready")
        self.assertEqual(payload["stage"], "ready_for_gpt_sovits")
        self.assertGreaterEqual(payload["progress"], 70)
        self.assertIn("零样本", payload["message"])

    async def test_reference_audio_diagnostics_classifies_short_and_ready_material(self):
        diagnose_reference_audio = _load_attr("harness.pet.voice_training", "diagnose_reference_audio")
        temp_dir = tempfile.TemporaryDirectory()
        self.addCleanup(temp_dir.cleanup)
        short_audio = Path(temp_dir.name) / "short.wav"
        ready_audio = Path(temp_dir.name) / "ready.wav"
        _write_wav(short_audio, duration_sec=1.0)
        _write_wav(ready_audio, duration_sec=65.0)

        short_payload = diagnose_reference_audio(str(short_audio))
        ready_payload = diagnose_reference_audio(str(ready_audio))

        self.assertEqual(short_payload["material_status"], "too_short")
        self.assertTrue(short_payload["needs_more_audio_for_inference"])
        self.assertFalse(short_payload["zero_shot_ready"])
        self.assertEqual(ready_payload["material_status"], "few_shot_ready")
        self.assertFalse(ready_payload["needs_more_audio_for_training"])
        self.assertTrue(ready_payload["needs_reference_clip"])
        self.assertFalse(ready_payload["direct_reference_ready"])
        self.assertTrue(ready_payload["zero_shot_ready"])
        self.assertTrue(ready_payload["few_shot_ready"])


async def _maybe_await(value):
    if inspect.isawaitable(value):
        return await value
    return value


async def _collect_voice_stream(value):
    stream = await _maybe_await(value)
    if hasattr(stream, "__aiter__"):
        items = []
        async for item in stream:
            items.append(item)
        return items
    if isinstance(stream, (list, tuple)):
        return list(stream)
    return [stream]


def _voice_payload(result) -> dict:
    payload = result.to_dict() if hasattr(result, "to_dict") else dict(result)
    payload.setdefault("metadata", {})
    return payload


def _response_text(response) -> str:
    if isinstance(response, str):
        return response
    if isinstance(response, dict):
        return str(response.get("text") or response.get("content") or response)
    return str(getattr(response, "text", getattr(response, "content", response)))


def _write_wav(path: Path, duration_sec: float, sample_rate: int = 16000) -> None:
    import wave

    frame_count = int(duration_sec * sample_rate)
    with wave.open(str(path), "wb") as handle:
        handle.setnchannels(1)
        handle.setsampwidth(2)
        handle.setframerate(sample_rate)
        handle.writeframes(b"\0\0" * frame_count)


if __name__ == "__main__":
    unittest.main()
