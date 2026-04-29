from __future__ import annotations


def __getattr__(name: str):
    if name == "SkillRegistry":
        from .registry import SkillRegistry

        return SkillRegistry
    raise AttributeError(f"module 'harness.skills' has no attribute {name!r}")

__all__ = ["SkillRegistry"]
