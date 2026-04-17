import asyncio
import logging
import functools
from typing import Annotated, TypedDict, List, Optional
from langchain_core.messages import BaseMessage, HumanMessage, AIMessage
from langgraph.graph import StateGraph, END
from langgraph.prebuilt import ToolNode
from langgraph.checkpoint.memory import MemorySaver

from config import settings
from tools.hitl_manager import HITLManager
from tools import calculator_tool, search_tool, knowledge_base_tool

logger = logging.getLogger(__name__)


class AgentState(TypedDict):
    uid: int
    session_id: str
    messages: List[dict]
    tools_used: List[str]
    tokens_used: int
    pending_confirm: Optional[dict]
    iteration_count: int


def with_timeout(func, timeout_sec: int):
    """为异步函数添加超时装饰器"""
    @functools.wraps(func)
    async def wrapper(*args, **kwargs):
        try:
            return await asyncio.wait_for(func(*args, **kwargs), timeout=timeout_sec)
        except asyncio.TimeoutError:
            logger.warning(f"{func.__name__} timed out after {timeout_sec}s")
            return "[超时] 该操作花费时间过长，请尝试简化问题。"
    return wrapper


def detect_risky_action(content: str) -> bool:
    """检测高风险操作"""
    risky_keywords = ["发送消息", "转账", "删除", "取消", "公开", "广播", "泄露"]
    return any(kw in content for kw in risky_keywords)


async def call_llm(state: AgentState, llm):
    """调用 LLM 生成回复或工具调用"""
    model = llm
    lc_messages = []
    for m in state["messages"]:
        if m.get("role") == "user":
            lc_messages.append(HumanMessage(content=m["content"]))
        else:
            lc_messages.append(AIMessage(content=m.get("content", "")))

    response = await model.ainvoke(lc_messages)
    new_msg = {"role": "assistant", "content": response.content}

    state["messages"].append(new_msg)
    state["tokens_used"] += len(str(response.content)) // 4
    state["iteration_count"] += 1

    return {"messages": state["messages"], "tokens_used": state["tokens_used"],
            "iteration_count": state["iteration_count"]}


async def human_confirm_node(state: AgentState, hitl_manager: HITLManager):
    """人类确认节点"""
    last_msg = state["messages"][-1]["content"]
    confirm_id = f"confirm_{state['session_id']}_{state['iteration_count']}"

    state["pending_confirm"] = {
        "confirm_id": confirm_id,
        "content": last_msg,
        "uid": state["uid"],
    }

    hitl_manager.submit(confirm_id, last_msg, state["uid"])

    state["messages"].append({
        "role": "assistant",
        "content": "⚠️ 这是一个高风险操作，已提交等待您确认。",
    })

    return {"pending_confirm": state["pending_confirm"], "messages": state["messages"]}


def should_confirm(state: AgentState) -> str:
    """判断是否需要人类确认"""
    if not settings.hitl_enabled:
        return "call_tool"

    last_msg = state["messages"][-1]["content"]
    if detect_risky_action(last_msg):
        return "human_confirm"

    if state.get("pending_confirm"):
        return "wait_for_confirm"

    return "call_tool"


def stop_if_max_iterations(state: AgentState) -> bool:
    return state["iteration_count"] >= settings.max_iterations


def build_agent(llm, tools: list, hitl_manager: HITLManager):
    """构建 LangGraph Agent"""

    tool_node = ToolNode(tools)

    graph = StateGraph(AgentState)

    graph.add_node("llm", lambda state: call_llm(state, llm))
    graph.add_node("call_tool", tool_node)
    graph.add_node("human_confirm", lambda state: human_confirm_node(state, hitl_manager))

    graph.set_entry_point("llm")

    graph.add_conditional_edges(
        "llm",
        should_confirm,
        {
            "call_tool": "call_tool",
            "human_confirm": "human_confirm",
            "wait_for_confirm": END,
        }
    )

    graph.add_edge("call_tool", "llm")

    graph.add_conditional_edges(
        "human_confirm",
        lambda state: "llm" if state.get("pending_confirm") is None else END,
    )

    graph.add_conditional_edges(
        "llm",
        lambda state: END if stop_if_max_iterations(state) else "call_tool",
    )

    checkpointer = MemorySaver()
    return graph.compile(checkpointer=checkpointer)
