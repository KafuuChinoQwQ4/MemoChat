from __future__ import annotations

from ..persona import PetPromptContext
from ..voice import DeterministicVoiceProvider, VoiceSynthesisRequest
from .router import PetProviderError, ProviderChunk


class DeterministicPetProvider:
    name = "scripted"

    def __init__(self, voice_provider: DeterministicVoiceProvider | None = None) -> None:
        self._voice_provider = voice_provider or DeterministicVoiceProvider()

    async def generate_text(self, content: str, model_type: str = "", model_name: str = "") -> str:
        style_hint = f"{model_type}:{model_name}".strip(":")
        reply = f"收到：{content.strip()}"
        if style_hint:
            reply = f"{reply}（{style_hint} scripted）"
        return reply

    async def generate(self, prompt: PetPromptContext) -> list[ProviderChunk]:
        text = prompt.user_text.strip()
        if text.lower() in {"provider:error", "__pet_provider_error__"}:
            raise PetProviderError("deterministic provider error requested", provider=self.name)

        reply = await self.generate_text(text, model_type=prompt.model_type, model_name=prompt.model_name)

        chunks = _chunk_text(reply, 8)
        result: list[ProviderChunk] = []
        for index, chunk in enumerate(chunks):
            final = index == len(chunks) - 1
            voice = await self._voice_provider.synthesize(
                VoiceSynthesisRequest(
                    session_id=prompt.session_id,
                    turn_id="deterministic-chunk",
                    text=chunk,
                    provider=self.name,
                    voice="deterministic",
                    language="zh-CN",
                    metadata={"chunk_index": index},
                )
            )
            result.append(ProviderChunk(
                text=chunk,
                emotion="cheerful" if final else "speaking",
                intensity=0.66,
                final=final,
                voice=voice,
            ))
        return result


def _chunk_text(text: str, size: int) -> list[str]:
    return [text[index:index + size] for index in range(0, len(text), size)] or [""]


DeterministicProvider = DeterministicPetProvider
