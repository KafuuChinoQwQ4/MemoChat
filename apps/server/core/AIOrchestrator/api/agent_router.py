"""
AI Harness 管理 API
"""
from fastapi import APIRouter, HTTPException

from harness import HarnessContainer
from harness.layers import list_harness_layers
from schemas.api import (
    AgentLayerInfo,
    AgentLayerListRsp,
    AgentRunReq,
    AgentRunRsp,
    AgentTraceRsp,
    FeedbackReq,
    FeedbackRsp,
    SkillInfo,
    SkillListRsp,
    ToolInfo,
    ToolListRsp,
    TraceEventModel,
)

router = APIRouter()


@router.post("/run", response_model=AgentRunRsp)
async def run_agent(req: AgentRunReq):
    if not req.content.strip():
        raise HTTPException(status_code=400, detail="content cannot be empty")

    container = HarnessContainer.get_instance()
    try:
        result = await container.agent_service.run_turn(req)
    except Exception as exc:
        raise HTTPException(
            status_code=503,
            detail=f"agent run failed: {type(exc).__name__}: {str(exc)[:300]}",
        ) from exc
    return AgentRunRsp(
        code=0,
        message="ok",
        session_id=result.session_id,
        content=result.content,
        tokens=result.tokens,
        model=result.model,
        trace_id=result.trace_id,
        skill=result.skill,
        feedback_summary=result.feedback_summary,
        observations=result.observations,
        events=[TraceEventModel(**event.to_dict()) for event in result.events],
    )


@router.get("/runs/{trace_id}", response_model=AgentTraceRsp)
async def get_trace(trace_id: str):
    container = HarnessContainer.get_instance()
    trace = await container.trace_store.get_trace_or_load(trace_id)
    if trace is None:
        raise HTTPException(status_code=404, detail="trace not found")

    return AgentTraceRsp(
        code=0,
        trace_id=trace.trace_id,
        uid=trace.uid,
        session_id=trace.session_id,
        skill=trace.skill,
        status=trace.status,
        request_summary=trace.request_summary,
        response_content=trace.response_content,
        model=trace.model,
        feedback_summary=trace.feedback_summary,
        observations=trace.observations,
        events=[TraceEventModel(**event.to_dict()) for event in trace.events],
    )


@router.get("/layers", response_model=AgentLayerListRsp)
async def list_layers():
    return AgentLayerListRsp(
        code=0,
        layers=[AgentLayerInfo(**layer) for layer in list_harness_layers()],
    )


@router.post("/feedback", response_model=FeedbackRsp)
async def submit_feedback(req: FeedbackReq):
    container = HarnessContainer.get_instance()
    await container.trace_store.submit_feedback(
        trace_id=req.trace_id,
        uid=req.uid,
        rating=req.rating,
        feedback_type=req.feedback_type,
        comment=req.comment,
        payload=req.payload,
    )
    return FeedbackRsp(code=0, message="ok")


@router.get("/skills", response_model=SkillListRsp)
async def list_skills():
    container = HarnessContainer.get_instance()
    return SkillListRsp(
        code=0,
        skills=[
            SkillInfo(
                name=skill.name,
                display_name=skill.display_name,
                description=skill.description,
                allow_web=skill.allow_web,
                allow_knowledge=skill.allow_knowledge,
                allow_graph=skill.allow_graph,
                allow_mcp=skill.allow_mcp,
            )
            for skill in container.skill_registry.list_skills()
        ],
    )


@router.get("/tools", response_model=ToolListRsp)
async def list_tools():
    container = HarnessContainer.get_instance()
    return ToolListRsp(
        code=0,
        tools=[
            ToolInfo(name=tool["name"], description=tool["description"], source=tool["source"])
            for tool in container.tool_executor.list_tools()
        ],
    )
