from __future__ import annotations

from .contracts import PetGaze


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

    def for_observation_vision(self, vision: dict, audio_rms: float = 0.0) -> dict:
        payload = dict(vision or {})
        face_present = bool(payload.get("face_present", False))
        confidence = _vision_confidence(payload)
        pose = _dict_or_empty(payload.get("pose"))
        head_pose = _dict_or_empty(payload.get("head_pose"))
        blendshapes = _dict_or_empty(payload.get("blendshapes"))
        scene = _dict_or_empty(payload.get("scene"))

        raw_expression = str(payload.get("expression") or "neutral")
        expression = self.for_observation_expression(raw_expression, face_present)
        emotion = _emotion_from_vision(raw_expression, blendshapes, face_present, confidence)
        gaze = _gaze_from_vision(face_present, head_pose, pose, confidence)

        jaw_open = _clamp_float(blendshapes.get("jawOpen"), 0.0, 1.0, 0.0)
        base_lip_sync = _clamp_float(audio_rms, 0.0, 1.0, 0.0)
        vision_lip_sync = jaw_open * confidence if face_present else 0.0
        lip_sync = max(base_lip_sync, vision_lip_sync)
        scene_brightness = _clamp_float(scene.get("brightness"), 0.0, 1.0, 0.0)
        intensity = max(0.25, base_lip_sync, 0.25 + confidence * 0.45, 0.25 + scene_brightness * 0.05)
        motion = "talk" if lip_sync >= 0.45 else "observe" if face_present else "idle"

        return {
            "phase": "listening" if face_present else "idle",
            "emotion": emotion,
            "intensity": intensity,
            "motion": motion,
            "expression": expression,
            "lip_sync": lip_sync,
            "gaze": gaze,
            "action_name": "observe",
        }


def _dict_or_empty(value) -> dict:
    return value if isinstance(value, dict) else {}


def _clamp_float(value, minimum: float, maximum: float, fallback: float) -> float:
    try:
        number = float(value)
    except (TypeError, ValueError):
        return fallback
    if number != number:
        return fallback
    return max(minimum, min(maximum, number))


def _vision_confidence(vision: dict) -> float:
    confidence = _clamp_float(vision.get("confidence"), 0.0, 1.0, 0.0)
    if confidence > 0.0:
        return confidence
    pose = _dict_or_empty(vision.get("pose"))
    return _clamp_float(pose.get("face_confidence"), 0.0, 1.0, 0.0)


def _gaze_from_vision(face_present: bool, head_pose: dict, pose: dict, confidence: float) -> PetGaze:
    target = "user_face" if face_present else "screen"
    if face_present and head_pose:
        yaw = _clamp_float(head_pose.get("yaw"), -45.0, 45.0, 0.0)
        pitch = _clamp_float(head_pose.get("pitch"), -45.0, 45.0, 0.0)
        x = 0.5 + _clamp_float(yaw / 120.0, -0.25, 0.25, 0.0)
        y = 0.5 - _clamp_float(pitch / 120.0, -0.25, 0.25, 0.0)
    else:
        face_center = _dict_or_empty(pose.get("face_center"))
        x = _clamp_float(face_center.get("x"), 0.15, 0.85, 0.5)
        y = _clamp_float(face_center.get("y"), 0.15, 0.85, 0.5)

    x = 0.5 + (x - 0.5) * confidence
    y = 0.5 + (y - 0.5) * confidence
    return PetGaze(
        target=target,
        x=round(_clamp_float(x, 0.0, 1.0, 0.5), 4),
        y=round(_clamp_float(y, 0.0, 1.0, 0.5), 4),
    )


def _emotion_from_vision(expression: str, blendshapes: dict, face_present: bool, confidence: float) -> str:
    if not face_present or confidence < 0.2:
        return "neutral"

    normalized = expression.strip().lower()
    smile = max(
        _clamp_float(blendshapes.get("mouthSmileLeft"), 0.0, 1.0, 0.0),
        _clamp_float(blendshapes.get("mouthSmileRight"), 0.0, 1.0, 0.0),
        _clamp_float(blendshapes.get("smile"), 0.0, 1.0, 0.0),
    )
    jaw_open = _clamp_float(blendshapes.get("jawOpen"), 0.0, 1.0, 0.0)
    blink = max(
        _clamp_float(blendshapes.get("eyeBlinkLeft"), 0.0, 1.0, 0.0),
        _clamp_float(blendshapes.get("eyeBlinkRight"), 0.0, 1.0, 0.0),
    )
    brow_down = max(
        _clamp_float(blendshapes.get("browDownLeft"), 0.0, 1.0, 0.0),
        _clamp_float(blendshapes.get("browDownRight"), 0.0, 1.0, 0.0),
        _clamp_float(blendshapes.get("browInnerDown"), 0.0, 1.0, 0.0),
    )
    brow_up = max(
        _clamp_float(blendshapes.get("browInnerUp"), 0.0, 1.0, 0.0),
        _clamp_float(blendshapes.get("browOuterUpLeft"), 0.0, 1.0, 0.0),
        _clamp_float(blendshapes.get("browOuterUpRight"), 0.0, 1.0, 0.0),
    )

    if normalized in {"smile", "happy", "cheerful"} or smile >= 0.55:
        return "cheerful"
    if normalized in {"surprised", "surprise"} or (jaw_open >= 0.55 and brow_up >= 0.35):
        return "surprised"
    if normalized in {"sad", "tired"} or brow_down >= 0.45 or blink >= 0.7:
        return "concerned"
    if jaw_open >= 0.45:
        return "cheerful"
    return "focus"
