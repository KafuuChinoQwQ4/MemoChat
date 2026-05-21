from __future__ import annotations

import sys
import types
import unittest
from pathlib import Path
from unittest import mock

AI_ORCHESTRATOR_ROOT = str(Path(__file__).resolve().parents[1])
_AI_PATH_ADDED = False
if AI_ORCHESTRATOR_ROOT not in sys.path:
    sys.path.append(AI_ORCHESTRATOR_ROOT)
    _AI_PATH_ADDED = True

try:
    import structlog  # noqa: F401
except ImportError:  # pragma: no cover - keeps this route-flow test runnable on lean hosts.
    sys.modules["structlog"] = types.SimpleNamespace(
        get_logger=lambda: types.SimpleNamespace(error=lambda *args, **kwargs: None)
    )

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

from harness.pet import PetObservation, PetRuntime
from harness.pet.runtime import PetRuntimeConfig

if _AI_PATH_ADDED:
    sys.path.remove(AI_ORCHESTRATOR_ROOT)


TEST_UID = 909007


def _rich_vision(expression: str = "smile", confidence: float = 0.94, face_present: bool = True) -> dict:
    return {
        "enabled": True,
        "mode": "mediapipe_face_landmarker",
        "face_present": face_present,
        "attention": "user_face" if face_present else "no_face",
        "expression": expression if face_present else "",
        "confidence": confidence,
        "pose": {"face_confidence": confidence, "brightness": 0.62},
        "head_pose": {"yaw": 12.0, "pitch": -4.0, "roll": 3.0},
        "blendshapes": {
            "jawOpen": 0.22,
            "eyeBlinkLeft": 0.1,
            "eyeBlinkRight": 0.1,
            "mouthSmileLeft": 0.8,
            "mouthSmileRight": 0.82,
        },
        "scene": {
            "summary": "画面里有杯子和键盘。",
            "object_labels": ["cup", "keyboard"],
            "object_count": 2,
            "lighting": "ambient",
            "brightness": 0.62,
        },
        "objects": [
            {"label": "cup", "score": 0.88},
            {"label": "keyboard", "score": 0.78},
        ],
        "source": "synthetic_test_user",
        "frame": {"width": 640, "height": 480},
        "captured_at_ms": 1760000000000,
    }


class _SyntheticVisionAnalyzer:
    def diagnostics(self, camera_index=None, open_camera: bool = False):
        return {
            "enabled": True,
            "ready": True,
            "status": "ready",
            "camera_index": 0 if camera_index is None else camera_index,
            "camera_open_tested": bool(open_camera),
            "source": "synthetic_test_user",
        }

    def capture(self, request):
        return PetObservation(
            session_id=request.session_id,
            audio={"vad": "idle", "rms": 0.0, "interrupt": False},
            vision={**_rich_vision(), "source": "uploaded_frame", "frame_mime": request.frame_mime},
            privacy={
                "camera_used": True,
                "cloud_vision_used": False,
                "raw_frame_sent": False,
                "raw_audio_recorded": False,
                "retention": "none",
            },
        )


@unittest.skipIf(_IMPORT_ERROR is not None, f"FastAPI/router test dependency unavailable: {_IMPORT_ERROR}")
class PetVisualLayerUserFlowTests(unittest.IsolatedAsyncioTestCase):
    async def asyncSetUp(self) -> None:
        self._old_runtime = pet_router._runtime
        self._old_pet_enabled = pet_router.settings.pet.enabled
        pet_router.settings.pet.enabled = True
        self.runtime = PetRuntime(PetRuntimeConfig(enabled=True, deterministic=True, local_vision_enabled=True))
        self.runtime._vision = _SyntheticVisionAnalyzer()  # type: ignore[attr-defined]
        pet_router._runtime = self.runtime

        app = FastAPI()
        app.include_router(pet_router.router, prefix="/pet")
        transport = httpx.ASGITransport(app=app)
        self.client = httpx.AsyncClient(transport=transport, base_url="http://testserver")

    async def asyncTearDown(self) -> None:
        await self.client.aclose()
        pet_router._runtime = self._old_runtime
        pet_router.settings.pet.enabled = self._old_pet_enabled

    async def _create_session(self, profile_id: str) -> str:
        response = await self.client.post(
            "/pet/sessions",
            json={"uid": TEST_UID, "profile_id": profile_id, "persona": "memo-pet", "provider": "scripted"},
        )
        self.assertEqual(response.status_code, 200, response.text)
        return response.json()["session"]["session_id"]

    async def test_synthetic_test_user_exercises_full_visual_layer_flow(self):
        diagnostics = await self.client.get("/pet/diagnostics/vision")
        self.assertEqual(diagnostics.status_code, 200)
        self.assertTrue(diagnostics.json()["diagnostics"]["ready"])

        capture_session_id = await self._create_session("vision-smoke-capture")
        capture = await self.client.post(
            f"/pet/sessions/{capture_session_id}/capture",
            json={
                "frame_base64": "iVBORw0KGgo=",
                "frame_mime": "image/png",
                "frame_width": 2,
                "frame_height": 1,
                "include_frame": False,
                "metadata": {"source": "uploaded_frame", "test_user": True},
            },
        )
        self.assertEqual(capture.status_code, 200, capture.text)
        capture_payload = capture.json()
        self.assertEqual(capture_payload["observation"]["vision"]["source"], "uploaded_frame")
        self.assertEqual(capture_payload["observation"]["privacy"]["retention"], "none")
        self.assertFalse(capture_payload["event"]["privacy"]["raw_frame_sent"])
        self.assertEqual(capture_payload["event"]["action"]["name"], "visual_react")

        agent_session_id = await self._create_session("vision-smoke-agent")
        high = await self.client.post(
            f"/pet/sessions/{agent_session_id}/observation",
            json={
                "type": "pet.observation",
                "audio": {"vad": "idle", "rms": 0.0, "interrupt": False},
                "vision": _rich_vision(),
                "privacy": {"raw_frame_sent": False, "raw_audio_recorded": False, "retention": "none"},
            },
        )
        self.assertEqual(high.status_code, 200, high.text)
        high_event = high.json()["event"]
        self.assertEqual(high_event["action"]["name"], "visual_react")
        self.assertTrue(high_event["debug"]["visual_summary"]["speak"])
        self.assertIn("杯子和键盘", high_event["debug"]["visual_summary"]["summary_text"])
        self.assertIn("我看到你啦", high_event["speech"]["text_delta"])
        self.assertIn("心情还不错", high_event["speech"]["text_delta"])
        self.assertEqual(high_event["debug"]["visual_summary"]["reason"], "first_user_seen")

        reply = await self.client.post(
            f"/pet/sessions/{agent_session_id}/input",
            json={
                "uid": TEST_UID,
                "content": "你刚才看到什么？",
                "model_type": "scripted",
                "model_name": "deterministic",
            },
        )
        self.assertEqual(reply.status_code, 200, reply.text)
        phases = [event["phase"] for event in reply.json()["events"]]
        self.assertEqual(phases[0], "listening")
        self.assertIn("speaking", phases)
        self.assertEqual(phases[-1], "idle")

        last_visual_at = self.runtime._policy.visual_summary(agent_session_id).get("updated_at_ms", 0)
        with mock.patch("harness.pet.policy.time", types.SimpleNamespace(time=mock.Mock(return_value=(last_visual_at / 1000.0) + 6.0))):
            changed = await self.client.post(
                f"/pet/sessions/{agent_session_id}/observation",
                json={
                    "type": "pet.observation",
                    "audio": {"vad": "idle", "rms": 0.0, "interrupt": False},
                    "vision": _rich_vision("surprised", 0.94, True),
                    "privacy": {"raw_frame_sent": False, "raw_audio_recorded": False, "retention": "none"},
                },
            )
        self.assertEqual(changed.status_code, 200, changed.text)
        changed_event = changed.json()["event"]
        self.assertEqual(changed_event["debug"]["visual_summary"]["reason"], "updated")
        self.assertTrue(changed_event["debug"]["visual_summary"]["speak"])

        boundaries = (
            ("cooldown", _rich_vision("smile", 0.96, True)),
            ("low_confidence", _rich_vision("smile", 0.35, True)),
            ("no_face", _rich_vision("", 0.91, False)),
        )
        for expected_reason, vision in boundaries:
            response = await self.client.post(
                f"/pet/sessions/{agent_session_id}/observation",
                json={
                    "type": "pet.observation",
                    "audio": {"vad": "idle", "rms": 0.0, "interrupt": False},
                    "vision": vision,
                    "privacy": {"raw_frame_sent": False, "raw_audio_recorded": False, "retention": "none"},
                },
            )
            self.assertEqual(response.status_code, 200, response.text)
            event = response.json()["event"]
            self.assertEqual(event["debug"]["visual_summary"]["reason"], expected_reason)
            self.assertFalse(event["debug"]["visual_summary"]["speak"])
            self.assertEqual(event["speech"]["text_delta"], "")

        interrupted = await self.client.post(f"/pet/sessions/{agent_session_id}/interrupt")
        self.assertEqual(interrupted.status_code, 200, interrupted.text)
        self.assertEqual(interrupted.json()["event"]["phase"], "interrupted")
        self.assertEqual(interrupted.json()["event"]["action"]["name"], "interrupt")

        sessions = await self.client.get("/pet/sessions", params={"uid": TEST_UID})
        session_ids = {item["session_id"] for item in sessions.json()["sessions"]}
        self.assertTrue({capture_session_id, agent_session_id}.issubset(session_ids))


if __name__ == "__main__":
    unittest.main()
