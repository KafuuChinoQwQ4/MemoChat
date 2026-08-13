import shutil
import subprocess
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


def render_chart(
    *set_values: str,
    values_files: tuple[Path, ...] = (),
) -> subprocess.CompletedProcess[str]:
    command = ["helm", "template", "memochat", str(CHART)]
    for values_file in values_files:
        command.extend(("-f", str(values_file)))
    for value in set_values:
        command.extend(("--set", value))
    return subprocess.run(
        command,
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )


def test_values_model_all_focused_gateway_services() -> None:
    values = read_chart_file("values.yaml")

    assert "focusedGateways:" in values
    assert "legacyGate:\n  enabled: false" in values
    assert "ingress:\n    enabled: false" in values
    service_model = values.split("\nfocusedGateways:\n", 1)[1]
    for name, expected in FOCUSED_GATEWAYS.items():
        assert f"{name}:" in service_model
        service_values = service_model.split(f"    {name}:\n", 1)[1].split("\n    ", 1)[0]
        assert "      enabled: true" in service_values
        assert f"serviceName: {expected['service']}" in values
        assert f"configKey: {expected['config']}" in values
        assert f"port: {expected['port']}" in values


def test_focused_gateway_configs_are_renderable_from_chart_configmap() -> None:
    configmap = read_chart_file("templates/bootstrap/configmap-services.yaml")

    for expected in FOCUSED_GATEWAYS.values():
        assert f"{expected['config']}: |" in configmap
        assert expected["section"] in configmap
        assert f"Port={{{{ .Values.focusedGateways.services.{expected['service']}.port }}}}" in configmap


def test_r18_gateway_uses_account_policy_db_and_operator_only_source_secret() -> None:
    configmap = read_chart_file("templates/bootstrap/configmap-services.yaml")
    focused = read_chart_file("templates/prod/focused-gateways.yaml")
    secret = read_chart_file("templates/bootstrap/secret.yaml")

    r18_config = configmap.split("  r18gateway.ini: |", 1)[1].split("  register.ini: |", 1)[0]
    assert "User={{ .Values.externalServices.postgres.roles.account }}" in r18_config
    assert "Database=memo_account" in r18_config
    assert "[R18SourceAdmin]" in r18_config
    assert "AdminKey=" in r18_config
    assert "[R18Picacg]" in r18_config
    assert "allowedImageHosts" in r18_config
    assert "MEMOCHAT_R18SOURCEADMIN_ADMINKEY" in focused
    assert "r18-source-admin-key" in focused
    assert "secrets.r18SourceAdminKey" in secret


def test_r18_account_policy_schema_is_migrated_before_gateway_rollout() -> None:
    init = read_chart_file("templates/bootstrap/configmap-ops-init.yaml")
    jobs = read_chart_file("templates/bootstrap/jobs.yaml")
    entrypoint = (ROOT / "apps" / "server" / "core" / "R18Service" / "app" / "R18GatewayServer.cpp").read_text(
        encoding="utf-8"
    )

    for token in (
        "009_memo_account_schema.sql: |",
        "013_r18_access_policy.sql: |",
        "adult_attested_at_ms",
        "r18_access_state",
        "ck_user_r18_access_state",
    ):
        assert token in init
    assert "-d memo_account" in jobs
    assert "-f /migrations/009_memo_account_schema.sql" in jobs
    assert "-f /migrations/013_r18_access_policy.sql" in jobs
    assert "-v ON_ERROR_STOP=1" in jobs
    assert "PostgresReadinessProbe()" in entrypoint
    assert "RedisReadinessProbe()" in entrypoint


def test_account_focused_services_do_not_restore_rabbit_cache_invalidation() -> None:
    configmap = read_chart_file("templates/bootstrap/configmap-services.yaml")
    focused = read_chart_file("templates/prod/focused-gateways.yaml")

    blocks = (
        configmap.split("  register.ini: |", 1)[1].split("  login.ini: |", 1)[0],
        configmap.split("  login.ini: |", 1)[1].split("  account.ini: |", 1)[0],
        configmap.split("  account.ini: |", 1)[1].split("  chatrelationquery.ini: |", 1)[0],
    )
    for block in blocks:
        assert "[RabbitMQ]" not in block
    assert "MEMOCHAT_RABBITMQ_PASSWORD" not in focused


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


def test_relation_service_values_are_first_class_release_inputs() -> None:
    values = read_chart_file("values.yaml")

    for token in (
        "chatRelationQuery:\n    repository: ghcr.io/kafuuchinoqwq4/memochat/chat-relation-query-service",
        "chatRelationService:\n    repository: ghcr.io/kafuuchinoqwq4/memochat/chat-relation-service-worker",
        "relationQuery:\n    enabled: true",
        "serviceName: chat-relation-query",
        "configKey: chatrelationquery.ini",
        "grpcPort: 50090",
        "relationService:\n    enabled: true",
        "serviceName: chat-relation-service",
        "configKey: chatrelationservice.ini",
        "grpcPort: 50091",
    ):
        assert token in values


def test_call_gateway_config_routes_relation_checks_to_the_internal_service() -> None:
    configmap = read_chart_file("templates/bootstrap/configmap-services.yaml")
    call_config = configmap.split("  callgateway.ini: |", 1)[1].split("  r18gateway.ini: |", 1)[0]

    for token in (
        "[AccountPostgres]",
        "User={{ .Values.externalServices.postgres.roles.account }}",
        "Database=memo_account",
        "[RelationQueryService]",
        "Endpoint={{ .Values.chat.relationQuery.serviceName }}.{{ .Values.namespaces.prod }}.svc.{{ .Values.global.clusterDomain }}:{{ .Values.chat.relationQuery.grpcPort }}",
    ):
        assert token in call_config


def test_relation_query_workload_and_call_dependency_fail_closed() -> None:
    configmap = read_chart_file("templates/bootstrap/configmap-services.yaml")
    chat = read_chart_file("templates/prod/chat.yaml")
    focused = read_chart_file("templates/prod/focused-gateways.yaml")

    relation_config = configmap.split("  chatrelationquery.ini: |", 1)[1].split("  chat.ini: |", 1)[0]
    for token in (
        "[RelationQueryRpc]",
        "Host=0.0.0.0",
        "Port={{ .Values.chat.relationQuery.grpcPort }}",
        "[Postgres]",
        "[AccountPostgres]",
        "[Redis]",
    ):
        assert token in relation_config

    for token in (
        "{{- if .Values.chat.relationQuery.enabled }}",
        "kind: Deployment",
        "name: {{ .Values.chat.relationQuery.serviceName }}",
        'image: {{ include "memochat.image" (dict "root" . "image" .Values.images.chatRelationQuery) | quote }}',
        "value: ChatRelationQueryService",
        "value: MEMOCHAT_POSTGRES_PASSWD MEMOCHAT_ACCOUNTPOSTGRES_PASSWD MEMOCHAT_REDIS_PASSWD",
        "name: MEMOCHAT_ACCOUNTPOSTGRES_PASSWD",
        "subPath: {{ .Values.chat.relationQuery.configKey }}",
        "containerPort: {{ .Values.chat.relationQuery.grpcPort }}",
        "kind: Service",
        "port: {{ .Values.chat.relationQuery.grpcPort }}",
    ):
        assert token in chat

    assert (
        "(or .Values.focusedGateways.services.callgateway.enabled .Values.focusedGateways.services.momentsgateway.enabled)"
        in focused
    )
    assert "CallGateway requires chat.relationQuery.enabled=true" in focused
    call_env = focused.rsplit('{{- if eq $name "callgateway" }}', 1)[1].split("{{- end }}", 1)[0]
    assert "name: MEMOCHAT_ACCOUNTPOSTGRES_PASSWD" in call_env
    assert "key: postgres-account-password" in call_env
    assert "MEMOCHAT_ACCOUNTPOSTGRES_PASSWD" in call_env


def test_relation_command_workload_is_deployed_and_chat_uses_it_remotely() -> None:
    configmap = read_chart_file("templates/bootstrap/configmap-services.yaml")
    chat = read_chart_file("templates/prod/chat.yaml")

    relation_config = configmap.split("  chatrelationservice.ini: |", 1)[1].split("  chat.ini: |", 1)[0]
    for token in (
        "[RelationServiceRpc]",
        "Host=0.0.0.0",
        "Port={{ .Values.chat.relationService.grpcPort }}",
        "[Postgres]",
        "[AccountPostgres]",
        "[Redis]",
        "[RabbitMQ]",
        "[Kafka]",
    ):
        assert token in relation_config

    chat_config = configmap.split("  chat.ini: |", 1)[1].split("  varify.ini: |", 1)[0]
    assert "Backend=remote" in chat_config
    assert (
        "Endpoint={{ .Values.chat.relationService.serviceName }}.{{ .Values.namespaces.prod }}.svc."
        "{{ .Values.global.clusterDomain }}:{{ .Values.chat.relationService.grpcPort }}" in chat_config
    )

    for token in (
        "{{- if .Values.chat.relationService.enabled }}",
        "name: {{ .Values.chat.relationService.serviceName }}",
        'image: {{ include "memochat.image" (dict "root" . "image" .Values.images.chatRelationService) | quote }}',
        "value: ChatRelationServiceWorker",
        "subPath: {{ .Values.chat.relationService.configKey }}",
        "containerPort: {{ .Values.chat.relationService.grpcPort }}",
        "port: {{ .Values.chat.relationService.grpcPort }}",
    ):
        assert token in chat


def test_focused_gateways_can_render_without_call_or_moments() -> None:
    if shutil.which("helm") is None:
        return

    result = render_chart(
        "externalSecrets.enabled=true",
        "externalSecrets.relationTokensDistinct=true",
        "envoy.tls.secretName=fixture-tls",
        "mesh.enabled=false",
        "chat.relationQuery.enabled=false",
        "chat.relationService.enabled=false",
        "focusedGateways.services.aigateway.enabled=false",
        "focusedGateways.services.mediagateway.enabled=false",
        "focusedGateways.services.momentsgateway.enabled=false",
        "focusedGateways.services.callgateway.enabled=false",
        "focusedGateways.services.r18gateway.enabled=false",
    )

    assert result.returncode == 0, result.stdout
    for service in ("register", "login", "account"):
        assert f"kind: Deployment\nmetadata:\n  name: {service}\n" in result.stdout
    for service in ("momentsgateway", "callgateway"):
        assert f"kind: Deployment\nmetadata:\n  name: {service}\n" not in result.stdout


def test_postgres_role_credentials_are_explicitly_isolated() -> None:
    values = read_chart_file("values.yaml")
    secret = read_chart_file("templates/bootstrap/secret.yaml")
    external_secret = read_chart_file("templates/bootstrap/external-secrets.yaml")
    configmap = read_chart_file("templates/bootstrap/configmap-services.yaml")
    chat = read_chart_file("templates/prod/chat.yaml")
    focused = read_chart_file("templates/prod/focused-gateways.yaml")

    assert "chat: memo_chat_app" in values
    assert "rolePasswords:" in values
    for key in ("chat", "media", "moments", "call", "account"):
        assert f'      {key}: ""' in values
        assert f"postgres-{key}-password" in secret
        assert f"postgres-{key}-password" in external_secret

    relation_query_config = configmap.split("  chatrelationquery.ini: |", 1)[1].split("  chat.ini: |", 1)[0]
    chat_config = configmap.split("  chat.ini: |", 1)[1].split("  varify.ini: |", 1)[0]
    assert "User={{ .Values.externalServices.postgres.roles.chat }}" in relation_query_config
    assert "User={{ .Values.externalServices.postgres.roles.chat }}" in chat_config
    assert "key: postgres-chat-password" in chat
    assert "key: postgres-account-password" in chat
    assert "key: {{ $postgresSecretKey }}" in focused


def test_relation_auth_tokens_are_role_scoped_and_fail_closed() -> None:
    values = read_chart_file("values.yaml")
    secret = read_chart_file("templates/bootstrap/secret.yaml")
    external_secret = read_chart_file("templates/bootstrap/external-secrets.yaml")
    configmap = read_chart_file("templates/bootstrap/configmap-services.yaml")
    chat = read_chart_file("templates/prod/chat.yaml")
    focused = read_chart_file("templates/prod/focused-gateways.yaml")

    value_keys = (
        "relationCommandAuthToken",
        "relationChatQueryAuthToken",
        "relationCallAuthToken",
        "relationMomentsAuthToken",
    )
    secret_keys = (
        "relation-command-auth-token",
        "relation-chat-query-auth-token",
        "relation-call-auth-token",
        "relation-moments-auth-token",
    )
    for value_key, secret_key in zip(value_keys, secret_keys, strict=True):
        assert f'{value_key}: ""' in values
        assert f'"secrets.{value_key}"' in secret
        assert f'{secret_key}: {{{{ required "secrets.{value_key} is required"' in secret
        assert secret_key in external_secret

    assert "must contain at least 32 bytes" in secret
    assert "must contain only printable ASCII bytes" in secret
    assert "must be distinct" in secret

    relation_query_config = configmap.split("  chatrelationquery.ini: |", 1)[1].split("  chat.ini: |", 1)[0]
    for token_key in ("ChatAuthToken=", "CallAuthToken=", "MomentsAuthToken="):
        assert token_key in relation_query_config

    chat_config = configmap.split("  chat.ini: |", 1)[1].split("  varify.ini: |", 1)[0]
    assert "[RelationQueryService]" in chat_config
    assert "[RelationService]" in chat_config
    assert "Backend=remote" in chat_config
    assert "Endpoint={{ .Values.chat.relationService.serviceName }}" in chat_config
    assert "AuthToken=" in chat_config
    assert "ChatAuthToken=" in chat_config
    moments_config = configmap.split("  momentsgateway.ini: |", 1)[1].split("  callgateway.ini: |", 1)[0]
    assert "[RelationQueryService]" in moments_config
    assert "MomentsAuthToken=" in moments_config
    call_config = configmap.split("  callgateway.ini: |", 1)[1].split("  r18gateway.ini: |", 1)[0]
    assert "CallAuthToken=" in call_config

    for env_name, secret_key in (
        ("MEMOCHAT_RELATIONSERVICE_AUTHTOKEN", "relation-command-auth-token"),
        ("MEMOCHAT_RELATIONQUERYSERVICE_CHATAUTHTOKEN", "relation-chat-query-auth-token"),
        ("MEMOCHAT_RELATIONQUERYSERVICE_CALLAUTHTOKEN", "relation-call-auth-token"),
        ("MEMOCHAT_RELATIONQUERYSERVICE_MOMENTSAUTHTOKEN", "relation-moments-auth-token"),
    ):
        assert env_name in chat
        assert secret_key in chat

    query_workload = chat.split("name: {{ .Values.chat.relationQuery.serviceName }}", 1)[1]
    assert "relation-command-auth-token" not in query_workload
    command_workload = chat.split("name: {{ .Values.chat.relationService.serviceName }}", 1)[1].split(
        "name: {{ .Values.chat.relationQuery.serviceName }}", 1
    )[0]
    assert "relation-command-auth-token" in command_workload
    assert "relation-chat-query-auth-token" not in command_workload

    assert '{{- if eq $name "momentsgateway" }}' in focused
    assert '{{- if eq $name "callgateway" }}' in focused
    assert "MEMOCHAT_RELATIONQUERYSERVICE_MOMENTSAUTHTOKEN" in focused
    assert "MEMOCHAT_RELATIONQUERYSERVICE_CALLAUTHTOKEN" in focused


def test_relation_rpc_requires_rendered_istio_strict_mtls() -> None:
    if shutil.which("helm") is None:
        return

    base_values = (
        "externalSecrets.enabled=true",
        "externalSecrets.relationTokensDistinct=true",
        "envoy.tls.secretName=fixture-tls",
    )

    mesh_disabled = render_chart(*base_values, "mesh.enabled=false")
    assert mesh_disabled.returncode != 0
    assert "requires mesh.enabled=true" in mesh_disabled.stdout

    permissive = render_chart(
        *base_values,
        "mesh.enabled=true",
        "mesh.provider=istio",
        "mesh.istio.mtlsMode=PERMISSIVE",
    )
    assert permissive.returncode != 0
    assert "requires mesh.istio.mtlsMode=STRICT" in permissive.stdout

    strict = render_chart(
        *base_values,
        "mesh.enabled=true",
        "mesh.provider=istio",
        "mesh.istio.mtlsMode=STRICT",
    )
    assert strict.returncode == 0, strict.stdout
    assert "kind: PeerAuthentication" in strict.stdout
    assert "name: memochat-strict-mtls" in strict.stdout
    assert "mode: STRICT" in strict.stdout
    assert 'sidecar.istio.io/inject: "true"' in strict.stdout

    in_process = render_chart(
        *base_values,
        "mesh.enabled=false",
        "chat.relationQuery.enabled=false",
        "chat.relationService.enabled=false",
        "focusedGateways.enabled=false",
    )
    assert in_process.returncode == 0, in_process.stdout
    chat_config = in_process.stdout.split("chat.ini: |", 1)[1].split("varify.ini: |", 1)[0]
    assert "[RelationQueryService]" in chat_config
    assert "Backend=inprocess" in chat_config
    assert "ChatAuthToken=" not in chat_config
    assert "Endpoint={{ .Values.chat.relationService.serviceName }}" not in chat_config
    assert "AuthToken=" not in chat_config.split("[RelationService]", 1)[1].split("[RelationQueryService]", 1)[0]


def test_production_images_are_ghcr_commit_sha_or_pinned_infrastructure_digests() -> None:
    if shutil.which("helm") is None:
        return

    prod_values = CHART / "values" / "prod.yaml"
    calls_profile = CHART / "values" / "profiles" / "calls.yaml"
    r18_profile = CHART / "values" / "profiles" / "r18.yaml"
    observability_profile = CHART / "values" / "profiles" / "observability.yaml"
    base_values = (
        "externalSecrets.enabled=true",
        "externalSecrets.relationTokensDistinct=true",
        "envoy.tls.secretName=fixture-tls",
    )
    valid_tag = "sha-0123456789abcdef0123456789abcdef01234567"

    unattested = render_chart(
        "externalSecrets.enabled=true",
        "envoy.tls.secretName=fixture-tls",
        f"images.releaseTag={valid_tag}",
        values_files=(prod_values,),
    )
    assert unattested.returncode != 0
    assert "externalSecrets.relationTokensDistinct=true" in unattested.stdout

    mutable = render_chart(*base_values, "images.releaseTag=latest", values_files=(prod_values,))
    assert mutable.returncode != 0
    assert "mutable tags are rejected" in mutable.stdout

    mutable_job = render_chart(
        *base_values,
        f"images.releaseTag={valid_tag}",
        "jobs.postgresMigrate.image=postgres:16-alpine",
        values_files=(prod_values,),
    )
    assert mutable_job.returncode != 0
    assert "immutable repository@sha256 digest" in mutable_job.stdout

    non_ghcr = render_chart(
        *base_values,
        f"images.releaseTag={valid_tag}",
        "images.chat.repository=docker.io/example/chat-server",
        values_files=(prod_values,),
    )
    assert non_ghcr.returncode != 0
    assert "images.chat.repository must be ghcr.io/" in non_ghcr.stdout

    rendered = render_chart(
        *base_values,
        f"images.releaseTag={valid_tag}",
        values_files=(prod_values, calls_profile, r18_profile, observability_profile),
    )
    assert rendered.returncode == 0, rendered.stdout
    release_prefix = "ghcr.io/kafuuchinoqwq4/memochat"
    for slug in (
        "ai-gateway",
        "ai-server",
        "account-server",
        "call-gateway",
        "chat-relation-query-service",
        "chat-relation-service-worker",
        "chat-server",
        "login-server",
        "media-gateway",
        "moments-gateway",
        "r18-gateway",
        "register-server",
        "varify-server",
    ):
        assert f'image: "{release_prefix}/{slug}:{valid_tag}"' in rendered.stdout
    assert "envoyproxy/envoy@sha256:8146b97e" in rendered.stdout
    assert "otel/opentelemetry-collector-contrib@sha256:b6552779" in rendered.stdout
    assert "postgres@sha256:78df81b1" in rendered.stdout
    assert rendered.stdout.count("python@sha256:46cb7cc2") == 2
    image_lines = [line.strip() for line in rendered.stdout.splitlines() if line.strip().startswith("image:")]
    assert all(":latest" not in line for line in image_lines)
    assert "kind: Deployment\nmetadata:\n  name: gateserver\n" not in rendered.stdout
    assert "kind: Deployment\nmetadata:\n  name: ai-orchestrator\n" not in rendered.stdout
    assert "kind: Deployment\nmetadata:\n  name: memo-ops-server\n" not in rendered.stdout


def test_ai_values_model_aiserver_and_ai_orchestrator_as_first_class_services() -> None:
    values = read_chart_file("values.yaml")

    for token in (
        "aiServer:",
        "repository: ghcr.io/kafuuchinoqwq4/memochat/ai-server",
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
