from __future__ import annotations

from dataclasses import asdict

from harness.contracts import AgentSkill, AgentSpec, KnowledgePolicy, MemoryPolicy, ModelPolicy, ToolPolicy


def _policy(policy) -> dict:
    return asdict(policy)


class AgentSpecRegistry:
    def __init__(self):
        self._specs = {spec.name: spec for spec in self._builtin_specs()}

    def list_specs(self) -> list[AgentSpec]:
        return list(self._specs.values())

    def get(self, name: str) -> AgentSpec | None:
        return self._specs.get((name or "").strip())

    def reload(self) -> None:
        self._specs = {spec.name: spec for spec in self._builtin_specs()}

    def as_skills(self) -> list[AgentSkill]:
        return [self.to_skill(spec) for spec in self.list_specs()]

    def to_skill(self, spec: AgentSpec) -> AgentSkill:
        tool_policy = dict(spec.tool_policy)
        knowledge_policy = dict(spec.knowledge_policy)
        memory_policy = dict(spec.memory_policy)
        allowed = set(spec.allowed_tools)
        default_actions = list(tool_policy.get("default_actions", []))
        if not default_actions:
            default_actions = self._default_actions_from_tools(spec.default_tools)
        return AgentSkill(
            name=spec.name,
            display_name=spec.display_name,
            description=spec.description,
            system_prompt=spec.system_prompt,
            default_actions=default_actions,
            allow_web="duckduckgo_search" in allowed or "web_search" in default_actions,
            allow_knowledge=bool(knowledge_policy.get("enabled")) or "knowledge_base_search" in allowed,
            allow_graph=bool(memory_policy.get("include_graph")) or "graph_recall" in default_actions,
            allow_mcp=bool(tool_policy.get("allow_mcp", False)),
            metadata={
                "agent_spec": spec.to_dict(),
                "model_policy": dict(spec.model_policy),
                "tool_policy": tool_policy,
                "knowledge_policy": knowledge_policy,
                "memory_policy": memory_policy,
                "guardrail_policy": dict(spec.guardrail_policy),
            },
        )

    def _default_actions_from_tools(self, tools: list[str]) -> list[str]:
        actions: list[str] = []
        if "knowledge_base_search" in tools:
            actions.append("knowledge_search")
        if "duckduckgo_search" in tools:
            actions.append("web_search")
        if "calculator" in tools:
            actions.append("calculate")
        if "graph_recommender" in tools:
            actions.append("graph_recall")
        return actions

    def _builtin_specs(self) -> list[AgentSpec]:
        return [
            AgentSpec(
                name="researcher",
                display_name="Researcher",
                description="Structured research assistant that gathers evidence before answering.",
                system_prompt=(
                    "你是研究型 Agent。优先收集证据、区分事实和假设、指出信息来源和不确定性。"
                    "回答应包含结论、证据、风险或缺口。"
                ),
                default_model="ollama:qwen3:4b",
                model_policy=_policy(ModelPolicy(provider="ollama", model="qwen3:4b", max_tokens=3072, temperature=0.35)),
                default_tools=["duckduckgo_search", "knowledge_base_search"],
                allowed_tools=["duckduckgo_search", "knowledge_base_search", "calculator"],
                tool_policy=_policy(ToolPolicy(
                    default_tools=["duckduckgo_search", "knowledge_base_search"],
                    allowed_tools=["duckduckgo_search", "knowledge_base_search", "calculator"],
                    default_actions=["web_search", "knowledge_search"],
                )),
                knowledge_policy=_policy(KnowledgePolicy(enabled=True, top_k=6, cite_sources=True)),
                memory_policy=_policy(MemoryPolicy(include_history=True, include_semantic_profile=True, include_graph=False)),
                guardrail_policy={"require_citations_when_using_search": True, "block_unconfirmed_writes": True},
                metadata={"category": "research"},
            ),
            AgentSpec(
                name="writer",
                display_name="Writer",
                description="Drafts clear user-facing content from supplied context.",
                system_prompt=(
                    "你是写作型 Agent。先理解目标读者和语气，再输出结构清楚、可直接使用的文本。"
                    "不要虚构事实；缺少背景时先标注假设。"
                ),
                default_model="ollama:qwen3:4b",
                model_policy=_policy(ModelPolicy(provider="ollama", model="qwen3:4b", max_tokens=4096, temperature=0.75)),
                default_tools=["knowledge_base_search"],
                allowed_tools=["knowledge_base_search", "translator"],
                tool_policy=_policy(ToolPolicy(
                    default_tools=["knowledge_base_search"],
                    allowed_tools=["knowledge_base_search", "translator"],
                    default_actions=["knowledge_search"],
                )),
                knowledge_policy=_policy(KnowledgePolicy(enabled=True, top_k=4, cite_sources=False)),
                memory_policy=_policy(MemoryPolicy(include_history=True, include_semantic_profile=True)),
                guardrail_policy={"avoid_hidden_reasoning": True},
                metadata={"category": "writing"},
            ),
            AgentSpec(
                name="reviewer",
                display_name="Reviewer",
                description="Reviews outputs for correctness, completeness, safety, and missing evidence.",
                system_prompt=(
                    "你是审阅型 Agent。优先指出问题和风险，按严重程度排序，给出可执行修复建议。"
                    "不要只做鼓励性总结。"
                ),
                default_model="ollama:qwen3:4b",
                model_policy=_policy(ModelPolicy(provider="ollama", model="qwen3:4b", max_tokens=3072, temperature=0.2)),
                default_tools=["knowledge_base_search", "calculator"],
                allowed_tools=["knowledge_base_search", "calculator"],
                tool_policy=_policy(ToolPolicy(
                    default_tools=["knowledge_base_search"],
                    allowed_tools=["knowledge_base_search", "calculator"],
                    default_actions=["knowledge_search"],
                )),
                knowledge_policy=_policy(KnowledgePolicy(enabled=True, top_k=5, cite_sources=True)),
                memory_policy=_policy(MemoryPolicy(include_history=True, include_semantic_profile=False)),
                guardrail_policy={"prefer_findings_first": True, "block_unconfirmed_writes": True},
                metadata={"category": "review"},
            ),
            AgentSpec(
                name="support",
                display_name="Support",
                description="Helps users resolve product or workflow issues with concise troubleshooting steps.",
                system_prompt=(
                    "你是支持型 Agent。先复述问题边界，再给出最短可行排查路径。"
                    "涉及账号、隐私、删除、支付等敏感操作时要求用户确认。"
                ),
                default_model="ollama:qwen3:4b",
                model_policy=_policy(ModelPolicy(provider="ollama", model="qwen3:4b", max_tokens=2048, temperature=0.45)),
                default_tools=["knowledge_base_search"],
                allowed_tools=["knowledge_base_search", "calculator"],
                tool_policy=_policy(ToolPolicy(
                    default_tools=["knowledge_base_search"],
                    allowed_tools=["knowledge_base_search", "calculator"],
                    default_actions=["knowledge_search"],
                )),
                knowledge_policy=_policy(KnowledgePolicy(enabled=True, top_k=5, cite_sources=True)),
                memory_policy=_policy(MemoryPolicy(include_history=True, include_semantic_profile=True)),
                guardrail_policy={"ask_confirmation_for_sensitive_actions": True},
                metadata={"category": "support"},
            ),
            AgentSpec(
                name="knowledge_curator",
                display_name="Knowledge Curator",
                description="Organizes user knowledge into durable summaries, tags, and maintenance suggestions.",
                system_prompt=(
                    "你是知识整理型 Agent。提取主题、术语、事实、待补充信息和可归档摘要。"
                    "不要删除或改写原始事实；对不确定内容标记待确认。"
                ),
                default_model="ollama:qwen3:4b",
                model_policy=_policy(ModelPolicy(provider="ollama", model="qwen3:4b", max_tokens=3072, temperature=0.4)),
                default_tools=["knowledge_base_search"],
                allowed_tools=["knowledge_base_search", "graph_recommender"],
                tool_policy=_policy(ToolPolicy(
                    default_tools=["knowledge_base_search"],
                    allowed_tools=["knowledge_base_search", "graph_recommender"],
                    default_actions=["knowledge_search", "graph_recall"],
                )),
                knowledge_policy=_policy(KnowledgePolicy(enabled=True, top_k=8, cite_sources=True)),
                memory_policy=_policy(MemoryPolicy(include_history=True, include_semantic_profile=True, include_graph=True)),
                guardrail_policy={"preserve_source_facts": True, "block_unconfirmed_writes": True},
                metadata={"category": "knowledge"},
            ),
        ]
