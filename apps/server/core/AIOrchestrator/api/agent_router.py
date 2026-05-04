"""
AI Harness 管理 API
"""
from fastapi import APIRouter, HTTPException

from harness import HarnessContainer
from harness.layers import list_harness_layers
from schemas.api import (
    AgentCardModel,
    AgentCardRsp,
    AgentEvalCaseModel,
    AgentEvalListRsp,
    AgentEvalResultModel,
    AgentEvalRunReq,
    AgentEvalRunRsp,
    AgentFlowListRsp,
    AgentFlowModel,
    AgentFlowRsp,
    AgentFlowRunReq,
    AgentFlowRunRsp,
    AgentMessageModel,
    AgentLayerInfo,
    AgentLayerListRsp,
    AgentSpecListRsp,
    AgentSpecModel,
    AgentSpecRsp,
    AgentTaskCreateReq,
    AgentTaskListRsp,
    AgentTaskModel,
    AgentTaskRsp,
    AgentRunGraphRsp,
    AgentRunReq,
    AgentRunRsp,
    AgentTraceRsp,
    FeedbackReq,
    FeedbackRsp,
    MemoryCreateReq,
    MemoryDeleteReq,
    MemoryItemModel,
    MemoryListRsp,
    MemoryMutationRsp,
    RemoteAgentListRsp,
    RemoteAgentModel,
    RemoteAgentRegisterReq,
    RemoteAgentRsp,
    RemoteAgentTaskCreateReq,
    RemoteAgentTaskModel,
    RemoteAgentTaskRsp,
    RunGraphModel,
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


@router.get("/runs/{trace_id}/graph", response_model=AgentRunGraphRsp)
async def get_run_graph(trace_id: str):
    container = HarnessContainer.get_instance()
    graph = await container.trace_store.get_run_graph_or_load(trace_id)
    if graph is None:
        raise HTTPException(status_code=404, detail="trace not found")

    return AgentRunGraphRsp(
        code=0,
        message="ok",
        trace_id=trace_id,
        graph=RunGraphModel(**graph.to_dict()),
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


@router.get("/specs", response_model=AgentSpecListRsp)
async def list_agent_specs():
    container = HarnessContainer.get_instance()
    return AgentSpecListRsp(
        code=0,
        message="ok",
        specs=[AgentSpecModel(**spec.to_dict()) for spec in container.skill_registry.list_specs()],
    )


@router.post("/specs/reload", response_model=AgentSpecListRsp)
async def reload_agent_specs():
    container = HarnessContainer.get_instance()
    container.skill_registry.reload_specs()
    return AgentSpecListRsp(
        code=0,
        message="ok",
        specs=[AgentSpecModel(**spec.to_dict()) for spec in container.skill_registry.list_specs()],
    )


@router.get("/specs/{spec_name}", response_model=AgentSpecRsp)
async def get_agent_spec(spec_name: str):
    container = HarnessContainer.get_instance()
    spec = container.skill_registry.get_spec(spec_name)
    if spec is None:
        raise HTTPException(status_code=404, detail="agent spec not found")
    return AgentSpecRsp(code=0, message="ok", spec=AgentSpecModel(**spec.to_dict()))


@router.get("/card", response_model=AgentCardRsp)
async def get_agent_card():
    container = HarnessContainer.get_instance()
    return AgentCardRsp(
        code=0,
        message="ok",
        card=AgentCardModel(**container.interop_service.local_agent_card().to_dict()),
    )


@router.get("/remote-agents", response_model=RemoteAgentListRsp)
async def list_remote_agents():
    container = HarnessContainer.get_instance()
    return RemoteAgentListRsp(
        code=0,
        message="ok",
        agents=[
            RemoteAgentModel(**agent.to_dict())
            for agent in container.interop_service.list_remote_agents()
        ],
    )


@router.post("/remote-agents", response_model=RemoteAgentRsp)
async def register_remote_agent(req: RemoteAgentRegisterReq):
    if not req.card_url.strip() and not req.card:
        raise HTTPException(status_code=400, detail="card_url or card is required")
    container = HarnessContainer.get_instance()
    agent = container.interop_service.register_remote_agent(
        peer_id=req.peer_id,
        name=req.name,
        card_url=req.card_url,
        base_url=req.base_url,
        trusted=req.trusted,
        card=req.card,
        metadata=req.metadata,
    )
    return RemoteAgentRsp(code=0, message="ok", agent=RemoteAgentModel(**agent.to_dict()))


@router.get("/remote-agents/{peer_id}", response_model=RemoteAgentRsp)
async def get_remote_agent(peer_id: str):
    container = HarnessContainer.get_instance()
    agent = container.interop_service.get_remote_agent(peer_id)
    if agent is None:
        raise HTTPException(status_code=404, detail="remote agent not found")
    return RemoteAgentRsp(code=0, message="ok", agent=RemoteAgentModel(**agent.to_dict()))


@router.delete("/remote-agents/{peer_id}", response_model=RemoteAgentRsp)
async def delete_remote_agent(peer_id: str):
    container = HarnessContainer.get_instance()
    deleted = container.interop_service.delete_remote_agent(peer_id)
    if not deleted:
        raise HTTPException(status_code=404, detail="remote agent not found")
    return RemoteAgentRsp(code=0, message="ok")


@router.post("/remote-agents/{peer_id}/tasks", response_model=RemoteAgentTaskRsp)
async def create_remote_agent_task(peer_id: str, req: RemoteAgentTaskCreateReq):
    container = HarnessContainer.get_instance()
    try:
        task = container.interop_service.create_remote_task_placeholder(
            peer_id=peer_id,
            input_payload=req.input,
            metadata=req.metadata,
        )
    except ValueError as exc:
        raise HTTPException(status_code=404, detail=str(exc)) from exc
    return RemoteAgentTaskRsp(code=0, message="blocked", task=RemoteAgentTaskModel(**task.to_dict()))


@router.get("/flows", response_model=AgentFlowListRsp)
async def list_agent_flows():
    container = HarnessContainer.get_instance()
    return AgentFlowListRsp(
        code=0,
        message="ok",
        flows=[AgentFlowModel(**flow.to_dict()) for flow in container.handoff_service.list_flows()],
    )


@router.post("/flows/run", response_model=AgentFlowRunRsp)
async def run_agent_flow(req: AgentFlowRunReq):
    if not req.content.strip():
        raise HTTPException(status_code=400, detail="content cannot be empty")
    container = HarnessContainer.get_instance()
    try:
        result = await container.handoff_service.run_flow(
            flow_name=req.flow_name,
            uid=req.uid,
            content=req.content,
            session_id=req.session_id,
            model_type=req.model_type,
            model_name=req.model_name,
            metadata=req.metadata,
        )
    except ValueError as exc:
        raise HTTPException(status_code=404, detail=str(exc)) from exc
    return AgentFlowRunRsp(
        code=0,
        message="ok",
        flow_id=result["flow_id"],
        status=result["status"],
        flow=AgentFlowModel(**result["flow"].to_dict()),
        messages=[AgentMessageModel(**message.to_dict()) for message in result["messages"]],
        graph=RunGraphModel(**result["graph"].to_dict()),
    )


@router.get("/flows/{flow_name}", response_model=AgentFlowRsp)
async def get_agent_flow(flow_name: str):
    container = HarnessContainer.get_instance()
    flow = container.handoff_service.get_flow(flow_name)
    if flow is None:
        raise HTTPException(status_code=404, detail="agent flow not found")
    return AgentFlowRsp(code=0, message="ok", flow=AgentFlowModel(**flow.to_dict()))


@router.get("/tools", response_model=ToolListRsp)
async def list_tools():
    container = HarnessContainer.get_instance()
    return ToolListRsp(
        code=0,
        tools=[
            ToolInfo(**tool)
            for tool in container.tool_executor.list_tools()
        ],
    )


@router.get("/memory", response_model=MemoryListRsp)
async def list_memory(uid: int):
    container = HarnessContainer.get_instance()
    memories = await container.memory_service.list_visible_memories(uid)
    return MemoryListRsp(
        code=0,
        message="ok",
        memories=[MemoryItemModel(**memory) for memory in memories],
    )


@router.post("/memory", response_model=MemoryMutationRsp)
async def create_memory(req: MemoryCreateReq):
    if not req.content.strip():
        raise HTTPException(status_code=400, detail="content cannot be empty")
    container = HarnessContainer.get_instance()
    memory = await container.memory_service.create_manual_memory(req.uid, req.content)
    return MemoryMutationRsp(code=0, message="ok", memory=MemoryItemModel(**memory))


@router.delete("/memory/{memory_id}", response_model=MemoryMutationRsp)
async def delete_memory(memory_id: str, uid: int):
    container = HarnessContainer.get_instance()
    deleted = await container.memory_service.delete_visible_memory(uid, memory_id)
    if not deleted:
        raise HTTPException(status_code=404, detail="memory not found")
    return MemoryMutationRsp(code=0, message="ok")


@router.post("/memory/delete", response_model=MemoryMutationRsp)
async def delete_memory_post(req: MemoryDeleteReq):
    container = HarnessContainer.get_instance()
    deleted = await container.memory_service.delete_visible_memory(req.uid, req.memory_id)
    if not deleted:
        raise HTTPException(status_code=404, detail="memory not found")
    return MemoryMutationRsp(code=0, message="ok")


@router.post("/tasks", response_model=AgentTaskRsp)
async def create_task(req: AgentTaskCreateReq):
    if not req.content.strip():
        raise HTTPException(status_code=400, detail="content cannot be empty")
    container = HarnessContainer.get_instance()
    task = await container.task_service.create_task(
        uid=req.uid,
        title=req.title,
        content=req.content,
        session_id=req.session_id,
        model_type=req.model_type,
        model_name=req.model_name,
        skill_name=req.skill_name,
        metadata=req.metadata,
    )
    return AgentTaskRsp(code=0, message="ok", task=AgentTaskModel(**task.to_dict()))


@router.get("/evals", response_model=AgentEvalListRsp)
async def list_evals():
    container = HarnessContainer.get_instance()
    return AgentEvalListRsp(
        code=0,
        message="ok",
        evals=[AgentEvalCaseModel(**case.to_dict()) for case in container.eval_service.list_cases()],
    )


@router.post("/evals/run", response_model=AgentEvalRunRsp)
async def run_evals(req: AgentEvalRunReq):
    container = HarnessContainer.get_instance()
    if req.run_all:
        results = await container.eval_service.run_suite(trace_id=req.trace_id, uid=req.uid)
    else:
        results = [
            await container.eval_service.run_eval(
                case_id=req.case_id,
                trace_id=req.trace_id,
                uid=req.uid,
            )
        ]
    if not results:
        raise HTTPException(status_code=404, detail="no eval cases found")
    return AgentEvalRunRsp(
        code=0,
        message="ok",
        passed=all(result.get("passed", False) for result in results),
        results=[AgentEvalResultModel(**result) for result in results],
    )


@router.get("/tasks", response_model=AgentTaskListRsp)
async def list_tasks(uid: int, limit: int = 50):
    container = HarnessContainer.get_instance()
    tasks = await container.task_service.list_tasks(uid, limit)
    return AgentTaskListRsp(
        code=0,
        message="ok",
        tasks=[AgentTaskModel(**task.to_dict()) for task in tasks],
    )


@router.get("/tasks/{task_id}", response_model=AgentTaskRsp)
async def get_task(task_id: str):
    container = HarnessContainer.get_instance()
    task = await container.task_service.get_task(task_id)
    if task is None:
        raise HTTPException(status_code=404, detail="task not found")
    return AgentTaskRsp(code=0, message="ok", task=AgentTaskModel(**task.to_dict()))


@router.post("/tasks/{task_id}/cancel", response_model=AgentTaskRsp)
async def cancel_task(task_id: str):
    container = HarnessContainer.get_instance()
    task = await container.task_service.cancel_task(task_id)
    if task is None:
        raise HTTPException(status_code=404, detail="task not found")
    return AgentTaskRsp(code=0, message="ok", task=AgentTaskModel(**task.to_dict()))


@router.post("/tasks/{task_id}/resume", response_model=AgentTaskRsp)
async def resume_task(task_id: str):
    container = HarnessContainer.get_instance()
    task = await container.task_service.resume_task(task_id)
    if task is None:
        raise HTTPException(status_code=404, detail="task not found")
    return AgentTaskRsp(code=0, message="ok", task=AgentTaskModel(**task.to_dict()))
