from __future__ import annotations

import unittest
import asyncio
import sys
from pathlib import Path

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
_TEXT_KEYS = {"delta", "final", "language", "display"}
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
        self.assertIn("收到：你好", speech)
        for payload in payloads:
            self.assertEqual(payload["speech"]["text_delta"], payload["text"]["delta"])
            self.assertEqual(payload["text"]["display"], payload["text"]["delta"])
        self.assertEqual(payloads[-1]["lip_sync"]["value"], 0.0)

    async def test_deterministic_voice_foundation_is_text_only_without_audio_retention(self):
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
            self.assertIsNone(payload["speech"]["audio_url"])
            if payload["audio"]["chunk_ref"] is not None:
                self.assertTrue(str(payload["audio"]["chunk_ref"]).startswith("deterministic-text-"))
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
        self.assertIn("收到：继续", "".join(event.to_dict()["text"]["delta"] for event in events))
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
        self.assertIn("收到：还在吗", "".join(event.to_dict()["text"]["delta"] for event in events))

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


if __name__ == "__main__":
    unittest.main()
