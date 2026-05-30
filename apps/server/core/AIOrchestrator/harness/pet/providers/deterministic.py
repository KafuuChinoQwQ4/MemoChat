from __future__ import annotations

import asyncio
import json
import re

from ..persona import PetPromptContext
from ..voice import (
    DeterministicVoiceProvider,
    VoiceProviderError,
    VoiceProviderRouter,
    VoiceSynthesisRequest,
    VoiceSynthesisResult,
    _scripted_rms,
    _scripted_viseme,
)
from .router import PetProviderError, ProviderChunk


class DeterministicPetProvider:
    name = "scripted"

    def __init__(self, voice_provider: DeterministicVoiceProvider | VoiceProviderRouter | None = None) -> None:
        self._voice_provider = voice_provider or DeterministicVoiceProvider()

    async def generate_text(
        self,
        content: str,
        model_type: str = "",
        model_name: str = "",
        language: str = "zh-CN",
    ) -> str:
        style_hint = f"{model_type}:{model_name}".strip(":")
        normalized = content.strip()
        language_key = _reply_text_language(language)
        reply = ""
        if language_key == "ja":
            if any(token in normalized for token in ("整理", "思路", "计划")):
                reply = "もちろんです。要点をまとめて、一つずつ進めましょう。"
            elif any(token in normalized for token in ("你好", "嗨", "hello", "Hello", "こんにちは")):
                reply = "こんにちは、ここにいます。"
            elif normalized.endswith("?") or normalized.endswith("？") or normalized.endswith(("吗", "么", "嘛", "か")):
                reply = "質問はわかりました。まず大事なところから答えます。"
        elif language_key == "en":
            if any(token in normalized for token in ("整理", "思路", "计划", "plan")):
                reply = "Sure. I will gather the key points first, then go step by step."
            elif any(token in normalized for token in ("你好", "嗨", "hello", "Hello")):
                reply = "Hello, I am here."
            elif normalized.endswith("?") or normalized.endswith("？") or normalized.endswith(("吗", "么", "嘛")):
                reply = "I understand your question. I will start with the most important part."
        else:
            if any(token in normalized for token in ("整理", "思路", "计划")):
                reply = "可以，我先陪你把重点收拢，再一步一步拆开。"
            elif any(token in normalized for token in ("你好", "嗨", "hello", "Hello")):
                reply = "你好，我在这里。"
            elif normalized.endswith("?") or normalized.endswith("？") or normalized.endswith(("吗", "么", "嘛")):
                reply = "我明白你的问题了，先从最关键的地方开始回答。"
        if reply and style_hint:
            reply = f"{reply}（{style_hint} scripted）"
        return reply

    async def generate(self, prompt: PetPromptContext) -> list[ProviderChunk]:
        text = prompt.user_text.strip()
        if text.lower() in {"provider:error", "__pet_provider_error__"}:
            raise PetProviderError("deterministic provider error requested", provider=self.name)

        reply, translation, language = await self._generate_reply(prompt)
        if not reply.strip():
            return []

        voice_provider_name = _voice_provider_name(prompt)
        metadata = _voice_metadata(prompt)
        if language:
            metadata["language"] = language
            metadata["text_lang"] = _reply_text_language(language)
        if translation:
            metadata["translation"] = translation
        voice = None
        if voice_provider_name in {"scripted", "deterministic"}:
            voice = await self._synthesize_voice(
                prompt,
                text=reply,
                language=language or _voice_language(prompt),
                provider_name=voice_provider_name,
                turn_id="deterministic-full",
                metadata=metadata,
            )
        else:
            metadata["voice_deferred"] = True
            metadata["voice_provider"] = voice_provider_name
            metadata["voice_name"] = _voice_name(prompt)
            metadata["voice_language"] = language or _voice_language(prompt)
        return [
            ProviderChunk(
                text=reply,
                emotion="cheerful",
                intensity=0.66,
                final=True,
                voice=voice,
                metadata={**metadata, "translation": translation, "language": language},
            )
        ]

    async def _synthesize_voice(
        self,
        prompt: PetPromptContext,
        text: str,
        language: str,
        provider_name: str,
        turn_id: str,
        metadata: dict,
    ) -> VoiceSynthesisResult:
        try:
            return await self._voice_provider.synthesize(
                VoiceSynthesisRequest(
                    session_id=prompt.session_id,
                    turn_id=turn_id,
                    text=text,
                    provider=provider_name,
                    voice=_voice_name(prompt),
                    language=language,
                    metadata=metadata,
                )
            )
        except VoiceProviderError as exc:
            fallback_metadata = dict(metadata)
            fallback_metadata.update(
                {
                    "audio_error": str(exc),
                    "audio_error_provider": exc.provider or provider_name,
                    "audio_error_recoverable": bool(exc.recoverable),
                }
            )
            return VoiceSynthesisResult(
                text=text,
                state="error",
                sample_rate=0,
                duration_ms=_estimated_voice_duration_ms(text) if text else 0,
                rms=_scripted_rms(text),
                chunk_ref=None,
                url=None,
                phoneme=None,
                viseme=_scripted_viseme(text),
                provider=exc.provider or provider_name or "scripted",
                voice=_voice_name(prompt),
                retention="none",
                metadata=fallback_metadata,
            )

    async def _generate_reply(self, prompt: PetPromptContext) -> tuple[str, str, str]:
        language = _reply_language(prompt)
        if not _should_use_llm(prompt):
            reply = await self._scripted_reply(prompt, language)
            translation = (
                await self._scripted_translation(prompt, language) if _reply_text_language(language) == "ja" else ""
            )
            return reply, translation, language
        try:
            reply, translation = await asyncio.wait_for(
                self._llm_reply(prompt, language),
                timeout=_text_generation_timeout_sec(prompt),
            )
            if reply.strip():
                return reply, translation, language
        except Exception:
            pass
        reply = await self._scripted_reply(prompt, language)
        translation = (
            await self._scripted_translation(prompt, language) if _reply_text_language(language) == "ja" else ""
        )
        return reply, translation, language

    async def _scripted_reply(self, prompt: PetPromptContext, language: str) -> str:
        return await self.generate_text(
            prompt.user_text,
            model_type=prompt.model_type,
            model_name=prompt.model_name,
            language=language,
        )

    async def _scripted_translation(self, prompt: PetPromptContext, language: str) -> str:
        if _reply_text_language(language) != "ja":
            return ""
        return await self.generate_text(
            prompt.user_text,
            model_type=prompt.model_type,
            model_name=prompt.model_name,
            language="zh-CN",
        )

    async def _llm_reply(self, prompt: PetPromptContext, language: str) -> tuple[str, str]:
        from harness.llm.service import LLMEndpointRegistry
        from llm.base import LLMMessage

        registry = LLMEndpointRegistry()
        target_language = _language_name(language)
        wants_translation = target_language.startswith("Japanese") or language.lower().startswith("ja")
        system_prompt = (
            "You are MemoChat's Live2D companion. Reply directly to the user. "
            f"Write the main reply in {target_language}. "
            "Keep the tone warm, concise, and natural. "
            "Return JSON only with keys text and translation. "
            "For non-Japanese targets, translation may be an empty string. "
            "For Japanese targets, text must be Japanese and translation must be Simplified Chinese. "
            "Do not add markdown fences or extra commentary."
        )
        speech_rules = str(prompt.runtime_metadata.get("speech_rules") or "").strip()
        if speech_rules:
            system_prompt += f" Speech rules: {speech_rules}"
        if prompt.observation_summary:
            system_prompt += f" Observation summary: {json.dumps(prompt.observation_summary, ensure_ascii=False)}."
        visual_summary = _visual_summary_text(prompt)
        if visual_summary:
            system_prompt += f" Visual summary: {visual_summary}."
        messages = [
            LLMMessage(role="system", content=system_prompt),
            LLMMessage(role="user", content=prompt.user_text),
        ]
        response = await registry.complete(
            messages,
            prefer_backend=prompt.model_type,
            model_name=prompt.model_name,
            deployment_preference="any",
            temperature=0.6,
            max_tokens=2048,
        )
        payload = _parse_reply_json(response.content)
        reply = str(payload.get("text") or payload.get("reply") or response.content or "").strip()
        translation = str(payload.get("translation") or payload.get("zh") or "").strip()
        if wants_translation and not translation:
            translation = await self._translate_reply(reply or prompt.user_text, language, prompt)
        return reply, translation

    async def _translate_reply(self, text: str, language: str, prompt: PetPromptContext) -> str:
        from harness.llm.service import LLMEndpointRegistry
        from llm.base import LLMMessage

        if not text.strip():
            return ""
        registry = LLMEndpointRegistry()
        messages = [
            LLMMessage(
                role="system",
                content="Translate the user's reply into Simplified Chinese. Return only the translation text.",
            ),
            LLMMessage(role="user", content=text),
        ]
        response = await registry.complete(
            messages,
            prefer_backend=prompt.model_type,
            model_name=prompt.model_name,
            deployment_preference="any",
            temperature=0.2,
            max_tokens=1024,
        )
        return response.content.strip()


def _chunk_text(text: str, size: int) -> list[str]:
    return [text[index : index + size] for index in range(0, len(text), size)] or [""]


def _estimated_voice_duration_ms(text: str) -> int:
    return max(240, min(60000, 90 * len(text))) if text else 0


def _voice_metadata(prompt: PetPromptContext) -> dict:
    metadata = {}
    if isinstance(prompt.runtime_metadata, dict):
        metadata.update(prompt.runtime_metadata)
    return metadata


def _visual_summary_text(prompt: PetPromptContext) -> str:
    metadata = _voice_metadata(prompt)
    visual_summary = metadata.get("visual_summary")
    if not isinstance(visual_summary, dict) or not visual_summary:
        return ""
    return str(
        visual_summary.get("summary_text")
        or visual_summary.get("cached_summary_text")
        or visual_summary.get("speech_text")
        or ""
    ).strip()


def _voice_provider_name(prompt: PetPromptContext) -> str:
    metadata = _voice_metadata(prompt)
    return str(metadata.get("voice_provider") or "scripted")


def _voice_name(prompt: PetPromptContext) -> str:
    metadata = _voice_metadata(prompt)
    return str(metadata.get("voice_name") or "deterministic")


def _voice_language(prompt: PetPromptContext) -> str:
    metadata = _voice_metadata(prompt)
    return str(metadata.get("voice_language") or "zh-CN")


def _should_use_llm(prompt: PetPromptContext) -> bool:
    normalized = f"{prompt.model_type}:{prompt.model_name}".strip(":").lower()
    if not normalized:
        return False
    return not (normalized.startswith("scripted") or normalized.startswith("deterministic"))


def _text_generation_timeout_sec(prompt: PetPromptContext) -> float:
    metadata = _voice_metadata(prompt)
    try:
        value = float(metadata.get("text_timeout_sec") or metadata.get("pet_text_timeout_sec") or 25.0)
    except (TypeError, ValueError):
        value = 25.0
    return max(0.001, value)


def _reply_language(prompt: PetPromptContext) -> str:
    metadata = _voice_metadata(prompt)
    language = str(
        metadata.get("reply_language")
        or metadata.get("language")
        or metadata.get("voice_language")
        or metadata.get("text_lang")
        or ""
    ).strip()
    if not language:
        return "zh-CN"
    if language.lower().startswith("ja"):
        return "ja-JP"
    if language.lower().startswith("zh"):
        return "zh-CN"
    return language


def _reply_text_language(language: str) -> str:
    normalized = str(language or "").lower()
    if normalized.startswith("ja"):
        return "ja"
    if normalized.startswith("zh"):
        return "zh"
    if normalized.startswith("en"):
        return "en"
    return normalized or "zh"


def _language_name(language: str) -> str:
    normalized = str(language or "").lower()
    if normalized.startswith("ja"):
        return "Japanese"
    if normalized.startswith("zh"):
        return "Simplified Chinese"
    if normalized.startswith("en"):
        return "English"
    if normalized.startswith("ko"):
        return "Korean"
    if normalized.startswith("fr"):
        return "French"
    if normalized.startswith("es"):
        return "Spanish"
    return language or "Simplified Chinese"


def _parse_reply_json(content: str) -> dict:
    text = content.strip()
    if not text:
        return {}
    if text.startswith("```"):
        text = re.sub(r"^```(?:json)?\s*", "", text, flags=re.I)
        text = re.sub(r"\s*```$", "", text)
    try:
        parsed = json.loads(text)
        return parsed if isinstance(parsed, dict) else {}
    except json.JSONDecodeError:
        pass
    match = re.search(r"\{.*\}", text, flags=re.S)
    if match:
        try:
            parsed = json.loads(match.group(0))
            return parsed if isinstance(parsed, dict) else {}
        except json.JSONDecodeError:
            return {}
    return {}


DeterministicProvider = DeterministicPetProvider
