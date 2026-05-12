from __future__ import annotations

import sys
import unittest
from typing import get_type_hints
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from harness.pet import PetAudio, PetControlEvent, PetLipSync, PetObservation, PetPrivacy, PetSafety, PetSpeech


class PetContractTests(unittest.TestCase):
    def test_legacy_control_event_serializes_v1_and_legacy_fields(self):
        event = PetControlEvent(
            session_id="pet-test",
            seq=12,
            timestamp_ms=123456789,
            emotion="happy",
            intensity=0.72,
            motion="wave",
            expression="smile_soft",
            lip_sync=PetLipSync(value=0.42),
            speech=PetSpeech(text_delta="hello", audio_url="https://example.invalid/a.wav"),
            safety=PetSafety(camera_used=True, vision_detail="landmarks_only"),
            action={"name": "speak", "args": {"legacy": True}},
        )

        payload = event.to_dict()

        self.assertEqual(payload["type"], "pet.control")
        for key in ("schema_version", "event_id", "turn_id", "phase", "animation", "audio", "text", "privacy"):
            self.assertIn(key, payload)
        self.assertEqual(payload["schema_version"], 1)
        self.assertEqual(payload["session_id"], "pet-test")
        self.assertEqual(payload["seq"], 12)
        self.assertIsInstance(payload["event_id"], str)
        self.assertTrue(payload["event_id"])
        self.assertIsInstance(payload["turn_id"], str)
        self.assertTrue(payload["turn_id"])
        self.assertIn(payload["phase"], {"idle", "listening", "thinking", "speaking", "interrupted", "error"})

        self.assertEqual(payload["animation"]["motion"], "wave")
        self.assertEqual(payload["animation"]["expression"], "smile_soft")
        self.assertEqual(payload["audio"]["rms"], 0.42)
        self.assertEqual(payload["audio"]["url"], "https://example.invalid/a.wav")
        self.assertEqual(payload["text"]["delta"], "hello")
        self.assertEqual(payload["privacy"]["camera_used"], True)
        self.assertEqual(payload["privacy"]["raw_frame_sent"], False)
        self.assertEqual(payload["privacy"]["raw_audio_recorded"], False)

        self.assertEqual(payload["motion"], payload["animation"]["motion"])
        self.assertEqual(payload["expression"], payload["animation"]["expression"])
        self.assertEqual(payload["lip_sync"]["value"], payload["audio"]["rms"])
        self.assertEqual(payload["speech"]["text_delta"], payload["text"]["delta"])
        self.assertEqual(payload["speech"]["audio_url"], payload["audio"]["url"])
        self.assertEqual(payload["safety"]["camera_used"], payload["privacy"]["camera_used"])

    def test_observation_from_dict_tolerates_unknown_fields_and_defaults_privacy(self):
        observation = PetObservation.from_dict(
            {
                "type": "pet.observation",
                "session_id": "pet-test",
                "audio": {"vad": "speaking", "rms": 0.71, "unexpected_audio": "ignored"},
                "vision": {
                    "enabled": True,
                    "mode": "landmarks_only",
                    "face_present": True,
                    "expression": "smile",
                    "unexpected_vision": {"nested": True},
                },
                "privacy": {"retention": "ephemeral"},
                "legacy_top_level": "ignored",
            }
        )

        payload = observation.to_dict()

        self.assertEqual(payload["type"], "pet.observation")
        self.assertEqual(payload["session_id"], "pet-test")
        self.assertEqual(payload["audio"]["rms"], 0.71)
        self.assertEqual(payload["vision"]["expression"], "smile")
        self.assertNotIn("legacy_top_level", payload)
        for key in ("raw_frame_sent", "raw_audio_recorded", "cloud_vision_used", "retention"):
            self.assertIn(key, payload["privacy"])
        self.assertFalse(payload["privacy"]["raw_frame_sent"])
        self.assertFalse(payload["privacy"]["raw_audio_recorded"])
        self.assertFalse(payload["privacy"]["cloud_vision_used"])
        self.assertEqual(payload["privacy"]["retention"], "ephemeral")

    def test_control_event_tolerates_missing_or_invalid_voice_metadata_defaults(self):
        event = PetControlEvent(
            session_id="pet-test",
            seq=1,
            timestamp_ms=123,
            phase="speaking",
            audio={"state": "", "rms": "not-a-number", "sample_rate": None, "duration_ms": None},
            privacy={"retention": "", "raw_audio_recorded": "true"},
            debug={"voice": None},
        )

        payload = event.to_dict()

        self.assertEqual(payload["audio"]["state"], "")
        self.assertEqual(payload["audio"]["rms"], 0.0)
        self.assertEqual(payload["audio"]["sample_rate"], None)
        self.assertEqual(payload["audio"]["duration_ms"], None)
        self.assertIsNone(payload["audio"]["url"])
        self.assertIsNone(payload["audio"]["chunk_ref"])
        self.assertEqual(payload["privacy"]["retention"], "none")
        self.assertFalse(payload["privacy"]["raw_audio_recorded"])
        self.assertIn("voice", payload["debug"])

        default_event = PetControlEvent(
            session_id="pet-test",
            seq=2,
            timestamp_ms=124,
            audio=PetAudio(),
            privacy=PetPrivacy(),
        )
        default_payload = default_event.to_dict()
        self.assertEqual(default_payload["audio"]["state"], "idle")
        self.assertEqual(default_payload["privacy"]["retention"], "none")

    def test_control_event_maps_voice_metadata_into_audio_speech_privacy_and_debug_when_available(self):
        event = PetControlEvent(
            session_id="pet-test",
            seq=3,
            timestamp_ms=125,
            phase="speaking",
            audio=PetAudio(
                state="text-only",
                rms=0.33,
                sample_rate=0,
                duration_ms=0,
                chunk_ref=None,
                url=None,
                phoneme="ni hao",
                viseme="A",
            ),
            privacy=PetPrivacy(retention="none", raw_audio_recorded=False),
            debug={
                "voice": {
                    "provider": "scripted",
                    "voice": "deterministic",
                    "state": "text-only",
                    "retention": "none",
                }
            },
            speech=PetSpeech(text_delta="hello"),
            lip_sync=PetLipSync(value=0.33),
        )

        payload = event.to_dict()

        self.assertEqual(payload["audio"]["state"], "text-only")
        self.assertEqual(payload["audio"]["rms"], 0.33)
        self.assertEqual(payload["audio"]["phoneme"], "ni hao")
        self.assertEqual(payload["audio"]["viseme"], "A")
        self.assertIsNone(payload["audio"]["url"])
        self.assertIsNone(payload["audio"]["chunk_ref"])
        self.assertIsNone(payload["speech"]["audio_url"])
        self.assertIsNone(payload["speech"]["audio_chunk_ref"])
        self.assertEqual(payload["privacy"]["retention"], "none")
        self.assertEqual(payload["debug"]["voice"]["provider"], "scripted")
        self.assertEqual(payload["debug"]["voice"]["voice"], "deterministic")

    def test_voice_provider_protocol_declares_stream_and_interrupt(self):
        from harness.pet.voice import VoiceProvider, VoiceProviderRouter

        provider_members = set(getattr(VoiceProvider, "__annotations__", {}))
        self.assertIn("name", provider_members)
        for method_name in ("synthesize", "stream", "interrupt"):
            self.assertTrue(callable(getattr(VoiceProvider, method_name, None)))
            self.assertTrue(callable(getattr(VoiceProviderRouter, method_name, None)))

        stream_hints = get_type_hints(VoiceProvider.stream)
        interrupt_hints = get_type_hints(VoiceProvider.interrupt)
        self.assertIn("request", stream_hints)
        self.assertIn("return", stream_hints)
        self.assertIn("request", interrupt_hints)
        self.assertIn("return", interrupt_hints)

    def test_observation_from_dict_accepts_non_dict_nested_fields(self):
        observation = PetObservation.from_dict(
            {
                "session_id": "pet-test",
                "audio": "legacy-audio-placeholder",
                "vision": None,
                "privacy": [],
                "unknown": object(),
            }
        )

        payload = observation.to_dict()

        self.assertEqual(payload["audio"], {})
        self.assertEqual(payload["vision"], {})
        self.assertIn("raw_frame_sent", payload["privacy"])
        self.assertIn("raw_audio_recorded", payload["privacy"])
        self.assertFalse(payload["privacy"]["raw_frame_sent"])
        self.assertFalse(payload["privacy"]["raw_audio_recorded"])


if __name__ == "__main__":
    unittest.main()
