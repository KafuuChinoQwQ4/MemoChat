from __future__ import annotations

from copy import deepcopy
from typing import Any


HARNESS_LAYERS: list[dict[str, Any]] = [
    {
        "name": "orchestration",
        "display_name": "编排层",
        "path": "harness/orchestration",
        "responsibilities": [
            "解析技能和请求意图",
            "生成执行计划",
            "串联记忆、工具、模型和反馈",
            "产出统一 trace",
        ],
        "primary_files": ["planner.py", "agent_service.py"],
    },
    {
        "name": "execution",
        "display_name": "执行层",
        "path": "harness/execution",
        "responsibilities": [
            "执行计划动作",
            "调用内置工具和 MCP 工具",
            "汇总知识库、搜索、计算和图谱观察",
        ],
        "primary_files": ["tool_executor.py"],
    },
    {
        "name": "feedback",
        "display_name": "反馈层",
        "path": "harness/feedback",
        "responsibilities": [
            "记录 run trace 和 step trace",
            "记录用户反馈",
            "生成自动反馈摘要",
            "支持持久化 trace 回读",
        ],
        "primary_files": ["trace_store.py", "evaluator.py"],
    },
    {
        "name": "memory",
        "display_name": "记忆层",
        "path": "harness/memory",
        "responsibilities": [
            "加载短期会话记忆",
            "加载和写入情景记忆",
            "维护语义偏好记忆",
            "投影和召回 Neo4j 图谱记忆",
        ],
        "primary_files": ["service.py", "graph_memory.py"],
    },
    {
        "name": "knowledge",
        "display_name": "知识库层",
        "path": "harness/knowledge",
        "responsibilities": [
            "处理知识库上传",
            "调用 RAG 分块、向量化、检索和删除",
            "维护 Postgres 知识库元数据",
        ],
        "primary_files": ["service.py"],
    },
    {
        "name": "llm",
        "display_name": "模型路由层",
        "path": "harness/llm",
        "responsibilities": [
            "发现 local_api 和 external_api 模型端点",
            "按模型、后端和部署偏好解析端点",
            "提供非流式和流式模型调用",
        ],
        "primary_files": ["service.py"],
    },
    {
        "name": "mcp",
        "display_name": "MCP 桥接层",
        "path": "harness/mcp",
        "responsibilities": [
            "暴露 MCP 工具清单",
            "与底层 ToolRegistry 保持单向依赖",
        ],
        "primary_files": ["service.py"],
    },
    {
        "name": "skills",
        "display_name": "技能注册层",
        "path": "harness/skills",
        "responsibilities": [
            "维护技能元数据",
            "按显式技能、feature_type 和内容意图解析技能",
            "定义技能默认动作和权限",
        ],
        "primary_files": ["registry.py"],
    },
    {
        "name": "runtime",
        "display_name": "运行装配层",
        "path": "harness/runtime",
        "responsibilities": [
            "集中装配各层服务",
            "隔离具体实现构造逻辑",
            "管理 harness 生命周期入口",
        ],
        "primary_files": ["container.py"],
    },
]


def list_harness_layers() -> list[dict[str, Any]]:
    return deepcopy(HARNESS_LAYERS)
