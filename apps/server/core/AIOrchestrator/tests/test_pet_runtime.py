from __future__ import annotations

import unittest
import asyncio
import sys
import tempfile
import types
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from harness.pet import PetObservation, PetRuntime
from harness.pet.runtime import PetRuntimeConfig


_ALLOWED_PHASES = {"idle", "listening", "thinking", "speaking", "interrupted", "error"}
_ANIMATION_KEYS = {
    "expression",
    "motion_group",
    "motion",
    "priority",
    "loop",
    "fade_in_ms",
    "fade_out_ms",
    "parameters",
}
_AUDIO_KEYS = {
    "state",
    "rms",
    "sample_rate",
    "duration_ms",
    "chunk_ref",
    "url",
    "phoneme",
    "viseme",
}
_TEXT_KEYS = {"delta", "final", "language", "display", "translation"}
_PRIVACY_KEYS = {
    "camera_used",
    "mic_used",
    "cloud_vision_used",
    "raw_frame_sent",
    "raw_audio_recorded",
    "retention",
}


class PetRuntimeTests(unittest.IsolatedAsyncioTestCase):
    def assert_v1_control_payload(self, payload: dict, session_id: str) -> None:
        for key in (
            "type",
            "schema_version",
            "session_id",
            "event_id",
            "turn_id",
            "phase",
            "animation",
            "audio",
            "text",
            "privacy",
            "motion",
            "expression",
            "lip_sync",
            "speech",
            "safety",
        ):
            self.assertIn(key, payload)

        self.assertEqual(payload["type"], "pet.control")
        self.assertEqual(payload["schema_version"], 1)
        self.assertEqual(payload["session_id"], session_id)
        self.assertIsInstance(payload["event_id"], str)
        self.assertTrue(payload["event_id"])
        self.assertIsInstance(payload["turn_id"], str)
        self.assertTrue(payload["turn_id"])
        self.assertIn(payload["phase"], _ALLOWED_PHASES)

        self.assertIsInstance(payload["animation"], dict)
        self.assertIsInstance(payload["audio"], dict)
        self.assertIsInstance(payload["text"], dict)
        self.assertIsInstance(payload["privacy"], dict)
        self.assertTrue(_ANIMATION_KEYS.issubset(payload["animation"]))
        self.assertTrue(_AUDIO_KEYS.issubset(payload["audio"]))
        self.assertTrue(_TEXT_KEYS.issubset(payload["text"]))
        self.assertTrue(_PRIVACY_KEYS.issubset(payload["privacy"]))

        # Legacy fields must keep mirroring the nested v1 contract for current clients.
        self.assertEqual(payload["motion"], payload["animation"]["motion"])
        self.assertEqual(payload["expression"], payload["animation"]["expression"])
        self.assertAlmostEqual(payload["lip_sync"]["value"], payload["audio"]["rms"])
        self.assertEqual(payload["speech"]["text_delta"], payload["text"]["delta"])
        self.assertEqual(payload["speech"]["audio_url"], payload["audio"]["url"])
        self.assertEqual(payload["speech"]["audio_chunk_ref"], payload["audio"]["chunk_ref"])
        self.assertEqual(payload["safety"]["camera_used"], payload["privacy"]["camera_used"])

    async def test_session_input_emits_decoupled_control_events(self):
        runtime = PetRuntime()
        session = await runtime.create_session(uid=7, profile_id="default")

        events = await runtime.submit_input(
            session.session_id,
            "你好",
            model_type="scripted",
            model_name="deterministic",
        )

        self.assertGreaterEqual(len(events), 3)
        payloads = [event.to_dict() for event in events]
        for payload in payloads:
            self.assert_v1_control_payload(payload, session.session_id)

        first = payloads[0]
        self.assertEqual(first["type"], "pet.control")
        self.assertEqual(first["session_id"], session.session_id)
        self.assertIn(first["motion"], {"listen", "talk", "idle"})
        self.assertEqual(first["phase"], "listening")
        self.assertIn("speaking", {payload["phase"] for payload in payloads})
        self.assertEqual(payloads[-1]["phase"], "idle")
        self.assertEqual(len({payload["turn_id"] for payload in payloads}), 1)
        self.assertEqual(len({payload["event_id"] for payload in payloads}), len(payloads))
        self.assertEqual(
            [payload["seq"] for payload in payloads],
            list(range(payloads[0]["seq"], payloads[0]["seq"] + len(payloads))),
        )

        speech = "".join(payload["speech"]["text_delta"] for payload in payloads)
        nested_text = "".join(payload["text"]["delta"] for payload in payloads)
        self.assertEqual(speech, nested_text)
        self.assertIn("你好，我在这里。", speech)
        self.assertNotIn("收到：你好", speech)
        for payload in payloads:
            self.assertEqual(payload["speech"]["text_delta"], payload["text"]["delta"])
            self.assertEqual(payload["text"]["display"], payload["text"]["delta"])
        self.assertEqual(payloads[-1]["lip_sync"]["value"], 0.0)

    async def test_deterministic_voice_foundation_stays_text_only(self):
        runtime = PetRuntime(PetRuntimeConfig(deterministic=True))
        session = await runtime.create_session(uid=7, profile_id="default")

        events = await runtime.submit_input(
            session.session_id,
            "voice check",
            model_type="scripted",
            model_name="deterministic",
        )

        speaking_payloads = [event.to_dict() for event in events if event.to_dict()["phase"] == "speaking"]
        self.assertTrue(speaking_payloads)
        for payload in speaking_payloads:
            self.assert_v1_control_payload(payload, session.session_id)
            self.assertEqual(payload["speech"]["text_delta"], payload["text"]["delta"])
            self.assertTrue(payload["text"]["delta"])
            self.assertIsNone(payload["audio"]["url"])
            self.assertEqual(payload["speech"]["audio_url"], payload["audio"]["url"])
            self.assertIsNone(payload["audio"]["chunk_ref"])
            self.assertEqual(payload["speech"]["audio_chunk_ref"], payload["audio"]["chunk_ref"])
            self.assertFalse(payload["privacy"]["raw_audio_recorded"])
            self.assertEqual(payload["privacy"]["retention"], "none")
            if "voice" in payload["debug"]:
                self.assertEqual(payload["debug"]["voice"]["state"], "text-only")
                self.assertEqual(payload["debug"]["voice"]["retention"], "none")

    async def test_observation_maps_to_visual_state_without_raw_media(self):
        runtime = PetRuntime()
        session = await runtime.create_session(uid=7)

        event = await runtime.update_observation(
            PetObservation.from_dict(
                {
                    "session_id": session.session_id,
                    "audio": {"vad": "speaking", "rms": 0.64, "interrupt": False},
                    "vision": {
                        "enabled": True,
                        "mode": "landmarks_only",
                        "face_present": True,
                        "expression": "smile",
                    },
                    "privacy": {"raw_frame_sent": False, "raw_audio_recorded": False},
                }
            )
        )

        payload = event.to_dict()
        self.assert_v1_control_payload(payload, session.session_id)
        self.assertEqual(payload["expression"], "smile_soft")
        self.assertEqual(payload["lip_sync"]["value"], 0.64)
        self.assertEqual(payload["audio"]["rms"], 0.64)
        self.assertTrue(payload["safety"]["camera_used"])
        self.assertEqual(payload["safety"]["vision_detail"], "landmarks_only")
        self.assertFalse(payload["privacy"]["raw_frame_sent"])
        self.assertFalse(payload["privacy"]["raw_audio_recorded"])
        self.assertFalse(payload["privacy"]["cloud_vision_used"])

    async def test_observation_preserves_extended_visual_layer_fields(self):
        runtime = PetRuntime()
        session = await runtime.create_session(uid=7)

        event = await runtime.update_observation(
            PetObservation.from_dict(
                {
                    "session_id": session.session_id,
                    "audio": {"vad": "idle", "rms": 0.0, "interrupt": False},
                    "vision": {
                        "enabled": True,
                        "mode": "mediapipe_face_landmarker",
                        "face_present": True,
                        "attention": "user_face",
                        "expression": "smile",
                        "confidence": 0.82,
                        "pose": {"face_confidence": 0.82, "brightness": 0.51},
                        "head_pose": {"yaw": 0.11, "pitch": -0.03, "roll": 0.01},
                        "blendshapes": {"jawOpen": 0.21, "eyeBlinkLeft": 0.04},
                        "scene": {"lighting": "ambient", "brightness": 0.51},
                        "objects": [{"label": "monitor", "score": 0.77}],
                        "source": "uploaded_frame",
                        "frame": {"width": 320, "height": 180},
                        "captured_at_ms": 123456,
                    },
                    "privacy": {"raw_frame_sent": False, "raw_audio_recorded": False},
                }
            )
        )

        payload = event.to_dict()

        self.assert_v1_control_payload(payload, session.session_id)
        self.assertEqual(payload["expression"], "smile_soft")
        self.assertEqual(payload["vision"]["mode"], "mediapipe_face_landmarker")
        self.assertEqual(payload["vision"]["confidence"], 0.82)
        self.assertEqual(payload["vision"]["head_pose"]["yaw"], 0.11)
        self.assertEqual(payload["vision"]["blendshapes"]["jawOpen"], 0.21)
        self.assertEqual(payload["vision"]["scene"]["lighting"], "ambient")
        self.assertEqual(payload["vision"]["objects"][0]["label"], "monitor")
        self.assertEqual(payload["vision"]["source"], "uploaded_frame")
        self.assertEqual(payload["vision"]["frame"], {"width": 320, "height": 180})
        self.assertEqual(payload["vision"]["captured_at_ms"], 123456)
        self.assertTrue(payload["privacy"]["camera_used"])

    async def test_observation_prefers_scene_summary_and_object_labels_for_agent_text(self):
        runtime = PetRuntime()
        session = await runtime.create_session(uid=7)

        event = await runtime.update_observation(
            PetObservation.from_dict(
                {
                    "session_id": session.session_id,
                    "audio": {"vad": "idle", "rms": 0.0, "interrupt": False},
                    "vision": {
                        "enabled": True,
                        "mode": "mediapipe_face_landmarker",
                        "face_present": True,
                        "attention": "user_face",
                        "expression": "smile",
                        "confidence": 0.96,
                        "pose": {"face_confidence": 0.96, "brightness": 0.51},
                        "scene": {
                            "lighting": "bright",
                            "brightness": 0.51,
                            "summary": "画面里有杯子和键盘。",
                            "object_labels": ["cup", "keyboard"],
                            "object_count": 2,
                        },
                        "objects": [
                            {"label": "cup", "score": 0.82},
                            {"label": "keyboard", "score": 0.76},
                        ],
                        "source": "camera_frame",
                        "frame": {"width": 640, "height": 480},
                        "captured_at_ms": 123456,
                    },
                    "privacy": {"raw_frame_sent": False, "raw_audio_recorded": False},
                }
            )
        )

        payload = event.to_dict()
        self.assert_v1_control_payload(payload, session.session_id)
        self.assertEqual(payload["vision"]["scene"]["summary"], "画面里有杯子和键盘。")
        self.assertEqual(payload["vision"]["scene"]["object_labels"], ["cup", "keyboard"])
        self.assertIn("画面里有杯子和键盘。", payload["debug"]["visual_summary"]["summary_text"])
        self.assertTrue(payload["debug"]["visual_summary"]["speak"])
        self.assertEqual(payload["action"]["name"], "visual_react")

    async def test_observation_maps_head_pose_and_blendshapes_to_live2d_visual_controls(self):
        runtime = PetRuntime()
        session = await runtime.create_session(uid=7)

        event = await runtime.update_observation(
            PetObservation.from_dict(
                {
                    "session_id": session.session_id,
                    "audio": {"vad": "idle", "rms": 0.12, "interrupt": False},
                    "vision": {
                        "enabled": True,
                        "mode": "mediapipe_face_landmarker",
                        "face_present": True,
                        "attention": "user_face",
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
                        "objects": [{"label": "monitor", "score": 0.77}],
                        "source": "camera_frame",
                        "frame": {"width": 640, "height": 480},
                        "captured_at_ms": 123456,
                    },
                    "privacy": {"raw_frame_sent": False, "raw_audio_recorded": False},
                }
            )
        )

        payload = event.to_dict()
        self.assert_v1_control_payload(payload, session.session_id)
        self.assertEqual(payload["emotion"], "cheerful")
        self.assertEqual(payload["expression"], "smile_soft")
        self.assertEqual(payload["motion"], "talk")
        self.assertEqual(payload["gaze"]["target"], "user_face")
        self.assertGreater(payload["gaze"]["x"], 0.6)
        self.assertLess(payload["gaze"]["y"], 0.45)
        self.assertGreater(payload["lip_sync"]["value"], 0.6)
        self.assertGreater(payload["intensity"], 0.6)
        self.assertEqual(payload["vision"]["mode"], "mediapipe_face_landmarker")
        self.assertEqual(payload["vision"]["confidence"], 0.92)
        self.assertEqual(payload["vision"]["head_pose"]["yaw"], 24.0)

    async def test_visual_observation_speaks_when_high_confidence_changes(self):
        runtime = PetRuntime()
        session = await runtime.create_session(uid=7)

        with _patched_policy_time(1000.0):
            event = await runtime.update_observation(
                PetObservation.from_dict(
                    {
                        "session_id": session.session_id,
                        "audio": {"vad": "idle", "rms": 0.0, "interrupt": False},
                        "vision": {
                            "enabled": True,
                            "mode": "mediapipe_face_landmarker",
                            "face_present": True,
                            "expression": "smile",
                            "confidence": 0.95,
                            "pose": {"face_confidence": 0.95},
                        },
                        "privacy": {"raw_frame_sent": False, "raw_audio_recorded": False},
                    }
                )
            )

        payload = event.to_dict()
        self.assert_v1_control_payload(payload, session.session_id)
        self.assertEqual(payload["phase"], "speaking")
        self.assertEqual(payload["action"]["name"], "visual_react")
        self.assertTrue(payload["text"]["final"])
        self.assertEqual(payload["speech"]["text_delta"], "我已经看到你了哦~")
        self.assertEqual(payload["debug"]["visual_summary"]["reason"], "first_user_seen")
        self.assertTrue(payload["debug"]["visual_summary"]["speak"])

    async def test_visual_observation_speaks_first_user_seen_for_opencv_face(self):
        runtime = PetRuntime()
        session = await runtime.create_session(uid=7)

        with _patched_policy_time(1000.0):
            event = await runtime.update_observation(
                PetObservation.from_dict(
                    {
                        "session_id": session.session_id,
                        "audio": {"vad": "idle", "rms": 0.0, "interrupt": False},
                        "vision": {
                            "enabled": True,
                            "mode": "opencv_face_detection",
                            "face_present": True,
                            "attention": "face_detected",
                            "expression": "neutral",
                            "confidence": 0.72,
                            "pose": {"face_confidence": 0.72},
                        },
                        "privacy": {"raw_frame_sent": False, "raw_audio_recorded": False},
                    }
                )
            )

        payload = event.to_dict()
        self.assert_v1_control_payload(payload, session.session_id)
        self.assertEqual(payload["phase"], "speaking")
        self.assertEqual(payload["speech"]["text_delta"], "我已经看到你了哦~")
        self.assertEqual(payload["debug"]["visual_summary"]["reason"], "first_user_seen")
        self.assertTrue(payload["debug"]["visual_summary"]["ever_seen_face"])

    async def test_visual_observation_uses_japanese_first_user_seen_language_and_voice(self):
        from harness.pet import VoiceSynthesisResult

        class RecordingVoiceRouter:
            def __init__(self):
                self.calls = []

            async def synthesize(self, request):
                self.calls.append(request)
                return VoiceSynthesisResult(
                    text=request.text,
                    state="ready",
                    sample_rate=24000,
                    duration_ms=900,
                    rms=0.5,
                    chunk_ref="visual-ja",
                    url="/audio/visual-ja.wav",
                    provider=request.provider,
                    voice=request.voice,
                    retention="ephemeral",
                    metadata={"language": request.language, "text_language": request.metadata["text_lang"]},
                )

        temp_dir = tempfile.TemporaryDirectory()
        self.addCleanup(temp_dir.cleanup)
        audio_path = Path(temp_dir.name) / "ref.wav"
        _write_wav(audio_path, duration_sec=8.0)
        runtime = PetRuntime(
            PetRuntimeConfig(
                voice_provider="gpt-sovits",
                voice_sovits_base_url="http://127.0.0.1:9880",
                voice_sovits_reference_audio=str(audio_path),
            )
        )
        voice_router = RecordingVoiceRouter()
        runtime._voice_router = voice_router  # type: ignore[attr-defined]
        session = await runtime.create_session(uid=7)

        with _patched_policy_time(1000.0):
            event = await runtime.update_observation(
                PetObservation.from_dict(
                    {
                        "session_id": session.session_id,
                        "audio": {"vad": "idle", "rms": 0.0, "interrupt": False},
                        "vision": {
                            "enabled": True,
                            "mode": "opencv_face_detection",
                            "face_present": True,
                            "attention": "face_detected",
                            "expression": "neutral",
                            "confidence": 0.72,
                            "pose": {"face_confidence": 0.72},
                            "reply_language": "ja-JP",
                        },
                        "privacy": {"raw_frame_sent": False, "raw_audio_recorded": False},
                    }
                )
            )

        payload = event.to_dict()
        self.assertEqual(payload["phase"], "speaking")
        self.assertEqual(payload["speech"]["text_delta"], "ちゃんと見えているよ~")
        self.assertEqual(payload["text"]["language"], "ja-JP")
        self.assertEqual(payload["audio"]["url"], "/audio/visual-ja.wav")
        self.assertEqual(payload["audio"]["state"], "ready")
        self.assertEqual(payload["debug"]["visual_summary"]["reason"], "first_user_seen")
        self.assertEqual(payload["debug"]["visual_summary"]["speech_language"], "ja-JP")
        self.assertEqual(payload["debug"]["voice"]["provider"], "gpt-sovits")
        self.assertEqual(len(voice_router.calls), 1)
        self.assertEqual(voice_router.calls[0].text, "ちゃんと見えているよ~")
        self.assertEqual(voice_router.calls[0].language, "ja-JP")
        self.assertEqual(voice_router.calls[0].metadata["text_lang"], "ja")

    async def test_visual_observation_does_not_use_scripted_voice_fallback(self):
        class RejectingVoiceRouter:
            async def synthesize(self, request):
                raise AssertionError("scripted visual speech should not synthesize fallback audio")

        runtime = PetRuntime()
        runtime._voice_router = RejectingVoiceRouter()  # type: ignore[attr-defined]
        session = await runtime.create_session(uid=7)

        with _patched_policy_time(1000.0):
            event = await runtime.update_observation(
                PetObservation.from_dict(
                    {
                        "session_id": session.session_id,
                        "audio": {"vad": "idle", "rms": 0.0, "interrupt": False},
                        "vision": {
                            "enabled": True,
                            "mode": "opencv_face_detection",
                            "face_present": True,
                            "attention": "face_detected",
                            "expression": "neutral",
                            "confidence": 0.72,
                            "pose": {"face_confidence": 0.72},
                        },
                        "privacy": {"raw_frame_sent": False, "raw_audio_recorded": False},
                    }
                )
            )

        payload = event.to_dict()
        self.assertEqual(payload["speech"]["text_delta"], "我已经看到你了哦~")
        self.assertIsNone(payload["audio"]["url"])
        self.assertEqual(payload["audio"]["state"], "error")
        self.assertEqual(payload["debug"]["voice"]["provider"], "scripted")
        self.assertIn("non-deterministic voice provider", payload["debug"]["voice"]["metadata"]["audio_error"])

    async def test_visual_observation_stays_animation_only_when_low_confidence(self):
        runtime = PetRuntime()
        session = await runtime.create_session(uid=7)

        with _patched_policy_time(1000.0):
            event = await runtime.update_observation(
                PetObservation.from_dict(
                    {
                        "session_id": session.session_id,
                        "audio": {"vad": "idle", "rms": 0.0, "interrupt": False},
                        "vision": {
                            "enabled": True,
                            "mode": "mediapipe_face_landmarker",
                            "face_present": True,
                            "expression": "smile",
                            "confidence": 0.35,
                            "pose": {"face_confidence": 0.35},
                        },
                        "privacy": {"raw_frame_sent": False, "raw_audio_recorded": False},
                    }
                )
            )

        payload = event.to_dict()
        self.assert_v1_control_payload(payload, session.session_id)
        self.assertEqual(payload["phase"], "listening")
        self.assertEqual(payload["motion"], "observe")
        self.assertFalse(payload["text"]["final"])
        self.assertEqual(payload["speech"]["text_delta"], "")
        self.assertEqual(payload["debug"]["visual_summary"]["reason"], "low_confidence")
        self.assertFalse(payload["debug"]["visual_summary"]["speak"])

    async def test_visual_observation_stays_animation_only_without_face(self):
        runtime = PetRuntime()
        session = await runtime.create_session(uid=7)

        with _patched_policy_time(1000.0):
            event = await runtime.update_observation(
                PetObservation.from_dict(
                    {
                        "session_id": session.session_id,
                        "audio": {"vad": "idle", "rms": 0.0, "interrupt": False},
                        "vision": {
                            "enabled": True,
                            "mode": "mediapipe_face_landmarker",
                            "face_present": False,
                            "expression": "smile",
                            "confidence": 0.88,
                            "pose": {"face_confidence": 0.88},
                        },
                        "privacy": {"raw_frame_sent": False, "raw_audio_recorded": False},
                    }
                )
            )

        payload = event.to_dict()
        self.assert_v1_control_payload(payload, session.session_id)
        self.assertEqual(payload["phase"], "idle")
        self.assertEqual(payload["emotion"], "neutral")
        self.assertEqual(payload["expression"], "neutral")
        self.assertEqual(payload["speech"]["text_delta"], "")
        self.assertEqual(payload["debug"]["visual_summary"]["reason"], "no_face")
        self.assertFalse(payload["debug"]["visual_summary"]["speak"])

    async def test_visual_summary_cooldown_suppresses_repeat_speech(self):
        runtime = PetRuntime()
        session = await runtime.create_session(uid=7)

        with _patched_policy_time(1000.0, 1001.0):
            first = await runtime.update_observation(
                PetObservation.from_dict(
                    {
                        "session_id": session.session_id,
                        "audio": {"vad": "idle", "rms": 0.0, "interrupt": False},
                        "vision": {
                            "enabled": True,
                            "mode": "mediapipe_face_landmarker",
                            "face_present": True,
                            "expression": "smile",
                            "confidence": 0.95,
                            "pose": {"face_confidence": 0.95},
                        },
                        "privacy": {"raw_frame_sent": False, "raw_audio_recorded": False},
                    }
                )
            )
            second = await runtime.update_observation(
                PetObservation.from_dict(
                    {
                        "session_id": session.session_id,
                        "audio": {"vad": "idle", "rms": 0.0, "interrupt": False},
                        "vision": {
                            "enabled": True,
                            "mode": "mediapipe_face_landmarker",
                            "face_present": True,
                            "expression": "surprised",
                            "confidence": 0.95,
                            "pose": {"face_confidence": 0.95},
                        },
                        "privacy": {"raw_frame_sent": False, "raw_audio_recorded": False},
                    }
                )
            )

        first_payload = first.to_dict()
        second_payload = second.to_dict()
        self.assertTrue(first_payload["text"]["final"])
        self.assertTrue(first_payload["speech"]["text_delta"])
        self.assertFalse(second_payload["text"]["final"])
        self.assertEqual(second_payload["speech"]["text_delta"], "")
        self.assertEqual(second_payload["debug"]["visual_summary"]["reason"], "cooldown")
        self.assertEqual(
            second_payload["debug"]["visual_summary"]["cached_summary_text"],
            "你看起来心情不错。",
        )

    async def test_submit_input_carries_cached_visual_summary_into_prompt_metadata(self):
        runtime = PetRuntime()

        class RecordingProvider:
            def __init__(self):
                self.calls = []

            async def generate(self, session, prompt, runtime_metadata=None):
                self.calls.append(
                    {
                        "session": session,
                        "prompt": prompt,
                        "runtime_metadata": dict(runtime_metadata or {}),
                    }
                )
                return [types.SimpleNamespace(text="你好", emotion="cheerful", final=True, voice=None)]

        provider = RecordingProvider()
        runtime._providers = provider  # type: ignore[attr-defined]
        session = await runtime.create_session(uid=7)

        with _patched_policy_time(1000.0):
            await runtime.update_observation(
                PetObservation.from_dict(
                    {
                        "session_id": session.session_id,
                        "audio": {"vad": "idle", "rms": 0.0, "interrupt": False},
                        "vision": {
                            "enabled": True,
                            "mode": "mediapipe_face_landmarker",
                            "face_present": True,
                            "expression": "smile",
                            "confidence": 0.95,
                            "pose": {"face_confidence": 0.95},
                        },
                        "privacy": {"raw_frame_sent": False, "raw_audio_recorded": False},
                    }
                )
            )

        events = await runtime.submit_input(
            session.session_id,
            "请继续说",
            model_type="openai",
            model_name="gpt-4o",
        )

        self.assertEqual(len(provider.calls), 1)
        call = provider.calls[0]
        self.assertIn("visual_summary", call["runtime_metadata"])
        self.assertEqual(call["runtime_metadata"]["visual_summary"]["summary_text"], "你看起来心情不错。")
        self.assertEqual(call["runtime_metadata"]["visual_summary"]["speech_text"], "我已经看到你了哦~")
        self.assertEqual(call["prompt"].observation_summary["vision"]["expression"], "smile")
        self.assertEqual(call["prompt"].observation_summary["vision"]["confidence"], 0.95)
        self.assertEqual(len(events), 3)
        self.assertEqual(events[1].to_dict()["phase"], "speaking")

    async def test_stream_receives_queued_event(self):
        runtime = PetRuntime()
        session = await runtime.create_session(uid=7)
        stream = runtime.stream(session.session_id)

        event = await stream.__anext__()

        self.assertEqual(event.session_id, session.session_id)
        payload = event.to_dict()
        self.assert_v1_control_payload(payload, session.session_id)
        self.assertEqual(payload["action"]["name"], "appear")
        await stream.aclose()

    async def test_interrupt_emits_interrupted_phase(self):
        runtime = PetRuntime()
        session = await runtime.create_session(uid=7)

        event = await runtime.interrupt(session.session_id)

        payload = event.to_dict()
        self.assert_v1_control_payload(payload, session.session_id)
        self.assertEqual(payload["phase"], "interrupted")
        self.assertEqual(payload["action"]["name"], "interrupt")
        self.assertEqual(payload["motion"], "stop")
        self.assertEqual(payload["lip_sync"]["value"], 0.0)

    async def test_interrupt_does_not_close_session_for_next_input(self):
        runtime = PetRuntime()
        session = await runtime.create_session(uid=7)

        interrupted = await runtime.interrupt(session.session_id)
        events = await runtime.submit_input(session.session_id, "继续", model_type="scripted")

        self.assertEqual(interrupted.to_dict()["phase"], "interrupted")
        self.assertGreaterEqual(len(events), 3)
        self.assertEqual(events[0].to_dict()["phase"], "listening")
        self.assertIn("我听到了，正在认真回应你。", "".join(event.to_dict()["text"]["delta"] for event in events))
        self.assertIsNotNone(runtime.get_session(session.session_id))

    async def test_repeated_interrupt_remains_session_safe_and_streamable(self):
        runtime = PetRuntime()
        session = await runtime.create_session(uid=7)

        first = await runtime.interrupt(session.session_id)
        second = await runtime.interrupt(session.session_id)
        stream = runtime.stream(session.session_id, heartbeat_sec=0.01)
        streamed = await stream.__anext__()
        await stream.aclose()
        events = await runtime.submit_input(session.session_id, "还在吗", model_type="scripted")

        first_payload = first.to_dict()
        second_payload = second.to_dict()
        streamed_payload = streamed.to_dict()
        for payload in (first_payload, second_payload, streamed_payload):
            self.assert_v1_control_payload(payload, session.session_id)
        self.assertEqual(first_payload["phase"], "interrupted")
        self.assertEqual(second_payload["phase"], "interrupted")
        self.assertEqual(streamed_payload["phase"], "interrupted")
        self.assertEqual(streamed_payload["action"]["name"], "interrupt")
        self.assertGreater(second_payload["seq"], first_payload["seq"])
        self.assertGreater(streamed_payload["seq"], first_payload["seq"])
        self.assertEqual(runtime.get_session(session.session_id).status, "active")
        self.assertIn("我明白你的问题了，先从最关键的地方开始回答。", "".join(event.to_dict()["text"]["delta"] for event in events))

    async def test_list_sessions_orders_by_recent_update_and_filters_uid(self):
        runtime = PetRuntime()
        older = await runtime.create_session(uid=7, profile_id="older")
        await asyncio.sleep(0.002)
        newer = await runtime.create_session(uid=7, profile_id="newer")
        await asyncio.sleep(0.002)
        other_uid = await runtime.create_session(uid=99, profile_id="other")
        await asyncio.sleep(0.002)

        await runtime.submit_input(older.session_id, "bump older")

        sessions = runtime.list_sessions(uid=7)
        self.assertEqual([session.session_id for session in sessions], [older.session_id, newer.session_id])
        self.assertGreaterEqual(sessions[0].updated_at_ms, sessions[1].updated_at_ms)
        self.assertNotIn(other_uid.session_id, [session.session_id for session in sessions])

        all_sessions = runtime.list_sessions()
        self.assertEqual(all_sessions[0].session_id, older.session_id)

    async def test_unknown_session_operations_raise_errors(self):
        runtime = PetRuntime()

        with self.assertRaises(KeyError):
            await runtime.submit_input("missing-session", "hello")
        with self.assertRaises(KeyError):
            await runtime.update_observation(PetObservation(session_id="missing-session"))
        with self.assertRaises(KeyError):
            await runtime.interrupt("missing-session")

        stream = runtime.stream("missing-session", heartbeat_sec=0.01)
        with self.assertRaises(KeyError):
            await stream.__anext__()
        await stream.aclose()

    async def test_sequences_are_monotonic_per_session_and_independent(self):
        runtime = PetRuntime()
        first = await runtime.create_session(uid=7)
        second = await runtime.create_session(uid=7)

        first_events = await runtime.submit_input(first.session_id, "first")
        second_events = await runtime.submit_input(second.session_id, "second")

        first_payloads = [event.to_dict() for event in first_events]
        second_payloads = [event.to_dict() for event in second_events]

        self.assertEqual(first_payloads[0]["seq"], 2)
        self.assertEqual(second_payloads[0]["seq"], 2)
        self.assertEqual(
            [payload["seq"] for payload in first_payloads],
            list(range(first_payloads[0]["seq"], first_payloads[-1]["seq"] + 1)),
        )
        self.assertEqual(
            [payload["seq"] for payload in second_payloads],
            list(range(second_payloads[0]["seq"], second_payloads[-1]["seq"] + 1)),
        )
        self.assertEqual({payload["session_id"] for payload in first_payloads}, {first.session_id})
        self.assertEqual({payload["session_id"] for payload in second_payloads}, {second.session_id})

    async def test_observation_privacy_flags_require_literal_true_and_track_devices(self):
        runtime = PetRuntime()
        session = await runtime.create_session(uid=7)

        event = await runtime.update_observation(
            PetObservation.from_dict(
                {
                    "session_id": session.session_id,
                    "audio": {"vad": "speaking", "rms": 0.1},
                    "vision": {"enabled": True, "mode": "landmarks_only", "face_present": False},
                    "privacy": {
                        "raw_frame_sent": "true",
                        "raw_audio_recorded": 1,
                        "cloud_vision_used": "yes",
                    },
                }
            )
        )

        payload = event.to_dict()
        self.assert_v1_control_payload(payload, session.session_id)
        self.assertTrue(payload["privacy"]["camera_used"])
        self.assertTrue(payload["privacy"]["mic_used"])
        self.assertFalse(payload["privacy"]["raw_frame_sent"])
        self.assertFalse(payload["privacy"]["raw_audio_recorded"])
        self.assertFalse(payload["privacy"]["cloud_vision_used"])

        explicit = await runtime.update_observation(
            PetObservation.from_dict(
                {
                    "session_id": session.session_id,
                    "audio": {"vad": "idle"},
                    "vision": {"enabled": False},
                    "privacy": {
                        "raw_frame_sent": True,
                        "raw_audio_recorded": True,
                        "cloud_vision_used": True,
                    },
                }
            )
        )
        explicit_payload = explicit.to_dict()
        self.assertFalse(explicit_payload["privacy"]["camera_used"])
        self.assertFalse(explicit_payload["privacy"]["mic_used"])
        self.assertTrue(explicit_payload["privacy"]["raw_frame_sent"])
        self.assertTrue(explicit_payload["privacy"]["raw_audio_recorded"])
        self.assertTrue(explicit_payload["privacy"]["cloud_vision_used"])

    async def test_provider_unavailable_returns_recoverable_error_event(self):
        runtime = PetRuntime(PetRuntimeConfig(deterministic=True))
        session = await runtime.create_session(uid=7, provider="unavailable")

        events = await runtime.submit_input(session.session_id, "hello", model_type="llm", model_name="missing")
        payloads = [event.to_dict() for event in events]

        self.assertEqual(len(payloads), 2)
        self.assert_v1_control_payload(payloads[-1], session.session_id)
        self.assertEqual(payloads[0]["phase"], "listening")
        self.assertEqual(payloads[-1]["phase"], "error")
        self.assertEqual(payloads[-1]["action"]["name"], "provider_error")
        self.assertTrue(payloads[-1]["text"]["final"])
        self.assertIn("provider", str(payloads[-1]["debug"]).lower())

    async def test_voice_training_job_prepares_gpt_sovits_reference_without_clone_sidecar(self):
        from harness.pet import VoiceTrainingRequest

        temp_dir = tempfile.TemporaryDirectory()
        self.addCleanup(temp_dir.cleanup)
        source_dir = Path(temp_dir.name) / "source"
        source_dir.mkdir()
        audio_path = source_dir / "Kafuuchino-voice.wav"
        _write_wav(audio_path, duration_sec=8.0)
        runtime = PetRuntime(
            PetRuntimeConfig(
                deterministic=True,
                voice_clone_enabled=False,
                voice_training_artifact_root=temp_dir.name,
            )
        )
        job = runtime.create_voice_training_job(
            VoiceTrainingRequest(
                uid=7,
                profile_id="default",
                voice_name="Kafuuchino-voice",
                language="zh-CN",
                reference_audio_path=str(audio_path),
                reference_audio_directory="src/KafuuChino/香风智乃voice",
                reference_audio_file="Kafuuchino-voice.wav",
                consent_confirmed=True,
            )
        )
        payload = job.to_dict()

        self.assertEqual(payload["status"], "ready")
        self.assertEqual(payload["stage"], "ready_for_gpt_sovits")
        self.assertEqual(payload["progress"], 70)
        self.assertEqual(payload["provider"], "gpt-sovits")
        self.assertTrue(payload["consent_confirmed"])
        self.assertIn("Kafuuchino-voice.wav", payload["reference_audio_path"])
        self.assertFalse(payload["diagnostics"]["training_started"])
        self.assertTrue(payload["diagnostics"]["audio_persisted"])
        self.assertTrue(payload["diagnostics"]["manifest_persisted"])
        self.assertTrue(payload["diagnostics"]["reference_audio_exists"])
        self.assertEqual(payload["diagnostics"]["reference_audio_extension"], "wav")
        self.assertEqual(payload["diagnostics"]["reference_audio_material_status"], "zero_shot_ready")
        self.assertFalse(payload["diagnostics"]["needs_more_audio_for_inference"])
        self.assertTrue(Path(payload["diagnostics"]["gpt_sovits_reference_audio"]).exists())
        self.assertTrue(Path(payload["manifest_path"]).exists())
        self.assertIn(payload["job_id"], payload["artifact_path"])
        self.assertEqual(runtime.get_voice_training_job(payload["job_id"]).job_id, payload["job_id"])

    async def test_runtime_voice_metadata_uses_latest_ready_training_reference(self):
        from harness.pet import VoiceTrainingRequest

        temp_dir = tempfile.TemporaryDirectory()
        self.addCleanup(temp_dir.cleanup)
        source_dir = Path(temp_dir.name) / "source"
        source_dir.mkdir()
        audio_path = source_dir / "Kafuuchino-voice.wav"
        _write_wav(audio_path, duration_sec=8.0)
        runtime = PetRuntime(
            PetRuntimeConfig(
                deterministic=True,
                voice_provider="scripted",
                voice_sovits_base_url="http://127.0.0.1:9880",
                voice_training_artifact_root=temp_dir.name,
            )
        )

        job = runtime.create_voice_training_job(
            VoiceTrainingRequest(reference_audio_path=str(audio_path), consent_confirmed=True)
        )
        payload = job.to_dict()
        metadata = runtime._voice_runtime_metadata()

        self.assertEqual(metadata["voice_provider"], "gpt-sovits")
        self.assertEqual(metadata["ref_audio_path"], payload["diagnostics"]["gpt_sovits_reference_audio"])

    async def test_submit_input_uses_selected_model_and_japanese_reply_translation(self):
        from harness.pet import ProviderChunk, VoiceSynthesisResult

        class FakeProviderRouter:
            def __init__(self):
                self.calls = []

            async def generate(self, session, prompt, runtime_metadata=None):
                self.calls.append(
                    {
                        "session": session,
                        "prompt": prompt,
                        "runtime_metadata": dict(runtime_metadata or {}),
                    }
                )
                voice = VoiceSynthesisResult(
                    text="こんにちは",
                    state="ready",
                    sample_rate=24000,
                    duration_ms=1280,
                    rms=0.52,
                    chunk_ref="gpt-sovits-test",
                    url="/audio/gpt-sovits-test.wav",
                    provider="gpt-sovits",
                    voice="kafuu-chino",
                    retention="ephemeral",
                    metadata={
                        "language": "ja-JP",
                        "translation": "你好，桌宠",
                    },
                )
                return [
                    ProviderChunk(
                        text="こんにちは",
                        emotion="cheerful",
                        final=True,
                        voice=voice,
                        metadata={
                            "language": "ja-JP",
                            "translation": "你好，桌宠",
                        },
                    )
                ]

        temp_dir = tempfile.TemporaryDirectory()
        self.addCleanup(temp_dir.cleanup)
        audio_path = Path(temp_dir.name) / "ref.wav"
        _write_wav(audio_path, duration_sec=8.0)
        runtime = PetRuntime(
            PetRuntimeConfig(
                deterministic=True,
                voice_provider="gpt-sovits",
                voice_sovits_base_url="http://127.0.0.1:9880",
                voice_sovits_reference_audio=str(audio_path),
            )
        )
        fake_provider = FakeProviderRouter()
        runtime._providers = fake_provider  # type: ignore[attr-defined]
        session = await runtime.create_session(uid=7)

        events = await runtime.submit_input(
            session.session_id,
            "请用日文回复我",
            model_type="openai",
            model_name="gpt-4o",
            metadata={"reply_language": "ja-JP"},
        )

        self.assertEqual(len(fake_provider.calls), 1)
        call = fake_provider.calls[0]
        self.assertEqual(call["prompt"].model_type, "openai")
        self.assertEqual(call["prompt"].model_name, "gpt-4o")
        self.assertEqual(call["runtime_metadata"]["reply_language"], "ja-JP")
        self.assertEqual(call["runtime_metadata"]["language"], "ja-JP")
        self.assertEqual(call["runtime_metadata"]["text_lang"], "ja")
        self.assertEqual(call["runtime_metadata"]["voice_provider"], "gpt-sovits")
        self.assertEqual(call["runtime_metadata"]["ref_audio_path"], str(audio_path))

        payloads = [event.to_dict() for event in events]
        speaking = next(payload for payload in payloads if payload["phase"] == "speaking")
        self.assertEqual(speaking["text"]["delta"], "こんにちは")
        self.assertEqual(speaking["text"]["translation"], "你好，桌宠")
        self.assertEqual(speaking["text"]["language"], "ja-JP")
        self.assertEqual(speaking["audio"]["url"], "/audio/gpt-sovits-test.wav")
        self.assertEqual(speaking["debug"]["voice"]["provider"], "gpt-sovits")
        self.assertEqual(speaking["debug"]["voice"]["metadata"]["translation"], "你好，桌宠")

    async def test_runtime_voice_diagnostics_identifies_when_user_material_is_needed(self):
        runtime = PetRuntime(PetRuntimeConfig(voice_provider="gpt-sovits", voice_sovits_base_url="http://127.0.0.1:9880"))

        payload = await runtime.voice_diagnostics()

        self.assertFalse(payload["ready"])
        self.assertTrue(payload["needs_user_material"])
        self.assertIn("reference audio", payload["message"].lower())

    async def test_runtime_voice_diagnostics_identifies_long_material_needs_reference_clip(self):
        from harness.pet.voice_training import diagnose_reference_audio

        temp_dir = tempfile.TemporaryDirectory()
        self.addCleanup(temp_dir.cleanup)
        audio_path = Path(temp_dir.name) / "long.wav"
        _write_wav(audio_path, duration_sec=65.0)
        runtime = PetRuntime(
            PetRuntimeConfig(
                voice_provider="gpt-sovits",
                voice_sovits_base_url="http://127.0.0.1:9880",
                voice_sovits_reference_audio=str(audio_path),
            )
        )

        audio = diagnose_reference_audio(str(audio_path))
        payload = await runtime.voice_diagnostics()

        self.assertTrue(audio["few_shot_ready"])
        self.assertTrue(payload["needs_reference_clip"])
        self.assertTrue(payload["needs_prompt_text"])
        self.assertIn("reference clip", payload["message"].lower())

    async def test_runtime_vision_diagnostics_reports_disabled_capture(self):
        runtime = PetRuntime(PetRuntimeConfig(local_vision_enabled=False))

        payload = runtime.vision_diagnostics()

        self.assertFalse(payload["ready"])
        self.assertEqual(payload["status"], "disabled")
        self.assertIn("message", payload)

    async def test_voice_training_can_be_disabled_by_config(self):
        from harness.pet import VoiceTrainingRequest

        runtime = PetRuntime(PetRuntimeConfig(voice_training_enabled=False))

        with self.assertRaisesRegex(RuntimeError, "voice training is disabled"):
            runtime.create_voice_training_job(
                VoiceTrainingRequest(reference_audio_path="voice.mp3", consent_confirmed=True)
            )

    async def test_capture_observation_requires_local_vision_feature(self):
        from harness.pet import VisionCaptureRequest

        runtime = PetRuntime(PetRuntimeConfig(local_vision_enabled=False))
        session = await runtime.create_session(uid=7)

        with self.assertRaisesRegex(Exception, "disabled"):
            await runtime.capture_observation(VisionCaptureRequest(session_id=session.session_id))

    async def test_stream_heartbeat_does_not_replay_duplicate_sequence(self):
        runtime = PetRuntime()
        session = await runtime.create_session(uid=7)
        stream = runtime.stream(session.session_id, heartbeat_sec=0.01)

        initial = await stream.__anext__()
        heartbeat = await stream.__anext__()
        heartbeat_payload = heartbeat.to_dict()

        self.assert_v1_control_payload(initial.to_dict(), session.session_id)
        self.assert_v1_control_payload(heartbeat_payload, session.session_id)
        self.assertEqual(heartbeat_payload["action"]["name"], "heartbeat")
        self.assertEqual(heartbeat_payload["phase"], "idle")
        await stream.aclose()

        second_stream = runtime.stream(session.session_id, heartbeat_sec=0.01)
        second_heartbeat_payload = (await second_stream.__anext__()).to_dict()

        self.assert_v1_control_payload(second_heartbeat_payload, session.session_id)
        self.assertEqual(second_heartbeat_payload["action"]["name"], "heartbeat")
        self.assertGreater(second_heartbeat_payload["seq"], heartbeat_payload["seq"])
        self.assertNotEqual(second_heartbeat_payload["event_id"], heartbeat_payload["event_id"])
        await second_stream.aclose()


def _write_wav(path: Path, duration_sec: float, sample_rate: int = 16000) -> None:
    import wave

    frame_count = int(duration_sec * sample_rate)
    with wave.open(str(path), "wb") as handle:
        handle.setnchannels(1)
        handle.setsampwidth(2)
        handle.setframerate(sample_rate)
        handle.writeframes(b"\0\0" * frame_count)


def _patched_policy_time(*values: float):
    if len(values) == 1:
        fake_time = types.SimpleNamespace(time=mock.Mock(return_value=values[0]))
    else:
        fake_time = types.SimpleNamespace(time=mock.Mock(side_effect=list(values)))
    return mock.patch("harness.pet.policy.time", fake_time)


if __name__ == "__main__":
    unittest.main()
