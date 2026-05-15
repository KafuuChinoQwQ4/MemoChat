from __future__ import annotations


class AnimationMapper:
    def map_emotion(self, emotion: str, phase: str = "idle") -> dict:
        normalized = str(emotion or "").strip().lower()
        if normalized in {"provider_error", "error"} or phase == "error":
            return {"expression": "concerned", "motion": "stop", "phase": "error"}
        if normalized in {"happy", "cheerful", "smile"}:
            return {"expression": "smile_soft", "motion": "talk" if phase == "speaking" else "idle", "phase": phase}
        if normalized in {"surprised", "surprise"}:
            return {"expression": "surprised", "motion": "idle", "phase": phase}
        return {"expression": "focus" if phase == "speaking" else "neutral", "motion": "talk" if phase == "speaking" else "idle", "phase": phase}

    def for_provider_chunk(
        self,
        text: str,
        emotion: str,
        final: bool,
        language: str = "zh-CN",
        translation: str = "",
    ) -> dict:
        return {
            "phase": "speaking",
            "emotion": emotion or ("cheerful" if final else "speaking"),
            "intensity": 0.66,
            "motion": "talk",
            "expression": "smile_soft",
            "speech_text": text,
            "speech_language": language or "zh-CN",
            "speech_translation": translation or "",
            "text_final": final,
            "lip_sync": min(1.0, 0.25 + 0.08 * len(text)),
            "action_name": "speak",
        }

    def for_voice_chunk(self, text: str, emotion: str, final: bool, voice) -> dict:
        metadata = {}
        if voice is not None:
            voice_payload = voice.to_dict() if hasattr(voice, "to_dict") else dict(voice)
            metadata = voice_payload.get("metadata") or {}
        language = str(metadata.get("language") or metadata.get("text_language") or "zh-CN")
        translation = str(metadata.get("translation") or metadata.get("zh_translation") or "")
        event = self.for_provider_chunk(text, emotion, final, language=language, translation=translation)
        if voice is None:
            return event

        rms = voice_payload.get("rms", event["lip_sync"])
        event["lip_sync"] = rms
        event["audio_state"] = voice_payload.get("state") or "text-only"
        event["audio_sample_rate"] = voice_payload.get("sample_rate") or 0
        event["audio_duration_ms"] = voice_payload.get("duration_ms") or 0
        event["audio_chunk_ref"] = voice_payload.get("chunk_ref")
        event["audio_url"] = voice_payload.get("url")
        event["audio_phoneme"] = voice_payload.get("phoneme")
        event["audio_viseme"] = voice_payload.get("viseme")
        event["privacy_retention"] = voice_payload.get("retention") or "none"
        event["debug"] = {
            "voice": {
                "provider": voice_payload.get("provider") or "",
                "voice": voice_payload.get("voice") or "",
                "state": voice_payload.get("state") or "text-only",
                "duration_ms": voice_payload.get("duration_ms") or 0,
                "retention": voice_payload.get("retention") or "none",
                "metadata": voice_payload.get("metadata") or {},
            }
        }
        return event

    def for_observation_expression(self, expression: str, face_present: bool) -> str:
        if not face_present:
            return "neutral"
        normalized = expression.strip().lower()
        if normalized in {"smile", "happy", "cheerful"}:
            return "smile_soft"
        if normalized in {"surprised", "surprise"}:
            return "surprised"
        if normalized in {"sad", "tired"}:
            return "concerned"
        return "focus"
