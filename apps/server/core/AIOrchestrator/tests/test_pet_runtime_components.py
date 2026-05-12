from __future__ import annotations

import asyncio
import importlib
import inspect
import sys
import unittest
from pathlib import Path

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
        self.assertIn("hello", _response_text(first))

    async def test_voice_router_deterministic_synthesis_is_text_only(self):
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
        self.assertTrue(str(payload["chunk_ref"]).startswith("deterministic-text-"))
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


if __name__ == "__main__":
    unittest.main()
