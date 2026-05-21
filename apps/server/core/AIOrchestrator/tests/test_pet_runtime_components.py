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

    def test_animation_mapper_maps_observation_vision_to_live2d_controls(self):
        AnimationMapper = _load_attr("harness.pet.animation", "AnimationMapper")
        mapper = AnimationMapper()

        mapped = mapper.for_observation_vision(
            {
                "enabled": True,
                "mode": "mediapipe_face_landmarker",
                "face_present": True,
                "expression": "smile",
                "confidence": 0.92,
                "pose": {"face_confidence": 0.92, "brightness": 0.52},
                "head_pose": {"yaw": 24.0, "pitch": 18.0, "roll": 2.0},
                "blendshapes": {
                    "jawOpen": 0.66,
                    "mouthSmileLeft": 0.68,
                    "mouthSmileRight": 0.74,
                    "eyeBlinkLeft": 0.03,
                    "eyeBlinkRight": 0.04,
                },
                "scene": {"lighting": "ambient", "brightness": 0.52},
            },
            audio_rms=0.12,
        )

        gaze = mapped["gaze"].to_dict() if hasattr(mapped["gaze"], "to_dict") else dict(mapped["gaze"])
        self.assertEqual(mapped["phase"], "listening")
        self.assertEqual(mapped["emotion"], "cheerful")
        self.assertEqual(mapped["expression"], "smile_soft")
        self.assertEqual(mapped["motion"], "talk")
        self.assertEqual(gaze["target"], "user_face")
        self.assertGreater(gaze["x"], 0.6)
        self.assertLess(gaze["y"], 0.45)
        self.assertGreater(mapped["lip_sync"], 0.6)
        self.assertGreater(mapped["intensity"], 0.6)

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

    async def test_voice_router_deterministic_synthesis_emits_scripted_wav(self):
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
        self.assertEqual(payload["state"], "ready")
        self.assertEqual(payload["sample_rate"], 24000)
        self.assertGreater(payload["duration_ms"], 0)
        self.assertTrue(str(payload["url"]).startswith("/audio/deterministic-voice-"))
        self.assertTrue(str(payload["url"]).endswith(".wav"))
        self.assertTrue(str(payload["chunk_ref"]).startswith("deterministic-voice-"))
        self.assertEqual(payload["provider"], "scripted")
        self.assertEqual(payload["voice"], "deterministic")
        self.assertEqual(payload["retention"], "ephemeral")
        self.assertTrue(payload["metadata"].get("audio_persisted"))

    async def test_voice_router_exposes_stream_and_interrupt_contract(self):
        VoiceProviderRouter = _load_attr("harness.pet.voice", "VoiceProviderRouter")
        DeterministicVoiceProvider = _load_attr("harness.pet.voice", "DeterministicVoiceProvider")

        router = VoiceProviderRouter(deterministic=True)
        provider = DeterministicVoiceProvider()

        self.assertTrue(callable(getattr(router, "stream", None)))
        self.assertTrue(callable(getattr(router, "interrupt", None)))
        self.assertTrue(callable(getattr(provider, "stream", None)))
        self.assertTrue(callable(getattr(provider, "interrupt", None)))

    async def test_voice_router_deterministic_stream_yields_scripted_wav_segments(self):
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
            self.assertEqual(payload["state"], "ready")
            self.assertEqual(payload["sample_rate"], 24000)
            self.assertTrue(str(payload["url"]).startswith("/audio/deterministic-voice-"))
            self.assertEqual(payload["retention"], "ephemeral")
            self.assertTrue(payload["metadata"].get("audio_persisted"))

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

    async def test_gpt_sovits_voice_provider_ignores_unreadable_reference_audio(self):
        GPTSoVITSVoiceProvider = _load_attr("harness.pet.voice", "GPTSoVITSVoiceProvider")
        VoiceSynthesisRequest = _load_attr("harness.pet.voice", "VoiceSynthesisRequest")
        VoiceSynthesisResult = _load_attr("harness.pet.voice", "VoiceSynthesisResult")

        temp_dir = tempfile.TemporaryDirectory()
        self.addCleanup(temp_dir.cleanup)
        fallback_audio = Path(temp_dir.name) / "fallback.wav"
        _write_wav(fallback_audio, duration_sec=8.0)

        captured = {}

        class FakeResponse:
            def __init__(self, payload):
                self.status_code = 200
                self.headers = {"content-type": "audio/wav"}
                self.content = payload
                self.text = ""

            def raise_for_status(self):
                return None

        class FakeAsyncClient:
            def __init__(self, timeout=None):
                self.timeout = timeout

            async def __aenter__(self):
                return self

            async def __aexit__(self, exc_type, exc, tb):
                return False

            async def post(self, url, json=None):
                captured["url"] = url
                captured["payload"] = dict(json or {})
                return FakeResponse(fallback_audio.read_bytes())

        fake_httpx = types.SimpleNamespace(AsyncClient=FakeAsyncClient)
        provider = GPTSoVITSVoiceProvider(
            base_url="http://127.0.0.1:9880",
            reference_audio=str(fallback_audio),
            prompt_text="おはようございます",
        )

        with mock.patch.dict(sys.modules, {"httpx": fake_httpx}):
            result = await provider.synthesize(
                VoiceSynthesisRequest(
                    session_id="pet-session",
                    turn_id="pet-turn",
                    text="你好",
                    provider="gpt-sovits",
                    voice="kafuu-chino",
                    metadata={"ref_audio_path": "/no/such/audio.wav"},
                )
            )

        self.assertIsInstance(result, VoiceSynthesisResult)
        self.assertEqual(captured["url"], "http://127.0.0.1:9880/tts")
        self.assertEqual(captured["payload"]["ref_audio_path"], str(fallback_audio))
        self.assertEqual(captured["payload"]["prompt_text"], "おはようございます")

    async def test_gpt_sovits_voice_provider_includes_http_error_body(self):
        GPTSoVITSVoiceProvider = _load_attr("harness.pet.voice", "GPTSoVITSVoiceProvider")
        VoiceProviderError = _load_attr("harness.pet.voice", "VoiceProviderError")
        VoiceSynthesisRequest = _load_attr("harness.pet.voice", "VoiceSynthesisRequest")

        temp_dir = tempfile.TemporaryDirectory()
        self.addCleanup(temp_dir.cleanup)
        fallback_audio = Path(temp_dir.name) / "fallback.wav"
        _write_wav(fallback_audio, duration_sec=8.0)

        class FakeHTTPStatusError(Exception):
            def __init__(self, response):
                super().__init__(f"HTTP {response.status_code}")
                self.response = response

        class FakeResponse:
            def __init__(self):
                self.status_code = 400
                self.headers = {"content-type": "application/json"}
                self.content = b""
                self.text = '{"message":"tts failed","Exception":"/no/such/audio.wav not exists"}'

            def raise_for_status(self):
                raise FakeHTTPStatusError(self)

        class FakeAsyncClient:
            def __init__(self, timeout=None):
                self.timeout = timeout

            async def __aenter__(self):
                return self

            async def __aexit__(self, exc_type, exc, tb):
                return False

            async def post(self, url, json=None):
                return FakeResponse()

        fake_httpx = types.SimpleNamespace(AsyncClient=FakeAsyncClient)
        provider = GPTSoVITSVoiceProvider(
            base_url="http://127.0.0.1:9880",
            reference_audio=str(fallback_audio),
        )

        with mock.patch.dict(sys.modules, {"httpx": fake_httpx}):
            with self.assertRaises(VoiceProviderError) as ctx:
                await provider.synthesize(
                    VoiceSynthesisRequest(
                        session_id="pet-session",
                        turn_id="pet-turn",
                        text="你好",
                        provider="gpt-sovits",
                        voice="kafuu-chino",
                    )
                )

        self.assertIn("HTTP 400", str(ctx.exception))
        self.assertIn("/no/such/audio.wav not exists", str(ctx.exception))

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

    async def test_deterministic_pet_provider_includes_visual_summary_in_llm_prompt(self):
        DeterministicProvider = _load_attr(
            "harness.pet.providers.deterministic", "DeterministicProvider"
        )
        PetPromptContext = _load_attr("harness.pet.persona", "PetPromptContext")

        fake_response = types.SimpleNamespace(
            content='{"text":"我看到了，你看起来心情不错。","translation":""}'
        )
        fake_registry = types.SimpleNamespace(
            complete=mock.AsyncMock(return_value=fake_response),
        )
        provider = DeterministicProvider()
        prompt = PetPromptContext(
            session_id="pet-session",
            uid=7,
            profile_id="default",
            persona="memo-pet",
            provider="scripted",
            user_text="请根据刚才的观察回复我",
            model_type="openai",
            model_name="gpt-4o",
            observation_summary={"vision": {"face_present": True}},
            runtime_metadata={
                "speech_rules": "keep it short",
                "visual_summary": {
                    "summary_text": "你看起来心情不错。",
                    "reason": "updated",
                },
            },
        )

        fake_llm_service = types.ModuleType("harness.llm.service")
        fake_llm_service.LLMEndpointRegistry = lambda: fake_registry

        with mock.patch.dict(sys.modules, {"harness.llm.service": fake_llm_service}):
            reply, translation = await provider._llm_reply(prompt, "zh-CN")

        self.assertEqual(reply, "我看到了，你看起来心情不错。")
        self.assertEqual(translation, "")
        self.assertEqual(fake_registry.complete.await_count, 1)
        messages = fake_registry.complete.call_args.args[0]
        system_prompt = messages[0].content
        self.assertIn("Observation summary:", system_prompt)
        self.assertIn("Speech rules: keep it short", system_prompt)
        self.assertIn("Visual summary: 你看起来心情不错。", system_prompt)

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

    async def test_disabled_local_camera_still_accepts_authorized_uploaded_frame(self):
        LocalVisionAnalyzer = _load_attr("harness.pet.vision", "LocalVisionAnalyzer")
        VisionCaptureRequest = _load_attr("harness.pet.vision", "VisionCaptureRequest")
        calls = {"video_capture": 0, "imdecode": 0, "frombuffer": 0}

        class FakeFrame:
            shape = (1, 2, 3)
            size = 6

        class FakeCapture:
            def __init__(self, camera_index):
                calls["video_capture"] += 1

        class FakeCascade:
            def detectMultiScale(self, *args, **kwargs):
                return []

        def fake_frombuffer(data, dtype):
            calls["frombuffer"] += 1
            return {"decoded": data, "dtype": dtype}

        def fake_imdecode(buffer, flags):
            calls["imdecode"] += 1
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
        analyzer = LocalVisionAnalyzer(enabled=False, analyzer="opencv")

        with mock.patch.dict(sys.modules, {"cv2": fake_cv2, "numpy": fake_np}):
            observation = analyzer.capture(
                VisionCaptureRequest.from_dict(
                    {
                        "session_id": "pet-session",
                        "frame_base64": "AQIDBA==",
                        "frame_mime": "image/jpeg",
                        "metadata": {"source": "wsl_windows_camera_bridge"},
                    }
                )
            )

        payload = observation.to_dict()
        self.assertEqual(calls, {"video_capture": 0, "imdecode": 1, "frombuffer": 1})
        self.assertEqual(payload["vision"]["source"], "uploaded_frame")
        self.assertTrue(payload["privacy"]["camera_used"])
        self.assertFalse(payload["privacy"]["raw_frame_sent"])

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
                        "metadata": {"source": "qml", "reply_language": "ja-JP", "speech_rules": "短く話す"},
                    }
                )
            )

        payload = observation.to_dict()
        self.assertEqual(calls, {"video_capture": 0, "imdecode": 1, "frombuffer": 1})
        self.assertEqual(payload["vision"]["frame"], {"width": 2, "height": 1})
        self.assertEqual(payload["vision"].get("source"), "uploaded_frame")
        self.assertEqual(payload["vision"].get("reply_language"), "ja-JP")
        self.assertEqual(payload["vision"].get("speech_rules"), "短く話す")
        self.assertFalse(payload["privacy"]["raw_frame_sent"])
        self.assertEqual(payload["privacy"]["retention"], "none")

    async def test_local_vision_analyzer_aggregates_uploaded_segment_frames(self):
        vision_module = importlib.import_module("harness.pet.vision")
        LocalVisionAnalyzer = _load_attr("harness.pet.vision", "LocalVisionAnalyzer")
        VisionSegmentRequest = _load_attr("harness.pet.vision", "VisionSegmentRequest")

        class FakeFrame:
            def __init__(self, width, height):
                self.shape = (height, width, 3)

        decoded_frames = [FakeFrame(320, 240), FakeFrame(320, 240), FakeFrame(320, 240)]
        analyzed_frames = [
            {
                "enabled": True,
                "mode": "opencv_face_detection",
                "face_present": True,
                "attention": "face_detected",
                "expression": "smile",
                "confidence": 0.8,
                "pose": {"brightness": 0.5},
                "scene": {"summary": "看到了你的脸。"},
                "objects": [{"label": "cup", "confidence": 0.9}],
            },
            {
                "enabled": True,
                "mode": "opencv_face_detection",
                "face_present": True,
                "attention": "face_detected",
                "expression": "smile",
                "confidence": 0.9,
                "pose": {"brightness": 0.52},
                "scene": {"summary": "看到了你的脸。"},
                "objects": [],
            },
            {
                "enabled": True,
                "mode": "opencv_frame_stats",
                "face_present": False,
                "attention": "ambient",
                "expression": "",
                "confidence": 0.1,
                "pose": {"brightness": 0.48},
                "scene": {"summary": "周围光线稳定。"},
                "objects": [{"label": "keyboard", "confidence": 0.8}],
            },
        ]
        analyzer = LocalVisionAnalyzer(enabled=False, analyzer="opencv")

        with mock.patch.dict(sys.modules, {"cv2": types.SimpleNamespace()}), mock.patch.object(
            vision_module, "_decode_frame_payload", side_effect=decoded_frames
        ), mock.patch.object(vision_module, "_analyze_frame", side_effect=analyzed_frames):
            observation = analyzer.capture_segment(
                VisionSegmentRequest.from_dict(
                    {
                        "session_id": "pet-session",
                        "segment_id": "seg-1",
                        "duration_ms": 3000,
                        "frames": [
                            {"frame_base64": "AQID", "frame_mime": "image/jpeg", "frame_width": 320, "frame_height": 240, "t_ms": 0},
                            {"frame_base64": "BAUG", "frame_mime": "image/jpeg", "frame_width": 320, "frame_height": 240, "t_ms": 1500},
                            {"frame_base64": "BwgJ", "frame_mime": "image/jpeg", "frame_width": 320, "frame_height": 240, "t_ms": 3000},
                        ],
                        "metadata": {"reply_language": "zh-CN"},
                    }
                )
            )

        payload = observation.to_dict()
        self.assertEqual(payload["vision"]["source"], "uploaded_segment")
        self.assertEqual(payload["vision"]["segment"]["segment_id"], "seg-1")
        self.assertEqual(payload["vision"]["segment"]["frame_count"], 3)
        self.assertEqual(payload["vision"]["segment"]["duration_ms"], 3000)
        self.assertEqual(payload["vision"]["segment"]["dominant_expression"], "smile")
        self.assertEqual(payload["vision"]["segment"]["face_present_ratio"], 0.667)
        self.assertEqual(payload["vision"]["segment"]["object_labels"], ["cup", "keyboard"])
        self.assertIn("segment", payload["vision"]["scene"])
        self.assertEqual(payload["privacy"]["raw_frame_count"], 3)
        self.assertFalse(payload["privacy"]["raw_frame_sent"])
        self.assertEqual(payload["privacy"]["retention"], "none")

    async def test_local_vision_analyzer_uses_mediapipe_face_landmarker_when_model_is_available(self):
        LocalVisionAnalyzer = _load_attr("harness.pet.vision", "LocalVisionAnalyzer")
        VisionCaptureRequest = _load_attr("harness.pet.vision", "VisionCaptureRequest")
        temp_dir = tempfile.TemporaryDirectory()
        self.addCleanup(temp_dir.cleanup)
        model_path = Path(temp_dir.name) / "face_landmarker.task"
        model_path.write_bytes(b"fake-model")

        class FakeFrame:
            shape = (480, 640, 3)

        class FakeCapture:
            def __init__(self, camera_index):
                self.camera_index = camera_index

            def isOpened(self):
                return True

            def read(self):
                return True, FakeFrame()

            def release(self):
                pass

        class FakeLandmark:
            def __init__(self, x, y, z=0.0):
                self.x = x
                self.y = y
                self.z = z

        class FakeCategory:
            def __init__(self, category_name, score):
                self.category_name = category_name
                self.score = score

        class FakeGrayFrame:
            def __init__(self, mean_value):
                self._mean_value = mean_value

            def mean(self):
                return self._mean_value

        class FakeLandmarker:
            def __init__(self, options):
                self.options = options
                self.detected = []
                self.closed = False

            def detect(self, mp_image):
                self.detected.append(mp_image)
                return types.SimpleNamespace(
                    face_landmarks=[
                        [
                            FakeLandmark(0.1, 0.2),
                            FakeLandmark(0.8, 0.2),
                            FakeLandmark(0.8, 0.7),
                            FakeLandmark(0.1, 0.7),
                        ]
                    ],
                    face_blendshapes=[
                        [
                            FakeCategory("mouthSmileLeft", 0.82),
                            FakeCategory("mouthSmileRight", 0.79),
                            FakeCategory("jawOpen", 0.12),
                            FakeCategory("eyeBlinkLeft", 0.02),
                            FakeCategory("eyeBlinkRight", 0.03),
                        ]
                    ],
                    facial_transformation_matrixes=[
                        [
                            [1.0, 0.0, 0.0, 0.0],
                            [0.0, 1.0, 0.0, 0.0],
                            [0.0, 0.0, 1.0, 0.0],
                            [0.0, 0.0, 0.0, 1.0],
                        ]
                    ],
                )

            def close(self):
                self.closed = True

        fake_landmarker_instances = []

        def fake_create_from_options(options):
            instance = FakeLandmarker(options)
            fake_landmarker_instances.append(instance)
            return instance

        fake_mp = types.SimpleNamespace(
            Image=lambda image_format, data: types.SimpleNamespace(image_format=image_format, data=data),
            ImageFormat=types.SimpleNamespace(SRGB="SRGB"),
            tasks=types.SimpleNamespace(
                BaseOptions=lambda model_asset_path=None: types.SimpleNamespace(model_asset_path=model_asset_path),
                vision=types.SimpleNamespace(
                    RunningMode=types.SimpleNamespace(IMAGE="IMAGE"),
                    FaceLandmarkerOptions=lambda **kwargs: types.SimpleNamespace(**kwargs),
                    FaceLandmarker=types.SimpleNamespace(create_from_options=fake_create_from_options),
                ),
            ),
        )
        fake_cv2 = types.SimpleNamespace(
            VideoCapture=FakeCapture,
            COLOR_BGR2GRAY=0,
            COLOR_BGR2RGB=1,
            cvtColor=lambda frame, mode: types.SimpleNamespace(shape=(480, 640, 3))
            if mode == 1
            else FakeGrayFrame(100.0),
            data=types.SimpleNamespace(haarcascades=""),
            CascadeClassifier=lambda path: types.SimpleNamespace(detectMultiScale=lambda *args, **kwargs: []),
        )
        analyzer = LocalVisionAnalyzer(
            enabled=True,
            default_camera_index=1,
            analyzer="mediapipe_face_landmarker",
            face_landmarker_model_path=str(model_path),
        )

        with mock.patch.dict(sys.modules, {"cv2": fake_cv2, "mediapipe": fake_mp}):
            observation = analyzer.capture(
                VisionCaptureRequest(session_id="pet-session", analyzer="mediapipe_face_landmarker")
            )

        payload = observation.to_dict()
        self.assertEqual(len(fake_landmarker_instances), 1)
        self.assertEqual(
            Path(fake_landmarker_instances[0].options.base_options.model_asset_path).resolve(),
            model_path.resolve(),
        )
        self.assertEqual(payload["vision"]["analyzer"], "mediapipe_face_landmarker")
        self.assertEqual(payload["vision"]["mode"], "mediapipe_face_landmarker")
        self.assertTrue(payload["vision"]["face_present"])
        self.assertEqual(payload["vision"]["expression"], "smile")
        self.assertEqual(payload["vision"]["confidence"], 1.0)
        self.assertEqual(payload["vision"]["blendshapes"]["mouthSmileLeft"], 0.82)
        self.assertEqual(payload["vision"]["head_pose"]["yaw"], 0.0)
        self.assertEqual(payload["vision"]["scene"]["lighting"], "ambient")
        self.assertAlmostEqual(payload["vision"]["pose"]["face_box"]["x"], 0.1, places=3)
        self.assertAlmostEqual(payload["vision"]["pose"]["face_box"]["width"], 0.7, places=3)
        self.assertEqual(payload["vision"]["pose"]["face_landmark_count"], 4)
        self.assertTrue(payload["privacy"]["camera_used"])

    async def test_local_vision_analyzer_reports_object_detector_readiness_when_model_is_available(self):
        LocalVisionAnalyzer = _load_attr("harness.pet.vision", "LocalVisionAnalyzer")
        temp_dir = tempfile.TemporaryDirectory()
        self.addCleanup(temp_dir.cleanup)
        model_path = Path(temp_dir.name) / "lite-model_efficientdet_lite0_detection_metadata_1.tflite"
        model_path.write_bytes(b"fake-model")

        class FakeFrame:
            shape = (480, 640, 3)

        class FakeCapture:
            def __init__(self, camera_index):
                self.camera_index = camera_index

            def isOpened(self):
                return True

            def read(self):
                return True, FakeFrame()

            def release(self):
                pass

        fake_cv2 = types.SimpleNamespace(
            VideoCapture=FakeCapture,
            __version__="4.test",
        )
        fake_mp = types.SimpleNamespace()
        analyzer = LocalVisionAnalyzer(
            enabled=True,
            default_camera_index=1,
            analyzer="opencv",
            object_detector_model_path=str(model_path),
        )

        with mock.patch.dict(sys.modules, {"cv2": fake_cv2, "mediapipe": fake_mp}):
            payload = analyzer.diagnostics(open_camera=True)

        self.assertTrue(payload["object_detector_ready"])
        self.assertEqual(Path(payload["object_detector_model_resolved"]).resolve(), model_path.resolve())
        self.assertEqual(payload["object_detector_status"], "ready")
        self.assertTrue(payload["ready"])

    async def test_local_vision_analyzer_uses_mediapipe_object_detector_when_model_is_available(self):
        LocalVisionAnalyzer = _load_attr("harness.pet.vision", "LocalVisionAnalyzer")
        VisionCaptureRequest = _load_attr("harness.pet.vision", "VisionCaptureRequest")
        temp_dir = tempfile.TemporaryDirectory()
        self.addCleanup(temp_dir.cleanup)
        model_path = Path(temp_dir.name) / "lite-model_efficientdet_lite0_detection_metadata_1.tflite"
        model_path.write_bytes(b"fake-model")

        class FakeFrame:
            shape = (480, 640, 3)

        class FakeCapture:
            def __init__(self, camera_index):
                self.camera_index = camera_index

            def isOpened(self):
                return True

            def read(self):
                return True, FakeFrame()

            def release(self):
                pass

        class FakeCategory:
            def __init__(self, category_name, score):
                self.category_name = category_name
                self.score = score

        class FakeDetection:
            def __init__(self, category_name, score, origin_x, origin_y, width, height):
                self.categories = [FakeCategory(category_name, score)]
                self.bounding_box = types.SimpleNamespace(
                    origin_x=origin_x,
                    origin_y=origin_y,
                    width=width,
                    height=height,
                )

        class FakeDetector:
            def __init__(self, options):
                self.options = options
                self.closed = False

            def detect(self, mp_image):
                return types.SimpleNamespace(
                    detections=[
                        FakeDetection("cup", 0.92, 64, 48, 160, 120),
                        FakeDetection("keyboard", 0.81, 300, 220, 220, 90),
                    ]
                )

            def close(self):
                self.closed = True

        fake_detector_instances = []

        def fake_create_from_options(options):
            instance = FakeDetector(options)
            fake_detector_instances.append(instance)
            return instance

        fake_mp = types.SimpleNamespace(
            Image=lambda image_format, data: types.SimpleNamespace(image_format=image_format, data=data),
            ImageFormat=types.SimpleNamespace(SRGB="SRGB"),
            tasks=types.SimpleNamespace(
                BaseOptions=lambda model_asset_path=None: types.SimpleNamespace(model_asset_path=model_asset_path),
                vision=types.SimpleNamespace(
                    RunningMode=types.SimpleNamespace(IMAGE="IMAGE"),
                    ObjectDetectorOptions=lambda **kwargs: types.SimpleNamespace(**kwargs),
                    ObjectDetector=types.SimpleNamespace(create_from_options=fake_create_from_options),
                ),
            ),
        )
        fake_cv2 = types.SimpleNamespace(
            VideoCapture=FakeCapture,
            COLOR_BGR2GRAY=0,
            COLOR_BGR2RGB=1,
            cvtColor=lambda frame, mode: types.SimpleNamespace(mean=lambda: 120.0)
            if mode == 0
            else frame,
            data=types.SimpleNamespace(haarcascades=""),
            CascadeClassifier=lambda path: types.SimpleNamespace(detectMultiScale=lambda *args, **kwargs: []),
        )
        analyzer = LocalVisionAnalyzer(
            enabled=True,
            default_camera_index=1,
            analyzer="opencv",
            object_detector_model_path=str(model_path),
        )

        with mock.patch.dict(sys.modules, {"cv2": fake_cv2, "mediapipe": fake_mp}):
            observation = analyzer.capture(VisionCaptureRequest(session_id="pet-session"))

        payload = observation.to_dict()
        self.assertEqual(len(fake_detector_instances), 1)
        self.assertEqual(
            Path(fake_detector_instances[0].options.base_options.model_asset_path).resolve(),
            model_path.resolve(),
        )
        self.assertEqual(payload["vision"]["objects"][0]["label"], "cup")
        self.assertEqual(payload["vision"]["objects"][1]["label"], "keyboard")
        self.assertEqual(payload["vision"]["scene"]["object_count"], 2)
        self.assertEqual(payload["vision"]["scene"]["object_labels"], ["cup", "keyboard"])
        self.assertIn("cup", payload["vision"]["scene"]["summary"])
        self.assertIn("keyboard", payload["vision"]["scene"]["summary"])
        self.assertTrue(payload["vision"]["scene"]["object_detector_ready"])
        self.assertEqual(payload["vision"]["scene"]["object_detector_status"], "ready")

    async def test_local_vision_analyzer_falls_back_to_opencv_when_mediapipe_model_is_missing(self):
        LocalVisionAnalyzer = _load_attr("harness.pet.vision", "LocalVisionAnalyzer")
        VisionCaptureRequest = _load_attr("harness.pet.vision", "VisionCaptureRequest")

        class FakeFrame:
            shape = (360, 640, 3)

        class FakeCapture:
            def __init__(self, camera_index):
                self.camera_index = camera_index

            def isOpened(self):
                return True

            def read(self):
                return True, FakeFrame()

            def release(self):
                pass

        fake_cv2 = types.SimpleNamespace(
            VideoCapture=FakeCapture,
            COLOR_BGR2GRAY=0,
            COLOR_BGR2RGB=1,
            cvtColor=lambda frame, mode: types.SimpleNamespace(mean=lambda: 160.0)
            if mode == 0
            else frame,
            data=types.SimpleNamespace(haarcascades=""),
            CascadeClassifier=lambda path: types.SimpleNamespace(
                detectMultiScale=lambda *args, **kwargs: [(12, 18, 160, 120)]
            ),
        )
        analyzer = LocalVisionAnalyzer(
            enabled=True,
            default_camera_index=2,
            analyzer="mediapipe_face_landmarker",
            face_landmarker_model_path="",
        )

        with mock.patch.dict(sys.modules, {"cv2": fake_cv2}):
            observation = analyzer.capture(VisionCaptureRequest(session_id="pet-session"))

        payload = observation.to_dict()
        self.assertEqual(payload["vision"]["analyzer"], "mediapipe_face_landmarker")
        self.assertEqual(payload["vision"]["mode"], "opencv_face_detection")
        self.assertEqual(payload["vision"]["attention"], "face_detected")
        self.assertTrue(payload["vision"]["face_present"])
        self.assertEqual(payload["vision"]["pose"]["face_confidence"], 0.72)
        self.assertEqual(payload["vision"]["confidence"], 0.72)
        self.assertEqual(payload["vision"]["scene"]["summary"], "看到了你的脸，周围很明亮。")
        self.assertEqual(payload["vision"]["scene"]["object_count"], 0)
        self.assertEqual(payload["vision"]["scene"]["object_labels"], [])
        self.assertFalse(payload["vision"]["scene"]["object_detector_ready"])
        self.assertEqual(payload["vision"]["scene"]["object_detector_status"], "heuristic_fallback")

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

    async def test_voice_training_service_keeps_missing_runtime_audio_at_zero_progress(self):
        VoiceTrainingService = _load_attr("harness.pet.voice_training", "VoiceTrainingService")
        VoiceTrainingRequest = _load_attr("harness.pet.voice_training", "VoiceTrainingRequest")
        temp_dir = tempfile.TemporaryDirectory()
        self.addCleanup(temp_dir.cleanup)
        artifact_root = Path(temp_dir.name)

        service = VoiceTrainingService(voice_clone_enabled=False, artifact_root=temp_dir.name)
        job = service.create_job(
            VoiceTrainingRequest(
                uid=7,
                profile_id="default",
                voice_name="Kafuuchino-voice",
                language="zh-CN",
                reference_audio_path=str(artifact_root / "missing.wav"),
                reference_audio_directory="src/KafuuChino/香风智乃voice",
                reference_audio_file="Kafuuchino-voice.wav",
                consent_confirmed=True,
            )
        )
        payload = job.to_dict()

        self.assertEqual(payload["status"], "blocked")
        self.assertEqual(payload["stage"], "reference_not_visible")
        self.assertEqual(payload["progress"], 0)
        self.assertFalse(payload["diagnostics"]["reference_audio_exists"])
        self.assertEqual(payload["diagnostics"]["reference_audio_persist_source"], "none")

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
