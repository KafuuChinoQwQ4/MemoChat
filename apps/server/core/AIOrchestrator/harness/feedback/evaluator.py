from __future__ import annotations

from harness.contracts import ToolObservation


class FeedbackEvaluator:
    def build_summary(
        self,
        skill_name: str,
        observations: list[ToolObservation],
        response_text: str,
    ) -> str:
        tool_names = ",".join(observation.name for observation in observations) if observations else "none"
        response_size = len(response_text)

        confidence = "medium"
        if observations:
            confidence = "high"
        elif skill_name in {"knowledge_copilot", "research_assistant", "graph_recommender"}:
            confidence = "low"

        return (
            f"skill={skill_name}; "
            f"tools={tool_names}; "
            f"response_chars={response_size}; "
            f"confidence={confidence}"
        )
