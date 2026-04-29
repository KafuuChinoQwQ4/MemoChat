from __future__ import annotations


def __getattr__(name: str):
    if name == "HarnessContainer":
        from .container import HarnessContainer

        return HarnessContainer
    raise AttributeError(f"module 'harness.runtime' has no attribute {name!r}")

__all__ = ["HarnessContainer"]
