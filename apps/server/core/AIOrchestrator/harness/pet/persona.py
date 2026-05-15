from __future__ import annotations

from dataclasses import dataclass, field

from .contracts import PetObservation, PetSession


@dataclass(frozen=True)
class PetPromptContext:
    session_id: str
    uid: int
    profile_id: str
    persona: str
    provider: str
    user_text: str
    model_type: str = ""
    model_name: str = ""
    observation_summary: dict = field(default_factory=dict)
    runtime_metadata: dict = field(default_factory=dict)
    memory_snippets: list[str] = field(default_factory=list)
    safety_notes: list[str] = field(default_factory=list)


class PetPromptBuilder:
    """Builds provider-neutral prompt context without binding to any LLM vendor."""

    def build_user_turn(
        self,
        session: PetSession,
        content: str,
        model_type: str = "",
        model_name: str = "",
        observation: PetObservation | None = None,
    ) -> PetPromptContext:
        return PetPromptContext(
            session_id=session.session_id,
            uid=session.uid,
            profile_id=session.profile_id,
            persona=session.persona,
            provider=session.provider,
            user_text=content.strip(),
            model_type=model_type,
            model_name=model_name,
            observation_summary=_observation_summary(observation),
            runtime_metadata={},
            safety_notes=["no raw media persistence by default"],
        )


def _observation_summary(observation: PetObservation | None) -> dict:
    if observation is None:
        return {}
    return {
        "audio": dict(observation.audio),
        "vision": dict(observation.vision),
        "privacy": dict(observation.privacy),
    }
