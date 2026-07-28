"""Rendered Helm workload security and writable-volume contracts."""

from __future__ import annotations

import shutil
import subprocess
from pathlib import Path

import pytest
import yaml

from tests.python.support.paths import repo_root

ROOT = repo_root()
CHART = ROOT / "infra" / "deploy" / "kubernetes" / "charts" / "memochat"
WORKLOAD_KINDS = {"Deployment", "StatefulSet", "Job", "DaemonSet"}
EXPECTED_UID = 10001
pytestmark = pytest.mark.skipif(shutil.which("helm") is None, reason="helm is required")


def render_all_workloads() -> list[dict]:
    """Render every optional chart workload without embedding production secrets."""
    command = [
        "helm",
        "template",
        "memochat",
        str(CHART),
        "-f",
        str(CHART / "values" / "dev.yaml"),
        "--set",
        "mesh.enabled=true",
        "--set",
        "mesh.provider=istio",
        "--set",
        "mesh.istio.mtlsMode=STRICT",
        "--set",
        "externalSecrets.enabled=true",
        "--set",
        "externalSecrets.relationTokensDistinct=true",
        "--set",
        "envoy.tls.secretName=memochat-tls",
        "--set",
        "legacyGate.enabled=true",
    ]
    result = subprocess.run(
        command,
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    assert result.returncode == 0, "Helm workload render failed"
    return [
        document
        for document in yaml.safe_load_all(result.stdout)
        if document and document.get("kind") in WORKLOAD_KINDS
    ]


def pod_containers(pod_spec: dict) -> list[dict]:
    return [
        *pod_spec.get("initContainers", []),
        *pod_spec.get("containers", []),
    ]


def volume_map(pod_spec: dict) -> dict[str, dict]:
    return {volume["name"]: volume for volume in pod_spec.get("volumes", [])}


def assert_secure_context(workload: dict) -> None:
    pod_spec = workload["spec"]["template"]["spec"]
    assert pod_spec.get("hostNetwork", False) is False
    assert pod_spec.get("hostPID", False) is False
    assert pod_spec.get("hostIPC", False) is False
    assert pod_spec.get("automountServiceAccountToken") is False
    pod_security = pod_spec["securityContext"]
    assert pod_security["runAsNonRoot"] is True
    assert pod_security["runAsUser"] == EXPECTED_UID
    assert pod_security["runAsGroup"] == EXPECTED_UID
    assert pod_security["seccompProfile"] == {"type": "RuntimeDefault"}

    containers = pod_containers(pod_spec)
    assert containers, workload["metadata"]["name"]
    for container in containers:
        security = container["securityContext"]
        assert security["privileged"] is False
        assert security["runAsNonRoot"] is True
        assert security["runAsUser"] == EXPECTED_UID
        assert security["runAsGroup"] == EXPECTED_UID
        assert security["allowPrivilegeEscalation"] is False
        assert security["readOnlyRootFilesystem"] is True
        assert security["seccompProfile"] == {"type": "RuntimeDefault"}
        assert set(security["capabilities"]["drop"]) == {"ALL"}
        assert not security["capabilities"].get("add")


def test_every_rendered_workload_has_non_root_immutable_security_context() -> None:
    workloads = render_all_workloads()

    assert len(workloads) >= 20
    for workload in workloads:
        with_name = f"{workload['kind']}/{workload['metadata']['name']}"
        try:
            assert_secure_context(workload)
        except (AssertionError, KeyError) as error:
            raise AssertionError(with_name) from error


def test_read_only_workloads_declare_only_explicit_writable_state_volumes() -> None:
    workloads = render_all_workloads()

    for workload in workloads:
        pod_spec = workload["spec"]["template"]["spec"]
        volumes = volume_map(pod_spec)
        for container in pod_containers(pod_spec):
            mounts = {mount["mountPath"]: mount for mount in container.get("volumeMounts", [])}
            assert "/tmp" in mounts
            assert "emptyDir" in volumes[mounts["/tmp"]["name"]]

        name = workload["metadata"]["name"]
        if name == "r18gateway":
            mounts = {mount["mountPath"]: mount for mount in pod_spec["containers"][0]["volumeMounts"]}
            r18_volume = volumes[mounts["/run/memochat/data/r18"]["name"]]
            assert "persistentVolumeClaim" in r18_volume
        if name == "mediagateway":
            mounts = {mount["mountPath"]: mount for mount in pod_spec["containers"][0]["volumeMounts"]}
            assert "/run/memochat/uploads" in mounts
            assert "/data/uploads" in mounts
        if name == "gateserver":
            mounts = {mount["mountPath"]: mount for mount in pod_spec["containers"][0]["volumeMounts"]}
            assert "/data/uploads" in mounts
        if name == "ai-orchestrator":
            mounts = {mount["mountPath"]: mount for mount in pod_spec["containers"][0]["volumeMounts"]}
            assert {"/app/.cache", "/app/.data"}.issubset(mounts)

        for volume in volumes.values():
            assert "hostPath" not in volume


def test_local_media_root_rejects_a_broad_root_mount() -> None:
    command = [
        "helm",
        "template",
        "memochat",
        str(CHART),
        "-f",
        str(CHART / "values" / "dev.yaml"),
        "--set",
        "mesh.enabled=true",
        "--set",
        "mesh.provider=istio",
        "--set",
        "mesh.istio.mtlsMode=STRICT",
        "--set",
        "externalSecrets.enabled=true",
        "--set",
        "externalSecrets.relationTokensDistinct=true",
        "--set",
        "envoy.tls.secretName=memochat-tls",
        "--set",
        "legacyGate.enabled=true",
        "--set",
        "gate.media.rootPath=/",
    ]
    result = subprocess.run(
        command,
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    assert result.returncode != 0
    assert "gate.media.rootPath must be a non-root absolute path" in result.stdout
