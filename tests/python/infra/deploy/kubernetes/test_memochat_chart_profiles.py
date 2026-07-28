import shutil
import subprocess
from pathlib import Path

import pytest
import yaml

ROOT = Path(__file__).resolve().parents[5]
CHART = ROOT / "infra" / "deploy" / "kubernetes" / "charts" / "memochat"
PROD_VALUES = CHART / "values" / "prod.yaml"
PROFILES = CHART / "values" / "profiles"

pytestmark = pytest.mark.skipif(shutil.which("helm") is None, reason="helm is required")


def render_values(*values_files: Path) -> list[dict]:
    command = [
        "helm",
        "template",
        "memochat",
        str(CHART),
    ]
    for values_file in values_files:
        command.extend(("-f", str(values_file)))
    command.extend(
        (
            "--set",
            "externalSecrets.enabled=true",
            "--set",
            "externalSecrets.relationTokensDistinct=true",
            "--set",
            "envoy.tls.secretName=fixture-tls",
            "--set",
            "images.releaseTag=sha-0123456789abcdef0123456789abcdef01234567",
        )
    )
    result = subprocess.run(
        command,
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    assert result.returncode == 0, result.stdout
    return [
        document
        for document in yaml.safe_load_all(result.stdout)
        if isinstance(document, dict) and document.get("kind") and document.get("metadata")
    ]


def render_profile(profile: str) -> list[dict]:
    return render_values(PROD_VALUES, PROFILES / f"{profile}.yaml")


def index_resources(documents: list[dict]) -> dict[tuple[str, str], dict]:
    return {(document["kind"], document["metadata"]["name"]): document for document in documents}


@pytest.mark.parametrize(
    ("profile", "expected", "forbidden"),
    (
        (
            "base",
            {
                ("Deployment", "aigateway"),
                ("Deployment", "momentsgateway"),
                ("Service", "chat-relation-query"),
                ("Deployment", "chat-relation-service"),
                ("Service", "chat-relation-service"),
            },
            {
                ("Deployment", "callgateway"),
                ("Deployment", "r18gateway"),
                ("PersistentVolumeClaim", "memochat-r18-data"),
                ("Deployment", "otel-collector"),
                ("Service", "otel-collector"),
                ("ConfigMap", "otel-collector-config"),
                ("ServiceMonitor", "otel-collector"),
                ("Namespace", "memochat-observability"),
            },
        ),
        (
            "calls",
            {
                ("Deployment", "callgateway"),
                ("Service", "callgateway"),
                ("Deployment", "chat-relation-query"),
            },
            {
                ("Deployment", "r18gateway"),
                ("PersistentVolumeClaim", "memochat-r18-data"),
                ("Deployment", "otel-collector"),
                ("ConfigMap", "otel-collector-config"),
                ("ServiceMonitor", "otel-collector"),
            },
        ),
        (
            "r18",
            {
                ("Deployment", "r18gateway"),
                ("Service", "r18gateway"),
                ("PersistentVolumeClaim", "memochat-r18-data"),
            },
            {
                ("Deployment", "callgateway"),
                ("Service", "callgateway"),
                ("Deployment", "otel-collector"),
                ("ConfigMap", "otel-collector-config"),
                ("ServiceMonitor", "otel-collector"),
            },
        ),
        (
            "observability",
            {
                ("Namespace", "memochat-observability"),
                ("Deployment", "otel-collector"),
                ("Service", "otel-collector"),
                ("ConfigMap", "otel-collector-config"),
                ("ServiceMonitor", "otel-collector"),
            },
            {
                ("Deployment", "callgateway"),
                ("Deployment", "r18gateway"),
                ("PersistentVolumeClaim", "memochat-r18-data"),
            },
        ),
    ),
)
def test_production_profiles_render_only_their_opt_in_resources(
    profile: str,
    expected: set[tuple[str, str]],
    forbidden: set[tuple[str, str]],
) -> None:
    resources = index_resources(render_profile(profile))

    for resource in expected:
        assert resource in resources
    for resource in forbidden:
        assert resource not in resources


def test_prod_values_alone_default_to_the_same_minimal_base() -> None:
    resources = index_resources(render_values(PROD_VALUES))

    for resource in (
        ("Deployment", "callgateway"),
        ("Deployment", "r18gateway"),
        ("PersistentVolumeClaim", "memochat-r18-data"),
        ("Deployment", "otel-collector"),
        ("ConfigMap", "otel-collector-config"),
        ("ServiceMonitor", "otel-collector"),
        ("Namespace", "memochat-observability"),
    ):
        assert resource not in resources


def test_r18_profile_keeps_encrypted_persistence_and_key_injection() -> None:
    resources = index_resources(render_profile("r18"))

    pvc = resources[("PersistentVolumeClaim", "memochat-r18-data")]
    assert pvc["spec"]["accessModes"] == ["ReadWriteMany"]

    deployment = resources[("Deployment", "r18gateway")]
    env = deployment["spec"]["template"]["spec"]["containers"][0]["env"]
    key_env = next(item for item in env if item["name"] == "MEMOCHAT_R18_CREDENTIAL_MASTER_KEY")
    assert key_env["valueFrom"]["secretKeyRef"]["key"] == "r18-credential-master-key"


def test_disabled_optional_gateways_are_absent_from_envoy_routes_and_clusters() -> None:
    resources = index_resources(render_profile("base"))
    envoy = resources[("ConfigMap", "envoy-lb-config")]["data"]["envoy.yaml"]

    assert "cluster: callgateway_backend" not in envoy
    assert "cluster: r18gateway_backend" not in envoy
    assert "prefix: /api/call/" not in envoy
    assert "prefix: /api/r18/" not in envoy


def test_enabled_optional_gateways_add_their_envoy_route_and_cluster() -> None:
    calls = index_resources(render_profile("calls"))[("ConfigMap", "envoy-lb-config")]["data"]["envoy.yaml"]
    r18 = index_resources(render_profile("r18"))[("ConfigMap", "envoy-lb-config")]["data"]["envoy.yaml"]

    assert "cluster: callgateway_backend" in calls
    assert "prefix: /api/call/" in calls
    assert "cluster: r18gateway_backend" not in calls
    assert "cluster: r18gateway_backend" in r18
    assert "prefix: /api/r18/" in r18
    assert "cluster: callgateway_backend" not in r18


def test_observability_profile_controls_service_telemetry_switches() -> None:
    base_config = index_resources(render_profile("base"))[("ConfigMap", "memochat-config")]["data"]
    observability_config = index_resources(render_profile("observability"))[("ConfigMap", "memochat-config")]["data"]

    assert "[Telemetry]\nEnabled=false" in base_config["chat.ini"]
    assert "[Telemetry]\nEnabled=true" in observability_config["chat.ini"]
