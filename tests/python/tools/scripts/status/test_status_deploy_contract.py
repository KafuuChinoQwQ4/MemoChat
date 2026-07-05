import configparser
import re
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
DEPLOY_SCRIPT = REPO_ROOT / "tools/scripts/status/deploy_services.sh"
START_SERVICES_SCRIPT = REPO_ROOT / "tools/scripts/status/start-all-services.sh"
TOPOLOGY_SCRIPT = REPO_ROOT / "tools/scripts/status/runtime_topology.sh"
LOCAL_ENVOY_CONFIG = REPO_ROOT / "infra/deploy/local/compose/envoy.yaml"
RELATION_QUERY_GRPC_SMOKE_SCRIPT = REPO_ROOT / "tools/scripts/status/smoke_relation_query_grpc_runtime.sh"
RELATION_SERVICE_GRPC_SMOKE_SCRIPT = REPO_ROOT / "tools/scripts/status/smoke_relation_service_grpc_runtime.sh"
MESSAGE_SERVICE_GRPC_SMOKE_SCRIPT = REPO_ROOT / "tools/scripts/status/smoke_message_service_grpc_runtime.sh"
DEFAULT_GRPC_SMOKE_SCRIPT = REPO_ROOT / "tools/scripts/status/smoke_chatserver_default_grpc_runtime.sh"
START_QML_SCRIPT = REPO_ROOT / "tools/scripts/status/start-memochat-qml-wslg.sh"
RUN_FULL_STACK_SCRIPT = REPO_ROOT / "tools/scripts/status/run-linux-full-stack.sh"
GPT_SOVITS_VOICE_SCRIPT = REPO_ROOT / "tools/scripts/pet/apply_gpt_sovits_voice_wsl.sh"
LOCAL_COMPOSE = REPO_ROOT / "infra/deploy/local/docker-compose.yml"
AI_DOCKER_COMPOSE = REPO_ROOT / "apps/server/core/AIOrchestrator/docker-compose.yml"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def topology_rows() -> list[dict[str, str]]:
    source = read(TOPOLOGY_SCRIPT)
    block_match = re.search(r"MEMOCHAT_RUNTIME_SERVICE_TOPOLOGY=\(\n(?P<body>.*?)\n\)", source, flags=re.S)
    if block_match is None:
        raise AssertionError("runtime topology manifest array is missing")
    rows = []
    for row in re.findall(r'^\s*"([^"]+)"', block_match.group("body"), flags=re.M):
        fields = row.split("|")
        if len(fields) != 14:
            raise AssertionError(f"topology row has {len(fields)} fields: {row}")
        rows.append(
            {
                "group": fields[0],
                "runtime_dir": fields[1],
                "executable": fields[2],
                "source_executable": fields[3],
                "config_source": fields[4],
                "display_name": fields[5],
                "tcp_wait_port": fields[6],
                "udp_wait_ports": fields[7],
                "instance_name": fields[8],
                "stop_tcp_ports": fields[9],
                "stop_udp_ports": fields[10],
                "log_dir": fields[11],
                "telemetry_service_name": fields[12],
                "telemetry_namespace": fields[13],
            }
        )
    return rows


def topology_array_values(name: str) -> list[str]:
    source = read(TOPOLOGY_SCRIPT)
    match = re.search(rf"readonly\s+-a\s+{re.escape(name)}=\(\n(?P<body>.*?)\n\)", source, flags=re.S)
    if match is None:
        raise AssertionError(f"{name} array is missing from runtime topology manifest")
    return re.findall(r'^\s*"([^"]+)"', match.group("body"), flags=re.M)


def read_ini(path: Path) -> configparser.ConfigParser:
    parser = configparser.ConfigParser()
    parser.optionxform = str
    parser.read(path, encoding="utf-8-sig")
    return parser


class StatusDeployContractTests(unittest.TestCase):
    def test_linux_deploy_prefers_client_build_for_memochat_qml(self):
        source = read(DEPLOY_SCRIPT)

        self.assertIn("CLIENT_BUILD_BIN", source)
        self.assertIn("MEMOCHAT_CLIENT_BUILD_BIN", source)
        self.assertIn("--client-build-bin", source)
        self.assertIn("copy_optional_from_candidates", source)
        self.assertIn("copy_memochat_qml_from_candidates", source)
        self.assertIn("require_live2d_native_memo_chat_qml", source)

        match = re.search(
            r"copy_memochat_qml_from_candidates\s+\"\$\{RUNTIME_DIR\}/MemoChatQml/MemoChatQml\"(?P<body>.*?)\ncopy_optional_from_candidates",
            source,
            flags=re.S,
        )
        self.assertIsNotNone(match, "MemoChatQml deploy should use ordered source candidates")
        body = match.group("body")
        client_index = body.find("${CLIENT_BUILD_BIN}/MemoChatQml")
        server_index = body.find("${BUILD_BIN}/MemoChatQml")
        self.assertGreaterEqual(client_index, 0)
        self.assertGreater(server_index, client_index)

    def test_linux_deploy_prints_client_build_path_for_diagnostics(self):
        source = read(DEPLOY_SCRIPT)

        self.assertIn("CLIENT_BUILD:", source)
        self.assertIn("MEMOCHAT_CLIENT_BUILD_BIN", source)
        self.assertIn('BUILD_BIN="${MEMOCHAT_BUILD_BIN:-${PROJECT_ROOT}/build-linux-full-gcc16/bin}"', source)
        self.assertIn('CLIENT_BUILD_BIN="${PROJECT_ROOT}/build-linux-full-gcc16/bin"', source)

    def test_linux_deploy_refuses_stale_non_native_live2d_qml_builds(self):
        source = read(DEPLOY_SCRIPT)

        for token in (
            "live2d_native_cache_is_valid",
            "MEMOCHAT_ENABLE_LIVE2D_NATIVE",
            "MEMOCHAT_LIVE2D_SDK_ROOT",
            "Refusing to deploy non-native/stale Live2D MemoChatQml build",
            "Live2D Cubism Native OpenGL is not enabled in this build",
        ):
            self.assertIn(token, source)

    def test_wslg_client_startup_sets_ime_defaults_when_available(self):
        source = read(START_QML_SCRIPT)

        for token in (
            "QT_IM_MODULE",
            "GTK_IM_MODULE",
            "XMODIFIERS",
            "fcitx5",
            "ibus-daemon",
            "ibus-daemon -drx",
            "ibus-daemon -drx --replace",
            "ibus engine libpinyin",
            "ibus exit",
            "pkill -x ibus-daemon",
            "MEMOCHAT_RESTART_IBUS",
            "IBUS_ENABLE_SYNC_MODE",
            "GDK_BACKEND=x11",
            "QT_QPA_PLATFORM=xcb",
            "env -u WAYLAND_DISPLAY",
            "unset WAYLAND_DISPLAY",
            "sleep 0.1",
            "libfcitx5platforminputcontextplugin.so",
            "libibusplatforminputcontextplugin.so",
            "QT_VIRTUALKEYBOARD_DESKTOP_DISABLE",
            "qtvirtualkeyboard",
        ):
            self.assertIn(token, source)

        self.assertIn("select_input_method", source)
        self.assertIn("MEMOCHAT_KEEP_QT_IM_MODULE", source)
        self.assertIn('export QT_IM_MODULE="$qt_module"', source)
        self.assertIn("has_qt_fcitx_plugin", source)
        self.assertIn("has_qt_ibus_plugin", source)
        self.assertIn("env -u WAYLAND_DISPLAY fcitx5 -d", source)
        self.assertIn("start_ibus_daemon", source)
        self.assertIn("select_ibus_libpinyin", source)
        self.assertIn("stop_ibus_daemons", source)
        self.assertIn("start_ibus_daemon replace", source)
        self.assertIn("env -u WAYLAND_DISPLAY ibus address", source)
        self.assertIn("ibus engine libpinyin", source)
        self.assertIn("sleep 0.1", source)
        self.assertIn("sleep 0.15", source)
        self.assertIn("IBUS_SYNC_MODE:", source)
        self.assertIn("GDK_BACKEND:", source)
        fcitx_index = source.index("has_qt_fcitx_plugin")
        ibus_index = source.index('elif [[ "$has_qt_ibus_plugin" -eq 1 ]]')
        virtual_keyboard_index = source.index(
            'elif [[ -f "${QT_ROOT}/plugins/platforminputcontexts/libqtvirtualkeyboardplugin.so" ]]'
        )
        self.assertLess(fcitx_index, virtual_keyboard_index)
        self.assertLess(ibus_index, virtual_keyboard_index)
        self.assertIn("dbus-launch", source)
        self.assertIn("[input method]", source)

    def test_wslg_client_startup_falls_back_when_d3d12_glx_is_unavailable(self):
        launcher = read(START_QML_SCRIPT)
        for token in (
            "maybe_fallback_wslg_rendering",
            "can_create_glx_context",
            "glxinfo -B",
            "MEMOCHAT_DISABLE_WSLG_GL_FALLBACK",
            "set_wslg_llvmpipe_rendering_defaults",
            'export QSG_RHI_BACKEND="opengl"',
            'export QT_OPENGL="software"',
            'export GALLIUM_DRIVER="llvmpipe"',
            'export MESA_LOADER_DRIVER_OVERRIDE="llvmpipe"',
            'export LIBGL_ALWAYS_SOFTWARE="1"',
            'export MEMOCHAT_WSLG_GL_FALLBACK="llvmpipe"',
            "WSLg d3d12 OpenGL context is unavailable",
            "GL_FALLBACK:",
        ):
            self.assertIn(token, launcher)

        self.assertLess(
            launcher.index('set_default GALLIUM_DRIVER "d3d12"'),
            launcher.rindex("maybe_fallback_wslg_rendering"),
        )

    def test_linux_full_stack_launcher_chains_build_deploy_backend_and_client(self):
        source = read(RUN_FULL_STACK_SCRIPT)

        for token in (
            'source "$ENV_FILE"',
            'cmake --preset "$PRESET"',
            'cmake --build --preset "$PRESET" --parallel "$BUILD_PARALLEL"',
            'docker compose -f "$AI_COMPOSE_FILE" up -d --build',
            '"${SCRIPT_DIR}/deploy_services.sh" --build-bin "$BUILD_BIN"',
            '"${SCRIPT_DIR}/start-all-services.sh" --no-deploy',
            '"${SCRIPT_DIR}/start-memochat-qml-wslg.sh" --exe "$CLIENT_EXE"',
            'CLIENT_EXE="$(project_path "$CLIENT_EXE")"',
            "linux-full-gcc16",
            "build-linux-full-gcc16",
            "apps/server/core/AIOrchestrator/docker-compose.yml",
            "MEMOCHAT_BUILD_PARALLEL",
            "MEMOCHAT_REQUIRE_GPT_SOVITS",
            "MEMOCHAT_AI_BASE_URL",
            "MEMOCHAT_AI_VOICE_WAIT_SECONDS",
            "ensure_ai_voice_ready",
            "/pet/diagnostics/voice?probe_endpoint=true",
            "--skip-build",
            "--skip-ai-build",
            "--skip-deploy",
            "--skip-backend",
            "--no-client",
            "--client-diagnose",
        ):
            self.assertIn(token, source)

        self.assertIn("set -Eeuo pipefail", source)
        self.assertIn('exec "${client_args[@]}"', source)

    def test_linux_runtime_scripts_generate_and_share_ai_internal_key_for_local_launches(self):
        start = read(START_SERVICES_SCRIPT)
        full_stack = read(RUN_FULL_STACK_SCRIPT)
        ai_compose = read(AI_DOCKER_COMPOSE)

        for label, source in (("start", start), ("full_stack", full_stack)):
            with self.subTest(script=label):
                self.assertIn("ensure_ai_internal_api_key()", source)
                self.assertIn('if [[ -n "${MEMOCHAT_AI_INTERNAL_API_KEY:-}" ]]', source)
                self.assertIn("openssl rand -hex 32", source)
                self.assertIn("/proc/sys/kernel/random/uuid", source)
                self.assertIn('export MEMOCHAT_AI_INTERNAL_API_KEY="$generated"', source)
                self.assertNotIn("MEMOCHAT_AI_INTERNAL_API_KEY=memochat", source)

        self.assertIn('MEMOCHAT_AI_INTERNAL_API_KEY="${MEMOCHAT_AI_INTERNAL_API_KEY:-}"', start)
        self.assertIn("ensure_ai_internal_api_key\nexport_minio_runtime_credentials", start)
        self.assertIn(
            'AI_VOICE_WAIT_SECONDS="${MEMOCHAT_AI_VOICE_WAIT_SECONDS:-$AI_VOICE_WAIT_SECONDS}"\nensure_ai_internal_api_key',
            full_stack,
        )
        self.assertLess(start.index("ensure_ai_internal_api_key"), start.index("export_minio_runtime_credentials"))
        self.assertLess(
            full_stack.index("ensure_ai_internal_api_key"),
            full_stack.index('docker compose -f "$AI_COMPOSE_FILE" up -d --build'),
        )
        self.assertIn("MEMOCHAT_AI_INTERNAL_API_KEY=${MEMOCHAT_AI_INTERNAL_API_KEY:-}", ai_compose)

    def test_linux_local_startup_allows_documented_dev_chat_auth_secret(self):
        start = read(START_SERVICES_SCRIPT)

        self.assertIn('export MEMOCHAT_ALLOW_DEV_SECRETS="${MEMOCHAT_ALLOW_DEV_SECRETS:-1}"', start)
        self.assertIn('MEMOCHAT_ALLOW_DEV_SECRETS="${MEMOCHAT_ALLOW_DEV_SECRETS:-}"', start)
        self.assertLess(
            start.index('export MEMOCHAT_ALLOW_DEV_SECRETS="${MEMOCHAT_ALLOW_DEV_SECRETS:-1}"'),
            start.index("START_CORE_SERVICES="),
        )

    def test_linux_start_requires_gpt_sovits_by_default(self):
        source = read(START_SERVICES_SCRIPT)

        for token in (
            "MEMOCHAT_REQUIRE_GPT_SOVITS",
            "REQUIRE_GPT_SOVITS",
            "print_gpt_sovits_failure_hint",
            "GPT-SoVITS did not become ready",
            "smoke_gpt_sovits_tts_wsl.sh",
            "apply_gpt_sovits_voice_wsl.sh",
            "To intentionally allow text-only pet replies",
            "return 1",
            "--skip-gpt-sovits",
        ):
            self.assertIn(token, source)

    def test_linux_start_brings_up_post_login_docker_dependencies(self):
        source = read(START_SERVICES_SCRIPT)

        for token in (
            "START_DOCKER_DEPS",
            "--skip-docker-deps",
            "ensure_docker_dependencies",
            "memochat-redis",
            "memochat-postgres",
            "memochat-mongo",
            "memochat-minio",
            "memochat-redpanda",
            "memochat-rabbitmq",
            "wait_for_minio",
            "wait_for_redpanda",
            'docker compose -f "$LOCAL_COMPOSE_FILE" up -d',
        ):
            self.assertIn(token, source)

        self.assertLess(
            source.index("[STEP] Start Docker dependencies"), source.index("[STEP] Start Docker Envoy Gateway")
        )

    def test_linux_runtime_scripts_wire_chat_delivery_worker_explicitly(self):
        deploy = read(DEPLOY_SCRIPT)
        start = read(START_SERVICES_SCRIPT)
        stop = read(REPO_ROOT / "tools/scripts/status/stop-all-services.sh")
        topology = read(TOPOLOGY_SCRIPT)

        self.assertIn('TOPOLOGY_FILE="${SCRIPT_DIR}/runtime_topology.sh"', deploy)
        self.assertIn('source "$TOPOLOGY_FILE"', deploy)
        self.assertIn("require_service_binaries", deploy)
        self.assertIn("deploy_topology_services", deploy)
        self.assertIn("required_runtime_files", deploy)

        self.assertIn(
            '"worker|ChatDeliveryWorker1|ChatDeliveryWorker|ChatDeliveryWorker|'
            "ChatServer/chatdeliveryworker1.ini|ChatDeliveryWorker-1|||chatdeliveryworker1|||"
            '../../artifacts/logs/services/chatdeliveryworker1|ChatDeliveryWorker|memochat"',
            topology,
        )

        for token in (
            "MEMOCHAT_START_CHAT_DELIVERY_WORKER",
            "START_CHAT_DELIVERY_WORKER",
            "--start-chat-delivery-worker",
            "--skip-core-services",
            "START_CORE_SERVICES",
            "verify_started_pid",
            "exited early",
            "[STEP] Start ChatDeliveryWorker",
            'launch_topology_group "$MEMOCHAT_TOPOLOGY_GROUP_WORKER"',
            "start_topology_core_groups",
            'for group in "${MEMOCHAT_CORE_START_GROUPS[@]}"',
            'MEMOCHAT_INSTANCE_NAME="$instance_name"',
        ):
            self.assertIn(token, start)

        self.assertLess(start.index("[STEP] Start ChatDeliveryWorker"), start.rindex("start_topology_core_groups"))
        self.assertIn("ChatDeliveryWorker-1", topology)
        self.assertIn("MEMOCHAT_STOP_PID_ORDER", topology)
        self.assertIn('for name in "${MEMOCHAT_STOP_PID_ORDER[@]}"', stop)
        self.assertIn("MEMOCHAT_STOP_EXECUTABLE_ORDER", topology)
        self.assertIn("MEMOCHAT_STOP_PORT_GROUP_ORDER", topology)
        self.assertIn("stop_topology_port_groups", stop)
        self.assertIn('"ChatDeliveryWorker"', topology)
        self.assertIn('stop_by_name_under_runtime "$exe_name" "$exe_name"', stop)
        self.assertIn('pgrep -f "(^|[[:space:]/])${exe_name}([[:space:]]|$)"', stop)

    def test_linux_runtime_scripts_wire_chat_relation_query_service_explicitly(self):
        deploy = read(DEPLOY_SCRIPT)
        start = read(START_SERVICES_SCRIPT)
        stop = read(REPO_ROOT / "tools/scripts/status/stop-all-services.sh")
        topology = read(TOPOLOGY_SCRIPT)
        rows = topology_rows()
        by_dir = {row["runtime_dir"]: row for row in rows}

        self.assertIn("ChatRelationQueryService1", by_dir)
        row = by_dir["ChatRelationQueryService1"]
        self.assertEqual("relation_query", row["group"])
        self.assertEqual("ChatRelationQueryService", row["executable"])
        self.assertEqual("ChatRelationQueryService", row["source_executable"])
        self.assertEqual("ChatServer/chatrelationquery1.ini", row["config_source"])
        self.assertEqual("ChatRelationQueryService-1", row["display_name"])
        self.assertEqual("50090", row["tcp_wait_port"])
        self.assertEqual("", row["udp_wait_ports"])
        self.assertEqual("chatrelationquery1", row["instance_name"])
        self.assertEqual("50090", row["stop_tcp_ports"])
        self.assertEqual("", row["stop_udp_ports"])
        self.assertEqual("../../artifacts/logs/services/chatrelationquery1", row["log_dir"])
        self.assertEqual("ChatRelationQueryService", row["telemetry_service_name"])
        self.assertEqual("memochat", row["telemetry_namespace"])

        self.assertIn("require_service_binaries", deploy)
        self.assertIn("deploy_topology_services", deploy)
        self.assertIn(
            '"relation_query|ChatRelationQueryService1|ChatRelationQueryService|ChatRelationQueryService|'
            "ChatServer/chatrelationquery1.ini|ChatRelationQueryService-1|50090||chatrelationquery1|50090||"
            '../../artifacts/logs/services/chatrelationquery1|ChatRelationQueryService|memochat"',
            topology,
        )

        for token in (
            "MEMOCHAT_START_RELATION_QUERY_SERVICE",
            "START_RELATION_QUERY_SERVICE",
            "--start-relation-query-service",
            "--skip-relation-query-service",
            "[STEP] Start ChatRelationQueryService",
            'launch_topology_group "$MEMOCHAT_TOPOLOGY_GROUP_RELATION_QUERY"',
        ):
            self.assertIn(token, start)
        self.assertIn("MEMOCHAT_START_RELATION_QUERY_SERVICE:-1", start)

        self.assertNotIn("relation_query", topology_array_values("MEMOCHAT_CORE_START_GROUPS"))
        self.assertIn("ChatRelationQueryService-1", topology_array_values("MEMOCHAT_STOP_PID_ORDER"))
        self.assertIn("ChatRelationQueryService", topology_array_values("MEMOCHAT_STOP_EXECUTABLE_ORDER"))
        self.assertIn(
            "relation_query|ChatRelationQueryService|tcp", topology_array_values("MEMOCHAT_STOP_PORT_GROUP_ORDER")
        )
        self.assertIn('for name in "${MEMOCHAT_STOP_PID_ORDER[@]}"', stop)
        self.assertIn("stop_topology_port_groups", stop)
        self.assertIn('stop_by_name_under_runtime "$exe_name" "$exe_name"', stop)

    def test_linux_runtime_scripts_wire_chat_relation_service_worker_explicitly(self):
        deploy = read(DEPLOY_SCRIPT)
        start = read(START_SERVICES_SCRIPT)
        stop = read(REPO_ROOT / "tools/scripts/status/stop-all-services.sh")
        topology = read(TOPOLOGY_SCRIPT)
        rows = topology_rows()
        by_dir = {row["runtime_dir"]: row for row in rows}

        self.assertIn("ChatRelationServiceWorker1", by_dir)
        row = by_dir["ChatRelationServiceWorker1"]
        self.assertEqual("relation_service", row["group"])
        self.assertEqual("ChatRelationServiceWorker", row["executable"])
        self.assertEqual("ChatRelationServiceWorker", row["source_executable"])
        self.assertEqual("ChatServer/chatrelationservice1.ini", row["config_source"])
        self.assertEqual("ChatRelationServiceWorker-1", row["display_name"])
        self.assertEqual("50091", row["tcp_wait_port"])
        self.assertEqual("", row["udp_wait_ports"])
        self.assertEqual("chatrelationservice1", row["instance_name"])
        self.assertEqual("50091", row["stop_tcp_ports"])
        self.assertEqual("", row["stop_udp_ports"])
        self.assertEqual("../../artifacts/logs/services/chatrelationservice1", row["log_dir"])
        self.assertEqual("ChatRelationServiceWorker", row["telemetry_service_name"])
        self.assertEqual("memochat", row["telemetry_namespace"])

        self.assertIn("require_service_binaries", deploy)
        self.assertIn("deploy_topology_services", deploy)
        self.assertIn(
            '"relation_service|ChatRelationServiceWorker1|ChatRelationServiceWorker|ChatRelationServiceWorker|'
            "ChatServer/chatrelationservice1.ini|ChatRelationServiceWorker-1|50091||chatrelationservice1|50091||"
            '../../artifacts/logs/services/chatrelationservice1|ChatRelationServiceWorker|memochat"',
            topology,
        )

        for token in (
            "MEMOCHAT_START_RELATION_SERVICE_WORKER",
            "START_RELATION_SERVICE_WORKER",
            "--start-relation-service-worker",
            "--skip-relation-service-worker",
            "[STEP] Start ChatRelationServiceWorker",
            'launch_topology_group "$MEMOCHAT_TOPOLOGY_GROUP_RELATION_SERVICE"',
        ):
            self.assertIn(token, start)
        self.assertIn("MEMOCHAT_START_RELATION_SERVICE_WORKER:-1", start)

        self.assertNotIn("relation_service", topology_array_values("MEMOCHAT_CORE_START_GROUPS"))
        self.assertIn("ChatRelationServiceWorker-1", topology_array_values("MEMOCHAT_STOP_PID_ORDER"))
        self.assertIn("ChatRelationServiceWorker", topology_array_values("MEMOCHAT_STOP_EXECUTABLE_ORDER"))
        self.assertIn(
            "relation_service|ChatRelationServiceWorker|tcp", topology_array_values("MEMOCHAT_STOP_PORT_GROUP_ORDER")
        )
        self.assertIn('for name in "${MEMOCHAT_STOP_PID_ORDER[@]}"', stop)
        self.assertIn("stop_topology_port_groups", stop)
        self.assertIn('stop_by_name_under_runtime "$exe_name" "$exe_name"', stop)

    def test_linux_runtime_scripts_wire_chat_message_service_explicitly(self):
        deploy = read(DEPLOY_SCRIPT)
        start = read(START_SERVICES_SCRIPT)
        stop = read(REPO_ROOT / "tools/scripts/status/stop-all-services.sh")
        topology = read(TOPOLOGY_SCRIPT)
        rows = topology_rows()
        by_dir = {row["runtime_dir"]: row for row in rows}

        self.assertIn("ChatMessageService1", by_dir)
        row = by_dir["ChatMessageService1"]
        self.assertEqual("message_service", row["group"])
        self.assertEqual("ChatMessageService", row["executable"])
        self.assertEqual("ChatMessageService", row["source_executable"])
        self.assertEqual("ChatServer/chatmessageservice1.ini", row["config_source"])
        self.assertEqual("ChatMessageService-1", row["display_name"])
        self.assertEqual("50092", row["tcp_wait_port"])
        self.assertEqual("", row["udp_wait_ports"])
        self.assertEqual("chatmessageservice1", row["instance_name"])
        self.assertEqual("50092", row["stop_tcp_ports"])
        self.assertEqual("", row["stop_udp_ports"])
        self.assertEqual("../../artifacts/logs/services/chatmessageservice1", row["log_dir"])
        self.assertEqual("ChatMessageService", row["telemetry_service_name"])
        self.assertEqual("memochat", row["telemetry_namespace"])

        self.assertIn("require_service_binaries", deploy)
        self.assertIn("deploy_topology_services", deploy)
        self.assertIn(
            '"message_service|ChatMessageService1|ChatMessageService|ChatMessageService|'
            "ChatServer/chatmessageservice1.ini|ChatMessageService-1|50092||chatmessageservice1|50092||"
            '../../artifacts/logs/services/chatmessageservice1|ChatMessageService|memochat"',
            topology,
        )

        for token in (
            "MEMOCHAT_START_MESSAGE_SERVICE",
            "START_MESSAGE_SERVICE",
            "--start-message-service",
            "--skip-message-service",
            "[STEP] Start ChatMessageService",
            'launch_topology_group "$MEMOCHAT_TOPOLOGY_GROUP_MESSAGE_SERVICE"',
        ):
            self.assertIn(token, start)
        self.assertIn("MEMOCHAT_START_MESSAGE_SERVICE:-1", start)

        self.assertNotIn("message_service", topology_array_values("MEMOCHAT_CORE_START_GROUPS"))
        self.assertIn("ChatMessageService-1", topology_array_values("MEMOCHAT_STOP_PID_ORDER"))
        self.assertIn("ChatMessageService", topology_array_values("MEMOCHAT_STOP_EXECUTABLE_ORDER"))
        self.assertIn("message_service|ChatMessageService|tcp", topology_array_values("MEMOCHAT_STOP_PORT_GROUP_ORDER"))
        self.assertIn('for name in "${MEMOCHAT_STOP_PID_ORDER[@]}"', stop)
        self.assertIn("stop_topology_port_groups", stop)
        self.assertIn('stop_by_name_under_runtime "$exe_name" "$exe_name"', stop)

    def test_relation_query_grpc_runtime_smoke_uses_temp_override_and_real_probe(self):
        source = read(RELATION_QUERY_GRPC_SMOKE_SCRIPT)

        for token in (
            "MEMOCHAT_RELATION_QUERY_SMOKE_CONFIG",
            "MEMOCHAT_RELATION_QUERY_SMOKE_ENDPOINT",
            "chatserver_relation_query_remote_smoke_gtest",
            "ChatRelationQueryService",
            "ChatServer",
            "Backend=grpc",
            "Endpoint=127.0.0.1:50090",
            "MEMOCHAT_ENABLE_QUIC=0",
            "MEMOCHAT_ENABLE_KAFKA=0",
            "MEMOCHAT_ENABLE_RABBITMQ=0",
            "--start-relation-query-service",
            "--skip-relation-service-worker",
            "--skip-message-service",
            "--skip-core-services",
            "stop-all-services.sh",
            "cleanup",
            "relation_query_remote_error",
        ):
            self.assertIn(token, source)

        self.assertNotIn("Backend=inprocess", source)

    def test_relation_service_grpc_runtime_smoke_uses_temp_override_and_real_probe(self):
        source = read(RELATION_SERVICE_GRPC_SMOKE_SCRIPT)

        for token in (
            "MEMOCHAT_RELATION_SERVICE_SMOKE_ENDPOINT",
            "chatserver_relation_grpc_client_gtest",
            "ChatRelationServiceWorker",
            "ChatServer",
            "Backend=grpc",
            "Endpoint=127.0.0.1:50091",
            "MEMOCHAT_ENABLE_QUIC=0",
            "MEMOCHAT_ENABLE_KAFKA=0",
            "MEMOCHAT_ENABLE_RABBITMQ=0",
            "--start-relation-service-worker",
            "--skip-relation-query-service",
            "--skip-message-service",
            "--skip-core-services",
            "stop-all-services.sh",
            "cleanup",
            "RuntimeSmokeSearchUserUsesRealRelationServiceWorker",
            "relation_remote_error",
        ):
            self.assertIn(token, source)

        self.assertNotIn("Backend=inprocess", source)

    def test_message_service_grpc_runtime_smoke_uses_temp_override_and_real_probe(self):
        source = read(MESSAGE_SERVICE_GRPC_SMOKE_SCRIPT)

        for token in (
            "MEMOCHAT_MESSAGE_SERVICE_SMOKE_ENDPOINT",
            "chatserver_message_remote_smoke_gtest",
            "ChatMessageService",
            "ChatServer",
            "ChatServer/chatmessageservice1.ini",
            "Backend=grpc",
            "Endpoint=127.0.0.1:50092",
            "MEMOCHAT_ENABLE_QUIC=0",
            "MEMOCHAT_ENABLE_KAFKA=0",
            "MEMOCHAT_ENABLE_RABBITMQ=0",
            "--start-message-service",
            "--skip-relation-query-service",
            "--skip-relation-service-worker",
            "--skip-core-services",
            "wait_for_tcp_port",
            "50092",
            "stop-all-services.sh",
            "cleanup",
            "temp_runtime_processes",
            "MessageRemoteSmokeTest",
            "message_remote_error",
        ):
            self.assertIn(token, source)

        self.assertNotIn("Backend=inprocess", source)

    def test_chatserver_default_grpc_runtime_smoke_uses_default_config_and_all_remote_workers(self):
        source = read(DEFAULT_GRPC_SMOKE_SCRIPT)

        for token in (
            "ChatServer default config",
            "ChatRelationQueryService",
            "ChatRelationServiceWorker",
            "ChatMessageService",
            "chatserver1.ini",
            "grep -q '^Backend=grpc$'",
            "Endpoint=127.0.0.1:50090",
            "Endpoint=127.0.0.1:50091",
            "Endpoint=127.0.0.1:50092",
            "chatserver_relation_query_remote_smoke_gtest",
            "chatserver_relation_grpc_client_gtest",
            "chatserver_message_remote_smoke_gtest",
            "MEMOCHAT_RELATION_QUERY_SMOKE_CONFIG",
            "MEMOCHAT_RELATION_SERVICE_SMOKE_ENDPOINT",
            "MEMOCHAT_MESSAGE_SERVICE_SMOKE_CONFIG",
            "--skip-core-services",
            "stop-all-services.sh",
            "ChatServer TCP",
            "ChatServer gRPC",
        ):
            self.assertIn(token, source)

        self.assertNotIn('set_ini_value "$chat_config" "MessageService" "Backend" "grpc"', source)
        self.assertNotIn('set_ini_value "$chat_config" "RelationService" "Backend" "grpc"', source)
        self.assertNotIn('set_ini_value "$chat_config" "RelationQueryService" "Backend" "grpc"', source)

    def test_linux_runtime_topology_manifest_covers_current_services_and_ports(self):
        deploy = read(DEPLOY_SCRIPT)
        start = read(START_SERVICES_SCRIPT)
        stop = read(REPO_ROOT / "tools/scripts/status/stop-all-services.sh")
        rows = topology_rows()

        self.assertEqual(15, len(rows))

        by_dir = {row["runtime_dir"]: row for row in rows}
        expected = {
            "chatserver1": (
                "chat",
                "ChatServer",
                "ChatServer/chatserver1.ini",
                "ChatServer-1",
                "8090",
                "8190",
                "8090 50055",
                "8190",
                "../../artifacts/logs/services/chatserver1",
                "ChatServer",
                "memochat",
            ),
            "ChatDeliveryWorker1": (
                "worker",
                "ChatDeliveryWorker",
                "ChatServer/chatdeliveryworker1.ini",
                "ChatDeliveryWorker-1",
                "",
                "",
                "",
                "",
                "../../artifacts/logs/services/chatdeliveryworker1",
                "ChatDeliveryWorker",
                "memochat",
            ),
            "ChatRelationQueryService1": (
                "relation_query",
                "ChatRelationQueryService",
                "ChatServer/chatrelationquery1.ini",
                "ChatRelationQueryService-1",
                "50090",
                "",
                "50090",
                "",
                "../../artifacts/logs/services/chatrelationquery1",
                "ChatRelationQueryService",
                "memochat",
            ),
            "ChatRelationServiceWorker1": (
                "relation_service",
                "ChatRelationServiceWorker",
                "ChatServer/chatrelationservice1.ini",
                "ChatRelationServiceWorker-1",
                "50091",
                "",
                "50091",
                "",
                "../../artifacts/logs/services/chatrelationservice1",
                "ChatRelationServiceWorker",
                "memochat",
            ),
            "ChatMessageService1": (
                "message_service",
                "ChatMessageService",
                "ChatServer/chatmessageservice1.ini",
                "ChatMessageService-1",
                "50092",
                "",
                "50092",
                "",
                "../../artifacts/logs/services/chatmessageservice1",
                "ChatMessageService",
                "memochat",
            ),
            "AIGatewayService1": (
                "aigateway",
                "AIGatewayServer",
                "AIGatewayService/aigateway.ini",
                "AIGatewayService-1",
                "8093",
                "",
                "8093",
                "",
                "../../artifacts/logs/services/AIGatewayService1",
                "AIGatewayService1",
                "memochat",
            ),
            "MediaGatewayService1": (
                "mediagateway",
                "MediaGatewayServer",
                "MediaService/mediagateway.ini",
                "MediaGatewayService-1",
                "8094",
                "",
                "8094",
                "",
                "../../artifacts/logs/services/MediaGatewayService1",
                "MediaGatewayService1",
                "memochat",
            ),
            "MomentsGatewayService1": (
                "momentsgateway",
                "MomentsGatewayServer",
                "MomentsService/momentsgateway.ini",
                "MomentsGatewayService-1",
                "8099",
                "",
                "8099",
                "",
                "../../artifacts/logs/services/MomentsGatewayService1",
                "MomentsGatewayService1",
                "memochat",
            ),
            "CallGatewayService1": (
                "callgateway",
                "CallGatewayServer",
                "CallService/callgateway.ini",
                "CallGatewayService-1",
                "8097",
                "",
                "8097",
                "",
                "../../artifacts/logs/services/CallGatewayService1",
                "CallGatewayService1",
                "memochat",
            ),
            "R18GatewayService1": (
                "r18gateway",
                "R18GatewayServer",
                "R18Service/r18gateway.ini",
                "R18GatewayService-1",
                "8098",
                "",
                "8098",
                "",
                "../../artifacts/logs/services/R18GatewayService1",
                "R18GatewayService1",
                "memochat",
            ),
            "RegisterService1": (
                "register",
                "RegisterServer",
                "RegisterService/register.ini",
                "RegisterService-1",
                "8101",
                "",
                "8101",
                "",
                "../../artifacts/logs/services/RegisterService1",
                "RegisterService1",
                "memochat",
            ),
            "LoginService1": (
                "login",
                "LoginServer",
                "LoginService/login.ini",
                "LoginService-1",
                "8102",
                "",
                "8102",
                "",
                "../../artifacts/logs/services/LoginService1",
                "LoginService1",
                "memochat",
            ),
            "AccountService1": (
                "account",
                "AccountServer",
                "AccountService/account.ini",
                "AccountService-1",
                "8103",
                "",
                "8103",
                "",
                "../../artifacts/logs/services/AccountService1",
                "AccountService1",
                "memochat",
            ),
            "AIServer": (
                "ai",
                "AIServer",
                "AIServer/config.ini",
                "AIServer",
                "8095",
                "",
                "8095",
                "",
                "../../artifacts/logs/services/AIServer",
                "AIServer",
                "memochat",
            ),
            "VarifyServer1": (
                "varify",
                "VarifyServer",
                "VarifyServer/config.ini",
                "VarifyServer-1",
                "50051",
                "",
                "50051 8083",
                "",
                "../../artifacts/logs/services/VarifyServer1",
                "VarifyServer1",
                "memochat",
            ),
        }

        self.assertEqual(set(expected), set(by_dir))
        for runtime_dir, expected_values in expected.items():
            (
                group,
                exe,
                config,
                display_name,
                tcp_port,
                udp_ports,
                stop_tcp,
                stop_udp,
                log_dir,
                telemetry_service_name,
                telemetry_namespace,
            ) = expected_values
            row = by_dir[runtime_dir]
            with self.subTest(runtime_dir=runtime_dir):
                self.assertEqual(group, row["group"])
                self.assertEqual(exe, row["executable"])
                self.assertEqual(exe, row["source_executable"])
                self.assertEqual(config, row["config_source"])
                self.assertEqual(display_name, row["display_name"])
                self.assertEqual(tcp_port, row["tcp_wait_port"])
                self.assertEqual(udp_ports, row["udp_wait_ports"])
                self.assertEqual(stop_tcp, row["stop_tcp_ports"])
                self.assertEqual(stop_udp, row["stop_udp_ports"])
                self.assertEqual(log_dir, row["log_dir"])
                self.assertEqual(telemetry_service_name, row["telemetry_service_name"])
                self.assertEqual(telemetry_namespace, row["telemetry_namespace"])

        for source in (deploy, start, stop):
            self.assertIn('TOPOLOGY_FILE="${SCRIPT_DIR}/runtime_topology.sh"', source)
            self.assertIn('source "$TOPOLOGY_FILE"', source)

        self.assertIn("deploy_topology_services", deploy)
        self.assertIn("start_topology_core_groups", start)
        self.assertIn('for group in "${MEMOCHAT_CORE_START_GROUPS[@]}"', start)
        self.assertIn('launch_topology_group "$group"', start)
        self.assertIn('wait_for_topology_group_udp_ports "$MEMOCHAT_TOPOLOGY_GROUP_CHAT" "QUIC"', start)
        self.assertIn("stop_topology_port_groups", stop)
        self.assertIn('for row in "${MEMOCHAT_STOP_PORT_GROUP_ORDER[@]}"', stop)
        self.assertIn('stop_group_tcp_ports "$label" "$group"', stop)
        self.assertIn('stop_group_udp_ports "$label" "$group"', stop)

    def test_linux_runtime_topology_manifest_owns_start_and_stop_order(self):
        start = read(START_SERVICES_SCRIPT)
        stop = read(REPO_ROOT / "tools/scripts/status/stop-all-services.sh")
        rows = topology_rows()

        self.assertEqual(["varify", "chat", "ai"], topology_array_values("MEMOCHAT_CORE_START_GROUPS"))
        self.assertEqual(
            [
                "aigateway|AIGatewayServer|tcp",
                "mediagateway|MediaGatewayServer|tcp",
                "momentsgateway|MomentsGatewayServer|tcp",
                "callgateway|CallGatewayServer|tcp",
                "r18gateway|R18GatewayServer|tcp",
                "register|RegisterServer|tcp",
                "login|LoginServer|tcp",
                "account|AccountServer|tcp",
                "ai|AIServer|tcp",
                "message_service|ChatMessageService|tcp",
                "relation_service|ChatRelationServiceWorker|tcp",
                "relation_query|ChatRelationQueryService|tcp",
                "chat|ChatServer|tcp udp",
                "varify|VarifyServer|tcp",
            ],
            topology_array_values("MEMOCHAT_STOP_PORT_GROUP_ORDER"),
        )
        self.assertEqual(
            [
                "AIGatewayService-1",
                "MediaGatewayService-1",
                "MomentsGatewayService-1",
                "CallGatewayService-1",
                "R18GatewayService-1",
                "RegisterService-1",
                "LoginService-1",
                "AccountService-1",
                "AIServer",
                "ChatDeliveryWorker-1",
                "ChatMessageService-1",
                "ChatRelationServiceWorker-1",
                "ChatRelationQueryService-1",
                "ChatServer-1",
                "VarifyServer-1",
            ],
            topology_array_values("MEMOCHAT_STOP_PID_ORDER"),
        )
        self.assertEqual(
            [
                "AIGatewayServer",
                "MediaGatewayServer",
                "MomentsGatewayServer",
                "CallGatewayServer",
                "R18GatewayServer",
                "RegisterServer",
                "LoginServer",
                "AccountServer",
                "AIServer",
                "ChatDeliveryWorker",
                "ChatMessageService",
                "ChatRelationServiceWorker",
                "ChatRelationQueryService",
                "ChatServer",
                "VarifyServer",
            ],
            topology_array_values("MEMOCHAT_STOP_EXECUTABLE_ORDER"),
        )

        display_names = [row["display_name"] for row in rows]
        self.assertEqual(set(display_names), set(topology_array_values("MEMOCHAT_STOP_PID_ORDER")))

        for group in topology_array_values("MEMOCHAT_CORE_START_GROUPS"):
            with self.subTest(group=group):
                self.assertTrue(any(row["group"] == group for row in rows))

        for stop_row in topology_array_values("MEMOCHAT_STOP_PORT_GROUP_ORDER"):
            group, label, protocols = stop_row.split("|")
            with self.subTest(group=group):
                self.assertTrue(any(row["group"] == group for row in rows))
                self.assertIn(
                    label,
                    {
                        "GateServer",
                        "AIGatewayServer",
                        "MediaGatewayServer",
                        "MomentsGatewayServer",
                        "CallGatewayServer",
                        "R18GatewayServer",
                        "RegisterServer",
                        "LoginServer",
                        "AccountServer",
                        "AIServer",
                        "ChatMessageService",
                        "ChatRelationServiceWorker",
                        "ChatRelationQueryService",
                        "ChatServer",
                        "VarifyServer",
                    },
                )
                self.assertTrue(set(protocols.split()).issubset({"tcp", "udp"}))

        self.assertIn("start_topology_core_group", start)
        self.assertIn('label="$(memochat_topology_group_label "$group")"', start)
        self.assertIn('for group in "${MEMOCHAT_CORE_START_GROUPS[@]}"', start)
        self.assertIn('start_topology_core_group "$group"', start)
        self.assertNotIn('launch_topology_group "$MEMOCHAT_TOPOLOGY_GROUP_VARIFY"', start)
        self.assertNotIn('launch_topology_group "$MEMOCHAT_TOPOLOGY_GROUP_AI"', start)
        # GateServer monolith is fully retired: no START_GATE opt-in, no --start-gate.
        self.assertNotIn("START_GATE", start)
        self.assertNotIn("--start-gate", start)

        self.assertIn('for row in "${MEMOCHAT_STOP_PORT_GROUP_ORDER[@]}"', stop)
        self.assertNotIn('stop_group_tcp_ports "GateServer" "$MEMOCHAT_TOPOLOGY_GROUP_GATE"', stop)
        self.assertNotIn('stop_group_tcp_ports "AIServer" "$MEMOCHAT_TOPOLOGY_GROUP_AI"', stop)
        self.assertNotIn('stop_group_tcp_ports "ChatServer" "$MEMOCHAT_TOPOLOGY_GROUP_CHAT"', stop)
        self.assertNotIn('stop_group_tcp_ports "VarifyServer" "$MEMOCHAT_TOPOLOGY_GROUP_VARIFY"', stop)

    def test_local_envoy_no_longer_depends_on_gateserver_backend(self):
        source = read(LOCAL_ENVOY_CONFIG)

        self.assertNotIn("name: gate_backend", source)
        self.assertNotIn("cluster: gate_backend", source)
        self.assertIn("cluster: account_backend", source)
        self.assertIn("path: /__envoy_failover_probe", source)
        self.assertRegex(
            source,
            r"name:\s+other\s+match:\s+prefix:\s+/\s+direct_response:\s+status:\s+404",
            "local Envoy catch-all must return 404 instead of falling back to GateServer",
        )

    def test_linux_runtime_topology_observability_labels_match_service_configs(self):
        for row in topology_rows():
            config_path = REPO_ROOT / "apps/server/core" / row["config_source"]
            self.assertTrue(config_path.is_file(), f"missing config source: {row['config_source']}")
            config = read_ini(config_path)
            with self.subTest(runtime_dir=row["runtime_dir"]):
                self.assertTrue(config.has_section("Log"))
                self.assertTrue(config.has_section("Telemetry"))
                self.assertEqual(config.get("Log", "Dir"), row["log_dir"])
                self.assertEqual(config.get("Telemetry", "ServiceName"), row["telemetry_service_name"])
                self.assertEqual(config.get("Telemetry", "ServiceNamespace"), row["telemetry_namespace"])

    def test_local_redpanda_has_host_and_docker_network_listeners(self):
        local_compose = read(LOCAL_COMPOSE)
        ai_compose = read(AI_DOCKER_COMPOSE)

        for token in (
            "INTERNAL://0.0.0.0:9092,OUTSIDE://0.0.0.0:19092",
            "INTERNAL://memochat-redpanda:9092,OUTSIDE://127.0.0.1:19092",
            "--pandaproxy-addr=0.0.0.0:18082",
            "--advertise-pandaproxy-addr=127.0.0.1:18082",
        ):
            self.assertIn(token, local_compose)
        self.assertNotIn("--advertise-kafka-addr=host.docker.internal:19092", local_compose)

        self.assertIn("MEMOCHAT_AI_AGENT_QUEUE__REDPANDA__BOOTSTRAP_SERVERS:-memochat-redpanda:9092", ai_compose)
        self.assertIn("memochat_default", ai_compose)
        self.assertIn("external: true", ai_compose)

    def test_gpt_sovits_voice_script_recreates_ai_orchestrator_with_reference_audio(self):
        source = read(GPT_SOVITS_VOICE_SCRIPT)

        for token in (
            "smoke_gpt_sovits_tts_wsl.sh",
            "MEMOCHAT_PET_VOICE_PROVIDER",
            "gpt-sovits",
            "MEMOCHAT_PET_SOVITS_BASE_URL",
            "http://host.docker.internal:9880",
            "MEMOCHAT_PET_SOVITS_REFERENCE_AUDIO",
            "refs/kafuu-chino-ref.wav",
            "MEMOCHAT_PET_SOVITS_PROMPT_TEXT",
            "MEMOCHAT_PET_SOVITS_PROMPT_LANGUAGE",
            "MEMOCHAT_PET_SOVITS_TEXT_LANGUAGE",
            "MEMOCHAT_PET_SOVITS_HOST_DATA_DIR",
            "GPT_SOVITS_HOST",
            "0.0.0.0",
            "pkill -f",
            "docker compose",
            "up -d --force-recreate memochat-ai-orchestrator",
            "/pet/diagnostics/voice?probe_endpoint=true",
        ):
            self.assertIn(token, source)

    def test_gpt_sovits_api_launcher_daemonizes_and_waits_for_docs(self):
        source = read(REPO_ROOT / "tools/scripts/pet/start_gpt_sovits_api_wsl.sh")

        for token in (
            "PID_FILE",
            "START_WAIT_SECONDS",
            "--foreground",
            "nohup",
            "wait_for_api",
            "GPT-SoVITS already ready",
            "GPT-SoVITS ready (pid=",
            "GPT-SoVITS failed to become ready",
        ):
            self.assertIn(token, source)

    def test_gpt_sovits_smoke_starts_launcher_directly(self):
        source = read(REPO_ROOT / "tools/scripts/pet/smoke_gpt_sovits_tts_wsl.sh")

        self.assertNotIn('setsid -f "$START_SCRIPT"', source)
        self.assertIn('"$START_SCRIPT"', source)


if __name__ == "__main__":
    unittest.main()
