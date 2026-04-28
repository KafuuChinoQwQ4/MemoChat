"""
智能功能 API 路由
POST /smart — 摘要 / 建议回复 / 翻译
"""
import structlog
from fastapi import APIRouter, HTTPException

from harness import HarnessContainer
from schemas.api import AgentRunReq, SmartReq, SmartRsp

logger = structlog.get_logger()
router = APIRouter()


FEATURE_TO_SKILL = {
    "summary": "summarize_thread",
    "suggest": "reply_suggester",
    "translate": "translate_text",
}


@router.post("", response_model=SmartRsp)
async def smart(req: SmartReq):
    if not req.content.strip():
        raise HTTPException(status_code=400, detail="content cannot be empty")

    feature_type = req.feature_type.lower().strip()
    if feature_type not in FEATURE_TO_SKILL:
        raise HTTPException(status_code=400, detail=f"Unknown feature_type: {req.feature_type}")

    try:
        container = HarnessContainer.get_instance()
        result = await container.agent_service.run_turn(
            AgentRunReq(
                uid=req.uid,
                content=req.content,
                skill_name=FEATURE_TO_SKILL[feature_type],
                feature_type=feature_type,
                target_lang=req.target_lang,
            )
        )
        return SmartRsp(
            code=0,
            message="ok",
            result=result.content,
            content=result.content,
            trace_id=result.trace_id,
            skill=result.skill,
        )
    except Exception as exc:
        logger.error("smart.error", feature_type=feature_type, error=str(exc))
        raise HTTPException(status_code=500, detail=str(exc)) from exc
