"""
智能功能 API 路由
POST /smart — 摘要 / 建议回复 / 翻译
"""
import json
import structlog
from fastapi import APIRouter, HTTPException

from schemas.api import SmartReq, SmartRsp
from llm import LLMManager, LLMMessage

logger = structlog.get_logger()
router = APIRouter()

SUMMARY_PROMPT = """你是一个聊天摘要助手。用户给你一段对话记录，请用50字以内概括核心要点。

对话记录:
---
{chat_history}
---

摘要（50字以内）:"""

SUGGEST_PROMPT = """你是一个智能回复建议助手。根据以下对话上下文，生成3条合适的回复建议。
每条建议不超过20字，以 JSON 数组格式返回。

对话记录:
---
{chat_history}
---

回复建议（JSON数组）:"""

TRANSLATE_PROMPT = """你是一个翻译助手。请将以下内容翻译成{target_lang}，只返回翻译结果，无需解释。

原文:
{content}

翻译:"""


async def _do_summary(content: str) -> str:
    manager = LLMManager.get_instance()
    messages = [LLMMessage(role="user", content=SUMMARY_PROMPT.format(chat_history=content))]
    resp = await manager.achat(messages, prefer_backend="ollama")
    return resp.content.strip()


async def _do_suggest(content: str) -> str:
    manager = LLMManager.get_instance()
    messages = [LLMMessage(role="user", content=SUGGEST_PROMPT.format(chat_history=content))]
    resp = await manager.achat(messages, prefer_backend="ollama")
    return resp.content.strip()


async def _do_translate(content: str, target_lang: str) -> str:
    manager = LLMManager.get_instance()
    messages = [LLMMessage(role="user", content=TRANSLATE_PROMPT.format(
        target_lang=target_lang or "中文", content=content))]
    resp = await manager.achat(messages, prefer_backend="ollama")
    return resp.content.strip()


@router.post("", response_model=SmartRsp)
async def smart(req: SmartReq):
    """
    智能功能处理：summary / suggest / translate
    """
    if not req.content.strip():
        raise HTTPException(status_code=400, detail="content cannot be empty")

    feature_type = req.feature_type.lower()
    if feature_type not in ("summary", "suggest", "translate"):
        raise HTTPException(status_code=400, detail=f"Unknown feature_type: {req.feature_type}")

    try:
        if feature_type == "summary":
            result = await _do_summary(req.content)
        elif feature_type == "suggest":
            result = await _do_suggest(req.content)
        elif feature_type == "translate":
            result = await _do_translate(req.content, req.target_lang)

        return SmartRsp(code=0, message="ok", result=result)

    except Exception as e:
        logger.error("smart.error", feature_type=feature_type, error=str(e))
        raise HTTPException(status_code=500, detail=str(e))
