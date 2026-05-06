from __future__ import annotations

import json
import re
from collections import Counter

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
                plan_steps.append(self._step_for_action("calculate", "请求中显式选择了计算器工具", request, skill))
                normalized_tools.remove("calculator")
            if normalized_tools:
                plan_steps.append(self._step_for_action("mcp_tool", "请求中显式指定了工具调用", request, skill))

        for action in skill.default_actions:
            plan_steps.append(self._step_for_action(action, f"技能 {skill.name} 的默认执行动作", request, skill))

        if not plan_steps:
            if any(keyword in content for keyword in ("知识库", "文档", "根据文档", "上传的", "kb", "document")):
                plan_steps.append(self._step_for_action("knowledge_search", "问题与私有知识库相关", request, skill))
            if any(keyword in content for keyword in ("搜索", "最新", "新闻", "联网", "recent", "search")):
                plan_steps.append(self._step_for_action("web_search", "问题需要实时信息", request, skill))
            if any(keyword in content for keyword in ("计算", "等于", "+", "-", "*", "/", "sqrt", "pow", "math")):
                plan_steps.append(self._step_for_action("calculate", "问题包含计算请求", request, skill))
            if any(keyword in content for keyword in ("推荐", "关系", "好友", "群组", "graph")):
                plan_steps.append(self._step_for_action("graph_recall", "问题涉及关系图谱或推荐", request, skill))

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

    def build_react_followup_plan(
        self,
        request,
        skill: AgentSkill,
        plan_steps: list[PlanStep],
        observations: list[ToolObservation],
    ) -> list[PlanStep]:
        if len(plan_steps) >= settings.harness.max_tool_rounds:
            return []

        remaining = max(settings.harness.max_tool_rounds - len(plan_steps), 0)
        if remaining <= 0:
            return []

        assessment = self.assess_react_observations(request, skill, plan_steps, observations)
        next_actions = assessment.get("next_actions", [])
        if not isinstance(next_actions, list):
            return []

        for action in next_actions:
            if not isinstance(action, str) or not action:
                continue
            reason = self._react_reason_for_action(action, assessment)
            return [self._step_for_action(action, reason, request, skill, followup=True)]
        return []

    def assess_react_observations(
        self,
        request,
        skill: AgentSkill,
        plan_steps: list[PlanStep],
        observations: list[ToolObservation],
    ) -> dict:
        action_counts = Counter(step.action for step in plan_steps)
        content = str(getattr(request, "content", "") or "")
        needs = {
            "web": skill.allow_web or action_counts["web_search"] > 0 or self._content_needs_web(content),
            "knowledge": skill.allow_knowledge or action_counts["knowledge_search"] > 0 or self._content_needs_knowledge(content),
            "calculation": action_counts["calculate"] > 0 or self._content_needs_calculation(content),
            "graph": skill.allow_graph or action_counts["graph_recall"] > 0 or self._content_needs_graph(content),
        }
        statuses = {
            "web": self._observation_status(self._observations_named(observations, "duckduckgo_search"), "web"),
            "knowledge": self._observation_status(self._observations_named(observations, "knowledge_search"), "knowledge"),
            "calculation": self._observation_status(self._observations_named(observations, "calculator"), "calculation"),
            "graph": self._observation_status(self._observations_named(observations, "graph_recommender"), "graph"),
        }
        gaps: list[str] = []
        next_actions: list[str] = []

        if needs["calculation"] and statuses["calculation"] != "strong" and action_counts["calculate"] < 1:
            gaps.append("calculation_missing")
            next_actions.append("calculate")

        if needs["web"] and statuses["web"] != "strong":
            gaps.append(f"web_{statuses['web']}")
            if action_counts["web_search"] < 2:
                next_actions.append("web_search")
            elif needs["knowledge"] and statuses["knowledge"] != "strong" and action_counts["knowledge_search"] < 2:
                next_actions.append("knowledge_search")

        if needs["knowledge"] and statuses["knowledge"] != "strong":
            gaps.append(f"knowledge_{statuses['knowledge']}")
            if action_counts["knowledge_search"] < 2:
                next_actions.append("knowledge_search")
            elif needs["web"] and statuses["web"] != "strong" and action_counts["web_search"] < 2:
                next_actions.append("web_search")

        if needs["graph"] and statuses["graph"] != "strong" and action_counts["graph_recall"] < 1:
            gaps.append(f"graph_{statuses['graph']}")
            next_actions.append("graph_recall")

        next_actions = self._dedupe_actions(next_actions)
        if not gaps:
            confidence = "high"
        elif any(status == "strong" for key, status in statuses.items() if needs.get(key)):
            confidence = "medium"
        else:
            confidence = "low"

        return {
            "method": "react_observe_act",
            "confidence": confidence,
            "needs_followup": bool(next_actions),
            "needs": needs,
            "statuses": statuses,
            "gaps": gaps,
            "next_actions": next_actions,
            "action_counts": dict(action_counts),
            "observation_count": len(observations),
        }

    def _step_for_action(self, action: str, reason: str, request, skill: AgentSkill, followup: bool = False) -> PlanStep:
        parameters: dict = {}
        if action == "web_search":
            parameters = {
                "queries": self._search_queries(getattr(request, "content", ""), followup=followup),
                "max_results": 4 if followup else 5,
                "strategy": "react_followup" if followup else "plan_execute",
            }
        elif action == "knowledge_search":
            parameters = {
                "top_k": self._knowledge_top_k(skill, boosted=followup),
                "strategy": "react_followup" if followup else "plan_execute",
            }
        return PlanStep(action=action, reason=reason, parameters=parameters)

    def _react_reason_for_action(self, action: str, assessment: dict) -> str:
        confidence = assessment.get("confidence", "low")
        gaps = assessment.get("gaps", [])
        gap_text = ", ".join(gaps[:3]) if isinstance(gaps, list) else ""
        if action == "web_search":
            return f"ReAct 观察评估为 {confidence}，公开信息证据不足，追加联网检索: {gap_text}"
        if action == "knowledge_search":
            return f"ReAct 观察评估为 {confidence}，私有知识证据不足，追加知识库检索: {gap_text}"
        if action == "calculate":
            return f"ReAct 观察评估为 {confidence}，缺少计算结果，追加计算工具: {gap_text}"
        if action == "graph_recall":
            return f"ReAct 观察评估为 {confidence}，缺少图谱上下文，追加图谱召回: {gap_text}"
        return f"ReAct 观察评估为 {confidence}，追加工具动作: {gap_text}"

    def _knowledge_top_k(self, skill: AgentSkill, boosted: bool = False) -> int:
        policy = {}
        metadata = skill.metadata if isinstance(skill.metadata, dict) else {}
        raw_policy = metadata.get("knowledge_policy", {})
        if isinstance(raw_policy, dict):
            policy = raw_policy
        try:
            top_k = int(policy.get("top_k") or settings.harness.knowledge_top_k or settings.rag.top_k)
        except (TypeError, ValueError):
            top_k = settings.harness.knowledge_top_k or settings.rag.top_k
        return min(max(top_k + (2 if boosted else 0), 1), 12)

    def _search_queries(self, content: str, followup: bool = False) -> list[str]:
        compact = self._compact_query(content)
        queries = [] if followup else [compact]
        lowered = compact.lower()
        needs_fresh = any(token in compact for token in ("最新", "新闻", "实时", "今天", "近期")) or any(
            token in lowered for token in ("latest", "recent", "news", "today")
        )
        if followup and needs_fresh:
            queries.append(f"{compact} 官方")
            queries.append(f"{compact} 2026")
        elif needs_fresh:
            queries.append(f"{compact} 最新")
            queries.append(f"{compact} news")
        elif followup:
            queries.append(f"{compact} 资料")
            queries.append(f"{compact} official")
        return self._dedupe_queries(queries)

    def _compact_query(self, content: str) -> str:
        text = re.sub(r"\s+", " ", str(content or "")).strip()
        text = re.sub(r"^(请|帮我|麻烦|能不能|可以)?(搜索|查找|联网|查询|看看)\s*", "", text)
        return text[:160] or str(content or "").strip()[:160] or "MemoChat"

    def _dedupe_queries(self, queries: list[str]) -> list[str]:
        deduped: list[str] = []
        seen: set[str] = set()
        for query in queries:
            cleaned = query.strip()
            key = cleaned.lower()
            if not cleaned or key in seen:
                continue
            seen.add(key)
            deduped.append(cleaned)
            if len(deduped) >= 3:
                break
        return deduped

    def _needs_web_followup(
        self,
        skill: AgentSkill,
        plan_steps: list[PlanStep],
        observations: list[ToolObservation],
    ) -> bool:
        if not skill.allow_web and not any(step.action == "web_search" for step in plan_steps):
            return False
        web_observations = [obs for obs in observations if obs.name == "duckduckgo_search"]
        if not web_observations:
            return any(step.action == "web_search" for step in plan_steps)
        weak_markers = ("未找到", "搜索失败", "工具执行失败", "没有找到")
        return any(any(marker in obs.output for marker in weak_markers) for obs in web_observations)

    def _needs_knowledge_followup(
        self,
        skill: AgentSkill,
        plan_steps: list[PlanStep],
        observations: list[ToolObservation],
    ) -> bool:
        if not skill.allow_knowledge and not any(step.action == "knowledge_search" for step in plan_steps):
            return False
        knowledge_observations = [obs for obs in observations if obs.name == "knowledge_search"]
        if not knowledge_observations:
            return any(step.action == "knowledge_search" for step in plan_steps)
        weak_markers = ("没有找到相关内容", "工具执行失败", "知识库中没有找到")
        return any(any(marker in obs.output for marker in weak_markers) for obs in knowledge_observations)

    def _observations_named(self, observations: list[ToolObservation], name: str) -> list[ToolObservation]:
        return [observation for observation in observations if observation.name == name]

    def _observation_status(self, observations: list[ToolObservation], category: str) -> str:
        if not observations:
            return "missing"
        outputs = "\n".join(observation.output or "" for observation in observations)
        lowered = outputs.lower()
        weak_markers = ("未找到", "没有找到", "搜索失败", "工具执行失败", "not found", "failed", "error")
        if any(marker in lowered for marker in weak_markers):
            return "weak"
        if category == "web" and ("http://" in outputs or "https://" in outputs or "### 查询:" in outputs):
            return "strong"
        if category == "knowledge" and any(observation.metadata.get("hit_count", 0) for observation in observations):
            return "strong"
        if category == "calculation" and outputs.strip():
            return "strong"
        if category == "graph" and outputs.strip() and outputs.strip() not in {"[]", "{}"}:
            return "strong"
        return "strong" if len(outputs.strip()) >= 24 else "weak"

    def _dedupe_actions(self, actions: list[str]) -> list[str]:
        deduped: list[str] = []
        seen: set[str] = set()
        for action in actions:
            if action in seen:
                continue
            seen.add(action)
            deduped.append(action)
        return deduped

    def _content_needs_web(self, content: str) -> bool:
        lowered = content.lower()
        return any(keyword in content for keyword in ("搜索", "最新", "新闻", "联网", "实时", "今天", "近期")) or any(
            keyword in lowered for keyword in ("recent", "latest", "search", "news", "today")
        )

    def _content_needs_knowledge(self, content: str) -> bool:
        lowered = content.lower()
        return any(keyword in content for keyword in ("知识库", "文档", "根据文档", "上传的")) or any(
            keyword in lowered for keyword in ("kb", "document")
        )

    def _content_needs_calculation(self, content: str) -> bool:
        lowered = content.lower()
        return any(keyword in content for keyword in ("计算", "等于", "+", "-", "*", "/", "sqrt", "pow")) or any(
            keyword in lowered for keyword in ("math", "calculate")
        )

    def _content_needs_graph(self, content: str) -> bool:
        lowered = content.lower()
        return any(keyword in content for keyword in ("推荐", "关系", "好友", "群组")) or any(
            keyword in lowered for keyword in ("topic", "graph")
        )

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
