from __future__ import annotations

import hashlib
import time

from .animation import AnimationMapper
from .contracts import PetObservation, PetPrivacy, PetSafety, PetSession
from .persona import PetPromptBuilder, PetPromptContext
from .providers import PetProviderError, ProviderChunk


_VISUAL_SUMMARY_CONFIDENCE_THRESHOLD = 0.8
_FIRST_USER_SEEN_CONFIDENCE_THRESHOLD = 0.6
_VISUAL_SUMMARY_COOLDOWN_SEC = 45.0
_VISUAL_EMOTION_DELTA_COOLDOWN_SEC = 5.0
_VISUAL_MAJOR_EMOTIONS = {
    "cheerful",
    "concerned",
    "sad",
    "smile",
    "surprised",
    "surprise",
    "tired",
    "worried",
}
_OBJECT_LABEL_TEXT = {
    "book": {"zh": "书", "ja": "本", "en": "book"},
    "bottle": {"zh": "水杯", "ja": "ボトル", "en": "bottle"},
    "cell phone": {"zh": "手机", "ja": "スマホ", "en": "phone"},
    "computer keyboard": {"zh": "键盘", "ja": "キーボード", "en": "keyboard"},
    "cup": {"zh": "杯子", "ja": "カップ", "en": "cup"},
    "keyboard": {"zh": "键盘", "ja": "キーボード", "en": "keyboard"},
    "laptop": {"zh": "电脑", "ja": "パソコン", "en": "laptop"},
    "monitor": {"zh": "屏幕", "ja": "モニター", "en": "monitor"},
    "mouse": {"zh": "鼠标", "ja": "マウス", "en": "mouse"},
    "person": {"zh": "你", "ja": "あなた", "en": "you"},
    "phone": {"zh": "手机", "ja": "スマホ", "en": "phone"},
}
_FIRST_USER_SEEN_SPEECH_TEXT = "我已经看到你了哦~"
_FIRST_USER_SEEN_SPEECH_TEXT_BY_LANGUAGE = {
    "ja": "ちゃんと見えているよ~",
    "zh": _FIRST_USER_SEEN_SPEECH_TEXT,
    "en": "I can see you now~",
}


class PetPolicy:
    def __init__(
        self,
        prompt_builder: PetPromptBuilder | None = None,
        animation_mapper: AnimationMapper | None = None,
    ) -> None:
        self._prompt_builder = prompt_builder or PetPromptBuilder()
        self._animation_mapper = animation_mapper or AnimationMapper()
        self._visual_summary_state: dict[str, dict] = {}
        self._visual_state_history: dict[str, dict] = {}

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
            "speech_text": "",
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
        return self._animation_mapper.for_voice_chunk(
            chunk.text,
            chunk.emotion,
            chunk.final,
            chunk.voice,
            chunk_metadata=getattr(chunk, "metadata", {}),
        )

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
        vision_payload = _normalize_vision_payload(vision)
        rms = _clamp_float(audio.get("rms"), 0.0, 1.0, 0.0)
        reaction = self._animation_mapper.for_observation_vision(vision_payload, audio_rms=rms)
        visual_summary = self._update_visual_summary(observation.session_id, vision_payload, reaction)
        speak = bool(visual_summary.get("speak"))
        phase = "speaking" if speak else reaction["phase"]
        emotion = visual_summary.get("emotion") if speak else reaction["emotion"]
        intensity = max(reaction["intensity"], float(visual_summary.get("intensity", 0.0))) if speak else reaction["intensity"]
        motion = "talk" if speak else reaction["motion"]
        expression = visual_summary.get("expression") if speak else reaction["expression"]
        lip_sync = max(reaction["lip_sync"], float(visual_summary.get("lip_sync", 0.0))) if speak else reaction["lip_sync"]
        speech_text = str(visual_summary.get("speech_text") or "") if speak else ""
        speech_language = str(visual_summary.get("speech_language") or _vision_reply_language(vision_payload))
        speech_translation = str(visual_summary.get("speech_translation") or "")
        return {
            "phase": phase,
            "emotion": emotion,
            "intensity": intensity,
            "motion": motion,
            "expression": expression,
            "lip_sync": lip_sync,
            "speech_text": speech_text,
            "speech_translation": speech_translation,
            "speech_language": speech_language,
            "text_final": speak,
            "audio_state": "text-only" if speak else None,
            "gaze": reaction["gaze"],
            "safety": PetSafety(
                camera_used=bool(vision.get("enabled", False)),
                vision_detail=str(vision.get("mode") or "landmarks_only"),
            ),
            "vision": vision_payload,
            "privacy": PetPrivacy(
                camera_used=bool(vision.get("enabled", False)),
                mic_used=str(audio.get("vad") or "").lower() not in {"", "idle", "silent"},
                cloud_vision_used=observation.privacy.get("cloud_vision_used") is True,
                raw_frame_sent=observation.privacy.get("raw_frame_sent") is True,
                raw_audio_recorded=observation.privacy.get("raw_audio_recorded") is True,
                retention=str(observation.privacy.get("retention") or "none"),
            ),
            "debug": {"visual_summary": visual_summary},
            "action_name": "visual_react" if speak else "observe",
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

    def visual_summary(self, session_id: str) -> dict:
        return dict(self._visual_summary_state.get(session_id) or {})

    def _update_visual_summary(self, session_id: str, vision: dict, reaction: dict) -> dict:
        now_ms = int(time.time() * 1000)
        state = dict(self._visual_summary_state.get(session_id) or {})
        speech_language = _vision_reply_language(vision)
        state_snapshot = _visual_state_snapshot(vision, reaction, now_ms)
        previous_snapshot = dict(self._visual_state_history.get(session_id) or {})
        state_snapshot = _annotate_state_snapshot(state_snapshot, previous_snapshot, now_ms)
        self._visual_state_history[session_id] = dict(state_snapshot)
        vision_with_state = dict(vision)
        vision_with_state["state_snapshot"] = state_snapshot
        summary = _visual_reaction_summary(session_id, vision_with_state, reaction, state, now_ms, speech_language)
        summary["state_snapshot"] = state_snapshot
        summary["state_hash"] = state_snapshot.get("state_hash", "")
        confidence = _clamp_float(summary.get("confidence"), 0.0, 1.0, 0.0)
        if summary.get("speak"):
            summary["intensity"] = _visual_reaction_intensity(confidence)
            summary["lip_sync"] = _visual_reaction_lip_sync(vision, reaction, confidence)
        else:
            summary["intensity"] = float(reaction.get("intensity", 0.0))
            summary["lip_sync"] = float(reaction.get("lip_sync", 0.0))
        summary["summary_text"] = str(summary.get("summary_text") or "").strip()
        if summary.get("speak") and not summary.get("speech_text"):
            summary["speech_text"] = summary["summary_text"]
        self._visual_summary_state[session_id] = dict(summary)
        return summary


def _clamp_float(value, minimum: float, maximum: float, fallback: float) -> float:
    try:
        number = float(value)
    except (TypeError, ValueError):
        return fallback
    return max(minimum, min(maximum, number))


def _safe_int(value, fallback: int = 0) -> int:
    try:
        return int(value)
    except (TypeError, ValueError):
        return fallback


def _normalize_text(value: str) -> str:
    return " ".join(str(value or "").strip().split())


def _first_object_label(objects) -> str:
    if not isinstance(objects, list):
        return ""
    for item in objects[:3]:
        if not isinstance(item, dict):
            continue
        label = str(item.get("label") or item.get("name") or "").strip()
        if label:
            return label
    return ""


def _localized_object_label(label: str, language: str = "zh-CN") -> str:
    raw = str(label or "").strip()
    if not raw:
        return ""
    normalized = raw.lower()
    language_key = _language_key(language)
    mapped = _OBJECT_LABEL_TEXT.get(normalized)
    if mapped:
        return mapped.get(language_key) or mapped.get("zh") or raw
    return raw


def _language_key(language: str) -> str:
    normalized = _normalize_reply_language(language).lower()
    if normalized.startswith("ja"):
        return "ja"
    if normalized.startswith("en"):
        return "en"
    return "zh"


def _visual_expression_text(expression: str, language: str = "zh-CN") -> str:
    normalized = str(expression or "").strip().lower()
    language_key = _language_key(language)
    if language_key == "ja":
        if normalized in {"smile", "happy", "cheerful"}:
            return "いい表情をしているね。"
        if normalized in {"surprised", "surprise"}:
            return "少し驚いたみたいだね。"
        if normalized in {"sad", "tired"}:
            return "少し疲れているみたい。"
        if normalized in {"concerned", "worried"}:
            return "少し心配そうに見えるよ。"
        if normalized in {"neutral", ""}:
            return "落ち着いているように見えるよ。"
        return "あなたの様子を見ているよ。"
    if language_key == "en":
        if normalized in {"smile", "happy", "cheerful"}:
            return "You look like you are in a good mood."
        if normalized in {"surprised", "surprise"}:
            return "You looked a little surprised."
        if normalized in {"sad", "tired"}:
            return "You look a little tired."
        if normalized in {"concerned", "worried"}:
            return "You look a little worried."
        if normalized in {"neutral", ""}:
            return "You look calm right now."
        return "I am watching your state."
    if normalized in {"smile", "happy", "cheerful"}:
        return "你看起来心情不错。"
    if normalized in {"surprised", "surprise"}:
        return "你刚才有点惊讶。"
    if normalized in {"sad", "tired"}:
        return "你看起来有点累。"
    if normalized in {"concerned", "worried"}:
        return "你看起来有点担心。"
    if normalized in {"neutral", ""}:
        return "你现在看起来挺平静。"
    return "我在观察你的状态。"


def _visual_pose_text(head_pose: dict, language: str = "zh-CN") -> str:
    if not isinstance(head_pose, dict) or not head_pose:
        return ""
    yaw = _clamp_float(head_pose.get("yaw"), -90.0, 90.0, 0.0)
    pitch = _clamp_float(head_pose.get("pitch"), -90.0, 90.0, 0.0)
    language_key = _language_key(language)
    if language_key == "ja":
        if yaw >= 15.0:
            return "少し右を向いているね。"
        if yaw <= -15.0:
            return "少し左を向いているね。"
        if pitch >= 15.0:
            return "こちらを見上げているね。"
        if pitch <= -15.0:
            return "少し下を向いているね。"
        return ""
    if language_key == "en":
        if yaw >= 15.0:
            return "You are turned a little to the right."
        if yaw <= -15.0:
            return "You are turned a little to the left."
        if pitch >= 15.0:
            return "You are looking up at me."
        if pitch <= -15.0:
            return "You are looking down a little."
        return ""
    if yaw >= 15.0:
        return "你现在稍微偏向右边。"
    if yaw <= -15.0:
        return "你现在稍微偏向左边。"
    if pitch >= 15.0:
        return "你抬头看着我。"
    if pitch <= -15.0:
        return "你低头看向下面。"
    return ""


def _visual_scene_text(scene: dict, language: str = "zh-CN") -> str:
    if not isinstance(scene, dict) or not scene:
        return ""
    summary = _normalize_text(str(scene.get("summary") or scene.get("description") or scene.get("caption") or ""))
    if summary:
        return summary
    lighting = str(scene.get("lighting") or "").strip().lower()
    language_key = _language_key(language)
    if language_key == "ja":
        if lighting in {"dim", "dark"}:
            return "少し暗いみたい。"
        if lighting in {"bright", "sunny"}:
            return "明るく見えるね。"
        return ""
    if language_key == "en":
        if lighting in {"dim", "dark"}:
            return "It looks a little dark there."
        if lighting in {"bright", "sunny"}:
            return "It looks bright there."
        return ""
    if lighting in {"dim", "dark"}:
        return "这里光线有点暗。"
    if lighting in {"bright", "sunny"}:
        return "这里看起来很明亮。"
    return ""


def _visual_summary_text(vision: dict, reaction: dict, language: str = "zh-CN") -> str:
    semantic_event = str(_dict_or_empty(vision.get("state_snapshot")).get("semantic_event") or "").strip()
    if semantic_event in {"face_entered", "face_left", "new_object", "attention_changed", "expression_changed", "stable"}:
        summary = _visual_event_line(vision, reaction, language, speech_mode=False)
    else:
        language_key = _language_key(language)
        parts: list[str] = []
        parts.append(_visual_expression_text(str(vision.get("expression") or reaction.get("emotion") or ""), language))
        scene = _dict_or_empty(vision.get("scene"))
        scene_text = _visual_scene_text(scene, language)
        if scene_text:
            parts.append(scene_text)
        pose_text = _visual_pose_text(_dict_or_empty(vision.get("head_pose")), language)
        if pose_text:
            parts.append(pose_text)
        if not scene_text:
            label = _first_object_label(vision.get("objects"))
            if label:
                label = _localized_object_label(label, language)
                if language_key == "ja":
                    parts.append(f"近くに{label}があるのに気づいたよ。")
                elif language_key == "en":
                    parts.append(f"I noticed {label} near you.")
                else:
                    parts.append(f"我注意到你旁边有{label}。")
        summary = " ".join(part for part in parts if part)
    if summary:
        return _normalize_text(summary)
    return _visual_default_line(language)


def _visual_speech_text(
    vision: dict,
    reaction: dict,
    summary_text: str,
    language: str = "zh-CN",
    first_seen: bool = False,
) -> str:
    if first_seen:
        language_key = _language_key(language)
        if language_key == "ja":
            return _normalize_text(f"ちゃんと見えているよ。{_visual_summary_text(vision, reaction, 'zh-CN')}")
        if language_key == "en":
            return _normalize_text(f"I can see you now. {_visual_summary_text(vision, reaction, 'en-US')}")
        return _normalize_text(f"我看到你啦。{_visual_summary_text(vision, reaction, language)}")
    event_line = _visual_event_line(vision, reaction, language, speech_mode=True)
    if event_line:
        return _normalize_text(event_line)
    return _normalize_text(summary_text or _visual_default_line(language))


def _visual_speech_translation(text: str, vision: dict, reaction: dict, summary_text: str, language: str) -> str:
    if _language_key(language) != "ja":
        return ""
    zh_text = _visual_speech_text(
        vision,
        reaction,
        _visual_summary_text(vision, reaction, "zh-CN") or summary_text,
        "zh-CN",
        text.startswith("ちゃんと見えているよ。"),
    )
    return zh_text.strip()


def _visual_default_line(language: str) -> str:
    language_key = _language_key(language)
    if language_key == "ja":
        return "あなたの様子を見ているよ。"
    if language_key == "en":
        return "I am watching your state."
    return "我在观察你的状态。"


def _visual_event_line(vision: dict, reaction: dict, language: str, speech_mode: bool = False) -> str:
    language_key = _language_key(language)
    semantic_event = str(_dict_or_empty(vision.get("state_snapshot")).get("semantic_event") or "").strip()
    expression = str(vision.get("expression") or reaction.get("emotion") or "").strip().lower()
    scene = _dict_or_empty(vision.get("scene"))
    scene_text = _visual_scene_text(scene, language)
    object_label = _localized_object_label(_first_object_label(vision.get("objects")), language)
    attention = str(vision.get("attention") or "").strip().lower()

    if semantic_event == "face_entered":
        if language_key == "ja":
            return "顔を見つけたよ。"
        if language_key == "en":
            return "I found your face."
        return "我看到你了。"
    if semantic_event == "face_left":
        if language_key == "ja":
            return "顔が見えなくなったね。"
        if language_key == "en":
            return "I lost sight of your face."
        return "我暂时看不到你的脸了。"
    if semantic_event == "new_object" and object_label:
        if language_key == "ja":
            return f"近くに{object_label}が入ってきたよ。"
        if language_key == "en":
            return f"I noticed a new {object_label} nearby."
        return f"我注意到旁边新出现了{object_label}。"
    if semantic_event == "attention_changed":
        if attention in {"screen", "looking_at_screen", "face_detected"}:
            if language_key == "ja":
                return "今は画面のほうに意識が向いているね。"
            if language_key == "en":
                return "You are paying attention to the screen now."
            return "你现在在看屏幕。"
        if attention in {"user_face", "face_detected"}:
            if language_key == "ja":
                return "またこちらに意識が戻ってきたね。"
            if language_key == "en":
                return "Your attention came back to me."
            return "你又把注意力转回来了。"
    if semantic_event == "expression_changed":
        if expression in {"smile", "happy", "cheerful"}:
            if language_key == "ja":
                return "表情がやわらかくなったね。"
            if language_key == "en":
                return "Your expression just softened."
            return "你的表情变得柔和了。"
        if expression in {"surprised", "surprise"}:
            if language_key == "ja":
                return "少し驚いた表情になったね。"
            if language_key == "en":
                return "You look a little surprised now."
            return "你现在看起来有点惊讶。"
        if expression in {"sad", "tired", "concerned", "worried"}:
            if language_key == "ja":
                return "少し疲れた表情に見えるよ。"
            if language_key == "en":
                return "You look a little tired or worried."
            return "你看起来有点累或者有点担心。"
    if semantic_event == "stable":
        if scene_text:
            return scene_text
        if language_key == "ja":
            return "そのまま落ち着いているね。"
        if language_key == "en":
            return "Things still look steady."
        return "现在状态还挺稳定。"
    if speech_mode and scene_text:
        return scene_text
    if object_label:
        if language_key == "ja":
            return f"近くに{object_label}も見えているよ。"
        if language_key == "en":
            return f"I can still see {object_label} nearby."
        return f"我还看到你旁边有{object_label}。"
    if language_key == "ja":
        return "あなたの様子を見ているよ。"
    if language_key == "en":
        return "I am watching your state."
    return "我在观察你的状态。"


def _visual_signature(vision: dict, reaction: dict) -> str:
    scene = _dict_or_empty(vision.get("scene"))
    face_present = "1" if bool(vision.get("face_present", False)) else "0"
    expression = _visual_emotion_bucket(str(vision.get("expression") or reaction.get("emotion") or ""))
    emotion = _visual_emotion_bucket(str(reaction.get("emotion") or vision.get("expression") or ""))
    scene_summary = _normalize_text(str(scene.get("summary") or scene.get("description") or scene.get("caption") or ""))
    lighting = _normalize_text(str(scene.get("lighting") or "")).lower()
    label = _normalize_text(_first_object_label(vision.get("objects"))).lower()
    return "|".join([face_present, expression, emotion, scene_summary, lighting, label])


def _visual_state_snapshot(vision: dict, reaction: dict, now_ms: int) -> dict:
    payload = {
        "face_present": bool(vision.get("face_present", False)),
        "attention": str(vision.get("attention") or ""),
        "expression": str(vision.get("expression") or reaction.get("emotion") or ""),
        "objects": _list_or_empty(vision.get("objects")),
        "scene": _dict_or_empty(vision.get("scene")),
        "head_pose": _dict_or_empty(vision.get("head_pose")),
        "blendshapes": _dict_or_empty(vision.get("blendshapes")),
        "confidence": round(_clamp_float(vision.get("confidence"), 0.0, 1.0, 0.0), 4),
        "source": str(vision.get("source") or ""),
        "captured_at_ms": _safe_int(vision.get("captured_at_ms"), now_ms),
    }
    payload["state_hash"] = _visual_state_hash(payload)
    return payload


def _annotate_state_snapshot(snapshot: dict, previous_snapshot: dict, now_ms: int) -> dict:
    payload = dict(snapshot)
    changed_fields: list[str] = []
    for key in ("face_present", "attention", "expression", "objects", "scene", "head_pose", "blendshapes", "confidence"):
        if _normalize_snapshot_value(previous_snapshot.get(key)) != _normalize_snapshot_value(payload.get(key)):
            changed_fields.append(key)
    payload["changed_fields"] = changed_fields
    previous_at = _safe_int(previous_snapshot.get("captured_at_ms"), 0)
    payload["stable_for_ms"] = max(0, now_ms - previous_at) if previous_at > 0 and not changed_fields else 0
    payload["semantic_event"] = _visual_semantic_event(previous_snapshot, payload, changed_fields)
    payload["state_hash"] = _visual_state_hash(payload)
    return payload


def _normalize_snapshot_value(value) -> str:
    if isinstance(value, (dict, list)):
        return str(value)
    if value is True:
        return "true"
    if value is False:
        return "false"
    return str(value or "")


def _visual_state_hash(snapshot: dict) -> str:
    digest_source = "|".join(
        [
            "1" if snapshot.get("face_present") else "0",
            str(snapshot.get("attention") or ""),
            str(snapshot.get("expression") or ""),
            str(snapshot.get("objects") or []),
            str(snapshot.get("scene") or {}),
            str(snapshot.get("head_pose") or {}),
            str(snapshot.get("blendshapes") or {}),
        ]
    )
    return hashlib.sha1(digest_source.encode("utf-8")).hexdigest()[:16]


def _visual_semantic_event(previous_snapshot: dict, current_snapshot: dict, changed_fields: list[str]) -> str:
    if not previous_snapshot:
        return "first_snapshot"
    if not changed_fields:
        return "stable"
    if "face_present" in changed_fields:
        return "face_entered" if current_snapshot.get("face_present") else "face_left"
    if "attention" in changed_fields:
        return "attention_changed"
    if "expression" in changed_fields or "blendshapes" in changed_fields:
        return "expression_changed"
    if "objects" in changed_fields:
        return "new_object"
    return "state_changed"


def _visual_emotion_bucket(value: str) -> str:
    normalized = _normalize_text(str(value or "")).lower()
    if normalized in {"happy", "smile", "smiling"}:
        return "cheerful"
    if normalized in {"surprised", "surprise"}:
        return "surprised"
    if normalized in {"concerned", "worried"}:
        return "concerned"
    if normalized in {"sad", "tired"}:
        return normalized
    if normalized in {"neutral", "focus", "attentive", "calm", ""}:
        return "neutral"
    return normalized


def _is_special_emotion_change(previous_signature: str, signature: str) -> bool:
    if not previous_signature or not signature or previous_signature == signature:
        return False
    previous_parts = previous_signature.split("|")
    current_parts = signature.split("|")
    if len(previous_parts) < 3 or len(current_parts) < 3:
        return True
    previous_expression = previous_parts[1]
    previous_emotion = previous_parts[2]
    current_expression = current_parts[1]
    current_emotion = current_parts[2]
    if previous_expression == current_expression and previous_emotion == current_emotion:
        return False
    return current_expression in _VISUAL_MAJOR_EMOTIONS or current_emotion in _VISUAL_MAJOR_EMOTIONS


def _is_high_confidence(vision: dict) -> bool:
    confidence = _clamp_float(vision.get("confidence"), 0.0, 1.0, 0.0)
    if confidence >= _VISUAL_SUMMARY_CONFIDENCE_THRESHOLD:
        return True
    pose = _dict_or_empty(vision.get("pose"))
    face_confidence = _clamp_float(pose.get("face_confidence"), 0.0, 1.0, 0.0)
    return face_confidence >= _VISUAL_SUMMARY_CONFIDENCE_THRESHOLD


def _is_detected_user(vision: dict) -> bool:
    confidence = _clamp_float(vision.get("confidence"), 0.0, 1.0, 0.0)
    pose = _dict_or_empty(vision.get("pose"))
    face_confidence = _clamp_float(pose.get("face_confidence"), 0.0, 1.0, 0.0)
    return max(confidence, face_confidence) >= _FIRST_USER_SEEN_CONFIDENCE_THRESHOLD


def _visual_reaction_intensity(confidence: float) -> float:
    return max(0.42, min(0.95, 0.48 + confidence * 0.42))


def _visual_reaction_lip_sync(vision: dict, reaction: dict, confidence: float) -> float:
    jaw_open = _clamp_float(_dict_or_empty(vision.get("blendshapes")).get("jawOpen"), 0.0, 1.0, 0.0)
    baseline = max(float(reaction.get("lip_sync", 0.0)), jaw_open, 0.4 + confidence * 0.3)
    return max(0.42, min(0.95, baseline))


def _visual_summary_state(state: dict, should_speak: bool, summary_text: str, speech_text: str, now_ms: int) -> dict:
    payload = dict(state)
    payload["updated"] = should_speak
    payload["speak"] = should_speak
    payload["summary_text"] = summary_text if should_speak else ""
    payload["speech_text"] = speech_text if should_speak else ""
    payload["speech_translation"] = str(payload.get("speech_translation") or "") if should_speak else ""
    payload["updated_at_ms"] = now_ms if should_speak else _safe_int(state.get("updated_at_ms"), 0)
    payload["reason"] = state.get("reason") or "cached"
    return payload


def _visual_reaction_summary(
    session_id: str,
    vision: dict,
    reaction: dict,
    state: dict,
    now_ms: int,
    speech_language: str = "zh-CN",
) -> dict:
    face_present = bool(vision.get("face_present", False))
    confidence = _clamp_float(vision.get("confidence"), 0.0, 1.0, 0.0)
    if not face_present:
        return _visual_summary_state(
            state,
            False,
            "",
            "",
            now_ms,
        ) | {
            "session_id": session_id,
            "face_present": False,
            "confidence": confidence,
            "emotion": "neutral",
            "expression": "neutral",
            "motion": "idle",
            "signature": _visual_signature(vision, reaction),
            "cooldown_sec": _VISUAL_SUMMARY_COOLDOWN_SEC,
            "updated": False,
            "speak": False,
            "reason": "no_face",
        }

    previous_face_seen = bool(state.get("ever_seen_face", False))
    if not previous_face_seen and _is_detected_user(vision):
        signature = _visual_signature(vision, reaction)
        summary_text = _visual_summary_text(vision, reaction, speech_language)
        speech_text = _visual_speech_text(
            vision,
            reaction,
            summary_text,
            speech_language,
            first_seen=True,
        )
        return {
            "session_id": session_id,
            "face_present": True,
            "ever_seen_face": True,
            "confidence": confidence,
            "emotion": str(reaction.get("emotion") or "cheerful"),
            "expression": str(reaction.get("expression") or "smile_soft"),
            "motion": "talk",
            "summary_text": summary_text,
            "speech_text": speech_text,
            "speech_translation": _visual_speech_translation(
                speech_text,
                vision,
                reaction,
                summary_text,
                speech_language,
            ),
            "speech_language": speech_language,
            "cached_summary_text": summary_text,
            "signature": signature,
            "cooldown_sec": _VISUAL_SUMMARY_COOLDOWN_SEC,
            "reason": "first_user_seen",
            "updated": True,
            "speak": True,
            "updated_at_ms": now_ms,
            "spoken_signatures": [signature],
        }

    if not _is_high_confidence(vision):
        return _visual_summary_state(
            state,
            False,
            "",
            "",
            now_ms,
        ) | {
            "session_id": session_id,
            "face_present": True,
            "ever_seen_face": previous_face_seen,
            "speech_language": speech_language,
            "confidence": confidence,
            "emotion": str(reaction.get("emotion") or "neutral"),
            "expression": str(reaction.get("expression") or "neutral"),
            "motion": str(reaction.get("motion") or "idle"),
            "signature": _visual_signature(vision, reaction),
            "cooldown_sec": _VISUAL_SUMMARY_COOLDOWN_SEC,
            "updated": False,
            "speak": False,
            "reason": "low_confidence",
        }

    signature = _visual_signature(vision, reaction)
    previous_signature = str(state.get("signature") or "")
    previous_at = _safe_int(state.get("updated_at_ms"), 0)
    cooldown_ms = int(_VISUAL_SUMMARY_COOLDOWN_SEC * 1000)
    emotion_delta_cooldown_ms = int(_VISUAL_EMOTION_DELTA_COOLDOWN_SEC * 1000)
    cooldown_elapsed = previous_at <= 0 or now_ms - previous_at >= cooldown_ms
    emotion_delta_elapsed = previous_at <= 0 or now_ms - previous_at >= emotion_delta_cooldown_ms
    changed = signature != previous_signature
    spoken_signatures = _list_or_empty(state.get("spoken_signatures"))
    already_spoken = signature in {str(item) for item in spoken_signatures}
    special_emotion_change = changed and _is_special_emotion_change(previous_signature, signature)
    should_speak = changed and not already_spoken and (cooldown_elapsed or (special_emotion_change and emotion_delta_elapsed))
    summary_text = _visual_summary_text(vision, reaction, speech_language)
    speech_text = _visual_speech_text(vision, reaction, summary_text, speech_language) if should_speak else ""
    result = {
        "session_id": session_id,
        "face_present": True,
        "ever_seen_face": previous_face_seen,
        "speech_language": speech_language,
        "confidence": confidence,
        "emotion": str(reaction.get("emotion") or "neutral"),
        "expression": str(reaction.get("expression") or "neutral"),
        "motion": str(reaction.get("motion") or "idle"),
        "summary_text": summary_text,
        "speech_text": speech_text,
        "speech_translation": _visual_speech_translation(
            speech_text,
            vision,
            reaction,
            summary_text,
            speech_language,
        ) if should_speak else "",
        "signature": signature,
        "cooldown_sec": _VISUAL_SUMMARY_COOLDOWN_SEC,
        "reason": _visual_summary_skip_reason(
            should_speak,
            changed,
            already_spoken,
            cooldown_elapsed or (special_emotion_change and emotion_delta_elapsed),
        ),
        "significant_emotion_change": special_emotion_change,
        "updated": should_speak,
        "speak": should_speak,
        "updated_at_ms": now_ms if should_speak else previous_at,
    }
    if should_speak:
        state = dict(result)
        state["cached_summary_text"] = summary_text
        state["spoken_signatures"] = (spoken_signatures + [signature])[-24:]
        return state
    if state:
        result["cached_summary_text"] = str(state.get("cached_summary_text") or state.get("summary_text") or "")
        result["spoken_signatures"] = spoken_signatures
    return result


def _visual_summary_skip_reason(
    should_speak: bool,
    changed: bool,
    already_spoken: bool,
    timing_allows_speech: bool,
) -> str:
    if should_speak:
        return "updated"
    if not changed:
        return "unchanged"
    if not timing_allows_speech:
        return "cooldown"
    if already_spoken:
        return "already_spoken"
    return "filtered_change"


def _vision_reply_language(vision: dict) -> str:
    for key in ("reply_language", "language", "voice_language"):
        value = str(vision.get(key) or "").strip()
        if value:
            return _normalize_reply_language(value)
    text_lang = str(vision.get("text_lang") or "").strip()
    if text_lang:
        return _normalize_reply_language(text_lang)
    return "zh-CN"


def _normalize_reply_language(language: str) -> str:
    normalized = str(language or "").strip().lower().replace("_", "-")
    if normalized.startswith("ja") or normalized == "jp":
        return "ja-JP"
    if normalized.startswith("zh"):
        return "zh-CN"
    if normalized.startswith("en"):
        return "en-US"
    return str(language or "").strip() or "zh-CN"


def _dict_or_empty(value):
    return value if isinstance(value, dict) else {}


def _list_or_empty(value):
    if isinstance(value, list):
        return list(value)
    if isinstance(value, tuple):
        return list(value)
    return []


def _normalize_vision_payload(vision: dict) -> dict:
    payload = dict(vision or {})
    enabled = bool(payload.get("enabled", False))
    face_present = bool(payload.get("face_present", False)) if enabled else False
    payload["enabled"] = enabled
    payload["mode"] = str(payload.get("mode") or ("landmarks_only" if enabled else "none"))
    payload["face_present"] = face_present
    payload["attention"] = str(payload.get("attention") or "")
    payload["expression"] = str(payload.get("expression") or "") if face_present else ""
    payload["confidence"] = _clamp_float(payload.get("confidence"), 0.0, 1.0, 0.0) if enabled else 0.0
    payload["pose"] = _dict_or_empty(payload.get("pose"))
    payload["head_pose"] = _dict_or_empty(payload.get("head_pose"))
    payload["blendshapes"] = _dict_or_empty(payload.get("blendshapes"))
    payload["scene"] = _dict_or_empty(payload.get("scene"))
    payload["objects"] = _list_or_empty(payload.get("objects"))
    payload["gesture"] = str(payload.get("gesture") or "")
    payload["source"] = str(payload.get("source") or "")
    payload["frame"] = _dict_or_empty(payload.get("frame"))
    payload["client_frame"] = _dict_or_empty(payload.get("client_frame"))
    payload["frame_mime"] = str(payload.get("frame_mime") or "")
    payload["captured_at_ms"] = _safe_int(payload.get("captured_at_ms"))
    return payload
