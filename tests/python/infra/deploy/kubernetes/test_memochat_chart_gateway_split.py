from pathlib import Path

ROOT = Path(__file__).resolve().parents[5]
CHART = ROOT / "infra" / "deploy" / "kubernetes" / "charts" / "memochat"

FOCUSED_GATEWAYS = {
    "aigateway": {
        "cluster": "aigateway_backend",
        "service": "aigateway",
        "config": "aigateway.ini",
        "section": "[AIGateway]",
        "port": "8093",
    },
    "mediagateway": {
        "cluster": "mediagateway_backend",
        "service": "mediagateway",
        "config": "mediagateway.ini",
        "section": "[MediaGateway]",
        "port": "8094",
    },
    "momentsgateway": {
        "cluster": "momentsgateway_backend",
        "service": "momentsgateway",
        "config": "momentsgateway.ini",
        "section": "[MomentsGateway]",
        "port": "8099",
    },
    "callgateway": {
        "cluster": "callgateway_backend",
        "service": "callgateway",
        "config": "callgateway.ini",
        "section": "[CallGateway]",
        "port": "8097",
    },
    "r18gateway": {
        "cluster": "r18gateway_backend",
        "service": "r18gateway",
        "config": "r18gateway.ini",
        "section": "[R18Gateway]",
        "port": "8098",
    },
    "register": {
        "cluster": "register_backend",
        "service": "register",
        "config": "register.ini",
        "section": "[Register]",
        "port": "8101",
    },
    "login": {
        "cluster": "login_backend",
        "service": "login",
        "config": "login.ini",
        "section": "[Login]",
        "port": "8102",
    },
    "account": {
        "cluster": "account_backend",
        "service": "account",
        "config": "account.ini",
        "section": "[Account]",
        "port": "8103",
    },
}


def read_chart_file(relative: str) -> str:
    return (CHART / relative).read_text(encoding="utf-8")


def test_values_model_all_focused_gateway_services() -> None:
    values = read_chart_file("values.yaml")

    assert "focusedGateways:" in values
    assert "legacyGate:\n  enabled: false" in values
    assert "ingress:\n    enabled: false" in values
    for name, expected in FOCUSED_GATEWAYS.items():
        assert f"{name}:" in values
        assert f"serviceName: {expected['service']}" in values
        assert f"configKey: {expected['config']}" in values
        assert f"port: {expected['port']}" in values


def test_focused_gateway_configs_are_renderable_from_chart_configmap() -> None:
    configmap = read_chart_file("templates/bootstrap/configmap-services.yaml")

    for expected in FOCUSED_GATEWAYS.values():
        assert f"{expected['config']}: |" in configmap
        assert expected["section"] in configmap
        assert f"Port={{{{ .Values.focusedGateways.services.{expected['service']}.port }}}}" in configmap


def test_kubernetes_envoy_routes_each_domain_to_focused_cluster() -> None:
    envoy = read_chart_file("templates/lb/envoy.yaml")

    route_expectations = {
        "aigateway_backend": ("prefix: /ai/",),
        "mediagateway_backend": ("prefix: /upload_media", "prefix: /media/"),
        "momentsgateway_backend": ("prefix: /api/moments/",),
        "callgateway_backend": ("prefix: /api/call/",),
        "r18gateway_backend": ("prefix: /api/r18/",),
        "login_backend": ("path: /user_login",),
        "register_backend": ("path: /get_varifycode", "path: /user_register", "path: /reset_pwd"),
        "account_backend": ("path: /user_update_profile",),
    }

    for cluster, route_tokens in route_expectations.items():
        assert f"cluster: {cluster}" in envoy
        assert f"name: {cluster}" in envoy
        for token in route_tokens:
            assert token in envoy

    assert "{{- if .Values.focusedGateways.enabled }}" in envoy
    assert "{{- if .Values.legacyGate.enabled }}" in envoy


def test_direct_gate_ingress_requires_explicit_legacy_gate() -> None:
    gate = read_chart_file("templates/prod/gate.yaml")

    assert "{{- if and .Values.legacyGate.enabled .Values.gate.ingress.enabled }}" in gate


def test_focused_gateway_workload_template_mounts_service_configs() -> None:
    focused = read_chart_file("templates/prod/focused-gateways.yaml")

    assert "{{- if .Values.focusedGateways.enabled }}" in focused
    assert "name: {{ $svc.serviceName }}" in focused
    assert "subPath: {{ $svc.configKey }}" in focused
    assert "app.kubernetes.io/component: {{ $svc.serviceName }}" in focused
    assert "value: /app/{{ $svc.target }}" in focused
    for name, expected in FOCUSED_GATEWAYS.items():
        assert f'"{name}"' in focused


def test_ai_values_model_aiserver_and_ai_orchestrator_as_first_class_services() -> None:
    values = read_chart_file("values.yaml")

    for token in (
        "aiServer:",
        "repository: memochat/aiserver",
        "serviceName: aiserver",
        "grpcPort: 8095",
        "aiOrchestrator:",
        "repository: memochat/ai-orchestrator",
        "serviceName: ai-orchestrator",
        "port: 8096",
        "qdrantHost: qdrant.example.internal",
        "neo4jHost: neo4j.example.internal",
    ):
        assert token in values


def test_ai_config_chain_routes_aigateway_to_aiserver_to_orchestrator() -> None:
    configmap = read_chart_file("templates/bootstrap/configmap-services.yaml")

    assert "aigateway.ini: |" in configmap
    assert "[AIServer]" in configmap
    assert "Host={{ .Values.aiServer.serviceName }}" in configmap
    assert "Port={{ .Values.aiServer.service.grpcPort }}" in configmap
    assert "aiserver.ini: |" in configmap
    assert "[AIServer]\n    Host=0.0.0.0" in configmap
    assert "Host={{ .Values.aiOrchestrator.serviceName }}" in configmap
    assert "Port={{ .Values.aiOrchestrator.service.port }}" in configmap
    assert "ai-orchestrator.yaml: |" in configmap
    assert 'host: "0.0.0.0"' in configmap
    assert "port: {{ .Values.aiOrchestrator.service.port }}" in configmap
    assert 'host: "{{ .Values.aiOrchestrator.dependencies.qdrantHost }}"' in configmap


def test_ai_workloads_render_aiserver_and_orchestrator_services() -> None:
    ai = read_chart_file("templates/prod/ai.yaml")

    for token in (
        "{{- if .Values.aiServer.enabled }}",
        "name: {{ .Values.aiServer.serviceName }}",
        "value: /app/AIServer",
        "subPath: aiserver.ini",
        "containerPort: {{ .Values.aiServer.service.grpcPort }}",
        "{{- if .Values.aiOrchestrator.enabled }}",
        "name: {{ .Values.aiOrchestrator.serviceName }}",
        "subPath: ai-orchestrator.yaml",
        "httpGet: { path: /health, port: http }",
        "containerPort: {{ .Values.aiOrchestrator.service.port }}",
    ):
        assert token in ai


def test_ai_stream_envoy_route_has_long_timeout_and_no_buffering_header() -> None:
    envoy = read_chart_file("templates/lb/envoy.yaml")

    assert "prefix: /ai/chat/stream" in envoy
    assert "cluster: aigateway_backend" in envoy
    assert "timeout: 600s" in envoy
    assert "key: X-Accel-Buffering" in envoy
    assert 'value: "no"' in envoy
