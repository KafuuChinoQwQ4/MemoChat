#!/usr/bin/env python3
"""Hash a Git source tree while excluding the self-referential legal corpus."""

from __future__ import annotations

import argparse
import hashlib
import re
import subprocess
import sys
from pathlib import Path

EXCLUDED_PREFIX = b"legal/third-party/"


def fail(message: str) -> None:
    raise SystemExit(f"compute_release_source_snapshot.py: {message}")


def git_output(project_root: Path, *arguments: str) -> bytes:
    try:
        return subprocess.run(
            ["git", "-C", str(project_root), *arguments],
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        ).stdout
    except FileNotFoundError as error:
        fail("git is required")
        raise AssertionError from error
    except subprocess.CalledProcessError as error:
        detail = error.stderr.decode("utf-8", errors="replace").strip()
        fail(detail or f"git {' '.join(arguments)} failed")
        raise AssertionError from error


def source_snapshot_sha256(project_root: Path, source_sha: str) -> str:
    tree = git_output(project_root, "ls-tree", "-rz", "--full-tree", source_sha)
    records = tree.split(b"\0")
    if not records or records[-1] != b"":
        fail("Git tree listing is not NUL terminated")

    digest = hashlib.sha256()
    for record in records[:-1]:
        try:
            _, path = record.split(b"\t", 1)
        except ValueError as error:
            fail("Git tree listing contains a malformed record")
            raise AssertionError from error
        if path.startswith(EXCLUDED_PREFIX):
            continue
        digest.update(record)
        digest.update(b"\0")
    return digest.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--project-root", type=Path, required=True)
    parser.add_argument("--source-sha", required=True)
    arguments = parser.parse_args()

    try:
        project_root = arguments.project_root.resolve(strict=True)
    except OSError as error:
        fail(f"project root does not exist: {error}")
    if not project_root.is_dir():
        fail("project root is not a directory")
    if re.fullmatch(r"(?:[0-9a-f]{40}|[0-9a-f]{64})", arguments.source_sha) is None:
        fail("source SHA must be a lowercase 40- or 64-character Git object ID")

    git_root = Path(git_output(project_root, "rev-parse", "--show-toplevel").decode("utf-8").strip()).resolve(
        strict=True
    )
    if git_root != project_root:
        fail("project root must equal the Git checkout root")
    git_output(project_root, "cat-file", "-e", f"{arguments.source_sha}^{{commit}}")

    print(source_snapshot_sha256(project_root, arguments.source_sha))
    return 0


if __name__ == "__main__":
    sys.exit(main())
