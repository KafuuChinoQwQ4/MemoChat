from __future__ import annotations

import json

from config import settings
from harness.contracts import AgentSkill, MemorySnapshot, PlanStep, ToolObservation
from harness.skills.registry import SkillRegistry


class PlanningPolicy:
    def __init__(self, skill_registry: SkillRegistry):
        self._skills = skill_registry

    def resolve_skill(self, request) -> AgentSkill:
        return self._skills.resolve(
            requested_skill=getattr(request, "skill_name", ""),
            content=getattr(request, "content", ""),
            feature_type=getattr(request, "feature_type", ""),
        )

    def build_plan(self, request, skill: AgentSkill) -> list[PlanStep]:
        content = getattr(request, "content", "")
        requested_tools = getattr(request, "requested_tools", []) or []
        plan_steps: list[PlanStep] = []

        if requested_tools:
            normalized_tools = {str(tool).strip() for tool in requested_tools if str(tool).strip()}
            if "calculator" in normalized_tools:
                plan_steps.append(PlanStep(action="calculate", reason="请求中显式选择了计算器工具"))
                normalized_tools.remove("calculator")
            if normalized_tools:
                plan_steps.append(PlanStep(action="mcp_tool", reason="请求中显式指定了工具调用"))

        for action in skill.default_actions:
            plan_steps.append(PlanStep(action=action, reason=f"技能 {skill.name} 的默认执行动作"))

        if not plan_steps:
            if any(keyword in content for keyword in ("知识库", "文档", "根据文档", "上传的", "kb", "document")):
                plan_steps.append(PlanStep(action="knowledge_search", reason="问题与私有知识库相关"))
            if any(keyword in content for keyword in ("搜索", "最新", "新闻", "联网", "recent", "search")):
                plan_steps.append(PlanStep(action="web_search", reason="问题需要实时信息"))
            if any(keyword in content for keyword in ("计算", "等于", "+", "-", "*", "/", "sqrt", "pow", "math")):
                plan_steps.append(PlanStep(action="calculate", reason="问题包含计算请求"))
            if any(keyword in content for keyword in ("推荐", "关系", "好友", "群组", "graph")):
                plan_steps.append(PlanStep(action="graph_recall", reason="问题涉及关系图谱或推荐"))

        deduped: list[PlanStep] = []
        seen: set[str] = set()
        for step in plan_steps:
            if step.action in seen:
                continue
            seen.add(step.action)
            deduped.append(step)
            if len(deduped) >= settings.harness.max_tool_rounds:
                break
        return deduped

    def build_system_prompt(
        self,
        skill: AgentSkill,
        memory_snapshot: MemorySnapshot,
        observations: list[ToolObservation],
    ) -> str:
        observation_block = "\n".join(observation.to_summary() for observation in observations) if observations else "无"
        semantic_block = (
            json.dumps(memory_snapshot.semantic_profile, ensure_ascii=False)
            if memory_snapshot.semantic_profile
            else "{}"
        )
        graph_block = (
            json.dumps(memory_snapshot.graph_context, ensure_ascii=False)
            if memory_snapshot.graph_context
            else "[]"
        )
        return (
            f"{settings.agent.system_prompt}\n\n"
            f"你正在运行 MemoChat {settings.harness.industrial_target_year} 工业级 AI Agent Harness。\n"
            f"当前技能: {skill.display_name}\n"
            f"技能说明: {skill.description}\n"
            f"技能约束: {skill.system_prompt}\n"
            "执行原则:\n"
            "1. 优先使用系统提供的上下文和工具观察，不编造来源。\n"
            "2. 如果答案不确定，直接说明不确定或信息不足。\n"
            "3. 对知识库、搜索、图谱结果要明确区分为观察结果。\n"
            "4. 输出保持中文、简洁、可执行。\n"
            "5. 不要输出推理过程、思考过程、草稿或分析步骤，只输出最终答案。\n\n"
            f"工具观察:\n{observation_block}\n\n"
            f"语义记忆:\n{semantic_block}\n\n"
            f"图谱记忆:\n{graph_block}\n"
        )

    def build_user_prompt(self, request, observations: list[ToolObservation]) -> str:
        content = getattr(request, "content", "")
        feature_type = getattr(request, "feature_type", "").lower()
        target_lang = getattr(request, "target_lang", "") or "中文"
        metadata = getattr(request, "metadata", {}) or {}
        source_lang = metadata.get("source_lang", "自动检测") if isinstance(metadata, dict) else "自动检测"
        observation_context = "\n\n".join(observation.output for observation in observations)

        if feature_type == "summary" or getattr(request, "skill_name", "") == "summarize_thread":
            prompt = (
                "/no_think\n"
                "只根据【聊天内容】输出最终摘要，不要写推理过程，不要复述任务，不要解释。"
                "固定格式：结论：...；待办：...；分歧：...；需回复：...。"
                "没有的信息写“无”，总字数不超过 120 字。\n\n"
                f"【聊天内容】\n{content}"
            )
        elif feature_type == "suggest" or getattr(request, "skill_name", "") == "reply_suggester":
            prompt = (
                "/no_think\n"
                "请基于以下对话生成 3 条可直接发送的回复建议。"
                "只返回 JSON 数组，数组里每项是纯文本字符串，不要 Markdown，不要解释，每条不超过 30 字：\n"
                f"{content}"
            )
        elif feature_type == "translate" or getattr(request, "skill_name", "") == "translate_text":
            prompt = f"/no_think\n请把以下内容从 {source_lang or '自动检测'} 翻译成 {target_lang}，只返回翻译结果，不要解释：\n{content}"
        else:
            prompt = content

        if observation_context:
            return f"【执行观察】\n{observation_context}\n\n【用户请求】\n{prompt}"
        return prompt
