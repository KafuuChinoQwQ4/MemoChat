from __future__ import annotations

from harness.contracts import AgentSkill
from harness.skills.specs import AgentSpecRegistry


class SkillRegistry:
    def __init__(self):
        self._spec_registry = AgentSpecRegistry()
        self._skills = {
            skill.name: skill
            for skill in [
                AgentSkill(
                    name="general_chat",
                    display_name="General Chat",
                    description="通用对话和问题回答。",
                    system_prompt="以工业级 AI Agent 的方式回答问题：结论优先、信息可追溯、不臆造事实。",
                ),
                AgentSkill(
                    name="knowledge_copilot",
                    display_name="Knowledge Copilot",
                    description="面向知识库问答，优先引用用户私有文档。",
                    system_prompt="优先依据知识库检索结果回答；若知识不足，明确说明缺口。",
                    default_actions=["knowledge_search"],
                    allow_knowledge=True,
                ),
                AgentSkill(
                    name="research_assistant",
                    display_name="Research Assistant",
                    description="面向实时信息检索和联网研究。",
                    system_prompt="优先基于实时搜索结果回答，并明确哪些内容来自搜索观察。",
                    default_actions=["web_search"],
                    allow_web=True,
                ),
                AgentSkill(
                    name="summarize_thread",
                    display_name="Summarize Thread",
                    description="对输入文本或聊天内容做摘要。",
                    system_prompt="将用户提供的内容压缩成高信息密度摘要，默认 50 字以内。",
                ),
                AgentSkill(
                    name="reply_suggester",
                    display_name="Reply Suggester",
                    description="生成多条简洁回复建议。",
                    system_prompt="根据上下文生成 3 条风格自然的短回复建议，优先返回 JSON 数组。",
                ),
                AgentSkill(
                    name="translate_text",
                    display_name="Translate Text",
                    description="将文本翻译为目标语言。",
                    system_prompt="严格执行翻译任务，只返回翻译结果，不加解释。",
                ),
                AgentSkill(
                    name="graph_recommender",
                    display_name="Graph Recommender",
                    description="结合图数据库做关系和推荐相关回答。",
                    system_prompt="优先利用图谱上下文、推荐结果和关系数据回答问题。",
                    default_actions=["graph_recall"],
                    allow_graph=True,
                ),
                AgentSkill(
                    name="mcp_operator",
                    display_name="MCP Operator",
                    description="显式调用 MCP/工具目录中的能力。",
                    system_prompt="仅在给定的工具和参数范围内执行，不猜测工具参数。",
                    default_actions=["mcp_tool"],
                    allow_mcp=True,
                ),
            ]
        }
        for spec_skill in self._spec_registry.as_skills():
            self._skills[spec_skill.name] = spec_skill

    def list_skills(self) -> list[AgentSkill]:
        return list(self._skills.values())

    def get(self, skill_name: str) -> AgentSkill | None:
        return self._skills.get(skill_name)

    def resolve(self, requested_skill: str, content: str, feature_type: str = "") -> AgentSkill:
        if requested_skill and requested_skill in self._skills:
            return self._skills[requested_skill]

        feature_key = feature_type.lower().strip()
        if feature_key == "summary":
            return self._skills["summarize_thread"]
        if feature_key == "suggest":
            return self._skills["reply_suggester"]
        if feature_key == "translate":
            return self._skills["translate_text"]

        lowered = content.lower()
        if any(keyword in content for keyword in ("知识库", "文档", "根据文档", "上传的", "kb", "document")):
            return self._skills["knowledge_copilot"]
        if any(keyword in content for keyword in ("搜索", "最新", "新闻", "联网", "recent", "search")):
            return self._skills["research_assistant"]
        if any(keyword in content for keyword in ("推荐", "关系", "好友", "群组", "topic", "graph")):
            return self._skills["graph_recommender"]
        if any(keyword in lowered for keyword in ("translate", "翻译")):
            return self._skills["translate_text"]
        if any(keyword in content for keyword in ("总结", "摘要", "概括", "summary")):
            return self._skills["summarize_thread"]
        return self._skills["general_chat"]

    def list_specs(self):
        return self._spec_registry.list_specs()

    def get_spec(self, spec_name: str):
        return self._spec_registry.get(spec_name)

    def reload_specs(self) -> None:
        self._spec_registry.reload()
        for spec_skill in self._spec_registry.as_skills():
            self._skills[spec_skill.name] = spec_skill
