"""
LangGraph Agent — ReAct 推理模式，支持流式输出
"""
import asyncio
import json
from typing import Annotated, Any, AsyncIterator, Literal, Optional
from dataclasses import dataclass, field

import structlog
from langchain_core.messages import BaseMessage, SystemMessage, HumanMessage, AIMessage
from langchain_core.documents import Document
from langchain_core.tools import tool
from langchain_core.runnables import RunnablePassthrough
from langchain_core.prompts import ChatPromptTemplate
from langgraph.graph import StateGraph, END
from langgraph.prebuilt import ToolNode

from llm import LLMManager, LLMMessage
from llm.base import LLMStreamChunk
from tools.registry import ToolRegistry
from config import settings

logger = structlog.get_logger()


@dataclass
class AgentState:
    """Agent 状态"""
    messages: Annotated[list[BaseMessage], "add_messages"]
    intent: str = "chat"          # chat / kb_query / web_search / calculate / recommend
    retrieved_docs: list[dict] = field(default_factory=list)
    tool_calls: list[str] = field(default_factory=list)
    final_response: str = ""
    pending_confirm: bool = False
    confirm_action: str = ""
    hitl_result: str = ""
    iteration_count: int = 0
    error: str = ""


TOOL_REGISTRY = ToolRegistry()


def build_agent() -> StateGraph:
    """构建 LangGraph Agent 状态机"""
    graph = StateGraph(AgentState)

    graph.add_node("intent_classifier", intent_classifier_node)
    graph.add_node("llm_router", llm_router_node)
    graph.add_node("rag_retriever", rag_retriever_node)
    graph.add_node("tool_executor", ToolNode(TOOL_REGISTRY.get_tools()))
    graph.add_node("response_synthesizer", response_synthesizer_node)

    graph.add_edge("intent_classifier", "llm_router")

    graph.add_conditional_edges(
        "llm_router",
        route_based_on_intent,
        {
            "rag": "rag_retriever",
            "tool": "tool_executor",
            "chat": "response_synthesizer",
        }
    )

    graph.add_edge("rag_retriever", "response_synthesizer")
    graph.add_edge("tool_executor", "intent_classifier")

    graph.add_conditional_edges(
        "intent_classifier",
        stop_if_max_iterations,
        {"stop": END, "continue": "llm_router"}
    )

    graph.add_edge("response_synthesizer", END)

    return graph.compile()


async def intent_classifier_node(state: AgentState) -> dict:
    """意图分类节点"""
    last_message = state["messages"][-1] if state["messages"] else None
    if not last_message:
        return {"intent": "chat"}

    content = last_message.content if hasattr(last_message, "content") else str(last_message)

    keywords = {
        "kb_query": ["知识库", "文档", "上传", "记得", "根据文档", "基于文档", "knowledge base", "document", "上传的"],
        "web_search": ["搜索", "最新", "查询", "互联网", "search", "recent", "news"],
        "calculate": ["计算", "等于", "加减乘除", "数学", "calculate", "compute", "math"],
        "recommend": ["推荐", "建议认识", "可能感兴趣", "recommend", "suggest"],
    }

    for intent, kws in keywords.items():
        if any(kw in content for kw in kws):
            logger.info("agent.intent", intent=intent, content_preview=content[:50])
            return {"intent": intent}

    return {"intent": "chat"}


async def llm_router_node(state: AgentState) -> dict:
    """LLM 路由节点：决定使用工具还是直接回复"""
    manager = LLMManager.get_instance()
    model_type = settings.llm.default_backend

    last_message = state["messages"][-1] if state["messages"] else None
    if not last_message:
        return {"final_response": "我没有收到消息。"}

    system_prompt = (
        "你是一个专业的 AI 助手。你需要判断用户问题是否需要使用工具。\n"
        "可用的工具：duckduckgo_search（搜索互联网）、knowledge_base_search（知识库检索）、calculator（计算）、translator（翻译）。\n"
        "如果需要使用工具，请在回复中使用 tool_calls 字段指定工具和参数。\n"
        "如果可以直接回答，直接返回回复内容。\n"
        "只使用一个问题中的信息，不需要工具时直接回答。"
    )

    messages = [SystemMessage(content=system_prompt)] + state["messages"]

    try:
        resp = await manager.achat(
            [LLMMessage(role=m.type if hasattr(m, "type") else "user",
                        content=m.content if hasattr(m, "content") else str(m))
             for m in state["messages"]],
            prefer_backend=model_type,
            max_tokens=settings.agent.max_tokens_per_response,
            temperature=settings.agent.temperature,
        )

        if hasattr(resp, "tool_calls") and resp.tool_calls:
            return {"tool_calls": [str(tc) for tc in resp.tool_calls]}

        return {"final_response": resp.content}

    except Exception as e:
        logger.error("agent.llm_router.error", error=str(e))
        return {"error": str(e), "final_response": f"处理出错: {str(e)}"}


async def rag_retriever_node(state: AgentState) -> dict:
    """RAG 检索节点"""
    from rag import RAGChain, EmbeddingManager

    last_message = state["messages"][-1] if state["messages"] else None
    if not last_message:
        return {"retrieved_docs": []}

    content = last_message.content if hasattr(last_message, "content") else str(last_message)

    try:
        rag = RAGChain()
        embed_mgr = EmbeddingManager(settings.embedding)
        results = await rag.retrieve(uid=0, query=content, top_k=settings.rag.top_k, embedder=embed_mgr)
        return {"retrieved_docs": results}
    except Exception as e:
        logger.error("agent.rag.error", error=str(e))
        return {"retrieved_docs": [], "error": str(e)}


async def response_synthesizer_node(state: AgentState) -> dict:
    """答案合成节点"""
    manager = LLMManager.get_instance()
    model_type = settings.llm.default_backend

    context_parts = []
    if state.get("retrieved_docs"):
        docs_text = "\n\n".join(
            f"[来源: {d.get('source', 'unknown')}]\n{d.get('content', '')}"
            for d in state["retrieved_docs"]
        )
        context_parts.append(f"【相关知识】\n{docs_text}")

    if state.get("tool_calls"):
        context_parts.append(f"【工具调用结果】\n{', '.join(state['tool_calls'])}")

    system_prompt = (
        settings.agent.system_prompt +
        ("\n\n以下是与用户问题相关的知识，请基于这些知识回答。" if context_parts else "")
    )

    user_content = state["messages"][-1].content if state["messages"] else ""
    if context_parts:
        user_content = "\n\n".join(context_parts) + f"\n\n用户问题: {user_content}"

    messages = [SystemMessage(content=system_prompt), HumanMessage(content=user_content)]

    try:
        resp = await manager.achat(
            [LLMMessage(role="system", content=system_prompt),
             LLMMessage(role="user", content=user_content)],
            prefer_backend=model_type,
            max_tokens=settings.agent.max_tokens_per_response,
            temperature=settings.agent.temperature,
        )
        return {"final_response": resp.content}
    except Exception as e:
        logger.error("agent.synthesizer.error", error=str(e))
        return {"final_response": f"生成回复时出错: {str(e)}"}


def route_based_on_intent(state: AgentState) -> str:
    """根据意图路由"""
    intent = state.get("intent", "chat")
    if intent in ("kb_query", "web_search", "calculate", "recommend"):
        return "rag"
    if state.get("tool_calls"):
        return "tool"
    return "chat"


def stop_if_max_iterations(state: AgentState) -> str:
    """最大迭代次数控制"""
    if state.get("iteration_count", 0) >= settings.agent.max_iterations:
        return "stop"
    return "continue"


async def run_agent_stream(agent, messages: list[BaseMessage], model_type: str = "",
                            model_name: str = "", **kwargs) -> AsyncIterator[str]:
    """
    运行 Agent 并流式返回 token 增量。
    适用于 SSE 流式输出场景，逐 token yield 给前端。
    """
    manager = LLMManager.get_instance()

    system_prompt = (
        settings.agent.system_prompt +
        "\n\n可用的工具：duckduckgo_search（搜索互联网）、knowledge_base_search（知识库检索）、"
        "calculator（计算器）、translator（翻译）。"
    )

    all_messages = [SystemMessage(content=system_prompt)]
    if messages:
        for m in messages:
            if hasattr(m, "content"):
                if hasattr(m, "type") and m.type == "human":
                    all_messages.append(HumanMessage(content=m.content))
                elif hasattr(m, "type") and m.type == "ai":
                    all_messages.append(AIMessage(content=m.content))
                else:
                    all_messages.append(HumanMessage(content=m.content))
            elif isinstance(m, str):
                all_messages.append(HumanMessage(content=m))

    last_message = all_messages[-1].content if all_messages else ""

    intent = _classify_intent(last_message)

    if intent in ("kb_query", "web_search"):
        rag = RAGChain()
        from rag import EmbeddingManager
        embed_mgr = EmbeddingManager(settings.embedding)
        try:
            results = await rag.retrieve(uid=0, query=last_message,
                                         top_k=settings.rag.top_k, embedder=embed_mgr)
            if results:
                docs_text = "\n\n".join(
                    f"[来源: {d.get('source', 'unknown')}]\n{d.get('content', '')}"
                    for d in results
                )
                all_messages.append(HumanMessage(
                    content=f"\n\n【相关知识】\n{docs_text}\n\n用户问题: {last_message}"
                ))
        except Exception as e:
            logger.error("agent.rag_stream.error", error=str(e))

    try:
        async for chunk in manager.astream(
            [LLMMessage(role="system", content=all_messages[0].content)] +
            [LLMMessage(role="user" if hasattr(m, "type") and m.type == "human" else "assistant",
                        content=m.content)
             for m in all_messages[1:]],
            prefer_backend=model_type,
            model_name=model_name,
            max_tokens=settings.agent.max_tokens_per_response,
            temperature=settings.agent.temperature,
        ):
            if chunk.content:
                yield chunk.content

    except Exception as e:
        logger.error("agent.stream.error", error=str(e))
        yield f"发生错误: {str(e)}"


def _classify_intent(content: str) -> str:
    """简单的意图分类"""
    keywords = {
        "kb_query": ["知识库", "文档", "上传", "记得", "根据文档", "基于文档", "knowledge base", "document"],
        "web_search": ["搜索", "最新", "查询", "互联网", "search", "recent", "news"],
        "calculate": ["计算", "等于", "加减乘除", "calculate", "compute"],
    }
    for intent, kws in keywords.items():
        if any(kw in content for kw in kws):
            return intent
    return "chat"


async def run_agent(agent, messages: list[BaseMessage], model_type: str = "",
                   model_name: str = "", **kwargs) -> AsyncIterator[str]:
    """
    运行 Agent，返回流式文本片段。
    """
    state = AgentState(messages=messages)
    iterations = 0
    max_iter = settings.agent.max_iterations

    async def run_step(current_state: AgentState) -> AgentState:
        nonlocal iterations
        iterations += 1

        if iterations > max_iter:
            current_state.final_response = "已达到最大推理步数，请重新描述您的问题。"
            return current_state

        current_state.iteration_count = iterations

        try:
            result = await agent.ainvoke(current_state)
            return result
        except Exception as e:
            logger.error("agent.step.error", iteration=iterations, error=str(e))
            current_state.error = str(e)
            return current_state

    try:
        state = await run_step(state)

        if state.final_response and not state.tool_calls:
            yield state.final_response
            return

        while state.tool_calls and iterations <= max_iter:
            state = await run_step(state)
            if state.final_response:
                yield state.final_response
                return

        if not state.final_response:
            state = await response_synthesizer_node(state)
            yield state.final_response

    except Exception as e:
        logger.error("agent.run.error", error=str(e))
        yield f"发生错误: {str(e)}"
