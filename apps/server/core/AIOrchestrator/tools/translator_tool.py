"""
翻译工具 — 使用 LLM 进行翻译
"""
import structlog
from langchain_core.tools import tool
from llm import LLMManager, LLMMessage

logger = structlog.get_logger()

TRANSLATION_PROMPT = """你是一个专业的翻译助手。请将以下文本翻译成{target_lang}，只返回翻译结果，无需解释。

原文:
{text}

翻译:"""


class TranslatorTool:
    """翻译工具 — 基于 LLM 的多语言翻译"""

    async def _translate(self, text: str, target_lang: str = "zh") -> str:
        """
        翻译文本到目标语言。
        输入源文本和目标语言，返回翻译结果。
        """
        if not text.strip():
            return "待翻译文本为空。"

        lang_map = {
            "en": "English", "zh": "中文", "ja": "日语",
            "ko": "韩语", "fr": "法语", "de": "德语",
            "es": "西班牙语", "ru": "俄语", "auto": "自动",
        }

        lang_name = lang_map.get(target_lang.lower(), target_lang)

        try:
            manager = LLMManager.get_instance()
            prompt = TRANSLATION_PROMPT.format(target_lang=lang_name, text=text)
            resp = await manager.achat(
                [LLMMessage(role="user", content=prompt)],
                prefer_backend="ollama",
            )
            return resp.content.strip()

        except Exception as e:
            logger.error("translator.error", target_lang=target_lang, error=str(e))
            return f"翻译失败: {str(e)}"

    def get_tool(self):
        @tool("translator")
        async def translator(text: str, target_lang: str = "zh") -> str:
            """
            翻译文本到目标语言。
            输入源文本和目标语言，返回翻译结果。
            """
            return await self._translate(text, target_lang=target_lang)

        return translator
