"""
智能功能 API 路由
POST /smart — 摘要 / 建议回复 / 翻译
"""
import json

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

FEATURE_DEFAULT_METADATA = {
    "summary": {"max_tokens": 128, "temperature": 0.2},
    "suggest": {"max_tokens": 160, "temperature": 0.6},
    "translate": {"max_tokens": 512, "temperature": 0.2},
}


@router.post("", response_model=SmartRsp)
async def smart(req: SmartReq):
    if not req.content.strip():
        raise HTTPException(status_code=400, detail="content cannot be empty")

    feature_type = req.feature_type.lower().strip()
    if feature_type not in FEATURE_TO_SKILL:
        raise HTTPException(status_code=400, detail=f"Unknown feature_type: {req.feature_type}")

    try:
        metadata = dict(FEATURE_DEFAULT_METADATA.get(feature_type, {}))
        if req.context_json:
            try:
                parsed = json.loads(req.context_json)
                if isinstance(parsed, dict):
                    metadata.update(parsed)
            except json.JSONDecodeError:
                metadata["context_json"] = req.context_json
        model_type = req.model_type.strip() or str(metadata.get("model_type", "") or "")
        model_name = req.model_name.strip() or str(metadata.get("model_name", "") or "")
        deployment_preference = (
            req.deployment_preference.strip()
            or str(metadata.get("deployment_preference", "any") or "any")
        )
        if model_type:
            metadata["model_type"] = model_type
        if model_name:
            metadata["model_name"] = model_name
        metadata["deployment_preference"] = deployment_preference
        container = HarnessContainer.get_instance()
        result = await container.agent_service.run_turn(
            AgentRunReq(
                uid=req.uid,
                content=req.content,
                model_type=model_type,
                model_name=model_name,
                deployment_preference=deployment_preference,
                skill_name=FEATURE_TO_SKILL[feature_type],
                feature_type=feature_type,
                target_lang=req.target_lang,
                metadata=metadata,
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
