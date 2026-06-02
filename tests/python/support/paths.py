from __future__ import annotations

from pathlib import Path


def repo_root(start: Path | None = None) -> Path:
    current = (start or Path(__file__)).resolve()
    for candidate in (current, *current.parents):
        if (
            (candidate / "CMakeLists.txt").is_file()
            and (candidate / "CMakePresets.json").is_file()
            and (candidate / "pytest.ini").is_file()
        ):
            return candidate
    raise RuntimeError(f"Could not find MemoChat repository root from {current}")


def source_path(*parts: str) -> Path:
    return repo_root().joinpath(*parts)


def ai_orchestrator_root() -> Path:
    return source_path("apps", "server", "core", "AIOrchestrator")
