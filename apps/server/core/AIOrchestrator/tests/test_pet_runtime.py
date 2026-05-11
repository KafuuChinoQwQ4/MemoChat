from __future__ import annotations

import unittest
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from harness.pet import PetObservation, PetRuntime


class PetRuntimeTests(unittest.IsolatedAsyncioTestCase):
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
        first = events[0].to_dict()
        self.assertEqual(first["type"], "pet.control")
        self.assertEqual(first["session_id"], session.session_id)
        self.assertIn(first["motion"], {"listen", "talk", "idle"})

        speech = "".join(event.to_dict()["speech"]["text_delta"] for event in events)
        self.assertIn("收到：你好", speech)
        self.assertEqual(events[-1].to_dict()["lip_sync"]["value"], 0.0)

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
        self.assertEqual(payload["expression"], "smile_soft")
        self.assertEqual(payload["lip_sync"]["value"], 0.64)
        self.assertTrue(payload["safety"]["camera_used"])
        self.assertEqual(payload["safety"]["vision_detail"], "landmarks_only")

    async def test_stream_receives_queued_event(self):
        runtime = PetRuntime()
        session = await runtime.create_session(uid=7)
        stream = runtime.stream(session.session_id)

        event = await stream.__anext__()

        self.assertEqual(event.session_id, session.session_id)
        self.assertEqual(event.to_dict()["action"]["name"], "appear")
        await stream.aclose()


if __name__ == "__main__":
    unittest.main()
