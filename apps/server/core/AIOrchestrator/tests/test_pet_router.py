from __future__ import annotations

import json
import sys
import unittest
import tempfile
from pathlib import Path
from typing import AsyncIterator

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

try:
    import httpx
    from fastapi import FastAPI
    from api import pet_router
except ImportError as exc:  # pragma: no cover - exercised only on lean host images.
    httpx = None
    FastAPI = None
    pet_router = None
    _IMPORT_ERROR = exc
else:
    _IMPORT_ERROR = None

from harness.pet import PetRuntime
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


class _OneShotStreamRuntime(PetRuntime):
    async def stream(self, session_id: str, heartbeat_sec: float = 15.0) -> AsyncIterator[object]:
        stream = super().stream(session_id, heartbeat_sec=heartbeat_sec)
        try:
            yield await stream.__anext__()
        finally:
            await stream.aclose()


@unittest.skipIf(_IMPORT_ERROR is not None, f"FastAPI/router test dependency unavailable: {_IMPORT_ERROR}")
class PetRouterTests(unittest.IsolatedAsyncioTestCase):
    async def asyncSetUp(self) -> None:
        self._old_runtime = pet_router._runtime
        self._old_pet_enabled = pet_router.settings.pet.enabled
        pet_router.settings.pet.enabled = True
        self._voice_training_artifacts = tempfile.TemporaryDirectory()
        self.runtime = PetRuntime(
            PetRuntimeConfig(
                enabled=True,
                deterministic=True,
                voice_training_artifact_root=self._voice_training_artifacts.name,
            )
        )
        pet_router._runtime = self.runtime

        app = FastAPI()
        app.include_router(pet_router.router, prefix="/pet")
        transport = httpx.ASGITransport(app=app)
        self.client = httpx.AsyncClient(transport=transport, base_url="http://testserver")

    async def asyncTearDown(self) -> None:
        await self.client.aclose()
        pet_router._runtime = self._old_runtime
        pet_router.settings.pet.enabled = self._old_pet_enabled
        self._voice_training_artifacts.cleanup()

    def assert_ok(self, payload: dict) -> None:
        self.assertEqual(payload["code"], 0)
        self.assertEqual(payload["message"], "ok")

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
        self.assertTrue(_ANIMATION_KEYS.issubset(payload["animation"]))
        self.assertTrue(_AUDIO_KEYS.issubset(payload["audio"]))
        self.assertTrue(_TEXT_KEYS.issubset(payload["text"]))
        self.assertTrue(_PRIVACY_KEYS.issubset(payload["privacy"]))

        self.assertEqual(payload["motion"], payload["animation"]["motion"])
        self.assertEqual(payload["expression"], payload["animation"]["expression"])
        self.assertAlmostEqual(payload["lip_sync"]["value"], payload["audio"]["rms"])
        self.assertEqual(payload["speech"]["text_delta"], payload["text"]["delta"])
        self.assertEqual(payload["speech"]["audio_url"], payload["audio"]["url"])
        self.assertEqual(payload["speech"]["audio_chunk_ref"], payload["audio"]["chunk_ref"])
        self.assertEqual(payload["safety"]["camera_used"], payload["privacy"]["camera_used"])

    async def _create_session(self, uid: int = 7) -> str:
        response = await self.client.post(
            "/pet/sessions",
            json={"uid": uid, "profile_id": "default", "persona": "memo-pet", "provider": "scripted"},
        )
        self.assertEqual(response.status_code, 200)
        payload = response.json()
        self.assert_ok(payload)
        return payload["session"]["session_id"]

    async def test_create_and_list_sessions_are_isolated_by_runtime(self):
        first_id = await self._create_session(uid=7)
        second_id = await self._create_session(uid=99)

        all_response = await self.client.get("/pet/sessions")
        self.assertEqual(all_response.status_code, 200)
        all_payload = all_response.json()
        self.assert_ok(all_payload)
        self.assertEqual({item["session_id"] for item in all_payload["sessions"]}, {first_id, second_id})

        filtered_response = await self.client.get("/pet/sessions", params={"uid": 7})
        self.assertEqual(filtered_response.status_code, 200)
        filtered_payload = filtered_response.json()
        self.assert_ok(filtered_payload)
        self.assertEqual([item["session_id"] for item in filtered_payload["sessions"]], [first_id])

    async def test_voice_training_job_requires_consent_and_returns_prepared_status(self):
        missing_consent = await self.client.post(
            "/pet/voice-training/jobs",
            json={
                "uid": 7,
                "reference_audio_path": "src/KafuuChino/香风智乃voice/Kafuuchino-voice.mp3",
                "consent_confirmed": False,
            },
        )
        self.assertEqual(missing_consent.status_code, 400)

        response = await self.client.post(
            "/pet/voice-training/jobs",
            json={
                "uid": 7,
                "profile_id": "default",
                "voice_name": "Kafuuchino-voice",
                "language": "zh-CN",
                "reference_audio_path": "src/KafuuChino/香风智乃voice/Kafuuchino-voice.mp3",
                "reference_audio_directory": "src/KafuuChino/香风智乃voice",
                "reference_audio_file": "Kafuuchino-voice.mp3",
                "consent_confirmed": True,
                "metadata": {"character": "香风智乃"},
            },
        )

        self.assertEqual(response.status_code, 200)
        payload = response.json()
        self.assert_ok(payload)
        job = payload["job"]
        self.assertTrue(job["job_id"].startswith("voice-train-"))
        self.assertEqual(job["status"], "ready")
        self.assertEqual(job["stage"], "ready_for_gpt_sovits")
        self.assertEqual(job["progress"], 70)
        self.assertFalse(job["diagnostics"]["training_started"])
        self.assertIn("artifact_path", job)
        self.assertIn("manifest_path", job)
        self.assertTrue(job["diagnostics"]["manifest_persisted"])

        detail = await self.client.get(f"/pet/voice-training/jobs/{job['job_id']}")
        self.assertEqual(detail.status_code, 200)
        self.assertEqual(detail.json()["job"]["job_id"], job["job_id"])
        self.assertEqual(detail.json()["job"]["manifest_path"], job["manifest_path"])

        listing = await self.client.get("/pet/voice-training/jobs", params={"uid": 7})
        self.assertEqual(listing.status_code, 200)
        self.assertEqual([item["job_id"] for item in listing.json()["jobs"]], [job["job_id"]])

    async def test_diagnostics_routes_report_voice_and_vision_readiness(self):
        response = await self.client.get("/pet/diagnostics")

        self.assertEqual(response.status_code, 200)
        payload = response.json()
        self.assert_ok(payload)
        diagnostics = payload["diagnostics"]
        self.assertIn("voice", diagnostics)
        self.assertIn("vision", diagnostics)
        self.assertTrue(diagnostics["needs_user_voice_material"])
        self.assertTrue(diagnostics["needs_camera_runtime"])

        voice = await self.client.get("/pet/diagnostics/voice")
        self.assertEqual(voice.status_code, 200)
        self.assertIn("reference_audio", voice.json()["diagnostics"])

        vision = await self.client.get("/pet/diagnostics/vision")
        self.assertEqual(vision.status_code, 200)
        self.assertEqual(vision.json()["diagnostics"]["status"], "disabled")

    async def test_input_returns_ordered_v1_control_events_and_legacy_mirrors(self):
        session_id = await self._create_session()

        response = await self.client.post(
            f"/pet/sessions/{session_id}/input",
            json={"uid": 7, "content": "你好，桌宠", "model_type": "scripted", "model_name": "deterministic"},
        )

        self.assertEqual(response.status_code, 200)
        payload = response.json()
        self.assert_ok(payload)
        events = payload["events"]
        self.assertGreaterEqual(len(events), 3)
        for event in events:
            self.assert_v1_control_payload(event, session_id)
        self.assertEqual(events[0]["phase"], "listening")
        self.assertIn("speaking", {event["phase"] for event in events})
        self.assertEqual(events[-1]["phase"], "idle")
        self.assertEqual(len({event["turn_id"] for event in events}), 1)
        self.assertEqual(
            [event["seq"] for event in events],
            list(range(events[0]["seq"], events[0]["seq"] + len(events))),
        )
        speech = "".join(event["text"]["delta"] for event in events)
        self.assertIn("你好，我在这里。", speech)
        self.assertNotIn("收到：你好，桌宠", speech)
        speaking = [event for event in events if event["phase"] == "speaking"]
        self.assertTrue(speaking)
        audio_url = str(speaking[0]["audio"]["url"])
        self.assertTrue(audio_url.startswith("/audio/deterministic-voice-"))
        self.assertEqual(speaking[0]["audio"]["state"], "ready")
        audio = await self.client.get(f"/pet{audio_url}")
        self.assertEqual(audio.status_code, 200)
        self.assertGreater(len(audio.content), 44)
        self.assertEqual(audio.content[:4], b"RIFF")
        self.assertEqual(audio.content[8:12], b"WAVE")

    async def test_observation_returns_v1_control_event(self):
        session_id = await self._create_session()

        response = await self.client.post(
            f"/pet/sessions/{session_id}/observation",
            json={
                "type": "pet.observation",
                "session_id": "ignored-by-route",
                "audio": {"vad": "speaking", "rms": 0.44},
                "vision": {"enabled": True, "mode": "landmarks_only", "face_present": True, "expression": "smile"},
                "privacy": {"raw_frame_sent": False, "raw_audio_recorded": False},
            },
        )

        self.assertEqual(response.status_code, 200)
        payload = response.json()
        self.assert_ok(payload)
        event = payload["event"]
        self.assert_v1_control_payload(event, session_id)
        self.assertEqual(event["expression"], "smile_soft")
        self.assertEqual(event["lip_sync"]["value"], 0.44)
        self.assertTrue(event["privacy"]["camera_used"])
        self.assertFalse(event["privacy"]["raw_frame_sent"])
        self.assertFalse(event["privacy"]["raw_audio_recorded"])

    async def test_capture_route_reports_disabled_local_vision(self):
        session_id = await self._create_session()

        response = await self.client.post(
            f"/pet/sessions/{session_id}/capture",
            json={"camera_index": 0, "analyzer": "opencv", "include_frame": False},
        )

        self.assertEqual(response.status_code, 503)
        self.assertIn("disabled", response.json()["detail"])

    async def test_capture_route_accepts_uploaded_frame_without_raw_retention(self):
        from harness.pet import PetObservation

        class UploadedFrameRuntime(PetRuntime):
            def __init__(self, config):
                super().__init__(config)
                self.capture_request = None

            async def capture_observation(self, request):
                self._sessions.touch(request.session_id, status="active")
                self.capture_request = request
                observation = PetObservation(
                    session_id=request.session_id,
                    audio={"vad": "idle", "rms": 0.0, "interrupt": False},
                    vision={
                        "enabled": True,
                        "mode": "opencv_frame_stats",
                        "face_present": False,
                        "attention": "ambient",
                        "expression": "",
                        "pose": {"brightness": 0.5},
                        "gesture": "",
                        "frame": {"width": request.frame_width, "height": request.frame_height},
                        "source": "uploaded_frame",
                    },
                    privacy={
                        "camera_used": True,
                        "cloud_vision_used": False,
                        "raw_frame_sent": False,
                        "raw_audio_recorded": False,
                        "retention": "none",
                    },
                )
                event = await self.update_observation(observation)
                return observation, event

        pet_router._runtime = UploadedFrameRuntime(PetRuntimeConfig(enabled=True, deterministic=True))
        self.runtime = pet_router._runtime
        session_id = await self._create_session()

        response = await self.client.post(
            f"/pet/sessions/{session_id}/capture",
            json={
                "frame_base64": "iVBORw0KGgo=",
                "frame_mime": "image/png",
                "frame_width": 2,
                "frame_height": 1,
                "include_frame": False,
                "metadata": {"source": "uploaded_frame"},
            },
        )

        self.assertEqual(response.status_code, 200, response.text)
        payload = response.json()
        self.assert_ok(payload)
        event = payload["event"]
        observation = payload["observation"]
        self.assert_v1_control_payload(event, session_id)
        self.assertEqual(self.runtime.capture_request.frame_base64, "iVBORw0KGgo=")
        self.assertEqual(self.runtime.capture_request.frame_mime, "image/png")
        self.assertEqual(self.runtime.capture_request.frame_width, 2)
        self.assertEqual(self.runtime.capture_request.frame_height, 1)
        self.assertFalse(observation["privacy"]["raw_frame_sent"])
        self.assertEqual(observation["privacy"]["retention"], "none")
        self.assertFalse(event["privacy"]["raw_frame_sent"])
        self.assertEqual(event["privacy"]["retention"], "none")
        self.assertEqual(observation["vision"]["source"], "uploaded_frame")

    async def test_capture_segment_route_accepts_keyframes_without_raw_retention(self):
        from harness.pet import PetObservation

        class UploadedSegmentRuntime(PetRuntime):
            def __init__(self, config):
                super().__init__(config)
                self.segment_request = None

            async def capture_segment_observation(self, request):
                self._sessions.touch(request.session_id, status="active")
                self.segment_request = request
                observation = PetObservation(
                    session_id=request.session_id,
                    audio={"vad": "idle", "rms": 0.0, "interrupt": False},
                    vision={
                        "enabled": True,
                        "mode": "opencv_frame_stats",
                        "face_present": True,
                        "attention": "face_detected",
                        "expression": "smile",
                        "pose": {"brightness": 0.5},
                        "gesture": "",
                        "frame": {"width": request.frames[-1].frame_width, "height": request.frames[-1].frame_height},
                        "source": "uploaded_segment",
                        "segment": {
                            "segment_id": request.segment_id,
                            "frame_count": len(request.frames),
                            "duration_ms": request.duration_ms,
                        },
                    },
                    privacy={
                        "camera_used": True,
                        "cloud_vision_used": False,
                        "raw_frame_sent": False,
                        "raw_frame_count": len(request.frames),
                        "raw_audio_recorded": False,
                        "retention": "none",
                    },
                )
                event = await self.update_observation(observation)
                return observation, event

        pet_router._runtime = UploadedSegmentRuntime(PetRuntimeConfig(enabled=True, deterministic=True))
        self.runtime = pet_router._runtime
        session_id = await self._create_session()

        response = await self.client.post(
            f"/pet/sessions/{session_id}/capture-segment",
            json={
                "segment_id": "seg-router",
                "duration_ms": 3000,
                "include_frame": False,
                "frames": [
                    {"frame_base64": "AQIDBA==", "frame_mime": "image/jpeg", "frame_width": 320, "frame_height": 240, "t_ms": 0},
                    {"frame_base64": "BQYHCA==", "frame_mime": "image/jpeg", "frame_width": 320, "frame_height": 240, "t_ms": 3000},
                ],
                "metadata": {"source": "qt_video_sink_segment"},
            },
        )

        self.assertEqual(response.status_code, 200, response.text)
        payload = response.json()
        self.assert_ok(payload)
        self.assert_v1_control_payload(payload["event"], session_id)
        self.assertEqual(self.runtime.segment_request.segment_id, "seg-router")
        self.assertEqual(self.runtime.segment_request.duration_ms, 3000)
        self.assertEqual(len(self.runtime.segment_request.frames), 2)
        self.assertEqual(self.runtime.segment_request.frames[1].t_ms, 3000)
        self.assertFalse(payload["observation"]["privacy"]["raw_frame_sent"])
        self.assertEqual(payload["observation"]["privacy"]["raw_frame_count"], 2)
        self.assertEqual(payload["observation"]["vision"]["source"], "uploaded_segment")
        self.assertEqual(payload["observation"]["vision"]["segment"]["frame_count"], 2)

    async def test_interrupt_returns_interrupted_control_event(self):
        session_id = await self._create_session()

        response = await self.client.post(f"/pet/sessions/{session_id}/interrupt")

        self.assertEqual(response.status_code, 200)
        payload = response.json()
        self.assert_ok(payload)
        event = payload["event"]
        self.assert_v1_control_payload(event, session_id)
        self.assertEqual(event["phase"], "interrupted")
        self.assertEqual(event["action"]["name"], "interrupt")
        self.assertEqual(event["motion"], "stop")

    async def test_stream_returns_first_sse_payload_without_hanging(self):
        pet_router._runtime = _OneShotStreamRuntime(PetRuntimeConfig(enabled=True, deterministic=True))
        self.runtime = pet_router._runtime
        session_id = await self._create_session()

        response = await self.client.get(f"/pet/sessions/{session_id}/stream", timeout=1.0)

        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.headers["content-type"].split(";")[0], "text/event-stream")
        body = response.text.strip()
        self.assertTrue(body.startswith("data: "), body)
        payload = json.loads(body.removeprefix("data: "))
        self.assert_v1_control_payload(payload, session_id)
        self.assertEqual(payload["action"]["name"], "appear")

    async def test_input_rejects_empty_content(self):
        session_id = await self._create_session()

        response = await self.client.post(f"/pet/sessions/{session_id}/input", json={"content": "  \t\n  "})

        self.assertEqual(response.status_code, 400)
        self.assertEqual(response.json()["detail"], "content cannot be empty")

    async def test_unknown_session_operations_return_http_404(self):
        for method, path, body in (
            ("post", "/pet/sessions/missing-session/input", {"content": "hello"}),
            ("post", "/pet/sessions/missing-session/observation", {"audio": {"vad": "idle"}}),
            ("post", "/pet/sessions/missing-session/interrupt", None),
            ("get", "/pet/sessions/missing-session/stream", None),
        ):
            request = getattr(self.client, method)
            response = await request(path, json=body) if body is not None else await request(path)
            self.assertEqual(response.status_code, 404, f"{method.upper()} {path}: {response.text}")
            self.assertTrue(response.json()["detail"])


if __name__ == "__main__":
    unittest.main()
