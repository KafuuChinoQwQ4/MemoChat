from __future__ import annotations

from .animation import AnimationMapper
from .contracts import PetGaze, PetObservation, PetPrivacy, PetSafety, PetSession, PetVision
from .persona import PetPromptBuilder, PetPromptContext
from .providers import PetProviderError, ProviderChunk


class PetPolicy:
    def __init__(
        self,
        prompt_builder: PetPromptBuilder | None = None,
        animation_mapper: AnimationMapper | None = None,
    ) -> None:
        self._prompt_builder = prompt_builder or PetPromptBuilder()
        self._animation_mapper = animation_mapper or AnimationMapper()

    def build_prompt(
        self,
        session: PetSession,
        content: str,
        model_type: str = "",
        model_name: str = "",
        observation: PetObservation | None = None,
    ) -> PetPromptContext:
        return self._prompt_builder.build_user_turn(
            session,
            content,
            model_type=model_type,
            model_name=model_name,
            observation=observation,
        )

    def appear(self) -> dict:
        return {
            "phase": "idle",
            "emotion": "curious",
            "intensity": 0.48,
            "motion": "appear",
            "expression": "smile_soft",
            "speech_text": "我在。Live2D 控制通道已连接。",
            "action_name": "appear",
        }

    def input_started(self) -> dict:
        return {
            "phase": "listening",
            "emotion": "attentive",
            "intensity": 0.52,
            "motion": "listen",
            "expression": "focus",
            "speech_text": "",
            "lip_sync": 0.0,
            "action_name": "listen",
        }

    def provider_chunk(self, chunk: ProviderChunk) -> dict:
        return self._animation_mapper.for_voice_chunk(chunk.text, chunk.emotion, chunk.final, chunk.voice)

    def input_finished(self) -> dict:
        return {
            "phase": "idle",
            "emotion": "neutral",
            "intensity": 0.35,
            "motion": "idle",
            "expression": "neutral",
            "speech_text": "",
            "lip_sync": 0.0,
            "action_name": "idle",
        }

    def provider_error(self, error: PetProviderError) -> dict:
        return {
            "phase": "error",
            "emotion": "concerned",
            "intensity": 0.25,
            "motion": "error",
            "expression": "concerned",
            "speech_text": "桌宠提供者暂不可用，已保持当前会话。",
            "lip_sync": 0.0,
            "action_name": "provider_error",
            "debug": {
                "provider": error.provider,
                "recoverable": error.recoverable,
                "message": str(error),
            },
        }

    def observation(self, observation: PetObservation) -> dict:
        vision = observation.vision
        audio = observation.audio
        expression = str(vision.get("expression") or "neutral")
        face_present = bool(vision.get("face_present", False))
        rms = _clamp_float(audio.get("rms"), 0.0, 1.0, 0.0)
        return {
            "phase": "listening" if face_present else "idle",
            "emotion": expression if face_present else "neutral",
            "intensity": max(0.25, rms),
            "motion": "observe" if face_present else "idle",
            "expression": self._animation_mapper.for_observation_expression(expression, face_present),
            "lip_sync": rms,
            "speech_text": "",
            "gaze": PetGaze(target="user_face" if face_present else "screen", x=0.5, y=0.5),
            "safety": PetSafety(
                camera_used=bool(vision.get("enabled", False)),
                vision_detail=str(vision.get("mode") or "landmarks_only"),
            ),
            "vision": PetVision(
                enabled=bool(vision.get("enabled", False)),
                mode=str(vision.get("mode") or ("landmarks_only" if vision.get("enabled", False) else "none")),
                face_present=face_present,
                attention=str(vision.get("attention") or ""),
                expression=expression if face_present else "",
                pose=vision.get("pose") if isinstance(vision.get("pose"), dict) else {},
                gesture=str(vision.get("gesture") or ""),
            ),
            "privacy": PetPrivacy(
                camera_used=bool(vision.get("enabled", False)),
                mic_used=str(audio.get("vad") or "").lower() not in {"", "idle", "silent"},
                cloud_vision_used=observation.privacy.get("cloud_vision_used") is True,
                raw_frame_sent=observation.privacy.get("raw_frame_sent") is True,
                raw_audio_recorded=observation.privacy.get("raw_audio_recorded") is True,
                retention=str(observation.privacy.get("retention") or "none"),
            ),
            "action_name": "observe",
        }

    def interrupted(self) -> dict:
        return {
            "phase": "interrupted",
            "emotion": "neutral",
            "intensity": 0.2,
            "motion": "stop",
            "expression": "neutral",
            "lip_sync": 0.0,
            "speech_text": "",
            "action_name": "interrupt",
        }


def _clamp_float(value, minimum: float, maximum: float, fallback: float) -> float:
    try:
        number = float(value)
    except (TypeError, ValueError):
        return fallback
    return max(minimum, min(maximum, number))
