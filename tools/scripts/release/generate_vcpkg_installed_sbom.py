#!/usr/bin/env python3
"""Build a deterministic SPDX document for a validated vcpkg triplet closure."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
from pathlib import Path
from typing import Any

PACKAGE_NAME_PATTERN = re.compile(r"[a-z0-9][a-z0-9+.-]*")
TRIPLET_PATTERN = re.compile(r"[A-Za-z0-9][A-Za-z0-9._-]*")
SOURCE_SHA_PATTERN = re.compile(r"[0-9a-f]{40}")


def fail(message: str) -> None:
    raise SystemExit(f"generate_vcpkg_installed_sbom.py: [FAIL] {message}")


def parse_status(path: Path, triplet: str) -> dict[str, dict[str, str]]:
    if not path.is_file() or path.is_symlink():
        fail(f"vcpkg status database is missing or unsafe: {path}")
    try:
        paragraphs = path.read_text(encoding="utf-8").split("\n\n")
    except (OSError, UnicodeError) as error:
        fail(f"could not read vcpkg status database: {error}")

    packages: dict[str, dict[str, str]] = {}
    for paragraph in paragraphs:
        fields: dict[str, str] = {}
        for line in paragraph.splitlines():
            if not line or line[0].isspace() or ": " not in line:
                continue
            key, value = line.split(": ", 1)
            fields[key] = value
        if (
            fields.get("Architecture") != triplet
            or fields.get("Status") != "install ok installed"
            or "Feature" in fields
        ):
            continue
        name = fields.get("Package", "")
        version = fields.get("Version", "")
        if PACKAGE_NAME_PATTERN.fullmatch(name) is None or not version:
            fail("vcpkg status contains an invalid installed package record")
        if name in packages:
            fail(f"vcpkg status repeats installed base package: {name}")
        packages[name] = fields
    if not packages:
        fail(f"vcpkg status has no installed base packages for triplet {triplet}")
    return packages


def load_primary_package(path: Path, package_name: str, expected_version: str) -> dict[str, Any]:
    if not path.is_file() or path.is_symlink() or path.stat().st_size == 0:
        fail(f"vcpkg SPDX is missing or unsafe for package {package_name}")
    try:
        document = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        fail(f"vcpkg SPDX is invalid for package {package_name}: {error}")
    if not isinstance(document, dict) or document.get("spdxVersion") not in {"SPDX-2.2", "SPDX-2.3"}:
        fail(f"vcpkg SPDX has an unsupported document for package {package_name}")
    rows = document.get("packages")
    if not isinstance(rows, list):
        fail(f"vcpkg SPDX has no package array for {package_name}")
    matches = [row for row in rows if isinstance(row, dict) and row.get("name") == package_name]
    if len(matches) != 1:
        fail(f"vcpkg SPDX must describe package {package_name} exactly once")
    package = matches[0]
    if package.get("versionInfo") != expected_version:
        fail(
            f"vcpkg SPDX version mismatch for {package_name}: "
            f"expected {expected_version}, found {package.get('versionInfo')!r}"
        )
    return package


def string_or_default(value: object, default: str = "NOASSERTION") -> str:
    return value if isinstance(value, str) and value else default


def build_document(installed_root: Path, triplet: str, source_sha: str) -> dict[str, Any]:
    status_packages = parse_status(installed_root / "vcpkg/status", triplet)
    share_root = installed_root / triplet / "share"
    if not share_root.is_dir() or share_root.is_symlink():
        fail(f"vcpkg share root is missing or unsafe: {share_root}")
    if any(path.is_symlink() for path in share_root.rglob("*")):
        fail("vcpkg share root contains a symlink")

    spdx_paths = {
        path.parent.name: path
        for path in share_root.glob("*/vcpkg.spdx.json")
        if path.is_file() and not path.is_symlink()
    }
    if set(spdx_paths) != set(status_packages):
        missing = sorted(set(status_packages) - set(spdx_paths))
        extra = sorted(set(spdx_paths) - set(status_packages))
        fail(
            "installed package/SPDX set mismatch"
            f"; missing={','.join(missing) or 'none'}"
            f"; extra={','.join(extra) or 'none'}"
        )

    input_rows: list[str] = []
    packages: list[dict[str, Any]] = []
    relationships: list[dict[str, str]] = []
    for package_name in sorted(status_packages):
        fields = status_packages[package_name]
        version = fields["Version"]
        if fields.get("Port-Version") not in {None, "", "0"}:
            version = f"{version}#{fields['Port-Version']}"
        spdx_path = spdx_paths[package_name]
        spdx_sha256 = hashlib.sha256(spdx_path.read_bytes()).hexdigest()
        relative_spdx = spdx_path.relative_to(installed_root).as_posix()
        input_rows.append(f"{spdx_sha256}  {relative_spdx}\n")
        primary = load_primary_package(spdx_path, package_name, version)
        package_id = f"SPDXRef-Package-{package_name}-{spdx_sha256[:12]}"
        package: dict[str, Any] = {
            "name": package_name,
            "SPDXID": package_id,
            "versionInfo": version,
            "downloadLocation": string_or_default(primary.get("downloadLocation")),
            "filesAnalyzed": False,
            "licenseConcluded": string_or_default(primary.get("licenseConcluded")),
            "licenseDeclared": string_or_default(primary.get("licenseDeclared")),
            "copyrightText": string_or_default(primary.get("copyrightText")),
            "sourceInfo": (
                f"Validated from {relative_spdx}; input_sha256={spdx_sha256}; "
                f"vcpkg_abi={fields.get('Abi', 'unavailable')}"
            ),
        }
        if isinstance(primary.get("externalRefs"), list):
            package["externalRefs"] = primary["externalRefs"]
        packages.append(package)
        relationships.append(
            {
                "spdxElementId": "SPDXRef-DOCUMENT",
                "relationshipType": "DESCRIBES",
                "relatedSpdxElement": package_id,
            }
        )

    inputs_sha256 = hashlib.sha256("".join(input_rows).encode("utf-8")).hexdigest()
    return {
        "spdxVersion": "SPDX-2.3",
        "dataLicense": "CC0-1.0",
        "SPDXID": "SPDXRef-DOCUMENT",
        "name": f"MemoChat vcpkg installed closure ({triplet})",
        "documentNamespace": (f"https://memochat.invalid/spdx/vcpkg-installed-closure/{source_sha}/{inputs_sha256}"),
        "documentComment": (f"release_source_sha={source_sha}; coverage=installed-closure-overapproximation"),
        "creationInfo": {
            "created": "1970-01-01T00:00:00Z",
            "creators": ["Tool: MemoChat-generate-vcpkg-installed-sbom"],
            "comment": (
                "Fail-closed over-approximation of every installed base package in the release "
                f"vcpkg triplet; input_set_sha256={inputs_sha256}"
            ),
        },
        "packages": packages,
        "relationships": relationships,
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--installed-root", required=True, type=Path)
    parser.add_argument("--triplet", required=True)
    parser.add_argument("--source-sha", default="unbound")
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()

    if TRIPLET_PATTERN.fullmatch(args.triplet) is None:
        fail(f"unsafe vcpkg triplet: {args.triplet}")
    if args.source_sha != "unbound" and SOURCE_SHA_PATTERN.fullmatch(args.source_sha) is None:
        fail("--source-sha must be 'unbound' or a lowercase 40-character Git commit SHA")
    try:
        installed_root = args.installed_root.resolve(strict=True)
    except OSError as error:
        fail(f"vcpkg installed root cannot be resolved: {error}")
    if not installed_root.is_dir() or args.installed_root.is_symlink():
        fail("vcpkg installed root must be a real directory")
    output = args.output.resolve(strict=False)
    if not output.parent.is_dir() or output.exists() or output.is_symlink():
        fail("output must be a new file below an existing directory")
    if output == installed_root or installed_root in output.parents:
        fail("output cannot be written inside the verified vcpkg installed root")

    payload = (
        json.dumps(
            build_document(installed_root, args.triplet, args.source_sha),
            indent=2,
            sort_keys=True,
        )
        + "\n"
    )
    descriptor = os.open(output, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o444)
    try:
        with os.fdopen(descriptor, "w", encoding="utf-8", newline="\n") as stream:
            stream.write(payload)
    except BaseException:
        output.unlink(missing_ok=True)
        raise
    print(f"[OK] vcpkg installed closure SPDX: {output}")


if __name__ == "__main__":
    main()
