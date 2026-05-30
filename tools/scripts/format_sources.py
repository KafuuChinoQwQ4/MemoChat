#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import sysconfig
from pathlib import Path
from typing import Iterable

CPP_SUFFIXES = {
    ".c",
    ".cc",
    ".cpp",
    ".cxx",
    ".h",
    ".hh",
    ".hpp",
    ".hxx",
    ".ipp",
    ".inl",
}
PYTHON_SUFFIXES = {".py"}

EXCLUDED_PARTS = {
    ".ai",
    ".cache",
    ".git",
    ".mypy_cache",
    ".pytest_cache",
    ".ruff_cache",
    ".venv",
    "__pycache__",
    "build",
    "htmlcov",
    "node_modules",
    "vcpkg",
    "vcpkg_installed",
}
EXCLUDED_PREFIXES = ("build-",)
EXCLUDED_PATH_PREFIXES = (
    ("Memo_ops", "runtime"),
    ("Memo_ops", "artifacts"),
    ("infra", "Memo_ops", "runtime"),
    ("infra", "Memo_ops", "artifacts"),
    ("apps", "server", "core", "AIOrchestrator", ".cache"),
    ("apps", "server", "core", "AIOrchestrator", ".data"),
    ("apps", "server", "core", "AIOrchestrator", ".venv"),
)
SOURCE_ROOTS = ("apps", "infra", "tests", "tools")


def repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def is_excluded(path: Path) -> bool:
    parts = path.parts
    if any(part in EXCLUDED_PARTS or part.startswith(EXCLUDED_PREFIXES) for part in parts):
        return True
    return any(parts[: len(prefix)] == prefix for prefix in EXCLUDED_PATH_PREFIXES)


def iter_source_files(root: Path, suffixes: set[str]) -> list[Path]:
    files: list[Path] = []
    for source_root in SOURCE_ROOTS:
        directory = root / source_root
        if not directory.exists():
            continue
        for path in directory.rglob("*"):
            if not path.is_file() or path.suffix.lower() not in suffixes:
                continue
            relative = path.relative_to(root)
            if is_excluded(relative):
                continue
            files.append(relative)
    return sorted(files, key=lambda item: item.as_posix())


def chunked(items: list[Path], size: int) -> Iterable[list[Path]]:
    for index in range(0, len(items), size):
        yield items[index : index + size]


def find_clang_format() -> str:
    candidates = [shutil.which("clang-format")]
    scripts = sysconfig.get_path("scripts")
    if scripts:
        candidates.append(str(Path(scripts) / "clang-format"))
        candidates.append(str(Path(scripts) / "clang-format.exe"))

    user_base = sysconfig.get_config_var("userbase")
    if user_base:
        candidates.append(str(Path(user_base) / "bin" / "clang-format"))
        candidates.append(str(Path(user_base) / "Scripts" / "clang-format.exe"))
        candidates.append(
            str(
                Path(user_base)
                / f"Python{sys.version_info.major}{sys.version_info.minor}"
                / "Scripts"
                / "clang-format.exe"
            )
        )

    for candidate in candidates:
        if candidate and Path(candidate).exists():
            return candidate
    return "clang-format"


def run(command: list[str], cwd: Path) -> None:
    print("+", " ".join(command), flush=True)
    subprocess.run(command, cwd=cwd, check=True)


def format_cpp(root: Path, clang_format: str, check: bool) -> None:
    files = iter_source_files(root, CPP_SUFFIXES)
    if not files:
        print("No C/C++ files found for formatting.")
        return

    base_command = [clang_format]
    if check:
        base_command += ["--dry-run", "--Werror"]
    else:
        base_command += ["-i"]

    for group in chunked(files, 80):
        run(base_command + [path.as_posix() for path in group], root)


def format_python(root: Path, python: str, check: bool) -> None:
    files = iter_source_files(root, PYTHON_SUFFIXES)
    if not files:
        print("No Python files found for formatting.")
        return

    for group in chunked(files, 80):
        paths = [path.as_posix() for path in group]
        if check:
            run([python, "-m", "ruff", "check", "--select", "I", *paths], root)
            run([python, "-m", "ruff", "format", "--check", *paths], root)
        else:
            run([python, "-m", "ruff", "check", "--fix", "--select", "I", *paths], root)
            run([python, "-m", "ruff", "format", *paths], root)


def main() -> int:
    parser = argparse.ArgumentParser(description="Format MemoChat source files.")
    parser.add_argument("--mode", choices=("all", "cpp", "python"), default="all")
    parser.add_argument("--check", action="store_true", help="Check formatting without modifying files.")
    parser.add_argument("--clang-format", default=os.environ.get("CLANG_FORMAT") or find_clang_format())
    parser.add_argument("--python", default=sys.executable)
    args = parser.parse_args()

    root = repo_root()
    if args.mode in {"all", "cpp"}:
        format_cpp(root, args.clang_format, args.check)
    if args.mode in {"all", "python"}:
        format_python(root, args.python, args.check)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
