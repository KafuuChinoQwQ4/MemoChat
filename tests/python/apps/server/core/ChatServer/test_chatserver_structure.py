import re
import unittest

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CHATSERVER_DIR = REPO_ROOT / "apps" / "server" / "core" / "ChatServer"
SERVER_CORE_DIR = REPO_ROOT / "apps" / "server" / "core"
COMMON_PROTO_DIR = SERVER_CORE_DIR / "common" / "proto"
DOMAIN_DIR = CHATSERVER_DIR / "domain"
DOMAIN_DELIVERY_DIR = DOMAIN_DIR / "delivery"
DOMAIN_MESSAGE_DIR = DOMAIN_DIR / "message"
DOMAIN_ORCHESTRATION_DIR = DOMAIN_DIR / "orchestration"
DOMAIN_RELATION_DIR = DOMAIN_DIR / "relation"
DOMAIN_SESSION_DIR = DOMAIN_DIR / "session"
DOMAIN_USERS_DIR = DOMAIN_DIR / "users"


EXPECTED_GROUPS = {
    "app": {
        "ChatDeliveryWorker.cpp",
        "ChatMessageService.cpp",
        "ChatRelationQueryService.cpp",
        "ChatRelationServiceWorker.cpp",
        "ChatServer.cpp",
        "ChatRuntime.cpp",
        "ChatRuntime.hpp",
        "ChatServerCore.cpp",
    },
    "transport": {
        "CServer.cpp",
        "CServer.hpp",
        "CSession.cpp",
        "CSession.hpp",
        "ChatFrameCodec.cpp",
        "ChatFrameCodec.hpp",
        "ChatFrameDispatch.cpp",
        "ChatFrameDispatch.hpp",
        "ChatIngressCoordinator.cpp",
        "ChatIngressCoordinator.hpp",
        "ChatSessionCleanupSupport.cpp",
        "ChatSessionCleanupSupport.hpp",
        "ChatSessionState.hpp",
        "ChatServiceImpl.cpp",
        "ChatServiceImpl.hpp",
        "IChatSession.hpp",
        "IWebTransportProvider.hpp",
        "IWebTransportSession.hpp",
        "LibwebsocketsWebTransportProvider.cpp",
        "LibwebsocketsWebTransportProvider.hpp",
        "MsgNode.cpp",
        "MsgNode.hpp",
        "QuicChatServer.cpp",
        "QuicChatServer.hpp",
        "QuicSession.cpp",
        "QuicSession.hpp",
        "TcpSession.cpp",
        "TcpSession.hpp",
        "UnavailableWebTransportProvider.cpp",
        "UnavailableWebTransportProvider.hpp",
        "WebSocketChatServer.cpp",
        "WebSocketChatServer.hpp",
        "WebSocketSession.cpp",
        "WebSocketSession.hpp",
        "WebTransportChatServer.cpp",
        "WebTransportChatServer.hpp",
        "WebTransportProviderFactory.cpp",
        "WebTransportProviderFactory.hpp",
        "WebTransportSession.cpp",
        "WebTransportSession.hpp",
    },
    "domain": {
        "delivery/ChatDeliveryRuntime.cpp",
        "delivery/ChatDeliveryRuntime.hpp",
        "delivery/TaskDispatcher.cpp",
        "delivery/TaskDispatcher.hpp",
        "message/GroupManagementService.cpp",
        "message/GroupManagementService.hpp",
        "message/GroupMembershipService.cpp",
        "message/GroupMembershipService.hpp",
        "message/GroupMessageHistoryService.cpp",
        "message/GroupMessageHistoryService.hpp",
        "message/GroupMessageService.cpp",
        "message/GroupMessageService.hpp",
        "message/GroupMessageWorkflow.cpp",
        "message/GroupMessageWorkflow.hpp",
        "message/ChatMessageInternalGrpcService.cpp",
        "message/ChatMessageInternalGrpcService.hpp",
        "message/MessageDeliveryService.cpp",
        "message/MessageDeliveryService.hpp",
        "message/MessageServiceFactory.cpp",
        "message/MessageServiceFactory.hpp",
        "message/PrivateMessageService.cpp",
        "message/PrivateMessageService.hpp",
        "orchestration/ChatHandlerRegistrars.cpp",
        "orchestration/ChatHandlerRegistrars.hpp",
        "orchestration/ChatRuntimeComposition.cpp",
        "orchestration/ChatRuntimeComposition.hpp",
        "orchestration/LogicSystem.cpp",
        "orchestration/LogicSystem.hpp",
        "relation/ChatRelationInternalGrpcService.cpp",
        "relation/ChatRelationInternalGrpcService.hpp",
        "relation/ChatRelationSessionAdapter.cpp",
        "relation/ChatRelationSessionAdapter.hpp",
        "relation/ChatRelationService.cpp",
        "relation/ChatRelationService.hpp",
        "relation/RelationServiceFactory.cpp",
        "relation/RelationServiceFactory.hpp",
        "ports/IRelationSessionService.hpp",
        "session/ChatSessionService.cpp",
        "session/ChatSessionService.hpp",
        "users/ChatUserSupport.cpp",
        "users/ChatUserSupport.hpp",
        "users/UserMgr.cpp",
        "users/UserMgr.hpp",
    },
    "persistence": {
        "ChatOutboxService.cpp",
        "ChatOutboxService.hpp",
        "ChatSessionRepository.cpp",
        "ChatSessionRepository.hpp",
        "DistLock.cpp",
        "DistLock.hpp",
        "MongoDao.cpp",
        "MongoDao.hpp",
        "MongoMgr.cpp",
        "MongoMgr.hpp",
        "PostgresDao.cpp",
        "PostgresDao.hpp",
        "PostgresDaoDialogs.cpp",
        "PostgresDaoGroupMessages.cpp",
        "PostgresDaoGroups.cpp",
        "PostgresDaoPrivateMessages.cpp",
        "PostgresDaoUsers.cpp",
        "PostgresMgr.cpp",
        "PostgresMgr.hpp",
        "PostgresPool.cpp",
        "PostgresPool.hpp",
        "ChatMessageRepository.cpp",
        "ChatMessageRepository.hpp",
        "ChatOutboxRepairScheduler.cpp",
        "ChatOutboxRepairScheduler.hpp",
        "ChatRelationRepository.cpp",
        "ChatRelationRepository.hpp",
        "RedisRelationBootstrapCache.cpp",
        "RedisRelationBootstrapCache.hpp",
        "RedisOnlineRouteStore.cpp",
        "RedisOnlineRouteStore.hpp",
        "RedisMgr.cpp",
        "RedisMgr.hpp",
    },
    "messaging": {
        "AsyncEventDispatcher.cpp",
        "AsyncEventDispatcher.hpp",
        "ChatAsyncEvent.cpp",
        "ChatAsyncEvent.hpp",
        "ChatTaskEnvelope.cpp",
        "ChatTaskEnvelope.hpp",
        "IAsyncEventBus.hpp",
        "IAsyncTaskBus.hpp",
        "InlineTaskBus.cpp",
        "InlineTaskBus.hpp",
        "KafkaAsyncEventBus.cpp",
        "KafkaAsyncEventBus.hpp",
        "KafkaConfig.cpp",
        "KafkaConfig.hpp",
        "RabbitMqConfig.cpp",
        "RabbitMqConfig.hpp",
        "RabbitMqTaskBus.cpp",
        "RabbitMqTaskBus.hpp",
        "RedisAsyncEventBus.cpp",
        "RedisAsyncEventBus.hpp",
    },
    "clients": {
        "ChatGrpcClient.cpp",
        "ChatGrpcClient.hpp",
        "MessageGrpcClient.cpp",
        "MessageGrpcClient.hpp",
        "MessageGrpcServiceAdapter.cpp",
        "MessageGrpcServiceAdapter.hpp",
        "RelationGrpcClient.cpp",
        "RelationGrpcClient.hpp",
        "RelationGrpcServiceAdapter.cpp",
        "RelationGrpcServiceAdapter.hpp",
        "RelationQueryGrpcClient.cpp",
        "RelationQueryGrpcClient.hpp",
        "RelationQueryServiceFactory.cpp",
        "RelationQueryServiceFactory.hpp",
    },
    "config": {
        "ChatSessionConfig.cpp",
        "ChatSessionConfig.hpp",
        "ConfigMgr.cpp",
        "ConfigMgr.hpp",
        "MessageServiceConfig.cpp",
        "MessageServiceConfig.hpp",
        "RelationQueryServiceConfig.cpp",
        "RelationQueryServiceConfig.hpp",
        "RelationServiceConfig.cpp",
        "RelationServiceConfig.hpp",
    },
    "infrastructure": {
        "AsioIOServicePool.cpp",
        "AsioIOServicePool.hpp",
        "Singleton.hpp",
        "const.hpp",
        "data.hpp",
    },
}


ROOT_ALLOWLIST = {
    "_TREE.md",
    "CMakeLists.txt",
    "ChatServer.sln",
    "ChatServer.vcxproj",
    "ChatServer.vcxproj.filters",
    "ChatServer.vcxproj.user",
    "PropertySheet.props",
    "config.ini",
    "chatserver1.ini",
    "chatserver2.ini",
    "chatdeliveryworker1.ini",
    "chatmessageservice1.ini",
    "chatrelationquery1.ini",
    "chatrelationservice1.ini",
    "message.pb.cc",
    "message.pb.h",
    "message.grpc.pb.cc",
    "message.grpc.pb.h",
    "mysqlcppconn-9-vs14.dll",
    "mysqlcppconn8-2-vs14.dll",
    "start.bat",
}


def read_cmake() -> str:
    return (CHATSERVER_DIR / "CMakeLists.txt").read_text(encoding="utf-8")


EXPECTED_MICROSERVICE_TARGETS = {
    "ChatAppCore": {"app/ChatRuntime.cpp"},
    "ChatClientCore": {
        "clients/ChatGrpcClient.cpp",
        "clients/MessageGrpcClient.cpp",
        "clients/MessageGrpcServiceAdapter.cpp",
        "clients/RelationGrpcClient.cpp",
        "clients/RelationGrpcServiceAdapter.cpp",
        "clients/RelationQueryGrpcClient.cpp",
        "clients/RelationQueryServiceFactory.cpp",
    },
    "ChatConfigCore": {
        "config/ChatSessionConfig.cpp",
        "config/ConfigMgr.cpp",
        "config/MessageServiceConfig.cpp",
        "config/RelationQueryServiceConfig.cpp",
        "config/RelationServiceConfig.cpp",
    },
    "ChatInfrastructureCore": {"infrastructure/AsioIOServicePool.cpp"},
    "ChatPersistenceCore": {
        "persistence/ChatOutboxService.cpp",
        "persistence/ChatOutboxRepairScheduler.cpp",
        "persistence/ChatSessionRepository.cpp",
        "persistence/DistLock.cpp",
        "persistence/MongoDao.cpp",
        "persistence/MongoMgr.cpp",
        "persistence/PostgresDao.cpp",
        "persistence/PostgresDaoDialogs.cpp",
        "persistence/PostgresDaoGroupMessages.cpp",
        "persistence/PostgresDaoGroups.cpp",
        "persistence/PostgresDaoPrivateMessages.cpp",
        "persistence/PostgresDaoUsers.cpp",
        "persistence/PostgresMgr.cpp",
        "persistence/PostgresPool.cpp",
        "persistence/ChatMessageRepository.cpp",
        "persistence/ChatRelationRepository.cpp",
        "persistence/RedisRelationBootstrapCache.cpp",
        "persistence/RedisOnlineRouteStore.cpp",
        "persistence/RedisMgr.cpp",
    },
    "ChatMessagingCore": {
        "messaging/AsyncEventDispatcher.cpp",
        "messaging/ChatAsyncEvent.cpp",
        "messaging/ChatTaskEnvelope.cpp",
        "messaging/InlineTaskBus.cpp",
        "messaging/KafkaAsyncEventBus.cpp",
        "messaging/KafkaConfig.cpp",
        "messaging/RabbitMqConfig.cpp",
        "messaging/RabbitMqTaskBus.cpp",
        "messaging/RedisAsyncEventBus.cpp",
    },
    "ChatTransportCore": {
        "transport/CServer.cpp",
        "transport/ChatSessionCleanupSupport.cpp",
        "transport/CSession.cpp",
        "transport/ChatFrameCodec.cpp",
        "transport/ChatFrameDispatch.cpp",
        "transport/ChatIngressCoordinator.cpp",
        "transport/ChatServiceImpl.cpp",
        "transport/MsgNode.cpp",
        "transport/QuicChatServer.cpp",
        "transport/QuicSession.cpp",
        "transport/TcpSession.cpp",
        "transport/UnavailableWebTransportProvider.cpp",
        "transport/WebSocketChatServer.cpp",
        "transport/WebSocketSession.cpp",
        "transport/WebTransportChatServer.cpp",
        "transport/WebTransportProviderFactory.cpp",
        "transport/WebTransportSession.cpp",
    },
    "ChatOrchestrationCore": {
        "domain/orchestration/ChatHandlerRegistrars.cpp",
        "domain/orchestration/ChatRuntimeComposition.cpp",
        "domain/orchestration/LogicSystem.cpp",
    },
    "ChatSessionCore": {
        "domain/session/ChatSessionService.cpp",
        "domain/users/UserMgr.cpp",
    },
    "ChatUserSupportCore": {
        "domain/users/ChatUserSupport.cpp",
    },
    "ChatRelationCore": {
        "domain/relation/ChatRelationInternalGrpcService.cpp",
        "domain/relation/ChatRelationService.cpp",
        "domain/relation/RelationServiceFactory.cpp",
    },
    "ChatRelationSessionAdapterCore": {
        "domain/relation/ChatRelationSessionAdapter.cpp",
    },
    "ChatMessageCore": {
        "domain/message/ChatMessageInternalGrpcService.cpp",
        "domain/message/GroupManagementService.cpp",
        "domain/message/GroupMembershipService.cpp",
        "domain/message/GroupMessageHistoryService.cpp",
        "domain/message/GroupMessageService.cpp",
        "domain/message/GroupMessageWorkflow.cpp",
        "domain/message/MessageDeliveryService.cpp",
        "domain/message/MessageServiceFactory.cpp",
        "domain/message/PrivateMessageService.cpp",
    },
    "ChatDeliveryCore": {
        "domain/delivery/ChatDeliveryRuntime.cpp",
        "domain/delivery/TaskDispatcher.cpp",
    },
}


def extract_target_block(cmake: str, target_name: str) -> str:
    start = cmake.index(f"add_library({target_name} STATIC")
    next_target = cmake.find("\nadd_library(", start + 1)
    next_executable = cmake.find("\nadd_executable(", start + 1)
    candidates = [index for index in (next_target, next_executable) if index != -1]
    end = min(candidates) if candidates else len(cmake)
    return cmake[start:end]


def cmake_set_values(cmake: str, variable_name: str) -> set[str]:
    match = re.search(rf"set\({re.escape(variable_name)}\s+(.*?)\n\)", cmake, re.DOTALL)
    if not match:
        return set()
    return {line.strip() for line in match.group(1).splitlines() if line.strip() and not line.strip().startswith("#")}


def target_sources(cmake: str, target_block: str) -> set[str]:
    sources = set()
    for raw in target_block.splitlines():
        item = raw.strip()
        if not item or item.startswith("#"):
            continue
        variable_match = re.fullmatch(r"\$\{([A-Z0-9_]+)\}", item)
        if variable_match:
            sources.update(cmake_set_values(cmake, variable_match.group(1)))
        elif item.endswith(".cpp"):
            sources.add(item)
    return sources


def text_between(text: str, start_marker: str, end_marker: str) -> str:
    start = text.index(start_marker)
    end = text.index(end_marker, start)
    return text[start:end]


def extract_function(source: str, signature: str) -> str:
    start = source.index(signature)
    open_brace = source.index("{", start)
    depth = 0
    for index in range(open_brace, len(source)):
        char = source[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return source[start : index + 1]
    raise AssertionError(f"Function body not found for {signature}")


def logic_header_source() -> str:
    return (DOMAIN_ORCHESTRATION_DIR / "LogicSystem.hpp").read_text(encoding="utf-8")


def logic_source() -> str:
    return (DOMAIN_ORCHESTRATION_DIR / "LogicSystem.cpp").read_text(encoding="utf-8")


def runtime_composition_header() -> str:
    return (DOMAIN_ORCHESTRATION_DIR / "ChatRuntimeComposition.hpp").read_text(encoding="utf-8")


def runtime_composition_source() -> str:
    return (DOMAIN_ORCHESTRATION_DIR / "ChatRuntimeComposition.cpp").read_text(encoding="utf-8")


def extract_executable_link_block(cmake: str, target_name: str) -> str:
    match = re.search(
        rf"(?:target_link_libraries|chatserver_link_microservice_cores)\(\s*{re.escape(target_name)}\s+(?P<body>.*?)\)",
        cmake,
        re.S,
    )
    if not match:
        raise AssertionError(f"link block missing for {target_name}")
    return match.group("body")


def assert_executable_links(
    testcase: unittest.TestCase, cmake: str, target_name: str, expected_targets: set[str]
) -> None:
    block = extract_executable_link_block(cmake, target_name)
    testcase.assertNotIn("ChatServerCore", block, f"{target_name} must not link the full ChatServerCore aggregate")
    for expected in expected_targets:
        testcase.assertIn(expected, block, f"{target_name} should link {expected}")


def ini_section(text: str, section_name: str) -> str:
    marker = f"[{section_name}]"
    start = text.index(marker)
    next_section = text.find("\n[", start + len(marker))
    if next_section == -1:
        return text[start:]
    return text[start:next_section]


def assert_ini_section_has(testcase: unittest.TestCase, text: str, section_name: str, *tokens: str) -> None:
    section = ini_section(text, section_name)
    for token in tokens:
        testcase.assertIn(token, section)


class ChatServerStructureTests(unittest.TestCase):
    def test_chatserver_build_has_core_library_and_thin_executable(self):
        cmake = read_cmake()

        self.assertIn("add_library(ChatServerCore STATIC", cmake)
        self.assertIn("add_executable(ChatServer", cmake)
        self.assertIn("app/ChatServer.cpp", cmake)
        self.assertIn("target_link_libraries(ChatServer PRIVATE ChatServerCore)", cmake)

    def test_service_local_message_proto_copies_are_retired(self):
        canonical_proto = COMMON_PROTO_DIR / "message.proto"
        self.assertTrue(canonical_proto.is_file(), "canonical common/proto/message.proto is missing")

        for local_proto in (
            CHATSERVER_DIR / "message.proto",
            SERVER_CORE_DIR / "VarifyServer" / "message.proto",
        ):
            with self.subTest(proto=local_proto.relative_to(REPO_ROOT)):
                self.assertFalse(local_proto.exists(), "service-local message.proto copy should stay retired")

        start_script = (CHATSERVER_DIR / "start.bat").read_text(encoding="utf-8-sig")
        vcxproj = (CHATSERVER_DIR / "ChatServer.vcxproj").read_text(encoding="utf-8-sig")
        filters = (CHATSERVER_DIR / "ChatServer.vcxproj.filters").read_text(encoding="utf-8-sig")
        for text in (vcxproj, filters):
            with self.subTest(surface=text[:40]):
                self.assertIn("common\\proto\\message.proto", text)
                self.assertNotIn('Include="message.proto"', text)
        self.assertIn('set "PROTO_ROOT=%SERVER_CORE_DIR%\\common\\proto"', start_script)
        self.assertIn('set "PROTO_FILE=%PROTO_ROOT%\\message.proto"', start_script)
        self.assertNotIn("PROTO_FILE=%SCRIPT_DIR%\\message.proto", start_script)
        self.assertIn('--grpc_out="%SCRIPT_DIR%"', start_script)
        self.assertIn('--cpp_out="%SCRIPT_DIR%"', start_script)
        self.assertIn('<ClCompile Include="message.grpc.pb.cc" />', vcxproj)
        self.assertIn('<ClCompile Include="message.pb.cc" />', vcxproj)

    def test_chatserver_core_is_split_into_microservice_preparation_targets(self):
        cmake = read_cmake()

        for target_name, expected_sources in EXPECTED_MICROSERVICE_TARGETS.items():
            with self.subTest(target=target_name):
                self.assertIn(f"add_library({target_name} STATIC", cmake)
                target_block = extract_target_block(cmake, target_name)
                actual_sources = target_sources(cmake, target_block)
                for source in expected_sources:
                    self.assertIn(source, actual_sources)

        aggregate_block = extract_target_block(cmake, "ChatServerCore")
        for target_name in EXPECTED_MICROSERVICE_TARGETS:
            self.assertIn(target_name, aggregate_block)

        relation_core_block = extract_target_block(cmake, "ChatRelationCore")
        self.assertNotIn("domain/relation/ChatRelationSessionAdapter.cpp", relation_core_block)
        self.assertNotIn("ChatTransportCore", relation_core_block)

        self.assertNotIn("app/ChatServer.cpp", aggregate_block)
        self.assertNotIn("add_executable(ChatRelationService\n", cmake)

    def test_chat_delivery_worker_executable_is_compile_time_only_boundary(self):
        worker_source = CHATSERVER_DIR / "app" / "ChatDeliveryWorker.cpp"
        self.assertTrue(worker_source.is_file(), "ChatDeliveryWorker source is missing")

        worker = worker_source.read_text(encoding="utf-8")
        self.assertIn("memolog::Logger::Init(delivery_worker_modules::LoggerName(), log_cfg)", worker)
        self.assertIn("memolog::Telemetry::Init(delivery_worker_modules::LoggerName(), telemetry_cfg)", worker)
        self.assertIn("LogicSystem::GetInstance()", worker)
        self.assertNotIn("ChatIngressCoordinator", worker)
        self.assertNotIn("grpc::ServerBuilder", worker)
        self.assertNotIn("CServer", worker)

        cmake = read_cmake()
        self.assertIn("add_executable(ChatDeliveryWorker", cmake)
        self.assertIn("app/ChatDeliveryWorker.cpp", cmake)
        self.assertIn("memochat_configure_server_target(ChatDeliveryWorker)", cmake)
        assert_executable_links(
            self,
            cmake,
            "ChatDeliveryWorker",
            {
                "ChatAppCore",
                "ChatConfigCore",
                "ChatDeliveryCore",
                "ChatMessagingCore",
                "ChatOrchestrationCore",
                "ChatPersistenceCore",
                "ChatRelationCore",
                "ChatSessionCore",
                "ChatTransportCore",
                "ChatUserSupportCore",
            },
        )

    def test_chat_delivery_worker_runtime_config_is_worker_only(self):
        config_path = CHATSERVER_DIR / "chatdeliveryworker1.ini"
        self.assertTrue(config_path.is_file(), "ChatDeliveryWorker runtime config is missing")

        config = config_path.read_text(encoding="utf-8-sig")
        self.assertIn("[SelfServer]", config)
        self.assertIn("Name=chatdeliveryworker1", config)
        self.assertIn("[Snowflake]", config)
        self.assertIn("WorkerId=7", config)
        self.assertNotIn("WorkerId=101", config)
        self.assertIn("[Runtime]", config)
        self.assertIn("Role=worker", config)
        self.assertNotIn("Role=hybrid", config)
        self.assertIn("[RelationService]", config)
        self.assertIn("Backend=inprocess", config)
        self.assertIn("ClientId=memochat-chatdeliveryworker1", config)
        self.assertIn("Dir=../../artifacts/logs/services/chatdeliveryworker1", config)
        self.assertIn("ServiceName=ChatDeliveryWorker", config)

    def test_chat_delivery_worker_runtime_scripts_are_explicitly_gated(self):
        deploy = (REPO_ROOT / "tools" / "scripts" / "status" / "deploy_services.sh").read_text(encoding="utf-8")
        start = (REPO_ROOT / "tools" / "scripts" / "status" / "start-all-services.sh").read_text(encoding="utf-8")
        stop = (REPO_ROOT / "tools" / "scripts" / "status" / "stop-all-services.sh").read_text(encoding="utf-8")
        topology = (REPO_ROOT / "tools" / "scripts" / "status" / "runtime_topology.sh").read_text(encoding="utf-8")

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

        self.assertIn("MEMOCHAT_START_CHAT_DELIVERY_WORKER", start)
        self.assertIn("--start-chat-delivery-worker", start)
        self.assertIn("--skip-core-services", start)
        self.assertIn("verify_started_pid", start)
        self.assertIn("exited early", start)
        self.assertIn("[STEP] Start ChatDeliveryWorker", start)
        self.assertIn('launch_topology_group "$MEMOCHAT_TOPOLOGY_GROUP_WORKER"', start)
        self.assertIn("start_topology_core_groups", start)
        self.assertIn('for group in "${MEMOCHAT_CORE_START_GROUPS[@]}"', start)
        self.assertIn('MEMOCHAT_INSTANCE_NAME="$instance_name"', start)

        self.assertIn("ChatDeliveryWorker-1", topology)
        self.assertIn("MEMOCHAT_STOP_PID_ORDER", topology)
        self.assertIn('for name in "${MEMOCHAT_STOP_PID_ORDER[@]}"', stop)
        self.assertIn("MEMOCHAT_STOP_PORT_GROUP_ORDER", topology)
        self.assertIn("stop_topology_port_groups", stop)
        self.assertIn("MEMOCHAT_STOP_EXECUTABLE_ORDER", topology)
        self.assertIn('"ChatDeliveryWorker"', topology)
        self.assertIn('stop_by_name_under_runtime "$exe_name" "$exe_name"', stop)

    def test_chat_relation_query_service_executable_is_internal_grpc_boundary(self):
        service_source = CHATSERVER_DIR / "app" / "ChatRelationQueryService.cpp"
        config_path = CHATSERVER_DIR / "chatrelationquery1.ini"

        self.assertTrue(service_source.is_file(), "ChatRelationQueryService source is missing")
        self.assertTrue(config_path.is_file(), "ChatRelationQueryService runtime config is missing")

        source = service_source.read_text(encoding="utf-8")
        for token in (
            "memolog::Logger::Init(relation_query_service_modules::LoggerName(), log_cfg)",
            "memolog::Telemetry::Init(relation_query_service_modules::LoggerName(), telemetry_cfg)",
            "ChatRelationInternalGrpcService",
            "ChatRelationService",
            "ChatRelationRepository",
            "RedisRelationBootstrapCache",
            "grpc::ServerBuilder",
            "builder.RegisterService(&relation_grpc_service)",
            "ConfigMgr::InitConfigPath(ParseConfigPath(argc, argv))",
            'ConfigValueOrDefault(cfg, "RelationQueryRpc", "Host", relation_query_service_modules::DefaultRpcHost())',
            'ConfigValueOrDefault(cfg, "RelationQueryRpc", "Port", relation_query_service_modules::DefaultRpcPort())',
        ):
            self.assertIn(token, source)
        self.assertNotIn("ChatIngressCoordinator", source)
        self.assertNotIn("LogicSystem::GetInstance()", source)
        self.assertNotIn("CServer", source)

        cmake = read_cmake()
        self.assertIn("add_executable(ChatRelationQueryService", cmake)
        self.assertIn("app/ChatRelationQueryService.cpp", cmake)
        self.assertIn("memochat_configure_server_target(ChatRelationQueryService)", cmake)
        assert_executable_links(
            self,
            cmake,
            "ChatRelationQueryService",
            {
                "ChatAppCore",
                "ChatConfigCore",
                "ChatMessagingCore",
                "ChatPersistenceCore",
                "ChatRelationCore",
                "ChatUserSupportCore",
            },
        )

        config = config_path.read_text(encoding="utf-8-sig")
        for token in (
            "[SelfServer]",
            "Name=chatrelationquery1",
            "[Runtime]",
            "Role=worker",
            "[RelationQueryRpc]",
            "Host=127.0.0.1",
            "Port=50090",
            "[RelationQueryService]",
            "Backend=inprocess",
            "Dir=../../artifacts/logs/services/chatrelationquery1",
            "ServiceName=ChatRelationQueryService",
            "ServiceNamespace=memochat",
        ):
            self.assertIn(token, config)
        self.assertNotIn("[chatmessageservice1]", config)

    def test_chat_relation_query_service_runtime_scripts_are_default_started_and_overrideable(self):
        deploy = (REPO_ROOT / "tools" / "scripts" / "status" / "deploy_services.sh").read_text(encoding="utf-8")
        start = (REPO_ROOT / "tools" / "scripts" / "status" / "start-all-services.sh").read_text(encoding="utf-8")
        stop = (REPO_ROOT / "tools" / "scripts" / "status" / "stop-all-services.sh").read_text(encoding="utf-8")
        topology = (REPO_ROOT / "tools" / "scripts" / "status" / "runtime_topology.sh").read_text(encoding="utf-8")

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

        self.assertIn("ChatRelationQueryService-1", topology)
        self.assertIn('"ChatRelationQueryService"', topology)
        self.assertIn('for name in "${MEMOCHAT_STOP_PID_ORDER[@]}"', stop)
        self.assertIn('stop_by_name_under_runtime "$exe_name" "$exe_name"', stop)
        core_start_groups = text_between(
            topology, "MEMOCHAT_CORE_START_GROUPS=(", ")\n\nreadonly -a MEMOCHAT_STOP_PORT_GROUP_ORDER"
        )
        self.assertNotIn('"relation_query"', core_start_groups)

    def test_chat_relation_service_worker_executable_is_internal_grpc_boundary(self):
        service_source = CHATSERVER_DIR / "app" / "ChatRelationServiceWorker.cpp"
        config_path = CHATSERVER_DIR / "chatrelationservice1.ini"

        self.assertTrue(service_source.is_file(), "ChatRelationServiceWorker source is missing")
        self.assertTrue(config_path.is_file(), "ChatRelationServiceWorker runtime config is missing")

        source = service_source.read_text(encoding="utf-8")
        for token in (
            "memolog::Logger::Init(relation_service_worker_modules::LoggerName(), log_cfg)",
            "memolog::Telemetry::Init(relation_service_worker_modules::LoggerName(), telemetry_cfg)",
            "ChatRelationInternalGrpcService",
            "ChatRelationService",
            "ChatRelationRepository",
            "RedisRelationBootstrapCache",
            "grpc::ServerBuilder",
            "builder.RegisterService(&relation_grpc_service)",
            "ConfigMgr::InitConfigPath(ParseConfigPath(argc, argv))",
            'ConfigValueOrDefault(cfg, "RelationServiceRpc", "Host", relation_service_worker_modules::DefaultRpcHost())',
            'ConfigValueOrDefault(cfg, "RelationServiceRpc", "Port", relation_service_worker_modules::DefaultRpcPort())',
        ):
            self.assertIn(token, source)
        self.assertNotIn("ChatIngressCoordinator", source)
        self.assertNotIn("LogicSystem::GetInstance()", source)
        self.assertNotIn("CServer", source)

        cmake = read_cmake()
        self.assertIn("add_executable(ChatRelationServiceWorker", cmake)
        self.assertIn("app/ChatRelationServiceWorker.cpp", cmake)
        self.assertIn("memochat_configure_server_target(ChatRelationServiceWorker)", cmake)
        assert_executable_links(
            self,
            cmake,
            "ChatRelationServiceWorker",
            {
                "ChatAppCore",
                "ChatConfigCore",
                "ChatMessagingCore",
                "ChatPersistenceCore",
                "ChatRelationCore",
                "ChatUserSupportCore",
            },
        )

        config = config_path.read_text(encoding="utf-8-sig")
        for token in (
            "[SelfServer]",
            "Name=chatrelationservice1",
            "[Runtime]",
            "Role=worker",
            "[RelationServiceRpc]",
            "Host=127.0.0.1",
            "Port=50091",
            "[RelationService]",
            "Backend=inprocess",
            "Dir=../../artifacts/logs/services/chatrelationservice1",
            "ServiceName=ChatRelationServiceWorker",
            "ServiceNamespace=memochat",
        ):
            self.assertIn(token, config)
        self.assertNotIn("[chatmessageservice1]", config)

    def test_chat_relation_service_worker_runtime_scripts_are_default_started_and_overrideable(self):
        deploy = (REPO_ROOT / "tools" / "scripts" / "status" / "deploy_services.sh").read_text(encoding="utf-8")
        start = (REPO_ROOT / "tools" / "scripts" / "status" / "start-all-services.sh").read_text(encoding="utf-8")
        stop = (REPO_ROOT / "tools" / "scripts" / "status" / "stop-all-services.sh").read_text(encoding="utf-8")
        topology = (REPO_ROOT / "tools" / "scripts" / "status" / "runtime_topology.sh").read_text(encoding="utf-8")

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

        self.assertIn("ChatRelationServiceWorker-1", topology)
        self.assertIn('"ChatRelationServiceWorker"', topology)
        self.assertIn('for name in "${MEMOCHAT_STOP_PID_ORDER[@]}"', stop)
        self.assertIn('stop_by_name_under_runtime "$exe_name" "$exe_name"', stop)
        core_start_groups = text_between(
            topology, "MEMOCHAT_CORE_START_GROUPS=(", ")\n\nreadonly -a MEMOCHAT_STOP_PORT_GROUP_ORDER"
        )
        self.assertNotIn('"relation_service"', core_start_groups)

    def test_chat_message_service_executable_is_internal_grpc_boundary(self):
        service_source = CHATSERVER_DIR / "app" / "ChatMessageService.cpp"
        config_path = CHATSERVER_DIR / "chatmessageservice1.ini"

        self.assertTrue(service_source.is_file(), "ChatMessageService source is missing")
        self.assertTrue(config_path.is_file(), "ChatMessageService runtime config is missing")

        source = service_source.read_text(encoding="utf-8")
        for token in (
            "memolog::Logger::Init(message_service_modules::LoggerName(), log_cfg)",
            "memolog::Telemetry::Init(message_service_modules::LoggerName(), telemetry_cfg)",
            "ChatMessageInternalGrpcService",
            "PrivateMessageService",
            "GroupMessageService",
            "ChatMessageRepository",
            "ChatRelationRepository",
            "MessageDeliveryService",
            "RedisOnlineRouteStore",
            "DisabledMessageEventPublisher",
            "grpc::ServerBuilder",
            "static_cast<chatinternal::ChatPrivateMessageInternalService::Service*>(&message_grpc_service)",
            "static_cast<chatinternal::ChatGroupMessageInternalService::Service*>(&message_grpc_service)",
            "ConfigMgr::InitConfigPath(ParseConfigPath(argc, argv))",
            'ConfigValueOrDefault(cfg, "MessageServiceRpc", "Host", message_service_modules::DefaultRpcHost())',
            'ConfigValueOrDefault(cfg, "MessageServiceRpc", "Port", message_service_modules::DefaultRpcPort())',
        ):
            self.assertIn(token, source)
        self.assertNotIn("ChatIngressCoordinator", source)
        self.assertNotIn("LogicSystem::GetInstance()", source)
        self.assertNotIn("CServer", source)

        cmake = read_cmake()
        self.assertIn("add_executable(ChatMessageService", cmake)
        self.assertIn("app/ChatMessageService.cpp", cmake)
        self.assertIn("memochat_configure_server_target(ChatMessageService)", cmake)
        assert_executable_links(
            self,
            cmake,
            "ChatMessageService",
            {
                "ChatAppCore",
                "ChatConfigCore",
                "ChatDeliveryCore",
                "ChatMessageCore",
                "ChatOrchestrationCore",
                "ChatPersistenceCore",
                "ChatRelationCore",
                "ChatRelationSessionAdapterCore",
                "ChatSessionCore",
                "ChatTransportCore",
                "ChatUserSupportCore",
            },
        )
        self.assertIn(
            "add_executable(chatserver_message_remote_smoke_gtest",
            (REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt").read_text(
                encoding="utf-8"
            ),
        )

        config = config_path.read_text(encoding="utf-8-sig")
        for token in (
            "[SelfServer]",
            "Name=chatmessageservice1",
            "[Runtime]",
            "Role=worker",
            "[MessageServiceRpc]",
            "Host=127.0.0.1",
            "Port=50092",
            "[MessageService]",
            "Backend=inprocess",
            "[FeatureFlags]",
            "chat_private_kafka_shadow=false",
            "chat_private_kafka_primary=false",
            "chat_group_kafka_shadow=false",
            "chat_group_kafka_primary=false",
            "Dir=../../artifacts/logs/services/chatmessageservice1",
            "ServiceName=ChatMessageService",
            "ServiceNamespace=memochat",
        ):
            self.assertIn(token, config)
        self.assertNotIn("[chatmessageservice1]", config)

    def test_chat_message_service_runtime_scripts_are_default_started_and_overrideable(self):
        deploy = (REPO_ROOT / "tools" / "scripts" / "status" / "deploy_services.sh").read_text(encoding="utf-8")
        start = (REPO_ROOT / "tools" / "scripts" / "status" / "start-all-services.sh").read_text(encoding="utf-8")
        stop = (REPO_ROOT / "tools" / "scripts" / "status" / "stop-all-services.sh").read_text(encoding="utf-8")
        topology = (REPO_ROOT / "tools" / "scripts" / "status" / "runtime_topology.sh").read_text(encoding="utf-8")

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

        self.assertIn("ChatMessageService-1", topology)
        self.assertIn('"ChatMessageService"', topology)
        self.assertIn('for name in "${MEMOCHAT_STOP_PID_ORDER[@]}"', stop)
        self.assertIn('stop_by_name_under_runtime "$exe_name" "$exe_name"', stop)
        core_start_groups = text_between(
            topology, "MEMOCHAT_CORE_START_GROUPS=(", ")\n\nreadonly -a MEMOCHAT_STOP_PORT_GROUP_ORDER"
        )
        self.assertNotIn('"message_service"', core_start_groups)

    def test_worker_side_cluster_helpers_do_not_require_worker_as_chat_node(self):
        for relative_path in (
            "clients/ChatGrpcClient.cpp",
            "persistence/RedisOnlineRouteStore.cpp",
        ):
            source = (CHATSERVER_DIR / relative_path).read_text(encoding="utf-8")
            with self.subTest(path=relative_path):
                self.assertIn("LoadChatClusterConfig", source)
                self.assertIn("std::string()", source)
                self.assertNotIn('TrimCopyAsync(cfg["SelfServer"]["Name"])', source)
                self.assertNotIn('TrimCopyRouteStore(cfg["SelfServer"]["Name"])', source)

    def test_stale_async_event_dispatcher_snapshot_is_removed(self):
        self.assertFalse((CHATSERVER_DIR / "messaging" / "AsyncEventDispatcher_full.cpp").exists())

        cmake = read_cmake()
        messaging_sources = cmake_set_values(cmake, "CHATSERVER_MESSAGING_SOURCES")
        self.assertIn("messaging/AsyncEventDispatcher.cpp", messaging_sources)
        self.assertNotIn("messaging/AsyncEventDispatcher_full.cpp", messaging_sources)

    def test_chatserver_sources_are_grouped_by_responsibility(self):
        missing = []
        for group, expected_files in EXPECTED_GROUPS.items():
            group_dir = CHATSERVER_DIR / group
            self.assertTrue(group_dir.is_dir(), f"missing group directory: {group}")
            actual_files = {path.relative_to(group_dir).as_posix() for path in group_dir.rglob("*") if path.is_file()}
            missing.extend(f"{group}/{name}" for name in sorted(expected_files - actual_files))

        self.assertEqual([], missing)

    def test_browser_transports_are_modular_and_share_frame_codec(self):
        transport_dir = CHATSERVER_DIR / "transport"
        ws_server = (transport_dir / "WebSocketChatServer.cpp").read_text(encoding="utf-8")
        ws_session_header = (transport_dir / "WebSocketSession.hpp").read_text(encoding="utf-8")
        ws_session = (transport_dir / "WebSocketSession.cpp").read_text(encoding="utf-8")
        wt_server = (transport_dir / "WebTransportChatServer.cpp").read_text(encoding="utf-8")
        wt_session_header = (transport_dir / "WebTransportSession.hpp").read_text(encoding="utf-8")
        wt_session = (transport_dir / "WebTransportSession.cpp").read_text(encoding="utf-8")
        frame_dispatch_header = (transport_dir / "ChatFrameDispatch.hpp").read_text(encoding="utf-8")
        wt_provider = (transport_dir / "IWebTransportProvider.hpp").read_text(encoding="utf-8")
        wt_session_interface = (transport_dir / "IWebTransportSession.hpp").read_text(encoding="utf-8")
        wt_factory = (transport_dir / "WebTransportProviderFactory.cpp").read_text(encoding="utf-8")
        lws_provider = (transport_dir / "LibwebsocketsWebTransportProvider.cpp").read_text(encoding="utf-8")
        wt_unavailable = (transport_dir / "UnavailableWebTransportProvider.cpp").read_text(encoding="utf-8")
        quic_session = (transport_dir / "QuicSession.hpp").read_text(encoding="utf-8")
        chatserver_cmake = read_cmake()
        server_core_cmake = (SERVER_CORE_DIR / "CMakeLists.txt").read_text(encoding="utf-8")

        self.assertIn('#include "ChatFrameCodec.hpp"', ws_session)
        self.assertIn("ChatFrameCodec::Encode", ws_session)
        self.assertIn("ChatFrameCodec::DecodeOne", ws_session)
        self.assertIn('#include "ChatFrameCodec.hpp"', wt_session)
        self.assertIn("ChatFrameCodec::Encode", wt_session)
        self.assertIn("ChatFrameCodec::DecodeOne", wt_session)
        self.assertIn("AcceptStreamBytes", wt_session)
        self.assertIn("std::shared_ptr<IChatSession>", ws_session_header)
        self.assertIn("std::shared_ptr<IChatSession>", wt_session_header)
        self.assertNotIn("std::shared_ptr<CSession>&, short, std::string_view", ws_session_header)
        self.assertNotIn("std::shared_ptr<CSession>&, short, std::string_view", wt_session_header)
        self.assertNotIn('#include "CSession.hpp"', ws_session_header)
        self.assertNotIn('#include "CSession.hpp"', wt_session_header)
        self.assertNotIn(": public CSession", ws_session_header)
        self.assertNotIn(": public CSession", wt_session_header)
        self.assertNotIn(": public CSession", quic_session)
        self.assertIn("ChatSessionState", ws_session_header)
        self.assertIn("ChatSessionState", wt_session_header)
        self.assertIn("ChatSessionState", quic_session)
        self.assertIn("std::shared_ptr<IChatSession>", frame_dispatch_header)
        self.assertNotIn("std::shared_ptr<CSession>", frame_dispatch_header)
        self.assertNotIn("class CSession", frame_dispatch_header)
        self.assertIn("class IWebTransportProvider", wt_provider)
        self.assertIn('#include "IWebTransportSession.hpp"', wt_provider)
        self.assertNotIn('#include "WebTransportSession.hpp"', wt_provider)
        self.assertIn("class IWebTransportSession", wt_session_interface)
        self.assertIn("acceptStreamBytes", wt_session_interface)
        self.assertIn("std::shared_ptr<IWebTransportSession>", wt_provider)
        self.assertIn("WebTransportProviderSessionHooks", wt_provider)
        self.assertIn("cert_file", wt_provider)
        self.assertIn("private_key_file", wt_provider)
        self.assertIn("openProviderSession", wt_server)
        self.assertIn("CreateDefaultWebTransportProvider", wt_server)
        self.assertNotIn("webtransport_h3_library_not_configured", wt_server)
        self.assertIn("webtransport_h3_library_not_configured", wt_unavailable)
        self.assertIn("UnavailableWebTransportProvider", wt_factory)
        self.assertIn("LibwebsocketsWebTransportProvider", wt_factory)
        self.assertIn("lws_wt_create_stream", lws_provider)
        self.assertIn("sessionPathMatches", lws_provider)
        self.assertIn("WSI_TOKEN_HTTP_COLON_PATH", lws_provider)
        self.assertIn("webtransport.provider.lws.bad_path", lws_provider)
        self.assertIn("ensureWritableStreamLocked", lws_provider)
        self.assertIn("lws_write", lws_provider)
        self.assertIn("acceptStreamBytes", lws_provider)
        self.assertIn("ShouldBindToNamedInterface", lws_provider)
        self.assertIn('host == "127.0.0.1"', lws_provider)
        self.assertIn(
            "info.iface = ShouldBindToNamedInterface(_options.host) ? _options.host.c_str() : nullptr;", lws_provider
        )
        self.assertIn("MEMOCHAT_ENABLE_LWS_WEBTRANSPORT_PROVIDER", server_core_cmake)
        self.assertIn("LWS_WITH_HTTP3", server_core_cmake)
        self.assertIn("LWS_ROLE_WT", server_core_cmake)
        self.assertIn("lws_hdr_total_length", server_core_cmake)
        self.assertIn("WSI_TOKEN_HTTP_COLON_PATH", server_core_cmake)
        self.assertIn("MEMOCHAT_ENABLE_LWS_WEBTRANSPORT_PROVIDER_COMPILED", chatserver_cmake)
        self.assertIn("transport/LibwebsocketsWebTransportProvider.cpp", chatserver_cmake)
        self.assertIn("boost::beast::websocket::stream", (transport_dir / "WebSocketSession.hpp").read_text())
        self.assertNotIn("WebTransport", ws_server)
        self.assertNotIn("WebTransport", ws_session)
        self.assertNotIn("websocket::stream", wt_server)
        self.assertNotIn("websocket::stream", wt_session)
        self.assertNotIn("TcpSession", quic_session)
        self.assertNotIn("GetSocket", quic_session)

    def test_webtransport_local_config_uses_generated_tls_files(self):
        for config_name, sections in {
            "config.ini": ("chatserver1",),
            "chatserver1.ini": ("chatserver1",),
            "chatserver2.ini": ("chatserver1", "chatserver2"),
        }.items():
            text = (CHATSERVER_DIR / config_name).read_text(encoding="utf-8")
            for section in sections:
                with self.subTest(config=config_name, section=section):
                    assert_ini_section_has(
                        self,
                        text,
                        section,
                        "WtEnabled=false",
                        "WtCertFile=server.crt",
                        "WtKeyFile=server.key",
                    )

    def test_chatserver_root_keeps_only_compatibility_and_config_artifacts(self):
        root_files = {path.name for path in CHATSERVER_DIR.iterdir() if path.is_file()}
        unexpected = sorted(root_files - ROOT_ALLOWLIST)

        self.assertEqual([], unexpected)

    def test_delivery_task_publishing_uses_named_domain_port(self):
        port_header = CHATSERVER_DIR / "domain" / "ports" / "IDeliveryTaskPublisher.hpp"
        self.assertTrue(port_header.is_file(), "delivery task publisher port header is missing")

        port_source = port_header.read_text(encoding="utf-8")
        self.assertIn("class IDeliveryTaskPublisher", port_source)
        self.assertIn("virtual bool PublishDeliveryTask", port_source)

        delivery_header = (DOMAIN_MESSAGE_DIR / "MessageDeliveryService.hpp").read_text(encoding="utf-8")
        self.assertIn("ports/IDeliveryTaskPublisher.hpp", delivery_header)
        self.assertIn(
            "explicit MessageDeliveryService(IDeliveryTaskPublisher* task_publisher = nullptr", delivery_header
        )
        self.assertIn("IDeliveryTaskPublisher* _task_publisher", delivery_header)
        self.assertNotIn("PublishTaskFn", delivery_header)

        delivery_source = (DOMAIN_MESSAGE_DIR / "MessageDeliveryService.cpp").read_text(encoding="utf-8")
        self.assertIn("_task_publisher->PublishDeliveryTask", delivery_source)
        self.assertNotIn("_publish_task(", delivery_source)

        dispatcher_header = (DOMAIN_DELIVERY_DIR / "TaskDispatcher.hpp").read_text(encoding="utf-8")
        self.assertIn("ports/IDeliveryTaskPublisher.hpp", dispatcher_header)
        self.assertRegex(dispatcher_header, r"class\s+TaskDispatcher\s*:\s*public\s+IDeliveryTaskPublisher")
        self.assertIn("PublishDeliveryTask", dispatcher_header)

    def test_async_event_publishing_uses_named_domain_port(self):
        port_header = CHATSERVER_DIR / "domain" / "ports" / "IEventPublisher.hpp"
        self.assertTrue(port_header.is_file(), "async event publisher port header is missing")

        port_source = port_header.read_text(encoding="utf-8")
        self.assertIn("class IEventPublisher", port_source)
        self.assertRegex(port_source, r"virtual\s+bool\s+PublishEvent")

        dispatcher_header = (CHATSERVER_DIR / "messaging" / "AsyncEventDispatcher.hpp").read_text(encoding="utf-8")
        self.assertIn("ports/IEventPublisher.hpp", dispatcher_header)
        self.assertRegex(dispatcher_header, r"class\s+AsyncEventDispatcher\s*:\s*public\s+IEventPublisher")
        self.assertIn("PublishEvent", dispatcher_header)
        self.assertIn("PublishAsyncEvent", dispatcher_header)

        dispatcher_source = (CHATSERVER_DIR / "messaging" / "AsyncEventDispatcher.cpp").read_text(encoding="utf-8")
        self.assertIn("bool AsyncEventDispatcher::PublishEvent", dispatcher_source)
        self.assertIn("return PublishEvent(topic, payload, error);", dispatcher_source)

        self.assertIn("return _composition->PublishAsyncEvent(topic, payload, error);", logic_source())
        self.assertIn(
            "return _async_event_dispatcher->PublishEvent(topic, payload, error);",
            runtime_composition_source(),
        )

    def test_async_event_dispatcher_uses_repository_ports_for_data_ownership(self):
        dispatcher_header = (CHATSERVER_DIR / "messaging" / "AsyncEventDispatcher.hpp").read_text(encoding="utf-8")
        dispatcher_source = (CHATSERVER_DIR / "messaging" / "AsyncEventDispatcher.cpp").read_text(encoding="utf-8")
        composition_source = runtime_composition_source()

        for token in (
            "ports/IMessageRepository.hpp",
            "ports/IRelationRepository.hpp",
            "IMessageRepository* message_repository",
            "IRelationRepository* relation_repository",
            "IMessageRepository* _message_repository",
            "IRelationRepository* _relation_repository",
        ):
            self.assertIn(token, dispatcher_header)

        for token in (
            "_message_repository->SavePrivateMessage",
            "_message_repository->SaveGroupMessage",
            "_relation_repository->GetUserByUid",
            "_relation_repository->GetGroupById",
            "_relation_repository->GetGroupMemberList",
            "_relation_repository->RefreshDialogsForOwner",
        ):
            self.assertIn(token, dispatcher_source)

        forbidden_tokens = (
            '#include "PostgresMgr.hpp"',
            '#include "MongoMgr.hpp"',
            '#include "ChatMessageRepository.hpp"',
            '#include "ChatRelationRepository.hpp"',
            "PostgresMgr::GetInstance()->SavePrivateMessage",
            "MongoMgr::GetInstance()->SavePrivateMessage",
            "PostgresMgr::GetInstance()->SaveGroupMessage",
            "MongoMgr::GetInstance()->SaveGroupMessage",
            "PostgresMgr::GetInstance()->GetUser",
            "PostgresMgr::GetInstance()->GetGroupById",
            "PostgresMgr::GetInstance()->GetGroupMemberList",
            "PostgresMgr::GetInstance()->RefreshDialogsForOwner",
            "ChatMessageRepository",
            "ChatRelationRepository",
        )
        violations = [token for token in forbidden_tokens if token in dispatcher_source]
        self.assertEqual([], violations)

        async_dispatcher_wiring = text_between(
            composition_source,
            "_async_event_dispatcher = std::make_unique<AsyncEventDispatcher>",
            "_task_dispatcher = std::make_unique<TaskDispatcher>",
        )
        self.assertIn("_message_delivery_service.get()", async_dispatcher_wiring)
        self.assertIn("_message_repository.get()", async_dispatcher_wiring)
        self.assertIn("_relation_repository.get()", async_dispatcher_wiring)

    def test_async_event_dispatcher_uses_route_and_cache_ports(self):
        dispatcher_header = (CHATSERVER_DIR / "messaging" / "AsyncEventDispatcher.hpp").read_text(encoding="utf-8")
        dispatcher_source = (CHATSERVER_DIR / "messaging" / "AsyncEventDispatcher.cpp").read_text(encoding="utf-8")
        resolver_source = (CHATSERVER_DIR / "domain" / "ports" / "OnlineRouteResolver.hpp").read_text(encoding="utf-8")
        composition_source = runtime_composition_source()

        for token in (
            "ports/ISessionRegistry.hpp",
            "ports/IOnlineRouteStore.hpp",
            "ports/IRelationBootstrapCache.hpp",
            "ISessionRegistry* session_registry",
            "IOnlineRouteStore* online_route_store",
            "IRelationBootstrapCache* relation_bootstrap_cache",
            "ISessionRegistry* _session_registry",
            "IOnlineRouteStore* _online_route_store",
            "IRelationBootstrapCache* _relation_bootstrap_cache",
        ):
            self.assertIn(token, dispatcher_header)

        # Online-route resolution is shared across delivery paths via the
        # OnlineRouteResolver seam; the session/route-store port calls live there
        # now, and the dispatcher reaches them through the shared resolver.
        for token in (
            "session_registry->FindSession",
            "online_route_store->SelfServerName",
            "online_route_store->RepairOnlineRoute",
            "online_route_store->FindUserServer",
            "online_route_store->ResolveServerFromOnlineSets",
            "online_route_store->ClearTrackedOnlineRoute",
        ):
            self.assertIn(token, resolver_source)

        for token in (
            "ports/OnlineRouteResolver.hpp",
            "relation_bootstrap_cache->Invalidate",
            "ResolveOnlineRoute(uid, _session_registry, _online_route_store)",
            "ResolveOnlineRoute(to_uid, _session_registry, _online_route_store)",
            "InvalidateRelationBootstrapCacheAsync(uid, _relation_bootstrap_cache)",
        ):
            self.assertIn(token, dispatcher_source)

        forbidden_tokens = (
            '#include "ConfigMgr.hpp"',
            '#include "RedisMgr.hpp"',
            '#include "UserMgr.hpp"',
            "ConfigMgr::Inst()",
            "RedisMgr::GetInstance()",
            "UserMgr::GetInstance()",
            "LoadChatClusterConfig",
            "SERVER_ONLINE_USERS_PREFIX",
            "USERIPPREFIX",
            "USER_SESSION_PREFIX",
            "KnownChatServerNamesAsync",
            "ServerOnlineUsersKeyAsync",
            "RepairOnlineRouteStateAsync",
            "ResolveServerFromOnlineSetsAsync",
            "ClearTrackedOnlineRouteAsync",
        )
        violations = [token for token in forbidden_tokens if token in dispatcher_source]
        self.assertEqual([], violations)

        async_dispatcher_wiring = text_between(
            composition_source,
            "_async_event_dispatcher = std::make_unique<AsyncEventDispatcher>",
            "_task_dispatcher = std::make_unique<TaskDispatcher>",
        )
        self.assertIn("UserMgr::GetInstance().get()", async_dispatcher_wiring)
        self.assertIn("_online_route_store.get()", async_dispatcher_wiring)
        self.assertIn("_relation_bootstrap_cache.get()", async_dispatcher_wiring)

    def test_delivery_worker_lifecycle_uses_chat_delivery_runtime(self):
        runtime_header_path = DOMAIN_DELIVERY_DIR / "ChatDeliveryRuntime.hpp"
        runtime_source_path = DOMAIN_DELIVERY_DIR / "ChatDeliveryRuntime.cpp"
        self.assertTrue(runtime_header_path.is_file(), "ChatDeliveryRuntime header is missing")
        self.assertTrue(runtime_source_path.is_file(), "ChatDeliveryRuntime source is missing")

        runtime_header = runtime_header_path.read_text(encoding="utf-8")
        self.assertIn("class ChatDeliveryRuntime", runtime_header)
        self.assertIn("using LoopFn = std::function<void()>", runtime_header)
        self.assertIn("void Start()", runtime_header)
        self.assertIn("void StopAndJoin()", runtime_header)
        self.assertIn("bool StopRequested() const", runtime_header)

        runtime_source = runtime_source_path.read_text(encoding="utf-8")
        self.assertIn("std::thread(&ChatDeliveryRuntime::RunLoop", runtime_source)
        self.assertIn("delivery_runtime_modules::StopRequestedWhenStarting()", runtime_source)
        self.assertIn("delivery_runtime_modules::StopRequestedWhenStopping()", runtime_source)
        self.assertIn("_event_worker_thread.joinable()", runtime_source)
        self.assertIn("_task_worker_thread.joinable()", runtime_source)

        composition_header = runtime_composition_header()
        self.assertIn("class ChatDeliveryRuntime;", composition_header)
        self.assertIn("std::unique_ptr<ChatDeliveryRuntime> _delivery_runtime", composition_header)
        self.assertNotIn("_event_worker_thread", composition_header)
        self.assertNotIn("_task_worker_thread", composition_header)
        self.assertNotIn("_event_stop", composition_header)

        logic_header_text = logic_header_source()
        self.assertIn("class ChatRuntimeComposition;", logic_header_text)
        self.assertIn("std::unique_ptr<ChatRuntimeComposition> _composition", logic_header_text)
        self.assertNotIn("std::unique_ptr<ChatDeliveryRuntime> _delivery_runtime", logic_header_text)

        composition_source = runtime_composition_source()
        self.assertIn("_delivery_runtime = std::make_unique<ChatDeliveryRuntime>", composition_source)
        self.assertIn("_delivery_runtime->Start();", composition_source)
        self.assertIn("_delivery_runtime->StopAndJoin();", composition_source)
        self.assertNotIn("std::thread(&LogicSystem::DealAsyncEvents", logic_source())
        self.assertNotIn("std::thread(&LogicSystem::DealTasks", logic_source())

    def test_delivery_gateway_uses_named_domain_port(self):
        port_header = CHATSERVER_DIR / "domain" / "ports" / "IDeliveryGateway.hpp"
        self.assertTrue(port_header.is_file(), "delivery gateway port header is missing")

        port_source = port_header.read_text(encoding="utf-8")
        self.assertIn("class IDeliveryGateway", port_source)
        self.assertRegex(port_source, r"virtual\s+void\s+PushPayload")
        self.assertRegex(port_source, r"virtual\s+bool\s+TryPushPayload")

        delivery_header = (DOMAIN_MESSAGE_DIR / "MessageDeliveryService.hpp").read_text(encoding="utf-8")
        self.assertIn("ports/IDeliveryGateway.hpp", delivery_header)
        self.assertRegex(delivery_header, r"class\s+MessageDeliveryService\s*:\s*public\s+IDeliveryGateway")
        self.assertIn("void PushPayload", delivery_header)
        self.assertIn("bool TryPushPayload", delivery_header)

        dispatcher_header = (CHATSERVER_DIR / "messaging" / "AsyncEventDispatcher.hpp").read_text(encoding="utf-8")
        self.assertIn("ports/IDeliveryGateway.hpp", dispatcher_header)
        self.assertIn("IDeliveryGateway* delivery_gateway", dispatcher_header)
        self.assertIn("IDeliveryGateway* _delivery_gateway", dispatcher_header)
        self.assertNotIn("PushGroupPayloadFn", dispatcher_header)
        self.assertNotIn("_push_group_payload", dispatcher_header)

        task_header = (DOMAIN_DELIVERY_DIR / "TaskDispatcher.hpp").read_text(encoding="utf-8")
        self.assertIn("ports/IDeliveryGateway.hpp", task_header)
        self.assertIn("IDeliveryGateway* delivery_gateway", task_header)
        self.assertIn("IDeliveryGateway* _delivery_gateway", task_header)
        self.assertNotIn("DeliveryTaskHandler", task_header)
        self.assertNotIn("_delivery_handler", task_header)

        dispatcher_source = (CHATSERVER_DIR / "messaging" / "AsyncEventDispatcher.cpp").read_text(encoding="utf-8")
        self.assertIn("_delivery_gateway->PushPayload", dispatcher_source)
        self.assertNotIn("_push_group_payload(", dispatcher_source)

        task_source = (DOMAIN_DELIVERY_DIR / "TaskDispatcher.cpp").read_text(encoding="utf-8")
        self.assertIn("_delivery_gateway->TryPushPayload", task_source)
        self.assertNotIn("_delivery_handler(", task_source)

        composition_source = runtime_composition_source()
        self.assertRegex(
            composition_source,
            r"_message_delivery_service\s*=\s*std::make_unique<MessageDeliveryService>",
        )
        self.assertIn("_message_delivery_service.get()", composition_source)
        wiring_block = text_between(
            composition_source,
            "_message_delivery_service",
            "_delivery_runtime = std::make_unique<ChatDeliveryRuntime>",
        )
        self.assertNotIn("MessageDelivery().PushPayload", wiring_block)
        self.assertNotIn("MessageDelivery().TryPushPayload", wiring_block)

    def test_outbox_repair_uses_named_domain_port(self):
        port_header = CHATSERVER_DIR / "domain" / "ports" / "IOutboxRepairScheduler.hpp"
        self.assertTrue(port_header.is_file(), "outbox repair scheduler port header is missing")

        port_source = port_header.read_text(encoding="utf-8")
        self.assertIn("class IOutboxRepairScheduler", port_source)
        self.assertRegex(port_source, r"virtual\s+bool\s+ExpediteOutboxRepair")

        task_header = (DOMAIN_DELIVERY_DIR / "TaskDispatcher.hpp").read_text(encoding="utf-8")
        self.assertIn("ports/IOutboxRepairScheduler.hpp", task_header)
        self.assertIn("IOutboxRepairScheduler* outbox_repair_scheduler", task_header)
        self.assertIn("IOutboxRepairScheduler* _outbox_repair_scheduler", task_header)
        self.assertNotIn("OutboxRepairHandler", task_header)
        self.assertNotIn("_outbox_repair_handler", task_header)

        task_source = (DOMAIN_DELIVERY_DIR / "TaskDispatcher.cpp").read_text(encoding="utf-8")
        self.assertIn("_outbox_repair_scheduler->ExpediteOutboxRepair(outbox_id)", task_source)
        self.assertNotIn("_outbox_repair_handler(", task_source)

        adapter_header = CHATSERVER_DIR / "persistence" / "ChatOutboxRepairScheduler.hpp"
        adapter_source_path = CHATSERVER_DIR / "persistence" / "ChatOutboxRepairScheduler.cpp"
        self.assertTrue(adapter_header.is_file(), "outbox repair scheduler adapter header is missing")
        self.assertTrue(adapter_source_path.is_file(), "outbox repair scheduler adapter source is missing")

        adapter_header_source = adapter_header.read_text(encoding="utf-8")
        adapter_source = adapter_source_path.read_text(encoding="utf-8")
        self.assertIn("ports/IOutboxRepairScheduler.hpp", adapter_header_source)
        self.assertRegex(
            adapter_header_source,
            r"class\s+ChatOutboxRepairScheduler\s+final\s*:\s*public\s+IOutboxRepairScheduler",
        )
        self.assertIn('#include "PostgresMgr.hpp"', adapter_source)
        self.assertIn("PostgresMgr::GetInstance()->ExpediteChatOutboxEventRetry(outbox_id)", adapter_source)

        logic_header = (DOMAIN_ORCHESTRATION_DIR / "LogicSystem.hpp").read_text(encoding="utf-8")
        logic_source = (DOMAIN_ORCHESTRATION_DIR / "LogicSystem.cpp").read_text(encoding="utf-8")
        self.assertNotIn("ports/IOutboxRepairScheduler.hpp", logic_header)
        self.assertNotRegex(logic_header, r"public\s+IOutboxRepairScheduler")
        self.assertNotIn("bool ExpediteOutboxRepair(int64_t outbox_id) override", logic_header)
        self.assertNotIn('#include "PostgresMgr.hpp"', logic_source)
        self.assertNotIn("PostgresMgr::GetInstance()->ExpediteChatOutboxEventRetry", logic_source)

        composition_source = runtime_composition_source()
        composition_header = runtime_composition_header()
        self.assertIn("class IOutboxRepairScheduler;", composition_header)
        self.assertIn("std::unique_ptr<IOutboxRepairScheduler> _outbox_repair_scheduler", composition_header)
        self.assertIn('#include "ChatOutboxRepairScheduler.hpp"', composition_source)
        self.assertIn("_outbox_repair_scheduler = std::make_unique<ChatOutboxRepairScheduler>()", composition_source)
        wiring_block = text_between(
            composition_source,
            "_task_dispatcher = std::make_unique<TaskDispatcher>",
            "_message_delivery_service->SetTaskPublisher",
        )
        self.assertIn("_outbox_repair_scheduler.get()", wiring_block)
        self.assertNotIn("&_logic", wiring_block)
        self.assertNotIn("return ExpediteOutboxRepair(outbox_id);", wiring_block)

    def test_delivery_route_uses_session_and_online_route_ports(self):
        session_port = CHATSERVER_DIR / "domain" / "ports" / "ISessionRegistry.hpp"
        route_port = CHATSERVER_DIR / "domain" / "ports" / "IOnlineRouteStore.hpp"
        route_adapter = CHATSERVER_DIR / "persistence" / "RedisOnlineRouteStore.hpp"
        self.assertTrue(session_port.is_file(), "session registry port header is missing")
        self.assertTrue(route_port.is_file(), "online route store port header is missing")
        self.assertTrue(route_adapter.is_file(), "redis online route store adapter header is missing")

        session_port_source = session_port.read_text(encoding="utf-8")
        self.assertIn("class ISessionRegistry", session_port_source)
        self.assertRegex(session_port_source, r"virtual\s+std::shared_ptr<IChatSession>\s+FindSession")
        self.assertIn("class IChatSession;", session_port_source)
        self.assertNotIn("CSession", session_port_source)
        self.assertRegex(session_port_source, r"virtual\s+void\s+BindSession")
        self.assertRegex(session_port_source, r"virtual\s+void\s+UnbindSession")

        route_port_source = route_port.read_text(encoding="utf-8")
        self.assertIn("class IOnlineRouteStore", route_port_source)
        self.assertRegex(route_port_source, r"virtual\s+std::string\s+SelfServerName")
        self.assertRegex(route_port_source, r"virtual\s+void\s+RepairOnlineRoute")
        self.assertRegex(route_port_source, r"virtual\s+std::string\s+FindUserServer")
        self.assertRegex(route_port_source, r"virtual\s+std::string\s+ResolveServerFromOnlineSets")
        self.assertRegex(route_port_source, r"virtual\s+void\s+SetUserServer")
        self.assertRegex(route_port_source, r"virtual\s+void\s+ClearTrackedOnlineRoute")

        usermgr_header = (DOMAIN_USERS_DIR / "UserMgr.hpp").read_text(encoding="utf-8")
        self.assertIn("ports/ISessionRegistry.hpp", usermgr_header)
        self.assertRegex(
            usermgr_header, r"class\s+UserMgr\s*:\s*public\s+Singleton<UserMgr>\s*,\s*public\s+ISessionRegistry"
        )
        self.assertIn("std::shared_ptr<IChatSession> FindSession(int uid) override", usermgr_header)
        self.assertIn("void BindSession(int uid, std::shared_ptr<IChatSession> session) override", usermgr_header)
        self.assertIn("void UnbindSession(int uid, const std::string& session_id) override", usermgr_header)
        self.assertNotIn("std::shared_ptr<CSession>", usermgr_header)

        route_adapter_source = route_adapter.read_text(encoding="utf-8")
        self.assertIn("ports/IOnlineRouteStore.hpp", route_adapter_source)
        self.assertRegex(route_adapter_source, r"class\s+RedisOnlineRouteStore\s*:\s*public\s+IOnlineRouteStore")

        delivery_header = (DOMAIN_MESSAGE_DIR / "MessageDeliveryService.hpp").read_text(encoding="utf-8")
        self.assertIn("ports/ISessionRegistry.hpp", delivery_header)
        self.assertIn("ports/IOnlineRouteStore.hpp", delivery_header)
        self.assertIn("ISessionRegistry* session_registry", delivery_header)
        self.assertIn("IOnlineRouteStore* online_route_store", delivery_header)
        self.assertIn("ISessionRegistry* _session_registry", delivery_header)
        self.assertIn("IOnlineRouteStore* _online_route_store", delivery_header)

        delivery_source = (DOMAIN_MESSAGE_DIR / "MessageDeliveryService.cpp").read_text(encoding="utf-8")
        self.assertNotIn('#include "UserMgr.hpp"', delivery_source)
        self.assertNotIn('#include "RedisMgr.hpp"', delivery_source)
        # Online-route resolution is shared via the OnlineRouteResolver seam; the
        # session/route-store port calls live there, reached through the resolver.
        self.assertIn("ports/OnlineRouteResolver.hpp", delivery_source)
        self.assertIn("ResolveOnlineRoute(uid, _session_registry, _online_route_store)", delivery_source)
        resolver_source = (CHATSERVER_DIR / "domain" / "ports" / "OnlineRouteResolver.hpp").read_text(encoding="utf-8")
        self.assertIn("session_registry->FindSession(uid)", resolver_source)
        self.assertIn("online_route_store->RepairOnlineRoute", resolver_source)
        self.assertIn("online_route_store->FindUserServer", resolver_source)
        self.assertIn("online_route_store->ClearTrackedOnlineRoute", resolver_source)
        # The group-member fallback branch still resolves/sets routes directly.
        self.assertIn("_online_route_store->ResolveServerFromOnlineSets", delivery_source)
        self.assertIn("_online_route_store->SetUserServer", delivery_source)

        composition_header = runtime_composition_header()
        self.assertIn("class IOnlineRouteStore;", composition_header)
        self.assertIn("std::unique_ptr<IOnlineRouteStore> _online_route_store", composition_header)
        self.assertNotIn("std::unique_ptr<IOnlineRouteStore> _online_route_store", logic_header_source())

        composition_source = runtime_composition_source()
        self.assertIn('#include "RedisOnlineRouteStore.hpp"', composition_source)
        self.assertIn("_online_route_store = std::make_unique<RedisOnlineRouteStore>()", composition_source)
        delivery_wiring = text_between(
            composition_source,
            "_message_delivery_service",
            "_async_event_dispatcher = std::make_unique<AsyncEventDispatcher>",
        )
        self.assertIn("UserMgr::GetInstance().get()", delivery_wiring)
        self.assertIn("_online_route_store.get()", delivery_wiring)

    def test_chat_session_service_uses_session_and_online_route_ports(self):
        session_header = (DOMAIN_SESSION_DIR / "ChatSessionService.hpp").read_text(encoding="utf-8")
        self.assertIn("ports/ISessionRegistry.hpp", session_header)
        self.assertIn("ports/IOnlineRouteStore.hpp", session_header)
        self.assertIn("ports/IRelationQueryService.hpp", session_header)
        self.assertIn("ports/IRelationRepository.hpp", session_header)
        self.assertNotIn("ports/IRelationService.hpp", session_header)
        compact_session_header = " ".join(session_header.split())
        self.assertIn("ChatSessionService(LogicSystem& logic,", compact_session_header)
        self.assertIn("ISessionRegistry* session_registry", compact_session_header)
        self.assertIn("IOnlineRouteStore* online_route_store", compact_session_header)
        self.assertIn("IRelationQueryService* relation_query_service", compact_session_header)
        self.assertIn("IRelationRepository* relation_repository", compact_session_header)
        self.assertIn("ISessionRegistry* _session_registry", session_header)
        self.assertIn("IOnlineRouteStore* _online_route_store", session_header)
        self.assertIn("IRelationQueryService* _relation_query_service", session_header)
        self.assertIn("IRelationRepository* _relation_repository", session_header)

        session_source = (DOMAIN_SESSION_DIR / "ChatSessionService.cpp").read_text(encoding="utf-8")
        self.assertNotIn('#include "UserMgr.hpp"', session_source)
        self.assertNotIn("RepairOnlineRouteStateLocal", session_source)
        self.assertIn("_session_registry->BindSession(uid, session)", session_source)
        self.assertIn("_session_registry->FindSession(uid)", session_source)
        self.assertIn("_online_route_store->RepairOnlineRoute(uid, session)", session_source)
        self.assertIn("_online_route_store->FindUserServer(uid)", session_source)
        self.assertIn("_online_route_store->SelfServerName()", session_source)
        login_handler = text_between(
            session_source,
            "void ChatSessionService::HandleLogin",
            "void ChatSessionService::HandleRelationBootstrap",
        )
        bootstrap_handler = text_between(
            session_source,
            "void ChatSessionService::HandleRelationBootstrap",
            "void ChatSessionService::HandleHeartbeat",
        )
        self.assertNotIn("_relation_query_service->AppendRelationBootstrapJson", login_handler)
        self.assertIn("_relation_query_service->AppendRelationBootstrapJson(uid, rtvalue)", bootstrap_handler)
        self.assertNotIn("_logic.AppendRelationBootstrapJson", session_source)
        self.assertNotIn("UserMgr::GetInstance()->SetUserSession", session_source)
        self.assertNotIn("UserMgr::GetInstance()->GetSession", session_source)

        composition_source = runtime_composition_source()
        constructor_block = text_between(
            composition_source,
            "ChatRuntimeComposition::ChatRuntimeComposition(LogicSystem& logic)",
            "_delivery_runtime = std::make_unique<ChatDeliveryRuntime>",
        )
        self.assertIn("_online_route_store = std::make_unique<RedisOnlineRouteStore>()", constructor_block)
        self.assertIn("_chat_relation_service = CreateRelationService", constructor_block)
        self.assertRegex(
            constructor_block,
            r"_chat_session_service\s*=\s*std::make_unique<ChatSessionService>",
        )
        self.assertIn("UserMgr::GetInstance().get()", constructor_block)
        self.assertIn("_online_route_store.get()", constructor_block)
        self.assertIn("_chat_relation_service.get()", constructor_block)
        self.assertIn("_relation_repository.get()", constructor_block)

    def test_chat_session_service_uses_session_config_and_repository_ports(self):
        config_port_path = CHATSERVER_DIR / "domain" / "ports" / "IChatSessionConfig.hpp"
        repo_port_path = CHATSERVER_DIR / "domain" / "ports" / "IChatSessionRepository.hpp"
        config_adapter_header_path = CHATSERVER_DIR / "config" / "ChatSessionConfig.hpp"
        config_adapter_source_path = CHATSERVER_DIR / "config" / "ChatSessionConfig.cpp"
        repo_adapter_header_path = CHATSERVER_DIR / "persistence" / "ChatSessionRepository.hpp"
        repo_adapter_source_path = CHATSERVER_DIR / "persistence" / "ChatSessionRepository.cpp"
        for path, label in (
            (config_port_path, "session config port header"),
            (repo_port_path, "session repository port header"),
            (config_adapter_header_path, "session config adapter header"),
            (config_adapter_source_path, "session config adapter source"),
            (repo_adapter_header_path, "session repository adapter header"),
            (repo_adapter_source_path, "session repository adapter source"),
        ):
            self.assertTrue(path.is_file(), f"{label} is missing")

        config_port = config_port_path.read_text(encoding="utf-8")
        self.assertIn("class IChatSessionConfig", config_port)
        for method in (
            "ChatAuthSecret",
            "FeatureFlagDefaultTrue",
            "HeartbeatRouteRefreshSec",
            "LoginOfflinePushDelayMs",
            "SelfServerName",
        ):
            self.assertIn(method, config_port)

        repo_port = repo_port_path.read_text(encoding="utf-8")
        self.assertIn("class IChatSessionRepository", repo_port)
        for method in (
            "AcquireDuplicateLoginLock",
            "ReleaseDuplicateLoginLock",
            "GetUndeliveredPrivateMessages",
        ):
            self.assertIn(method, repo_port)
        self.assertNotIn("GetLegacyToken", repo_port)

        config_adapter_header = config_adapter_header_path.read_text(encoding="utf-8")
        self.assertIn("ports/IChatSessionConfig.hpp", config_adapter_header)
        self.assertRegex(config_adapter_header, r"class\s+ChatSessionConfig\s*:\s*public\s+IChatSessionConfig")
        config_adapter_source = config_adapter_source_path.read_text(encoding="utf-8")
        self.assertIn('#include "ConfigMgr.hpp"', config_adapter_source)
        self.assertIn("ConfigMgr::Inst()", config_adapter_source)

        repo_adapter_header = repo_adapter_header_path.read_text(encoding="utf-8")
        self.assertIn("ports/IChatSessionRepository.hpp", repo_adapter_header)
        self.assertRegex(repo_adapter_header, r"class\s+ChatSessionRepository\s*:\s*public\s+IChatSessionRepository")
        repo_adapter_source = repo_adapter_source_path.read_text(encoding="utf-8")
        self.assertIn('#include "PostgresMgr.hpp"', repo_adapter_source)
        self.assertIn('#include "RedisMgr.hpp"', repo_adapter_source)
        self.assertIn("DuplicateLoginLockKey", repo_adapter_source)
        self.assertIn("ShouldQueryUndeliveredPrivateMessages", repo_adapter_source)
        self.assertIn("RedisMgr::GetInstance()->acquireLock", repo_adapter_source)
        self.assertIn("RedisMgr::GetInstance()->releaseLock", repo_adapter_source)
        self.assertIn("PostgresMgr::GetInstance()->GetUndeliveredPrivateMessages", repo_adapter_source)

        session_header = (DOMAIN_SESSION_DIR / "ChatSessionService.hpp").read_text(encoding="utf-8")
        self.assertIn("ports/IChatSessionConfig.hpp", session_header)
        self.assertIn("ports/IChatSessionRepository.hpp", session_header)
        compact_session_header = " ".join(session_header.split())
        self.assertIn("IChatSessionConfig* session_config", compact_session_header)
        self.assertIn("IChatSessionRepository* session_repository", compact_session_header)
        self.assertIn("IChatSessionConfig* _session_config", session_header)
        self.assertIn("IChatSessionRepository* _session_repository", session_header)

        session_source = (DOMAIN_SESSION_DIR / "ChatSessionService.cpp").read_text(encoding="utf-8")
        for forbidden in (
            '#include "ConfigMgr.hpp"',
            '#include "MongoMgr.hpp"',
            '#include "PostgresMgr.hpp"',
            '#include "RedisMgr.hpp"',
            "ConfigMgr::Inst()",
            "PostgresMgr::GetInstance()",
            "MongoMgr::GetInstance()",
            "RedisMgr::GetInstance()",
        ):
            self.assertNotIn(forbidden, session_source)
        for method in (
            "ChatAuthSecret",
            "FeatureFlagDefaultTrue",
            "HeartbeatRouteRefreshSec",
            "LoginOfflinePushDelayMs",
            "SelfServerName",
        ):
            self.assertRegex(session_source, rf"_session_config\s*->\s*{method}")
        for method in (
            "AcquireDuplicateLoginLock",
            "ReleaseDuplicateLoginLock",
            "GetUndeliveredPrivateMessages",
        ):
            self.assertRegex(session_source, rf"_session_repository\s*->\s*{method}")
        self.assertNotIn("GetLegacyToken", session_source)

        cmake = read_cmake()
        self.assertIn("config/ChatSessionConfig.cpp", cmake_set_values(cmake, "CHATSERVER_CONFIG_SOURCES"))
        self.assertIn(
            "persistence/ChatSessionRepository.cpp", cmake_set_values(cmake, "CHATSERVER_PERSISTENCE_SOURCES")
        )

        composition_header = runtime_composition_header()
        self.assertIn("class IChatSessionConfig;", composition_header)
        self.assertIn("class IChatSessionRepository;", composition_header)
        self.assertIn("std::unique_ptr<IChatSessionConfig> _session_config", composition_header)
        self.assertIn("std::unique_ptr<IChatSessionRepository> _session_repository", composition_header)
        self.assertNotIn("std::unique_ptr<IChatSessionConfig> _session_config", logic_header_source())
        self.assertNotIn("std::unique_ptr<IChatSessionRepository> _session_repository", logic_header_source())

        composition_source = runtime_composition_source()
        self.assertIn('#include "ChatSessionConfig.hpp"', composition_source)
        self.assertIn('#include "ChatSessionRepository.hpp"', composition_source)
        constructor_block = text_between(
            composition_source,
            "_online_route_store = std::make_unique<RedisOnlineRouteStore>()",
            "_delivery_runtime = std::make_unique<ChatDeliveryRuntime>",
        )
        self.assertIn("_session_config = std::make_unique<ChatSessionConfig>()", constructor_block)
        self.assertIn("_session_repository = std::make_unique<ChatSessionRepository>()", constructor_block)
        self.assertIn("_session_config.get()", constructor_block)
        self.assertIn("_session_repository.get()", constructor_block)

    def test_chat_login_accepts_only_v3_login_ticket_contract(self):
        session_source = (DOMAIN_SESSION_DIR / "ChatSessionService.cpp").read_text(encoding="utf-8")
        login_handler = text_between(
            session_source,
            "void ChatSessionService::HandleLogin",
            "void ChatSessionService::HandleRelationBootstrap",
        )

        self.assertNotIn('#include "ChatUserSupport.hpp"', session_source)
        self.assertIn('rtvalue["protocol_version"] = 3;', login_handler)
        self.assertIn("protocol_version != 3", login_handler)
        self.assertIn('root.get("login_ticket", "").asString()', login_handler)
        self.assertIn("DecodeAndVerifyTicket", login_handler)
        self.assertIn("ticket_claims.protocol_version != 3", login_handler)
        self.assertIn('const std::string relation_bootstrap_mode = "explicit_pull"', login_handler)

        for forbidden in (
            "GetLegacyToken",
            'root, "token"',
            'root, "uid"',
            "token_value",
            "USERTOKENPREFIX",
            "USER_BASE_INFO",
            "protocol_version < 3",
            "protocol_version >= 3",
            'rtvalue["protocol_version"] = 2',
            "_relation_query_service->AppendRelationBootstrapJson",
            "chatusersupport::GetBaseInfo",
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, login_handler)

    def test_private_message_service_uses_session_and_online_route_ports(self):
        private_header = (DOMAIN_MESSAGE_DIR / "PrivateMessageService.hpp").read_text(encoding="utf-8")
        self.assertIn("ports/ISessionRegistry.hpp", private_header)
        self.assertIn("ports/IOnlineRouteStore.hpp", private_header)
        self.assertIn("ports/IRelationRepository.hpp", private_header)
        compact_private_header = " ".join(private_header.split())
        self.assertIn("PrivateMessageService(ISessionRegistry* session_registry,", compact_private_header)
        self.assertIn("ISessionRegistry* session_registry", compact_private_header)
        self.assertIn("IOnlineRouteStore* online_route_store", compact_private_header)
        self.assertIn("IRelationRepository* relation_repository", compact_private_header)
        self.assertIn("ISessionRegistry* _session_registry", private_header)
        self.assertIn("IOnlineRouteStore* _online_route_store", private_header)
        self.assertIn("IRelationRepository* _relation_repository", private_header)

        private_source = (DOMAIN_MESSAGE_DIR / "PrivateMessageService.cpp").read_text(encoding="utf-8")
        self.assertNotIn('#include "RedisMgr.hpp"', private_source)
        self.assertNotIn('#include "UserMgr.hpp"', private_source)
        self.assertNotIn('#include "ConfigMgr.hpp"', private_source)
        self.assertNotIn('#include "cluster/ChatClusterDiscovery.hpp"', private_source)
        self.assertNotIn("TrimCopyLocal", private_source)
        self.assertNotIn("ServerOnlineUsersKeyLocal", private_source)
        self.assertNotIn("KnownChatServerNamesLocal", private_source)
        self.assertNotIn("ResolveServerFromOnlineSetsLocal", private_source)
        self.assertNotIn("RepairOnlineRouteStateLocal", private_source)
        self.assertNotIn("ClearTrackedOnlineRouteLocal", private_source)

        # Online-route resolution is shared via the OnlineRouteResolver seam; the
        # session/route-store port calls live there, reached through the resolver.
        self.assertIn("ports/OnlineRouteResolver.hpp", private_source)
        self.assertIn("ResolveOnlineRoute(peer_uid, _session_registry, _online_route_store)", private_source)
        resolver_source = (CHATSERVER_DIR / "domain" / "ports" / "OnlineRouteResolver.hpp").read_text(encoding="utf-8")
        self.assertIn("session_registry->FindSession(uid)", resolver_source)
        self.assertIn("online_route_store->RepairOnlineRoute(uid, session)", resolver_source)
        self.assertIn("online_route_store->FindUserServer(uid)", resolver_source)
        self.assertIn("online_route_store->ResolveServerFromOnlineSets(uid)", resolver_source)
        self.assertIn("online_route_store->ClearTrackedOnlineRoute(uid, self_name)", resolver_source)
        # The forward-delivery follow-up still clears a stale tracked route directly.
        self.assertIn("_online_route_store->ClearTrackedOnlineRoute(peer_uid, route.redis_server)", private_source)
        self.assertNotIn("UserMgr::GetInstance()->GetSession", private_source)
        self.assertNotIn("RedisMgr::GetInstance()->Get(USERIPPREFIX", private_source)

        composition_source = runtime_composition_source()
        constructor_block = text_between(
            composition_source,
            "_message_delivery_service =",
            "_group_message_service =",
        )
        self.assertRegex(
            constructor_block,
            r"_private_message_service\s*=\s*CreatePrivateMessageService",
        )
        self.assertIn("*_message_service_config", constructor_block)
        self.assertIn("UserMgr::GetInstance().get()", constructor_block)
        self.assertIn("_online_route_store.get()", constructor_block)

    def test_private_message_service_uses_message_repository_port(self):
        port_header = CHATSERVER_DIR / "domain" / "ports" / "IMessageRepository.hpp"
        adapter_header = CHATSERVER_DIR / "persistence" / "ChatMessageRepository.hpp"
        adapter_source_path = CHATSERVER_DIR / "persistence" / "ChatMessageRepository.cpp"
        self.assertTrue(port_header.is_file(), "message repository port header is missing")
        self.assertTrue(adapter_header.is_file(), "message repository adapter header is missing")
        self.assertTrue(adapter_source_path.is_file(), "message repository adapter source is missing")

        port_source = port_header.read_text(encoding="utf-8")
        self.assertIn("class IMessageRepository", port_source)
        for method in (
            "SavePrivateMessage",
            "FindPrivateMessageByClientId",
            "IsPrivateFriend",
            "UpsertPrivateReadState",
            "UpdatePrivateMessageContent",
            "RevokePrivateMessage",
            "GetPrivateHistory",
        ):
            self.assertIn(method, port_source)

        adapter_header_source = adapter_header.read_text(encoding="utf-8")
        self.assertIn("ports/IMessageRepository.hpp", adapter_header_source)
        self.assertRegex(
            adapter_header_source,
            r"class\s+ChatMessageRepository\s*:\s*public\s+IMessageRepository",
        )

        adapter_source = adapter_source_path.read_text(encoding="utf-8")
        self.assertIn('#include "PostgresMgr.hpp"', adapter_source)
        self.assertIn('#include "MongoMgr.hpp"', adapter_source)
        self.assertIn("WritePrimaryAndMirrorToMongo", adapter_source)
        self.assertIn("ReadMongoThenPostgres", adapter_source)
        self.assertIn("ShouldFallbackToPostgres", adapter_source)
        self.assertIn("ShouldLogMongoWriteFailure", adapter_source)

        private_header = (DOMAIN_MESSAGE_DIR / "PrivateMessageService.hpp").read_text(encoding="utf-8")
        self.assertIn("ports/IMessageRepository.hpp", private_header)
        self.assertIn("IMessageRepository* message_repository", private_header)
        self.assertIn("IMessageRepository* _message_repository", private_header)
        self.assertIn("IRelationRepository* relation_repository", private_header)
        self.assertIn("IRelationRepository* _relation_repository", private_header)

        private_source = (DOMAIN_MESSAGE_DIR / "PrivateMessageService.cpp").read_text(encoding="utf-8")
        self.assertNotIn('#include "PostgresMgr.hpp"', private_source)
        self.assertNotIn('#include "MongoMgr.hpp"', private_source)
        self.assertNotIn("PostgresMgr::GetInstance()->SavePrivateMessage", private_source)
        self.assertNotIn("MongoMgr::GetInstance()->SavePrivateMessage", private_source)
        self.assertNotIn("PostgresMgr::GetInstance()->GetPrivateHistory", private_source)
        self.assertNotIn("MongoMgr::GetInstance()->GetPrivateHistory", private_source)
        self.assertIn("_message_repository->SavePrivateMessage", private_source)
        self.assertIn("_message_repository->FindPrivateMessageByClientId", private_source)
        self.assertIn("_message_repository->IsPrivateFriend", private_source)
        self.assertIn("_message_repository->UpsertPrivateReadState", private_source)
        self.assertIn("_message_repository->UpdatePrivateMessageContent", private_source)
        self.assertIn("_message_repository->RevokePrivateMessage", private_source)
        self.assertIn("_message_repository->GetPrivateHistory", private_source)

        composition_header = runtime_composition_header()
        self.assertIn("class IMessageRepository;", composition_header)
        self.assertIn("std::unique_ptr<IMessageRepository> _message_repository", composition_header)
        self.assertNotIn("std::unique_ptr<IMessageRepository> _message_repository", logic_header_source())

        composition_source = runtime_composition_source()
        self.assertIn('#include "ChatMessageRepository.hpp"', composition_source)
        constructor_block = text_between(
            composition_source,
            "_message_repository = std::make_unique<ChatMessageRepository>()",
            "const auto task_bus_backend = memochat::chatruntime::TaskBusBackend()",
        )
        self.assertIn("_message_repository = std::make_unique<ChatMessageRepository>()", constructor_block)
        service_wiring_block = text_between(
            composition_source,
            "_message_delivery_service =",
            "_group_message_service =",
        )
        self.assertIn("_message_repository.get()", service_wiring_block)

    def test_group_message_service_uses_message_repository_for_message_persistence(self):
        port_source = (CHATSERVER_DIR / "domain" / "ports" / "IMessageRepository.hpp").read_text(encoding="utf-8")
        for method in (
            "SaveGroupMessage",
            "FindGroupMessageByClientId",
            "GetGroupHistory",
            "UpdateGroupMessageContent",
            "RevokeGroupMessage",
            "UpsertGroupReadState",
        ):
            self.assertIn(method, port_source)

        adapter_header_source = (CHATSERVER_DIR / "persistence" / "ChatMessageRepository.hpp").read_text(
            encoding="utf-8"
        )
        for method in (
            "SaveGroupMessage",
            "FindGroupMessageByClientId",
            "GetGroupHistory",
            "UpdateGroupMessageContent",
            "RevokeGroupMessage",
            "UpsertGroupReadState",
        ):
            self.assertIn(method, adapter_header_source)

        adapter_source = (CHATSERVER_DIR / "persistence" / "ChatMessageRepository.cpp").read_text(encoding="utf-8")
        self.assertIn("WritePrimaryAndMirrorToMongo", adapter_source)
        self.assertIn("ReadMongoThenPostgres", adapter_source)
        self.assertIn("SaveGroupMessage dual-write", adapter_source)
        self.assertIn("GetGroupMessageByMsgId", adapter_source)

        group_header = (DOMAIN_MESSAGE_DIR / "GroupMessageService.hpp").read_text(encoding="utf-8")
        self.assertIn("ports/IMessageRepository.hpp", group_header)
        self.assertIn("IMessageRepository* message_repository", group_header)
        self.assertIn("IMessageRepository* _message_repository", group_header)

        group_facade_source = (DOMAIN_MESSAGE_DIR / "GroupMessageService.cpp").read_text(encoding="utf-8")
        group_workflow_source = (DOMAIN_MESSAGE_DIR / "GroupMessageWorkflow.cpp").read_text(encoding="utf-8")
        group_history_source = (DOMAIN_MESSAGE_DIR / "GroupMessageHistoryService.cpp").read_text(encoding="utf-8")
        self.assertIn("_workflow_service->GroupChatMessage(request)", group_facade_source)
        self.assertIn("_history_service->GroupHistory(request)", group_facade_source)
        self.assertIn("_message_repository->SaveGroupMessage", group_workflow_source)
        self.assertIn("_message_repository->FindGroupMessageByClientId", group_workflow_source)
        self.assertIn("_message_repository->GetGroupHistory", group_history_source)
        self.assertIn("_message_repository->UpdateGroupMessageContent", group_workflow_source)
        self.assertIn("_message_repository->RevokeGroupMessage", group_workflow_source)
        self.assertIn("_message_repository->UpsertGroupReadState", group_history_source)
        group_repository_source = group_workflow_source + "\n" + group_history_source
        self.assertNotIn("PostgresMgr::GetInstance()->SaveGroupMessage", group_repository_source)
        self.assertNotIn("MongoMgr::GetInstance()->SaveGroupMessage", group_repository_source)
        self.assertNotIn("PostgresMgr::GetInstance()->GetGroupHistory", group_repository_source)
        self.assertNotIn("MongoMgr::GetInstance()->GetGroupHistory", group_repository_source)
        self.assertNotIn("PostgresMgr::GetInstance()->GetGroupMessageByMsgId", group_repository_source)
        self.assertNotIn("MongoMgr::GetInstance()->GetGroupMessageByMsgId", group_repository_source)
        self.assertNotIn("PostgresMgr::GetInstance()->UpdateGroupMessageContent", group_repository_source)
        self.assertNotIn("MongoMgr::GetInstance()->UpdateGroupMessageContent", group_repository_source)
        self.assertNotIn("PostgresMgr::GetInstance()->RevokeGroupMessage", group_repository_source)
        self.assertNotIn("MongoMgr::GetInstance()->RevokeGroupMessage", group_repository_source)
        self.assertNotIn("PostgresMgr::GetInstance()->UpsertGroupReadState", group_repository_source)

        composition_source = runtime_composition_source()
        constructor_block = text_between(
            composition_source,
            "_private_message_service = CreatePrivateMessageService",
            "_chat_relation_service = CreateRelationService",
        )
        self.assertRegex(
            constructor_block,
            r"_group_message_service\s*=\s*CreateGroupMessageService",
        )
        self.assertIn("*_message_service_config", constructor_block)
        self.assertIn("_message_repository.get()", constructor_block)

    def test_chat_relation_service_uses_relation_repository_for_relation_persistence(self):
        port_header = CHATSERVER_DIR / "domain" / "ports" / "IRelationRepository.hpp"
        self.assertTrue(port_header.is_file(), "relation repository port header is missing")
        port_source = port_header.read_text(encoding="utf-8")
        for method in (
            "GetUidByUserId",
            "RefreshDialogsForOwner",
            "GetDialogMetaByOwner",
            "GetPrivateDialogRuntime",
            "GetGroupDialogRuntime",
            "GetUserGroupList",
            "AddFriendApply",
            "ReplaceApplyTags",
            "AuthFriendApply",
            "AddFriend",
            "GetApplyTags",
            "ReplaceFriendTags",
            "DeleteFriend",
            "IsPrivateFriend",
            "IsGroupMember",
            "UpsertDialogDraft",
            "UpsertDialogMuteState",
            "UpsertDialogPinned",
        ):
            self.assertIn(method, port_source)

        adapter_header_path = CHATSERVER_DIR / "persistence" / "ChatRelationRepository.hpp"
        adapter_source_path = CHATSERVER_DIR / "persistence" / "ChatRelationRepository.cpp"
        self.assertTrue(adapter_header_path.is_file(), "relation repository adapter header is missing")
        self.assertTrue(adapter_source_path.is_file(), "relation repository adapter source is missing")

        adapter_header_source = adapter_header_path.read_text(encoding="utf-8")
        self.assertIn("ports/IRelationRepository.hpp", adapter_header_source)
        self.assertRegex(
            adapter_header_source,
            r"class\s+ChatRelationRepository\s*:\s*public\s+IRelationRepository",
        )

        adapter_source = adapter_source_path.read_text(encoding="utf-8")
        self.assertIn('#include "PostgresMgr.hpp"', adapter_source)
        self.assertIn("import memochat.chat.relation_repository_algorithms", adapter_source)
        self.assertIn("ShouldQueryPrivateFriendship", adapter_source)
        self.assertIn("ShouldFilterFriendUids", adapter_source)
        self.assertIn("ShouldQueryGroupMembership", adapter_source)
        self.assertNotIn("ForwardingSurfaceCount", adapter_source)

        relation_header = (DOMAIN_RELATION_DIR / "ChatRelationService.hpp").read_text(encoding="utf-8")
        self.assertIn("ports/IRelationRepository.hpp", relation_header)
        self.assertIn("ChatRelationService(IRelationRepository* relation_repository", relation_header)
        self.assertIn("IRelationRepository* _relation_repository", relation_header)

        relation_source = (DOMAIN_RELATION_DIR / "ChatRelationService.cpp").read_text(encoding="utf-8")
        self.assertNotIn('#include "PostgresMgr.hpp"', relation_source)
        self.assertIn("_relation_repository->GetUidByUserId", relation_source)
        self.assertIn("_relation_repository->RefreshDialogsForOwner", relation_source)
        self.assertIn("_relation_repository->GetDialogMetaByOwner", relation_source)
        self.assertIn("_relation_repository->GetPrivateDialogRuntime", relation_source)
        self.assertIn("_relation_repository->GetGroupDialogRuntime", relation_source)
        self.assertIn("_relation_repository->GetUserGroupList", relation_source)
        self.assertIn("_relation_repository->AddFriendApply", relation_source)
        self.assertIn("_relation_repository->ReplaceApplyTags", relation_source)
        self.assertIn("_relation_repository->AuthFriendApply", relation_source)
        self.assertIn("_relation_repository->AddFriend", relation_source)
        self.assertIn("_relation_repository->GetApplyTags", relation_source)
        self.assertIn("_relation_repository->ReplaceFriendTags", relation_source)
        self.assertIn("_relation_repository->DeleteFriend", relation_source)
        self.assertIn("_relation_repository->IsPrivateFriend", relation_source)
        self.assertIn("_relation_repository->IsGroupMember", relation_source)
        self.assertRegex(relation_source, r"_relation_repository\s*->UpsertDialogDraft")
        self.assertRegex(relation_source, r"_relation_repository\s*->UpsertDialogMuteState")
        self.assertRegex(relation_source, r"_relation_repository\s*->UpsertDialogPinned")
        for forbidden in (
            "PostgresMgr::GetInstance()->GetUidByUserId",
            "PostgresMgr::GetInstance()->RefreshDialogsForOwner",
            "PostgresMgr::GetInstance()->GetDialogMetaByOwner",
            "PostgresMgr::GetInstance()->GetPrivateDialogRuntime",
            "PostgresMgr::GetInstance()->GetGroupDialogRuntime",
            "PostgresMgr::GetInstance()->GetUserGroupList",
            "PostgresMgr::GetInstance()->AddFriendApply",
            "PostgresMgr::GetInstance()->ReplaceApplyTags",
            "PostgresMgr::GetInstance()->AuthFriendApply",
            "PostgresMgr::GetInstance()->AddFriend",
            "PostgresMgr::GetInstance()->GetApplyTags",
            "PostgresMgr::GetInstance()->ReplaceFriendTags",
            "PostgresMgr::GetInstance()->DeleteFriend",
            "PostgresMgr::GetInstance()->IsFriend",
            "PostgresMgr::GetInstance()->IsUserInGroup",
            "PostgresMgr::GetInstance()->UpsertDialogDraft",
            "PostgresMgr::GetInstance()->UpsertDialogMuteState",
            "PostgresMgr::GetInstance()->UpsertDialogPinned",
        ):
            self.assertNotIn(forbidden, relation_source)

        composition_header = runtime_composition_header()
        self.assertIn("class IRelationRepository;", composition_header)
        self.assertIn("std::unique_ptr<IRelationRepository> _relation_repository", composition_header)
        self.assertNotIn("std::unique_ptr<IRelationRepository> _relation_repository", logic_header_source())

        composition_source = runtime_composition_source()
        self.assertIn('#include "ChatRelationRepository.hpp"', composition_source)
        constructor_block = text_between(
            composition_source,
            "_relation_repository = std::make_unique<ChatRelationRepository>()",
            "_delivery_runtime = std::make_unique<ChatDeliveryRuntime>",
        )
        self.assertIn("_relation_repository = std::make_unique<ChatRelationRepository>()", constructor_block)
        self.assertRegex(
            constructor_block,
            r"_chat_relation_service\s*=\s*CreateRelationService",
        )
        self.assertIn("_relation_repository.get()", constructor_block)

    def test_chat_relation_service_uses_cache_delivery_task_and_event_ports(self):
        cache_port_path = CHATSERVER_DIR / "domain" / "ports" / "IRelationBootstrapCache.hpp"
        cache_adapter_header_path = CHATSERVER_DIR / "persistence" / "RedisRelationBootstrapCache.hpp"
        cache_adapter_source_path = CHATSERVER_DIR / "persistence" / "RedisRelationBootstrapCache.cpp"
        self.assertTrue(cache_port_path.is_file(), "relation bootstrap cache port header is missing")
        self.assertTrue(cache_adapter_header_path.is_file(), "redis relation bootstrap cache adapter header is missing")
        self.assertTrue(cache_adapter_source_path.is_file(), "redis relation bootstrap cache adapter source is missing")

        cache_port = cache_port_path.read_text(encoding="utf-8")
        self.assertIn("class IRelationBootstrapCache", cache_port)
        self.assertRegex(cache_port, r"virtual\s+bool\s+TryAppend")
        self.assertRegex(cache_port, r"virtual\s+void\s+Store")
        self.assertRegex(cache_port, r"virtual\s+void\s+Invalidate")

        cache_adapter_header = cache_adapter_header_path.read_text(encoding="utf-8")
        self.assertIn("ports/IRelationBootstrapCache.hpp", cache_adapter_header)
        self.assertRegex(
            cache_adapter_header,
            r"class\s+RedisRelationBootstrapCache\s*:\s*public\s+IRelationBootstrapCache",
        )

        cache_adapter_source = cache_adapter_source_path.read_text(encoding="utf-8")
        self.assertIn('#include "ConfigMgr.hpp"', cache_adapter_source)
        self.assertIn('#include "RedisMgr.hpp"', cache_adapter_source)
        self.assertIn("RedisMgr::GetInstance()->Get", cache_adapter_source)
        self.assertIn("RedisMgr::GetInstance()->SetEx", cache_adapter_source)
        self.assertIn("RedisMgr::GetInstance()->Del", cache_adapter_source)
        self.assertIn("cache_modules::SchemaVersion()", cache_adapter_source)
        self.assertIn('std::string("relation_bootstrap_v")', cache_adapter_source)
        self.assertIn("cache_modules::IsCurrentSchemaVersion", cache_adapter_source)
        self.assertIn('versioned_payload["schema_version"] = cache_modules::SchemaVersion()', cache_adapter_source)

        relation_header = (DOMAIN_RELATION_DIR / "ChatRelationService.hpp").read_text(encoding="utf-8")
        self.assertIn("ports/IRelationBootstrapCache.hpp", relation_header)
        self.assertIn("ports/IDeliveryGateway.hpp", relation_header)
        self.assertIn("ports/IDeliveryTaskPublisher.hpp", relation_header)
        self.assertIn("ports/IEventPublisher.hpp", relation_header)
        compact_relation_header = " ".join(relation_header.split())
        self.assertIn("ChatRelationService(IRelationRepository* relation_repository,", compact_relation_header)
        self.assertIn("IRelationBootstrapCache* relation_bootstrap_cache", compact_relation_header)
        self.assertIn("IDeliveryGateway* delivery_gateway", compact_relation_header)
        self.assertIn("IDeliveryTaskPublisher* task_publisher", compact_relation_header)
        self.assertIn("IEventPublisher* event_publisher", compact_relation_header)
        self.assertIn("IRelationBootstrapCache* _relation_bootstrap_cache", relation_header)
        self.assertIn("IDeliveryGateway* _delivery_gateway", relation_header)
        self.assertIn("IDeliveryTaskPublisher* _task_publisher", relation_header)
        self.assertIn("IEventPublisher* _event_publisher", relation_header)
        self.assertNotIn("LogicSystem&", relation_header)

        relation_source = (DOMAIN_RELATION_DIR / "ChatRelationService.cpp").read_text(encoding="utf-8")
        for forbidden in (
            '#include "ConfigMgr.hpp"',
            '#include "RedisMgr.hpp"',
            '#include "UserMgr.hpp"',
            '#include "LogicSystem.hpp"',
            '#include "MessageDeliveryService.hpp"',
            "ConfigMgr::Inst",
            "RedisMgr::GetInstance()",
            "_logic.",
        ):
            self.assertNotIn(forbidden, relation_source)
        self.assertIn("_relation_bootstrap_cache->TryAppend", relation_source)
        self.assertIn("_relation_bootstrap_cache->Store", relation_source)
        self.assertIn("_relation_bootstrap_cache->Invalidate", relation_source)
        self.assertIn("_delivery_gateway->TryPushPayload", relation_source)
        self.assertIn("_task_publisher->PublishDeliveryTask", relation_source)
        self.assertIn("event_publisher->PublishEvent", relation_source)
        self.assertIn("PublishRelationStateEventLocal(_event_publisher", relation_source)

        cmake = read_cmake()
        self.assertIn(
            "persistence/RedisRelationBootstrapCache.cpp",
            cmake_set_values(cmake, "CHATSERVER_PERSISTENCE_SOURCES"),
        )

        composition_header = runtime_composition_header()
        self.assertIn("class IRelationBootstrapCache;", composition_header)
        self.assertIn("std::unique_ptr<IRelationBootstrapCache> _relation_bootstrap_cache", composition_header)
        self.assertNotIn("std::unique_ptr<IRelationBootstrapCache> _relation_bootstrap_cache", logic_header_source())

        composition_source = runtime_composition_source()
        self.assertIn('#include "RedisRelationBootstrapCache.hpp"', composition_source)
        self.assertIn("_relation_bootstrap_cache = std::make_unique<RedisRelationBootstrapCache>()", composition_source)
        constructor_block = text_between(
            composition_source,
            "_chat_relation_service = CreateRelationService",
            "_delivery_runtime = std::make_unique<ChatDeliveryRuntime>",
        )
        self.assertIn("*_relation_service_config", constructor_block)
        self.assertIn("_relation_repository.get()", constructor_block)
        self.assertIn("_relation_bootstrap_cache.get()", constructor_block)

    def test_user_base_info_cache_miss_when_public_user_id_is_absent(self):
        user_support = (CHATSERVER_DIR / "domain/users/ChatUserSupport.cpp").read_text(encoding="utf-8")
        transport = (CHATSERVER_DIR / "transport/ChatServiceImpl.cpp").read_text(encoding="utf-8")

        support_body = extract_function(
            user_support,
            "bool GetBaseInfo(const std::string& base_key, int uid, std::shared_ptr<UserInfo>& userinfo)",
        )
        self.assertIn("DecodeChatUserProfileCache(info_str, &profile);", support_body)
        self.assertIn("user_support_modules::ShouldUseCachedProfile(cache_hit, profile.user_id.empty())", support_body)
        self.assertIn("FillUserInfoFromChatUserProfile(profile, *userinfo);", support_body)
        self.assertIn("auto user_info = PostgresMgr::GetInstance()->GetUser(uid);", support_body)
        self.assertLess(
            support_body.index("user_support_modules::ShouldUseCachedProfile(cache_hit, profile.user_id.empty())"),
            support_body.index("PostgresMgr::GetInstance()->GetUser(uid)"),
        )

        transport_body = extract_function(
            transport,
            "bool ChatServiceImpl::GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo)",
        )
        self.assertIn('userinfo->user_id = root["user_id"].asString();', transport_body)
        self.assertIn("if (userinfo->user_id.empty())", transport_body)
        self.assertIn("auto user_info = PostgresMgr::GetInstance()->GetUser(uid);", transport_body)
        self.assertIn('redis_root["user_id"] = userinfo->user_id;', transport_body)

        constructor_block = text_between(
            runtime_composition_source(),
            "_chat_relation_service = CreateRelationService",
            "_chat_relation_session_service = std::make_unique<ChatRelationSessionAdapter>",
        )
        self.assertIn("_message_delivery_service.get()", constructor_block)
        self.assertIn("_task_dispatcher.get()", constructor_block)
        self.assertIn("_async_event_dispatcher.get()", constructor_block)

    def test_user_lookup_by_name_projects_icon_for_avatar_surfaces(self):
        user_support = (CHATSERVER_DIR / "domain/users/ChatUserSupport.cpp").read_text(encoding="utf-8")
        body = extract_function(
            user_support,
            "void GetUserByName(const std::string& name, memochat::json::JsonValue& rtvalue)",
        )

        self.assertEqual(body.count("AppendChatUserProfileToJsonValue(profile, rtvalue, true);"), 2)
        self.assertNotIn("AppendChatUserProfileToJsonValue(profile, rtvalue, false);", body)

    def test_chat_relation_service_has_ingress_facing_service_contract(self):
        relation_query_port_path = CHATSERVER_DIR / "domain" / "ports" / "IRelationQueryService.hpp"
        self.assertTrue(relation_query_port_path.is_file(), "relation query service contract header is missing")
        relation_query_port = relation_query_port_path.read_text(encoding="utf-8")
        self.assertIn("class IRelationQueryService", relation_query_port)
        self.assertIn("virtual ~IRelationQueryService() = default", relation_query_port)
        self.assertIn("AppendRelationBootstrapJson", relation_query_port)
        self.assertIn("BuildDialogListJson", relation_query_port)

        relation_command_port_path = CHATSERVER_DIR / "domain" / "ports" / "IRelationCommandService.hpp"
        self.assertTrue(relation_command_port_path.is_file(), "relation command service contract header is missing")
        relation_command_port = relation_command_port_path.read_text(encoding="utf-8")
        for token in (
            "struct RelationCommandRequest",
            "struct RelationCommandResult",
            "short request_msg_id",
            "std::string payload_json",
            "int session_uid",
            "short response_msg_id",
            "class IRelationCommandService",
            "virtual ~IRelationCommandService() = default",
        ):
            self.assertIn(token, relation_command_port)
        for method in (
            "SearchUser",
            "AddFriendApply",
            "AuthFriendApply",
            "DeleteFriend",
            "GetDialogList",
            "SyncDraft",
            "PinDialog",
        ):
            self.assertRegex(
                relation_command_port,
                rf"virtual\s+RelationCommandResult\s+{re.escape(method)}\(const\s+RelationCommandRequest&\s+request\)",
            )

        relation_service_port_path = CHATSERVER_DIR / "domain" / "ports" / "IRelationService.hpp"
        relation_session_port_path = CHATSERVER_DIR / "domain" / "ports" / "IRelationSessionService.hpp"
        self.assertTrue(relation_service_port_path.is_file(), "relation service contract header is missing")
        self.assertTrue(relation_session_port_path.is_file(), "relation session service contract header is missing")

        relation_service_port = relation_service_port_path.read_text(encoding="utf-8")
        self.assertIn("ports/IRelationQueryService.hpp", relation_service_port)
        self.assertIn("ports/IRelationCommandService.hpp", relation_service_port)
        self.assertRegex(
            relation_service_port,
            r"class\s+IRelationService\s*:\s*public\s+IRelationQueryService\s*,\s*public\s+IRelationCommandService",
        )
        self.assertIn("virtual ~IRelationService() = default", relation_service_port)
        self.assertNotIn("HandleSearchUser", relation_service_port)
        self.assertNotIn("HandlePinDialog", relation_service_port)

        relation_session_port = relation_session_port_path.read_text(encoding="utf-8")
        self.assertIn("class IRelationSessionService", relation_session_port)
        self.assertIn("virtual ~IRelationSessionService() = default", relation_session_port)
        for method in (
            "HandleSearchUser",
            "HandleAddFriendApply",
            "HandleAuthFriendApply",
            "HandleDeleteFriend",
            "HandleGetDialogList",
            "HandleSyncDraft",
            "HandlePinDialog",
        ):
            self.assertRegex(relation_session_port, rf"virtual\s+void\s+{re.escape(method)}")

        relation_header = (DOMAIN_RELATION_DIR / "ChatRelationService.hpp").read_text(encoding="utf-8")
        self.assertIn("ports/IRelationService.hpp", relation_header)
        self.assertIn("ports/IRelationCommandService.hpp", relation_header)
        self.assertRegex(relation_header, r"class\s+ChatRelationService\s*:\s*public\s+IRelationService")
        self.assertIn("AppendRelationBootstrapJson(int uid, memochat::json::JsonValue& out) override", relation_header)
        self.assertIn("BuildDialogListJson(int uid, memochat::json::JsonValue& out) override", relation_header)
        for method in (
            "SearchUser",
            "AddFriendApply",
            "AuthFriendApply",
            "DeleteFriend",
            "GetDialogList",
            "SyncDraft",
            "PinDialog",
        ):
            self.assertRegex(
                relation_header,
                rf"RelationCommandResult\s+{re.escape(method)}\(const\s+RelationCommandRequest&\s+request\)\s+override",
            )
        self.assertNotIn("HandleSearchUser(const std::shared_ptr<CSession>& session", relation_header)
        self.assertNotIn("CSession", relation_header)

        relation_source = (DOMAIN_RELATION_DIR / "ChatRelationService.cpp").read_text(encoding="utf-8")
        self.assertNotIn('#include "CSession.hpp"', relation_source)
        self.assertNotIn("SendRelationCommandResult", relation_source)

        relation_session_header = (DOMAIN_RELATION_DIR / "ChatRelationSessionAdapter.hpp").read_text(encoding="utf-8")
        self.assertIn("ports/IRelationSessionService.hpp", relation_session_header)
        self.assertRegex(
            relation_session_header,
            r"class\s+ChatRelationSessionAdapter\s+final\s*:\s*public\s+IRelationSessionService",
        )
        self.assertIn("HandleSearchUser(const std::shared_ptr<IChatSession>& session", relation_session_header)
        self.assertNotIn("std::shared_ptr<CSession>", relation_session_header)

        relation_session_adapter = (DOMAIN_RELATION_DIR / "ChatRelationSessionAdapter.cpp").read_text(encoding="utf-8")
        self.assertIn("BuildRelationCommandRequest(session, msg_id, msg_data)", relation_session_adapter)
        self.assertIn("SendRelationCommandResult", relation_session_adapter)
        self.assertIn(
            "_relation_service->SearchUser(BuildRelationCommandRequest(session, msg_id, msg_data))",
            relation_session_adapter,
        )
        self.assertIn(
            "_relation_service->PinDialog(BuildRelationCommandRequest(session, msg_id, msg_data))",
            relation_session_adapter,
        )
        self.assertIn('#include "IChatSession.hpp"', relation_session_adapter)
        self.assertNotIn('#include "CSession.hpp"', relation_session_adapter)
        self.assertIn("ChatRelationSessionAdapter::HandleSearchUser", relation_session_adapter)
        self.assertNotIn("ChatRelationService::HandleSearchUser", relation_session_adapter)

        self.assertNotIn("SendRelationCommandResult(session, SearchUser(", relation_source)
        self.assertNotIn("SendRelationCommandResult(session, AddFriendApply(", relation_source)
        self.assertNotIn("SendRelationCommandResult(session, AuthFriendApply(", relation_source)
        self.assertNotIn("SendRelationCommandResult(session, DeleteFriend(", relation_source)
        self.assertNotIn("SendRelationCommandResult(session, GetDialogList(", relation_source)
        self.assertNotIn("SendRelationCommandResult(session, SyncDraft(", relation_source)
        self.assertNotIn("SendRelationCommandResult(session, PinDialog(", relation_source)

    def test_relation_service_remote_backend_has_full_command_grpc_adapter(self):
        client_header_path = CHATSERVER_DIR / "clients" / "RelationGrpcClient.hpp"
        client_source_path = CHATSERVER_DIR / "clients" / "RelationGrpcClient.cpp"
        service_adapter_header_path = CHATSERVER_DIR / "clients" / "RelationGrpcServiceAdapter.hpp"
        service_adapter_source_path = CHATSERVER_DIR / "clients" / "RelationGrpcServiceAdapter.cpp"
        factory_source_path = DOMAIN_RELATION_DIR / "RelationServiceFactory.cpp"
        grpc_header_path = DOMAIN_RELATION_DIR / "ChatRelationInternalGrpcService.hpp"
        grpc_source_path = DOMAIN_RELATION_DIR / "ChatRelationInternalGrpcService.cpp"
        proto_path = COMMON_PROTO_DIR / "chat_internal.proto"
        test_cmake_path = REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt"
        client_test_path = (
            REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "RelationGrpcClientTest.cpp"
        )
        factory_test_path = (
            REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "RelationServiceFactoryTest.cpp"
        )

        for path, label in (
            (client_header_path, "relation grpc client header"),
            (client_source_path, "relation grpc client source"),
            (service_adapter_header_path, "relation grpc service adapter header"),
            (service_adapter_source_path, "relation grpc service adapter source"),
            (client_test_path, "relation grpc client C++ test"),
            (factory_test_path, "relation service factory C++ test"),
        ):
            self.assertTrue(path.is_file(), f"{label} is missing")

        proto = proto_path.read_text(encoding="utf-8")
        self.assertRegex(proto, r"message\s+JsonPayloadResponse\s*{[^}]*int32\s+tcp_msg_id\s*=\s*2\s*;", re.S)

        grpc_header = grpc_header_path.read_text(encoding="utf-8")
        self.assertIn("ports/IRelationService.hpp", grpc_header)
        self.assertIn("IRelationService* relation_service", grpc_header)
        for method in (
            "SearchUser",
            "AddFriendApply",
            "AuthFriendApply",
            "DeleteFriend",
            "GetDialogList",
            "SyncDraft",
            "PinDialog",
        ):
            self.assertRegex(
                grpc_header,
                rf"grpc::Status\s+{re.escape(method)}\(grpc::ServerContext\*\s+context,\s*const\s+chatinternal::JsonPayloadRequest\*\s+request,\s*chatinternal::JsonPayloadResponse\*\s+response\)\s+override",
            )

        grpc_source = grpc_source_path.read_text(encoding="utf-8")
        self.assertIn("BuildCommandRequest", grpc_source)
        self.assertIn("BuildCommandResponse", grpc_source)
        for method in (
            "SearchUser",
            "AddFriendApply",
            "AuthFriendApply",
            "DeleteFriend",
            "GetDialogList",
            "SyncDraft",
            "PinDialog",
        ):
            self.assertIn(f"&IRelationCommandService::{method}", grpc_source)

        client_header = client_header_path.read_text(encoding="utf-8")
        self.assertIn("common/proto/chat_internal.grpc.pb.h", client_header)
        self.assertIn("ports/IRelationQueryService.hpp", client_header)
        self.assertIn("ports/IRelationCommandService.hpp", client_header)
        self.assertNotIn("ports/IRelationService.hpp", client_header)
        self.assertRegex(
            client_header,
            r"class\s+RelationGrpcClient\s+final\s*:\s*public\s+IRelationQueryService\s*,\s*public\s+IRelationCommandService",
        )
        self.assertIn("RelationCommandResult SearchUser(const RelationCommandRequest& request) override", client_header)
        self.assertNotIn("void HandleSearchUser", client_header)

        client_source = client_source_path.read_text(encoding="utf-8")
        self.assertIn("_stub->SearchUser", client_source)
        self.assertIn("CallCommand", client_source)
        self.assertIn("CallQuery", client_source)
        self.assertIn("relation_grpc_modules::RemoteErrorField()", client_source)
        self.assertNotIn('#include "transport/CSession.hpp"', client_source)

        service_adapter_header = service_adapter_header_path.read_text(encoding="utf-8")
        self.assertIn('#include "RelationGrpcClient.hpp"', service_adapter_header)
        self.assertIn("ports/IRelationService.hpp", service_adapter_header)
        self.assertRegex(
            service_adapter_header, r"class\s+RelationGrpcServiceAdapter\s+final\s*:\s*public\s+IRelationService"
        )
        self.assertIn(
            "RelationCommandResult SearchUser(const RelationCommandRequest& request) override", service_adapter_header
        )
        self.assertNotIn("void HandleSearchUser", service_adapter_header)

        service_adapter_source = service_adapter_source_path.read_text(encoding="utf-8")
        self.assertNotIn('#include "transport/CSession.hpp"', service_adapter_source)
        self.assertNotIn("BuildSessionCommandRequest(session, msg_id, msg_data)", service_adapter_source)
        self.assertNotIn("SendSessionCommandResult(session, SearchUser(", service_adapter_source)
        self.assertNotIn("session->Send(result.payload_json, result.response_msg_id)", service_adapter_source)

        factory_source = factory_source_path.read_text(encoding="utf-8")
        self.assertIn('#include "RelationGrpcServiceAdapter.hpp"', factory_source)
        self.assertIn("std::make_unique<RelationGrpcServiceAdapter>", factory_source)
        self.assertNotIn("remote_backend_not_implemented", factory_source)
        self.assertIn("Relation service remote endpoint is empty", factory_source)

        cmake = read_cmake()
        self.assertIn("clients/RelationGrpcClient.cpp", cmake_set_values(cmake, "CHATSERVER_CLIENT_SOURCES"))
        self.assertIn("clients/RelationGrpcServiceAdapter.cpp", cmake_set_values(cmake, "CHATSERVER_CLIENT_SOURCES"))
        test_cmake = test_cmake_path.read_text(encoding="utf-8")
        self.assertIn("RelationGrpcClientTest.cpp", test_cmake)
        self.assertIn("RelationServiceFactoryTest.cpp", test_cmake)

        for cfg in (
            "chatserver1.ini",
            "chatserver2.ini",
        ):
            config_text = (CHATSERVER_DIR / cfg).read_text(encoding="utf-8")
            assert_ini_section_has(
                self,
                config_text,
                "RelationService",
                "Backend=grpc",
                "Endpoint=127.0.0.1:50091",
            )

        composition_header = runtime_composition_header()
        self.assertIn("class IRelationService;", composition_header)
        self.assertIn("class IRelationSessionService;", composition_header)
        self.assertIn("std::unique_ptr<IRelationService> _chat_relation_service", composition_header)
        self.assertIn("std::unique_ptr<IRelationSessionService> _chat_relation_session_service", composition_header)
        self.assertNotIn("std::unique_ptr<ChatRelationService> _chat_relation_service", composition_header)
        self.assertNotIn("std::unique_ptr<IRelationService> _chat_relation_service", logic_header_source())
        self.assertNotIn(
            "std::unique_ptr<IRelationSessionService> _chat_relation_session_service", logic_header_source()
        )

        registrar_source = (DOMAIN_ORCHESTRATION_DIR / "ChatHandlerRegistrars.cpp").read_text(encoding="utf-8")
        self.assertIn('#include "ports/IRelationSessionService.hpp"', registrar_source)
        self.assertNotIn('#include "ChatRelationService.hpp"', registrar_source)
        self.assertIn("&IRelationSessionService::HandleSearchUser", registrar_source)
        self.assertIn("&IRelationSessionService::HandleAddFriendApply", registrar_source)
        self.assertIn("&IRelationSessionService::HandleAuthFriendApply", registrar_source)
        self.assertIn("&IRelationSessionService::HandleDeleteFriend", registrar_source)
        self.assertIn("&IRelationSessionService::HandleGetDialogList", registrar_source)
        self.assertIn("&IRelationSessionService::HandleSyncDraft", registrar_source)
        self.assertIn("&IRelationSessionService::HandlePinDialog", registrar_source)
        self.assertNotIn("&ChatRelationService::", registrar_source)

    def test_relation_service_construction_is_hidden_behind_factory(self):
        factory_header_path = DOMAIN_RELATION_DIR / "RelationServiceFactory.hpp"
        factory_source_path = DOMAIN_RELATION_DIR / "RelationServiceFactory.cpp"
        self.assertTrue(factory_header_path.is_file(), "relation service factory header is missing")
        self.assertTrue(factory_source_path.is_file(), "relation service factory source is missing")

        factory_header = factory_header_path.read_text(encoding="utf-8")
        self.assertIn("ports/IRelationService.hpp", factory_header)
        self.assertIn("ports/IRelationRepository.hpp", factory_header)
        self.assertIn("ports/IRelationBootstrapCache.hpp", factory_header)
        self.assertIn("ports/IRelationServiceConfig.hpp", factory_header)
        self.assertIn("std::unique_ptr<IRelationService> CreateInProcessRelationService", factory_header)
        self.assertIn("std::unique_ptr<IRelationService> CreateRelationService", factory_header)
        self.assertIn("const IRelationServiceConfig& relation_service_config", factory_header)
        self.assertIn("IRelationRepository* relation_repository", factory_header)
        self.assertIn("IRelationBootstrapCache* relation_bootstrap_cache", factory_header)
        self.assertIn("IDeliveryGateway* delivery_gateway", factory_header)
        self.assertIn("IDeliveryTaskPublisher* task_publisher", factory_header)
        self.assertIn("IEventPublisher* event_publisher", factory_header)

        factory_source = factory_source_path.read_text(encoding="utf-8")
        self.assertIn('#include "ChatRelationService.hpp"', factory_source)
        self.assertIn('#include "logging/Logger.hpp"', factory_source)
        self.assertIn("<stdexcept>", factory_source)
        self.assertIn("std::make_unique<ChatRelationService>", factory_source)
        self.assertIn('#include "RelationGrpcServiceAdapter.hpp"', factory_source)
        self.assertIn("memochat::chat::factory::modules::IsInProcessBackend", factory_source)
        self.assertIn("memochat::chat::factory::modules::IsRemoteBackend", factory_source)
        self.assertIn("relation_service_config.RelationServiceEndpoint()", factory_source)
        self.assertIn("std::make_unique<RelationGrpcServiceAdapter>", factory_source)
        self.assertIn("Relation service remote endpoint is empty", factory_source)
        self.assertNotIn("chat.relation_service.remote_backend_not_implemented", factory_source)
        self.assertIn("chat.relation_service.unsupported_backend", factory_source)
        self.assertIn('{"fallback_backend", "inprocess"}', factory_source)
        self.assertNotIn('#include "LogicSystem.hpp"', factory_source)

        cmake = read_cmake()
        self.assertIn(
            "domain/relation/RelationServiceFactory.cpp", cmake_set_values(cmake, "CHATSERVER_RELATION_SOURCES")
        )

        composition_source = runtime_composition_source()
        self.assertIn('#include "RelationServiceFactory.hpp"', composition_source)
        self.assertNotIn('#include "ChatRelationService.hpp"', composition_source)
        self.assertIn("CreateRelationService(*_relation_service_config", composition_source)
        before_handler_boundary = composition_source.split("CHATSERVER_LEGACY_LOGICSYSTEM_HANDLERS_BEGIN", 1)[0]
        self.assertNotIn("_chat_relation_service = CreateInProcessRelationService(", before_handler_boundary)
        self.assertNotIn("std::make_unique<ChatRelationService>", composition_source)

    def test_relation_service_backend_config_defaults_to_inprocess(self):
        config_port_path = CHATSERVER_DIR / "domain" / "ports" / "IRelationServiceConfig.hpp"
        config_adapter_header_path = CHATSERVER_DIR / "config" / "RelationServiceConfig.hpp"
        config_adapter_source_path = CHATSERVER_DIR / "config" / "RelationServiceConfig.cpp"
        for path, label in (
            (config_port_path, "relation service config port header"),
            (config_adapter_header_path, "relation service config adapter header"),
            (config_adapter_source_path, "relation service config adapter source"),
        ):
            self.assertTrue(path.is_file(), f"{label} is missing")

        config_port = config_port_path.read_text(encoding="utf-8")
        self.assertIn("class IRelationServiceConfig", config_port)
        self.assertIn("virtual std::string RelationServiceBackend() const = 0", config_port)
        self.assertIn("virtual std::string RelationServiceEndpoint() const = 0", config_port)

        config_adapter_header = config_adapter_header_path.read_text(encoding="utf-8")
        self.assertIn("ports/IRelationServiceConfig.hpp", config_adapter_header)
        self.assertRegex(config_adapter_header, r"class\s+RelationServiceConfig\s*:\s*public\s+IRelationServiceConfig")
        self.assertIn("std::string RelationServiceEndpoint() const override", config_adapter_header)

        config_adapter_source = config_adapter_source_path.read_text(encoding="utf-8")
        self.assertIn('#include "ConfigMgr.hpp"', config_adapter_source)
        self.assertIn('GetValue("RelationService", "Backend")', config_adapter_source)
        self.assertIn('GetValue("RelationService", "Endpoint")', config_adapter_source)
        self.assertIn('return "inprocess"', config_adapter_source)
        self.assertIn("memochat::chat::config::modules::LowerAsciiInPlace", config_adapter_source)

        cmake = read_cmake()
        self.assertIn("config/RelationServiceConfig.cpp", cmake_set_values(cmake, "CHATSERVER_CONFIG_SOURCES"))

        composition_header = runtime_composition_header()
        self.assertIn("class IRelationServiceConfig;", composition_header)
        self.assertIn("std::unique_ptr<IRelationServiceConfig> _relation_service_config", composition_header)
        self.assertNotIn("std::unique_ptr<IRelationServiceConfig> _relation_service_config", logic_header_source())

        composition_source = runtime_composition_source()
        self.assertIn('#include "RelationServiceConfig.hpp"', composition_source)
        self.assertIn("_relation_service_config = std::make_unique<RelationServiceConfig>()", composition_source)
        self.assertIn("CreateRelationService(*_relation_service_config", composition_source)

    def test_relation_internal_grpc_adapter_wraps_synchronous_relation_port_methods(self):
        adapter_header_path = DOMAIN_RELATION_DIR / "ChatRelationInternalGrpcService.hpp"
        adapter_source_path = DOMAIN_RELATION_DIR / "ChatRelationInternalGrpcService.cpp"
        adapter_test_path = (
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "ChatServer"
            / "ChatRelationInternalGrpcServiceTest.cpp"
        )
        adapter_cmake_path = REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt"

        self.assertTrue(adapter_header_path.is_file(), "relation internal grpc adapter header is missing")
        self.assertTrue(adapter_source_path.is_file(), "relation internal grpc adapter source is missing")
        self.assertTrue(adapter_test_path.is_file(), "relation internal grpc adapter C++ test is missing")
        self.assertTrue(adapter_cmake_path.is_file(), "relation internal grpc adapter test CMake is missing")

        adapter_header = adapter_header_path.read_text(encoding="utf-8")
        self.assertIn("common/proto/chat_internal.grpc.pb.h", adapter_header)
        self.assertIn("ports/IRelationService.hpp", adapter_header)
        self.assertRegex(
            adapter_header,
            r"class\s+ChatRelationInternalGrpcService\s+final\s*:\s*public\s+chatinternal::ChatRelationInternalService::Service",
        )
        self.assertIn("IRelationService* _relation_service", adapter_header)
        self.assertIn("AppendRelationBootstrap", adapter_header)
        self.assertIn("BuildDialogList", adapter_header)
        self.assertIn("SearchUser", adapter_header)
        self.assertIn("PinDialog", adapter_header)

        adapter_source = adapter_source_path.read_text(encoding="utf-8")
        self.assertIn("&IRelationQueryService::AppendRelationBootstrapJson", adapter_source)
        self.assertIn("&IRelationQueryService::BuildDialogListJson", adapter_source)
        self.assertIn("(_relation_service->*builder)", adapter_source)
        self.assertIn("&IRelationCommandService::SearchUser", adapter_source)
        self.assertIn("&IRelationCommandService::PinDialog", adapter_source)
        self.assertIn("grpc::Status(grpc::StatusCode::FAILED_PRECONDITION", adapter_source)
        self.assertIn("grpc::Status(grpc::StatusCode::INVALID_ARGUMENT", adapter_source)

        cmake = read_cmake()
        relation_sources = cmake_set_values(cmake, "CHATSERVER_RELATION_SOURCES")
        self.assertIn("domain/relation/ChatRelationInternalGrpcService.cpp", relation_sources)

        tests_cmake = (REPO_ROOT / "tests" / "CMakeLists.txt").read_text(encoding="utf-8")
        self.assertIn("cpp/apps/server/core/ChatServer", tests_cmake)

        adapter_test = adapter_test_path.read_text(encoding="utf-8")
        self.assertIn("ChatRelationInternalGrpcServiceTest", adapter_test)
        self.assertIn("FakeRelationService", adapter_test)

        for config_name in (
            "config.ini",
            "chatserver1.ini",
            "chatserver2.ini",
        ):
            config_source = (CHATSERVER_DIR / config_name).read_text(encoding="utf-8-sig")
            assert_ini_section_has(
                self,
                config_source,
                "RelationService",
                "Backend=grpc",
                "Endpoint=127.0.0.1:50091",
            )
        worker_config_source = (CHATSERVER_DIR / "chatdeliveryworker1.ini").read_text(encoding="utf-8-sig")
        assert_ini_section_has(self, worker_config_source, "RelationService", "Backend=inprocess")

    def test_relation_query_grpc_client_adapter_is_query_port_only_and_not_enabled_by_factory(self):
        adapter_header_path = CHATSERVER_DIR / "clients" / "RelationQueryGrpcClient.hpp"
        adapter_source_path = CHATSERVER_DIR / "clients" / "RelationQueryGrpcClient.cpp"
        adapter_test_path = (
            REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "RelationQueryGrpcClientTest.cpp"
        )
        adapter_cmake_path = REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt"

        self.assertTrue(adapter_header_path.is_file(), "relation query grpc client header is missing")
        self.assertTrue(adapter_source_path.is_file(), "relation query grpc client source is missing")
        self.assertTrue(adapter_test_path.is_file(), "relation query grpc client C++ test is missing")

        adapter_header = adapter_header_path.read_text(encoding="utf-8")
        self.assertIn("common/proto/chat_internal.grpc.pb.h", adapter_header)
        self.assertIn("ports/IRelationQueryService.hpp", adapter_header)
        self.assertNotIn("ports/IRelationService.hpp", adapter_header)
        self.assertRegex(
            adapter_header, r"class\s+RelationQueryGrpcClient\s+final\s*:\s*public\s+IRelationQueryService"
        )
        self.assertIn("std::shared_ptr<grpc::Channel>", adapter_header)
        self.assertIn("AppendRelationBootstrapJson(int uid, memochat::json::JsonValue& out) override", adapter_header)
        self.assertIn("BuildDialogListJson(int uid, memochat::json::JsonValue& out) override", adapter_header)

        adapter_source = adapter_source_path.read_text(encoding="utf-8")
        self.assertIn("ChatRelationInternalService::NewStub", adapter_source)
        self.assertIn("AppendRelationBootstrap", adapter_source)
        self.assertIn("BuildDialogList", adapter_source)
        self.assertIn("grpc::ClientContext", adapter_source)
        self.assertIn("set_deadline", adapter_source)
        self.assertIn("memochat::json::reader_parse", adapter_source)
        self.assertIn("memochat::json::getMemberNames", adapter_source)
        self.assertIn("relation_query_grpc_modules::RemoteErrorField()", adapter_source)
        self.assertNotIn("IRelationService", adapter_source)

        cmake = read_cmake()
        self.assertIn("clients/RelationQueryGrpcClient.cpp", cmake_set_values(cmake, "CHATSERVER_CLIENT_SOURCES"))

        adapter_cmake = adapter_cmake_path.read_text(encoding="utf-8")
        self.assertIn("RelationQueryGrpcClientTest.cpp", adapter_cmake)
        self.assertIn("chatserver_relation_query_grpc_client_gtest", adapter_cmake)

        adapter_test = adapter_test_path.read_text(encoding="utf-8")
        self.assertIn("RelationQueryGrpcClientTest", adapter_test)
        self.assertIn("ChatRelationInternalGrpcService", adapter_test)

        relation_factory = (DOMAIN_RELATION_DIR / "RelationServiceFactory.cpp").read_text(encoding="utf-8")
        self.assertIn("memochat::chat::factory::modules::IsRemoteBackend", relation_factory)
        self.assertIn("std::make_unique<RelationGrpcServiceAdapter>", relation_factory)
        self.assertNotIn("Relation service remote backend is not implemented yet", relation_factory)

    def test_relation_query_service_selection_is_separate_from_full_relation_backend(self):
        config_port_path = CHATSERVER_DIR / "domain" / "ports" / "IRelationQueryServiceConfig.hpp"
        config_adapter_header_path = CHATSERVER_DIR / "config" / "RelationQueryServiceConfig.hpp"
        config_adapter_source_path = CHATSERVER_DIR / "config" / "RelationQueryServiceConfig.cpp"
        factory_header_path = CHATSERVER_DIR / "clients" / "RelationQueryServiceFactory.hpp"
        factory_source_path = CHATSERVER_DIR / "clients" / "RelationQueryServiceFactory.cpp"
        factory_test_path = (
            REPO_ROOT
            / "tests"
            / "cpp"
            / "apps"
            / "server"
            / "core"
            / "ChatServer"
            / "RelationQueryServiceFactoryTest.cpp"
        )
        adapter_cmake_path = REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt"

        for path, label in (
            (config_port_path, "relation query config port"),
            (config_adapter_header_path, "relation query config adapter header"),
            (config_adapter_source_path, "relation query config adapter source"),
            (factory_header_path, "relation query service factory header"),
            (factory_source_path, "relation query service factory source"),
            (factory_test_path, "relation query service factory C++ test"),
        ):
            self.assertTrue(path.is_file(), f"{label} is missing")

        config_port = config_port_path.read_text(encoding="utf-8")
        self.assertIn("class IRelationQueryServiceConfig", config_port)
        self.assertIn("virtual std::string RelationQueryServiceBackend() const = 0", config_port)
        self.assertIn("virtual std::string RelationQueryServiceEndpoint() const = 0", config_port)

        config_adapter_header = config_adapter_header_path.read_text(encoding="utf-8")
        self.assertIn("ports/IRelationQueryServiceConfig.hpp", config_adapter_header)
        self.assertRegex(
            config_adapter_header, r"class\s+RelationQueryServiceConfig\s*:\s*public\s+IRelationQueryServiceConfig"
        )

        config_adapter_source = config_adapter_source_path.read_text(encoding="utf-8")
        self.assertIn('GetValue("RelationQueryService", "Backend")', config_adapter_source)
        self.assertIn('GetValue("RelationQueryService", "Endpoint")', config_adapter_source)
        self.assertIn('return "inprocess"', config_adapter_source)
        self.assertIn("memochat::chat::config::modules::LowerAsciiInPlace", config_adapter_source)

        factory_header = factory_header_path.read_text(encoding="utf-8")
        self.assertIn("ports/IRelationQueryService.hpp", factory_header)
        self.assertIn("ports/IRelationQueryServiceConfig.hpp", factory_header)
        self.assertIn("CreateRemoteRelationQueryService", factory_header)
        self.assertIn("SelectRelationQueryService", factory_header)
        self.assertIn("std::unique_ptr<IRelationQueryService>& remote_relation_query_service", factory_header)

        factory_source = factory_source_path.read_text(encoding="utf-8")
        self.assertIn('#include "RelationQueryGrpcClient.hpp"', factory_source)
        self.assertIn("std::make_unique<RelationQueryGrpcClient>", factory_source)
        self.assertIn("memochat::chat::factory::modules::IsInProcessBackend", factory_source)
        self.assertIn("memochat::chat::factory::modules::IsRemoteBackend", factory_source)
        self.assertIn("Relation query service remote endpoint is empty", factory_source)
        self.assertIn("chat.relation_query_service.unsupported_backend", factory_source)
        self.assertIn('{"fallback_backend", "inprocess"}', factory_source)
        self.assertNotIn("IRelationService", factory_source)

        cmake = read_cmake()
        self.assertIn("clients/RelationQueryServiceFactory.cpp", cmake_set_values(cmake, "CHATSERVER_CLIENT_SOURCES"))
        self.assertIn("config/RelationQueryServiceConfig.cpp", cmake_set_values(cmake, "CHATSERVER_CONFIG_SOURCES"))

        composition_header = runtime_composition_header()
        self.assertIn("class IRelationQueryService;", composition_header)
        self.assertIn("class IRelationQueryServiceConfig;", composition_header)
        self.assertIn("std::unique_ptr<IRelationQueryServiceConfig> _relation_query_service_config", composition_header)
        self.assertIn("std::unique_ptr<IRelationQueryService> _relation_query_service_remote", composition_header)
        self.assertNotIn(
            "std::unique_ptr<IRelationQueryServiceConfig> _relation_query_service_config", logic_header_source()
        )
        self.assertNotIn("std::unique_ptr<IRelationQueryService> _relation_query_service_remote", logic_header_source())

        composition_source = runtime_composition_source()
        self.assertIn('#include "RelationQueryServiceConfig.hpp"', composition_source)
        self.assertIn('#include "RelationQueryServiceFactory.hpp"', composition_source)
        self.assertIn(
            "_relation_query_service_config = std::make_unique<RelationQueryServiceConfig>()", composition_source
        )
        self.assertIn("SelectRelationQueryService(*_relation_query_service_config", composition_source)
        self.assertIn("_relation_query_service_remote", composition_source)
        self.assertIn("relation_query_service", composition_source)
        self.assertIn("relation_query_service,", composition_source)

        adapter_cmake = adapter_cmake_path.read_text(encoding="utf-8")
        self.assertIn("RelationQueryServiceFactoryTest.cpp", adapter_cmake)
        self.assertIn("chatserver_relation_query_service_factory_gtest", adapter_cmake)

        factory_test = factory_test_path.read_text(encoding="utf-8")
        self.assertIn("RelationQueryServiceFactoryTest", factory_test)
        self.assertIn("FakeRelationQueryServiceConfig", factory_test)

        for config_name in (
            "config.ini",
            "chatserver1.ini",
            "chatserver2.ini",
        ):
            config_source = (CHATSERVER_DIR / config_name).read_text(encoding="utf-8-sig")
            assert_ini_section_has(
                self,
                config_source,
                "RelationQueryService",
                "Backend=grpc",
                "Endpoint=127.0.0.1:50090",
            )
        worker_config_source = (CHATSERVER_DIR / "chatdeliveryworker1.ini").read_text(encoding="utf-8-sig")
        assert_ini_section_has(self, worker_config_source, "RelationQueryService", "Backend=inprocess")

    def test_relation_query_remote_runtime_smoke_probe_uses_production_factory_path(self):
        probe_path = (
            REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "RelationQueryRemoteSmokeTest.cpp"
        )
        adapter_cmake_path = REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "ChatServer" / "CMakeLists.txt"

        self.assertTrue(probe_path.is_file(), "relation query remote runtime smoke probe is missing")

        probe = probe_path.read_text(encoding="utf-8")
        for token in (
            "RelationQueryRemoteSmokeTest",
            "MEMOCHAT_RELATION_QUERY_SMOKE_CONFIG",
            "ConfigMgr::InitConfigPath",
            "RelationQueryServiceConfig",
            "SelectRelationQueryService",
            "RelationQueryGrpcClient",
            "AppendRelationBootstrapJson",
            "BuildDialogListJson",
            "relation_query_remote_error",
            "GTEST_SKIP",
        ):
            self.assertIn(token, probe)
        self.assertNotIn("ChatRelationService", probe)

        adapter_cmake = adapter_cmake_path.read_text(encoding="utf-8")
        self.assertIn("RelationQueryRemoteSmokeTest.cpp", adapter_cmake)
        self.assertIn("chatserver_relation_query_remote_smoke_gtest", adapter_cmake)
        self.assertIn("ChatClientCore", adapter_cmake)
        self.assertIn("ChatConfigCore", read_cmake())

    def test_message_services_have_ingress_facing_handler_contracts(self):
        message_command_port_path = CHATSERVER_DIR / "domain" / "ports" / "IMessageCommandService.hpp"
        private_port_path = CHATSERVER_DIR / "domain" / "ports" / "IPrivateMessageService.hpp"
        group_port_path = CHATSERVER_DIR / "domain" / "ports" / "IGroupMessageService.hpp"
        self.assertTrue(message_command_port_path.is_file(), "message command service contract header is missing")
        self.assertTrue(private_port_path.is_file(), "private message service contract header is missing")
        self.assertTrue(group_port_path.is_file(), "group message service contract header is missing")

        message_command_port = message_command_port_path.read_text(encoding="utf-8")
        self.assertIn("struct MessageCommandRequest", message_command_port)
        self.assertIn("short request_msg_id", message_command_port)
        self.assertIn("std::string payload_json", message_command_port)
        self.assertIn("struct MessageCommandResult", message_command_port)
        self.assertIn("short response_msg_id", message_command_port)
        self.assertIn("std::string payload_json", message_command_port)
        self.assertIn("class IPrivateMessageCommandService", message_command_port)
        self.assertIn("class IGroupMessageCommandService", message_command_port)
        self.assertRegex(
            message_command_port,
            r"virtual\s+MessageCommandResult\s+TextChatMessage\(const\s+MessageCommandRequest&\s+request\)",
        )
        self.assertRegex(
            message_command_port,
            r"virtual\s+MessageCommandResult\s+GroupChatMessage\(const\s+MessageCommandRequest&\s+request\)",
        )
        # BuildGroupListJson is the group query method; it lives on the command/query base, not the
        # transport interface (which carries only HandleXxx).
        self.assertRegex(
            message_command_port,
            r"virtual\s+void\s+BuildGroupListJson\(int\s+uid,\s+memochat::json::JsonValue&\s+out\)",
        )

        private_port = private_port_path.read_text(encoding="utf-8")
        self.assertIn("ports/IMessageCommandService.hpp", private_port)
        self.assertIn("class IPrivateMessageService", private_port)
        self.assertRegex(private_port, r"class\s+IPrivateMessageService\s*:\s*public\s+IPrivateMessageCommandService")
        self.assertIn("virtual ~IPrivateMessageService() = default", private_port)
        for method in (
            "HandleTextChatMessage",
            "HandleForwardPrivateMessage",
            "HandlePrivateReadAck",
            "HandleEditPrivateMessage",
            "HandleRevokePrivateMessage",
            "HandlePrivateHistory",
        ):
            self.assertRegex(private_port, rf"virtual\s+void\s+{re.escape(method)}")

        group_port = group_port_path.read_text(encoding="utf-8")
        self.assertIn("ports/IMessageCommandService.hpp", group_port)
        self.assertIn("class IGroupMessageService", group_port)
        self.assertRegex(group_port, r"class\s+IGroupMessageService\s*:\s*public\s+IGroupMessageCommandService")
        self.assertIn("virtual ~IGroupMessageService() = default", group_port)
        # BuildGroupListJson is a query method on the IGroupMessageCommandService base (asserted
        # below); the transport interface adds only the session-bound HandleXxx handlers.
        for method in (
            "HandleCreateGroup",
            "HandleGetGroupList",
            "HandleInviteGroupMember",
            "HandleApplyJoinGroup",
            "HandleReviewGroupApply",
            "HandleGroupChatMessage",
            "HandleGroupHistory",
            "HandleEditGroupMessage",
            "HandleRevokeGroupMessage",
            "HandleForwardGroupMessage",
            "HandleGroupReadAck",
            "HandleUpdateGroupAnnouncement",
            "HandleUpdateGroupIcon",
            "HandleSetGroupAdmin",
            "HandleMuteGroupMember",
            "HandleKickGroupMember",
            "HandleQuitGroup",
            "HandleDissolveGroup",
        ):
            self.assertRegex(group_port, rf"virtual\s+void\s+{re.escape(method)}")

        private_header = (DOMAIN_MESSAGE_DIR / "PrivateMessageService.hpp").read_text(encoding="utf-8")
        self.assertIn("ports/IPrivateMessageService.hpp", private_header)
        self.assertRegex(private_header, r"class\s+PrivateMessageService\s*:\s*public\s+IPrivateMessageService")
        self.assertIn(
            "MessageCommandResult TextChatMessage(const MessageCommandRequest& request) override", private_header
        )
        self.assertIn("HandleTextChatMessage(const std::shared_ptr<IChatSession>& session", private_header)
        self.assertNotIn("std::shared_ptr<CSession>", private_header)
        self.assertIn("override", private_header)

        group_header = (DOMAIN_MESSAGE_DIR / "GroupMessageService.hpp").read_text(encoding="utf-8")
        self.assertIn("ports/IGroupMessageService.hpp", group_header)
        self.assertRegex(group_header, r"class\s+GroupMessageService\s*:\s*public\s+IGroupMessageService")
        self.assertIn("BuildGroupListJson(int uid, memochat::json::JsonValue& out) override", group_header)
        self.assertIn(
            "MessageCommandResult GroupChatMessage(const MessageCommandRequest& request) override", group_header
        )
        self.assertIn("HandleGroupChatMessage(const std::shared_ptr<IChatSession>& session", group_header)
        self.assertNotIn("std::shared_ptr<CSession>", group_header)
        self.assertIn("override", group_header)

        private_source = (DOMAIN_MESSAGE_DIR / "PrivateMessageService.cpp").read_text(encoding="utf-8")
        private_command_block = text_between(
            private_source,
            "MessageCommandResult PrivateMessageService::TextChatMessage(const MessageCommandRequest& request)",
            "void PrivateMessageService::HandleTextChatMessage",
        )
        self.assertIn("_delivery_gateway->TryPushPayload({touid}, ID_NOTIFY_TEXT_CHAT_MSG_REQ", private_command_block)
        self.assertNotIn("route.session->Send", private_command_block)
        self.assertNotIn("ChatGrpcClient::GetInstance()->NotifyTextChatMsg", private_command_block)
        self.assertNotIn("const auto route = ResolveOnlineRoute", private_command_block)

        private_handler_block = text_between(
            private_source,
            "void PrivateMessageService::HandleTextChatMessage",
            "void PrivateMessageService::HandleForwardPrivateMessage",
        )
        self.assertIn("BuildMessageCommandRequestLocal(session, msg_id, msg_data)", private_handler_block)
        self.assertIn("SendMessageCommandResultLocal(session, TextChatMessage", private_handler_block)

        group_source = (DOMAIN_MESSAGE_DIR / "GroupMessageService.cpp").read_text(encoding="utf-8")
        group_handler_block = text_between(
            group_source,
            "void GroupMessageService::HandleGroupChatMessage",
            "void GroupMessageService::HandleGroupHistory",
        )
        self.assertIn("BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)", group_handler_block)
        self.assertIn("SendGroupMessageCommandResultLocal", group_handler_block)

        composition_header = runtime_composition_header()
        self.assertIn("class IPrivateMessageService;", composition_header)
        self.assertIn("class IGroupMessageService;", composition_header)
        self.assertIn("std::unique_ptr<IPrivateMessageService> _private_message_service", composition_header)
        self.assertIn("std::unique_ptr<IGroupMessageService> _group_message_service", composition_header)
        self.assertNotIn("std::unique_ptr<PrivateMessageService> _private_message_service", composition_header)
        self.assertNotIn("std::unique_ptr<GroupMessageService> _group_message_service", composition_header)
        self.assertNotIn("class PrivateMessageService;", composition_header)
        self.assertNotIn("class GroupMessageService;", composition_header)
        logic_header_text = logic_header_source()
        self.assertNotIn("std::unique_ptr<IPrivateMessageService> _private_message_service", logic_header_text)
        self.assertNotIn("std::unique_ptr<IGroupMessageService> _group_message_service", logic_header_text)
        self.assertNotIn("class PrivateMessageService;", logic_header_text)
        self.assertNotIn("class GroupMessageService;", logic_header_text)
        self.assertNotIn("friend class PrivateMessageService;", logic_header_text)
        self.assertNotIn("friend class GroupMessageService;", logic_header_text)

        registrar_source = (DOMAIN_ORCHESTRATION_DIR / "ChatHandlerRegistrars.cpp").read_text(encoding="utf-8")
        self.assertIn('#include "ports/IPrivateMessageService.hpp"', registrar_source)
        self.assertIn('#include "ports/IGroupMessageService.hpp"', registrar_source)
        self.assertNotIn('#include "PrivateMessageService.hpp"', registrar_source)
        self.assertNotIn('#include "GroupMessageService.hpp"', registrar_source)
        for method in (
            "HandleTextChatMessage",
            "HandleForwardPrivateMessage",
            "HandlePrivateReadAck",
            "HandleEditPrivateMessage",
            "HandleRevokePrivateMessage",
            "HandlePrivateHistory",
        ):
            self.assertIn(f"&IPrivateMessageService::{method}", registrar_source)
            self.assertNotIn(f"&PrivateMessageService::{method}", registrar_source)
        for method in (
            "HandleCreateGroup",
            "HandleGetGroupList",
            "HandleInviteGroupMember",
            "HandleApplyJoinGroup",
            "HandleReviewGroupApply",
            "HandleGroupChatMessage",
            "HandleGroupHistory",
            "HandleEditGroupMessage",
            "HandleRevokeGroupMessage",
            "HandleForwardGroupMessage",
            "HandleGroupReadAck",
            "HandleUpdateGroupAnnouncement",
            "HandleUpdateGroupIcon",
            "HandleSetGroupAdmin",
            "HandleMuteGroupMember",
            "HandleKickGroupMember",
            "HandleQuitGroup",
            "HandleDissolveGroup",
        ):
            self.assertIn(f"&IGroupMessageService::{method}", registrar_source)
            self.assertNotIn(f"&GroupMessageService::{method}", registrar_source)

    def test_message_command_grpc_adapters_enable_explicit_remote_backend_selection(self):
        service_header_path = DOMAIN_MESSAGE_DIR / "ChatMessageInternalGrpcService.hpp"
        service_source_path = DOMAIN_MESSAGE_DIR / "ChatMessageInternalGrpcService.cpp"
        client_header_path = CHATSERVER_DIR / "clients" / "MessageGrpcClient.hpp"
        client_source_path = CHATSERVER_DIR / "clients" / "MessageGrpcClient.cpp"
        adapter_header_path = CHATSERVER_DIR / "clients" / "MessageGrpcServiceAdapter.hpp"
        adapter_source_path = CHATSERVER_DIR / "clients" / "MessageGrpcServiceAdapter.cpp"

        for path, label in (
            (service_header_path, "message internal grpc service header"),
            (service_source_path, "message internal grpc service source"),
            (client_header_path, "message grpc client header"),
            (client_source_path, "message grpc client source"),
            (adapter_header_path, "message grpc service adapter header"),
            (adapter_source_path, "message grpc service adapter source"),
        ):
            self.assertTrue(path.is_file(), f"{label} is missing")

        service_header = service_header_path.read_text(encoding="utf-8")
        self.assertIn("ChatPrivateMessageInternalService::Service", service_header)
        self.assertIn("ChatGroupMessageInternalService::Service", service_header)
        self.assertIn("IPrivateMessageCommandService* private_message_service", service_header)
        self.assertIn("IGroupMessageCommandService* group_message_service", service_header)
        self.assertIn("SendTextChatMessage", service_header)
        self.assertIn("GroupChatMessage", service_header)
        self.assertNotIn("CSession", service_header)

        service_source = service_source_path.read_text(encoding="utf-8")
        self.assertIn("BuildCommandRequest", service_source)
        self.assertIn("&IPrivateMessageCommandService::TextChatMessage", service_source)
        self.assertIn("&IGroupMessageCommandService::GroupChatMessage", service_source)
        self.assertIn("response->set_tcp_msg_id(result.response_msg_id)", service_source)
        self.assertIn("response->set_payload_json(result.payload_json)", service_source)
        self.assertNotIn("session->Send", service_source)

        client_header = client_header_path.read_text(encoding="utf-8")
        self.assertIn("class MessageGrpcClient final", client_header)
        self.assertIn("public IPrivateMessageCommandService", client_header)
        self.assertIn("public IGroupMessageCommandService", client_header)
        self.assertIn("TextChatMessage", client_header)
        self.assertIn("GroupChatMessage", client_header)

        client_source = client_source_path.read_text(encoding="utf-8")
        self.assertIn("ChatPrivateMessageInternalService::NewStub", client_source)
        self.assertIn("ChatGroupMessageInternalService::NewStub", client_source)
        self.assertIn("_private_stub->SendTextChatMessage", client_source)
        self.assertIn("_group_stub->GroupChatMessage", client_source)
        self.assertIn("message_grpc_modules::RemoteErrorField()", client_source)

        adapter_header = adapter_header_path.read_text(encoding="utf-8")
        self.assertIn("class MessageGrpcServiceAdapter final", adapter_header)
        self.assertIn("public IPrivateMessageService", adapter_header)
        self.assertIn("public IGroupMessageService", adapter_header)
        self.assertIn("MessageGrpcClient _client", adapter_header)
        self.assertIn("TextChatMessage", adapter_header)
        self.assertIn("GroupChatMessage", adapter_header)
        self.assertIn("HandleTextChatMessage", adapter_header)
        self.assertIn("HandleGroupChatMessage", adapter_header)

        adapter_source = adapter_source_path.read_text(encoding="utf-8")
        self.assertIn("MessageGrpcServiceAdapter::TextChatMessage", adapter_source)
        self.assertIn("_client.TextChatMessage", adapter_source)
        self.assertIn("MessageGrpcServiceAdapter::GroupChatMessage", adapter_source)
        self.assertIn("_client.GroupChatMessage", adapter_source)
        self.assertIn("MessageGrpcServiceAdapter::BuildGroupListJson", adapter_source)
        self.assertIn("_client.BuildGroupListJson(uid, out)", adapter_source)
        self.assertIn("MessageGrpcServiceAdapter::CreateGroup", adapter_source)
        self.assertIn("_client.CreateGroup", adapter_source)
        self.assertIn("MessageGrpcServiceAdapter::DissolveGroup", adapter_source)
        self.assertIn("_client.DissolveGroup", adapter_source)
        self.assertNotIn("SendUnsupportedSessionCommand", adapter_source)
        self.assertNotIn("message remote method is not implemented yet", adapter_source)

        cmake = read_cmake()
        self.assertIn("clients/MessageGrpcClient.cpp", cmake_set_values(cmake, "CHATSERVER_CLIENT_SOURCES"))
        self.assertIn("clients/MessageGrpcServiceAdapter.cpp", cmake_set_values(cmake, "CHATSERVER_CLIENT_SOURCES"))
        self.assertIn(
            "domain/message/ChatMessageInternalGrpcService.cpp",
            cmake_set_values(cmake, "CHATSERVER_MESSAGE_SOURCES"),
        )

        factory_source = (DOMAIN_MESSAGE_DIR / "MessageServiceFactory.cpp").read_text(encoding="utf-8")
        self.assertIn('#include "MessageGrpcServiceAdapter.hpp"', factory_source)
        self.assertIn("std::make_unique<MessageGrpcServiceAdapter>(endpoint)", factory_source)
        self.assertIn("Message service remote endpoint is empty", factory_source)
        self.assertNotIn("chat.message_service.remote_backend_not_implemented", factory_source)
        self.assertNotIn("Message service remote backend is not implemented yet", factory_source)
        self.assertNotIn("ChatMessageInternalGrpcService", factory_source)

    def test_message_service_construction_is_hidden_behind_factory(self):
        factory_header_path = DOMAIN_MESSAGE_DIR / "MessageServiceFactory.hpp"
        factory_source_path = DOMAIN_MESSAGE_DIR / "MessageServiceFactory.cpp"
        self.assertTrue(factory_header_path.is_file(), "message service factory header is missing")
        self.assertTrue(factory_source_path.is_file(), "message service factory source is missing")

        factory_header = factory_header_path.read_text(encoding="utf-8")
        self.assertIn("ports/IPrivateMessageService.hpp", factory_header)
        self.assertIn("ports/IGroupMessageService.hpp", factory_header)
        self.assertIn("ports/IMessageRepository.hpp", factory_header)
        self.assertIn("ports/IMessageServiceConfig.hpp", factory_header)
        self.assertIn("ports/IOnlineRouteStore.hpp", factory_header)
        self.assertIn("ports/IDeliveryGateway.hpp", factory_header)
        self.assertIn("ports/IEventPublisher.hpp", factory_header)
        self.assertIn("ports/IRelationRepository.hpp", factory_header)
        self.assertIn("ports/ISessionRegistry.hpp", factory_header)
        self.assertIn("std::unique_ptr<IPrivateMessageService> CreateInProcessPrivateMessageService", factory_header)
        self.assertIn("std::unique_ptr<IGroupMessageService> CreateInProcessGroupMessageService", factory_header)
        self.assertIn("std::unique_ptr<IPrivateMessageService> CreatePrivateMessageService", factory_header)
        self.assertIn("std::unique_ptr<IGroupMessageService> CreateGroupMessageService", factory_header)
        self.assertIn("const IMessageServiceConfig& message_service_config", factory_header)
        self.assertNotIn("LogicSystem& logic", factory_header)
        self.assertIn("IMessageRepository* message_repository", factory_header)
        self.assertIn("IRelationRepository* relation_repository", factory_header)
        self.assertIn("IDeliveryGateway* delivery_gateway", factory_header)
        self.assertIn("IEventPublisher* event_publisher", factory_header)

        factory_source = factory_source_path.read_text(encoding="utf-8")
        self.assertIn('#include "PrivateMessageService.hpp"', factory_source)
        self.assertIn('#include "GroupMessageService.hpp"', factory_source)
        self.assertIn('#include "logging/Logger.hpp"', factory_source)
        self.assertIn("<stdexcept>", factory_source)
        self.assertIn("std::make_unique<PrivateMessageService>", factory_source)
        self.assertIn("IRelationRepository* relation_repository", factory_source)
        self.assertIn("relation_repository,", factory_source)
        self.assertIn("std::make_unique<GroupMessageService>", factory_source)
        self.assertIn("memochat::chat::factory::modules::IsInProcessBackend", factory_source)
        self.assertIn("memochat::chat::factory::modules::IsRemoteBackend", factory_source)
        self.assertIn("message_service_config.MessageServiceEndpoint()", factory_source)
        self.assertIn("std::make_unique<MessageGrpcServiceAdapter>(endpoint)", factory_source)
        self.assertIn("Message service remote endpoint is empty", factory_source)
        self.assertNotIn("chat.message_service.remote_backend_not_implemented", factory_source)
        self.assertNotIn("Message service remote backend is not implemented yet", factory_source)
        self.assertIn("chat.message_service.unsupported_backend", factory_source)
        self.assertIn('{"fallback_backend", "inprocess"}', factory_source)
        self.assertNotIn('#include "LogicSystem.hpp"', factory_source)

        cmake = read_cmake()
        self.assertIn("domain/message/MessageServiceFactory.cpp", cmake_set_values(cmake, "CHATSERVER_MESSAGE_SOURCES"))
        self.assertIn(
            "domain/message/GroupManagementService.cpp", cmake_set_values(cmake, "CHATSERVER_MESSAGE_SOURCES")
        )
        self.assertIn(
            "domain/message/GroupMembershipService.cpp", cmake_set_values(cmake, "CHATSERVER_MESSAGE_SOURCES")
        )
        self.assertIn(
            "domain/message/GroupMessageHistoryService.cpp", cmake_set_values(cmake, "CHATSERVER_MESSAGE_SOURCES")
        )
        self.assertIn("domain/message/GroupMessageWorkflow.cpp", cmake_set_values(cmake, "CHATSERVER_MESSAGE_SOURCES"))

        composition_source = runtime_composition_source()
        self.assertIn('#include "MessageServiceFactory.hpp"', composition_source)
        self.assertNotIn('#include "PrivateMessageService.hpp"', composition_source)
        self.assertNotIn('#include "GroupMessageService.hpp"', composition_source)
        self.assertIn("CreatePrivateMessageService(*_message_service_config", composition_source)
        self.assertIn("CreateGroupMessageService(*_message_service_config", composition_source)
        self.assertIn("_message_delivery_service.get()", composition_source)
        self.assertIn("_async_event_dispatcher.get()", composition_source)
        before_handler_boundary = composition_source.split("CHATSERVER_LEGACY_LOGICSYSTEM_HANDLERS_BEGIN", 1)[0]
        self.assertNotIn("_private_message_service = CreateInProcessPrivateMessageService(", before_handler_boundary)
        self.assertNotIn("_group_message_service = CreateInProcessGroupMessageService(", before_handler_boundary)
        self.assertNotIn("std::make_unique<PrivateMessageService>", composition_source)
        self.assertNotIn("std::make_unique<GroupMessageService>", composition_source)

    def test_message_services_use_delivery_and_event_ports_instead_of_logicsystem(self):
        private_header = (DOMAIN_MESSAGE_DIR / "PrivateMessageService.hpp").read_text(encoding="utf-8")
        group_header = (DOMAIN_MESSAGE_DIR / "GroupMessageService.hpp").read_text(encoding="utf-8")
        for header in (private_header, group_header):
            self.assertIn("ports/IDeliveryGateway.hpp", header)
            self.assertIn("ports/IEventPublisher.hpp", header)
            self.assertIn("IDeliveryGateway* delivery_gateway", header)
            self.assertIn("IEventPublisher* event_publisher", header)
            self.assertIn("IDeliveryGateway* _delivery_gateway", header)
            self.assertIn("IEventPublisher* _event_publisher", header)
            self.assertNotIn("LogicSystem&", header)
            self.assertNotIn("_logic", header)

        private_source = (DOMAIN_MESSAGE_DIR / "PrivateMessageService.cpp").read_text(encoding="utf-8")
        group_source = (DOMAIN_MESSAGE_DIR / "GroupMessageService.cpp").read_text(encoding="utf-8")
        group_workflow_source = (DOMAIN_MESSAGE_DIR / "GroupMessageWorkflow.cpp").read_text(encoding="utf-8")
        group_history_source = (DOMAIN_MESSAGE_DIR / "GroupMessageHistoryService.cpp").read_text(encoding="utf-8")
        for source in (private_source, group_source, group_workflow_source, group_history_source):
            self.assertNotIn('#include "LogicSystem.hpp"', source)
            self.assertNotIn('#include "MessageDeliveryService.hpp"', source)
            self.assertNotIn("_logic.", source)
            self.assertNotIn("MessageDelivery().PushPayload", source)
            self.assertNotIn("PublishAsyncEvent", source)
        self.assertIn("_delivery_gateway->PushPayload", private_source)
        self.assertIn("_event_publisher->PublishEvent", private_source)
        self.assertIn("_delivery_gateway->PushPayload", group_workflow_source)
        self.assertIn("_event_publisher->PublishEvent", group_workflow_source)
        self.assertIn("_delivery_gateway->PushPayload", group_history_source)
        self.assertIn("_workflow_service->GroupChatMessage(request)", group_source)
        self.assertIn("_history_service->GroupHistory(request)", group_source)

        composition_source = runtime_composition_source()
        service_wiring_block = text_between(
            composition_source,
            "_message_delivery_service =",
            "_chat_relation_service = CreateRelationService",
        )
        self.assertIn("_async_event_dispatcher = std::make_unique<AsyncEventDispatcher>", service_wiring_block)
        self.assertIn("_private_message_service = CreatePrivateMessageService", service_wiring_block)
        self.assertIn("_group_message_service = CreateGroupMessageService", service_wiring_block)
        self.assertIn("_message_delivery_service.get()", service_wiring_block)
        self.assertIn("_async_event_dispatcher.get()", service_wiring_block)

    def test_message_service_code_only_uses_allowed_repositories_and_ports(self):
        checked_paths = (
            DOMAIN_MESSAGE_DIR / "PrivateMessageService.hpp",
            DOMAIN_MESSAGE_DIR / "PrivateMessageService.cpp",
            DOMAIN_MESSAGE_DIR / "GroupMessageService.hpp",
            DOMAIN_MESSAGE_DIR / "GroupMessageService.cpp",
            DOMAIN_MESSAGE_DIR / "GroupManagementService.hpp",
            DOMAIN_MESSAGE_DIR / "GroupManagementService.cpp",
            DOMAIN_MESSAGE_DIR / "GroupMembershipService.hpp",
            DOMAIN_MESSAGE_DIR / "GroupMembershipService.cpp",
            DOMAIN_MESSAGE_DIR / "GroupMessageHistoryService.hpp",
            DOMAIN_MESSAGE_DIR / "GroupMessageHistoryService.cpp",
            DOMAIN_MESSAGE_DIR / "GroupMessageWorkflow.hpp",
            DOMAIN_MESSAGE_DIR / "GroupMessageWorkflow.cpp",
        )
        forbidden_tokens = (
            '#include "LogicSystem.hpp"',
            '#include "PostgresMgr.hpp"',
            '#include "MongoMgr.hpp"',
            '#include "RedisMgr.hpp"',
            '#include "UserMgr.hpp"',
            '#include "ConfigMgr.hpp"',
            '#include "ChatMessageRepository.hpp"',
            '#include "ChatRelationRepository.hpp"',
            '#include "MessageDeliveryService.hpp"',
            '#include "AsyncEventDispatcher.hpp"',
            "LogicSystem&",
            "_logic",
            "PostgresMgr::GetInstance()",
            "MongoMgr::GetInstance()",
            "RedisMgr::GetInstance()",
            "UserMgr::GetInstance()",
            "ConfigMgr::Inst()",
            "ChatMessageRepository",
            "ChatRelationRepository",
            "MessageDeliveryService",
            "AsyncEventDispatcher",
        )
        violations = []
        for path in checked_paths:
            source = path.read_text(encoding="utf-8")
            for token in forbidden_tokens:
                if token in source:
                    violations.append(f"{path.relative_to(CHATSERVER_DIR)} contains {token}")
        self.assertEqual([], violations)

        private_header = (DOMAIN_MESSAGE_DIR / "PrivateMessageService.hpp").read_text(encoding="utf-8")
        private_source = (DOMAIN_MESSAGE_DIR / "PrivateMessageService.cpp").read_text(encoding="utf-8")
        group_header = (DOMAIN_MESSAGE_DIR / "GroupMessageService.hpp").read_text(encoding="utf-8")
        group_source = (DOMAIN_MESSAGE_DIR / "GroupMessageService.cpp").read_text(encoding="utf-8")
        group_management_header = (DOMAIN_MESSAGE_DIR / "GroupManagementService.hpp").read_text(encoding="utf-8")
        group_management_source = (DOMAIN_MESSAGE_DIR / "GroupManagementService.cpp").read_text(encoding="utf-8")
        group_membership_header = (DOMAIN_MESSAGE_DIR / "GroupMembershipService.hpp").read_text(encoding="utf-8")
        group_membership_source = (DOMAIN_MESSAGE_DIR / "GroupMembershipService.cpp").read_text(encoding="utf-8")
        group_history_header = (DOMAIN_MESSAGE_DIR / "GroupMessageHistoryService.hpp").read_text(encoding="utf-8")
        group_history_source = (DOMAIN_MESSAGE_DIR / "GroupMessageHistoryService.cpp").read_text(encoding="utf-8")
        group_workflow_header = (DOMAIN_MESSAGE_DIR / "GroupMessageWorkflow.hpp").read_text(encoding="utf-8")
        group_workflow_source = (DOMAIN_MESSAGE_DIR / "GroupMessageWorkflow.cpp").read_text(encoding="utf-8")

        for token in (
            "ports/IMessageRepository.hpp",
            "ports/ISessionRegistry.hpp",
            "ports/IOnlineRouteStore.hpp",
            "ports/IDeliveryGateway.hpp",
            "ports/IEventPublisher.hpp",
        ):
            self.assertIn(token, private_header)
        for token in (
            "_message_repository->",
            "_session_registry",
            "_online_route_store",
            "_delivery_gateway->PushPayload",
            "_event_publisher->PublishEvent",
        ):
            self.assertIn(token, private_source)

        for token in (
            "ports/IMessageRepository.hpp",
            "ports/IRelationRepository.hpp",
            "ports/IDeliveryGateway.hpp",
            "ports/IEventPublisher.hpp",
        ):
            self.assertIn(token, group_header)
        self.assertIn("class GroupManagementService;", group_header)
        self.assertIn("class GroupMembershipService;", group_header)
        self.assertIn("class GroupMessageHistoryService;", group_header)
        self.assertIn("class GroupMessageWorkflow;", group_header)
        self.assertIn("std::unique_ptr<GroupManagementService> _management_service", group_header)
        self.assertIn("std::unique_ptr<GroupMembershipService> _membership_service", group_header)
        self.assertIn("std::unique_ptr<GroupMessageHistoryService> _history_service", group_header)
        self.assertIn("std::unique_ptr<GroupMessageWorkflow> _workflow_service", group_header)
        self.assertIn("IRelationRepository* relation_repository", group_management_header)
        self.assertIn("IDeliveryGateway* delivery_gateway", group_management_header)
        self.assertIn("IRelationRepository* relation_repository", group_membership_header)
        self.assertIn("IDeliveryGateway* delivery_gateway", group_membership_header)
        self.assertIn("IMessageRepository* message_repository", group_history_header)
        self.assertIn("IRelationRepository* relation_repository", group_history_header)
        self.assertIn("IDeliveryGateway* delivery_gateway", group_history_header)
        self.assertIn("IMessageRepository* message_repository", group_workflow_header)
        self.assertIn("IRelationRepository* relation_repository", group_workflow_header)
        self.assertIn("IDeliveryGateway* delivery_gateway", group_workflow_header)
        self.assertIn("IEventPublisher* event_publisher", group_workflow_header)
        for token in (
            "_membership_service->",
            "_workflow_service->",
            "_history_service->",
            "_management_service->",
        ):
            self.assertIn(token, group_source)
        for token in (
            "_message_repository->",
            "_relation_repository->",
            "_delivery_gateway->PushPayload",
            "_event_publisher->PublishEvent",
        ):
            self.assertIn(token, group_workflow_source)
        for token in (
            "_message_repository->",
            "_relation_repository->",
            "_delivery_gateway->PushPayload",
        ):
            self.assertIn(token, group_history_source)
        for token in (
            "_relation_repository->",
            "_delivery_gateway->PushPayload",
            "UpdateGroupAnnouncement",
            "UpdateGroupIcon",
            "SetGroupAdmin",
            "MuteGroupMember",
            "KickGroupMember",
            "QuitGroup",
            "DissolveGroup",
        ):
            self.assertIn(token, group_management_source)
        for token in (
            "_relation_repository->",
            "_delivery_gateway->PushPayload",
            "BuildGroupListJson",
            "CreateGroup",
            "GetGroupList",
            "InviteGroupMember",
            "ApplyJoinGroup",
            "ReviewGroupApply",
        ):
            self.assertIn(token, group_membership_source)

        self.assertIn('#include "ChatGrpcClient.hpp"', private_source)
        self.assertIn("ChatGrpcClient::GetInstance()->NotifyTextChatMsg", private_source)
        self.assertNotIn("ChatGrpcClient", group_source)

    def test_chatserver_service_data_ownership_matrix(self):
        relation_paths = (
            DOMAIN_RELATION_DIR / "ChatRelationService.hpp",
            DOMAIN_RELATION_DIR / "ChatRelationService.cpp",
        )
        message_paths = (
            DOMAIN_MESSAGE_DIR / "PrivateMessageService.hpp",
            DOMAIN_MESSAGE_DIR / "PrivateMessageService.cpp",
            DOMAIN_MESSAGE_DIR / "GroupMessageService.hpp",
            DOMAIN_MESSAGE_DIR / "GroupMessageService.cpp",
            DOMAIN_MESSAGE_DIR / "GroupManagementService.hpp",
            DOMAIN_MESSAGE_DIR / "GroupManagementService.cpp",
            DOMAIN_MESSAGE_DIR / "GroupMembershipService.hpp",
            DOMAIN_MESSAGE_DIR / "GroupMembershipService.cpp",
            DOMAIN_MESSAGE_DIR / "GroupMessageHistoryService.hpp",
            DOMAIN_MESSAGE_DIR / "GroupMessageHistoryService.cpp",
            DOMAIN_MESSAGE_DIR / "GroupMessageWorkflow.hpp",
            DOMAIN_MESSAGE_DIR / "GroupMessageWorkflow.cpp",
        )
        delivery_paths = (
            DOMAIN_MESSAGE_DIR / "MessageDeliveryService.hpp",
            DOMAIN_MESSAGE_DIR / "MessageDeliveryService.cpp",
            DOMAIN_DELIVERY_DIR / "TaskDispatcher.hpp",
            DOMAIN_DELIVERY_DIR / "TaskDispatcher.cpp",
            DOMAIN_DELIVERY_DIR / "ChatDeliveryRuntime.hpp",
            DOMAIN_DELIVERY_DIR / "ChatDeliveryRuntime.cpp",
        )
        async_paths = (
            CHATSERVER_DIR / "messaging" / "AsyncEventDispatcher.hpp",
            CHATSERVER_DIR / "messaging" / "AsyncEventDispatcher.cpp",
        )

        def assert_no_tokens(paths, tokens):
            violations = []
            for path in paths:
                source = path.read_text(encoding="utf-8")
                for token in tokens:
                    if token in source:
                        violations.append(f"{path.relative_to(CHATSERVER_DIR)} contains {token}")
            self.assertEqual([], violations)

        concrete_manager_tokens = (
            '#include "PostgresMgr.hpp"',
            '#include "MongoMgr.hpp"',
            '#include "RedisMgr.hpp"',
            '#include "ConfigMgr.hpp"',
            '#include "UserMgr.hpp"',
            "PostgresMgr::GetInstance()",
            "MongoMgr::GetInstance()",
            "RedisMgr::GetInstance()",
            "ConfigMgr::Inst()",
            "UserMgr::GetInstance()",
        )
        concrete_repo_tokens = (
            '#include "ChatMessageRepository.hpp"',
            '#include "ChatRelationRepository.hpp"',
            "ChatMessageRepository",
            "ChatRelationRepository",
        )
        message_persistence_tokens = (
            "SavePrivateMessage",
            "SaveGroupMessage",
            "FindPrivateMessageByClientId",
            "FindGroupMessageByClientId",
            "UpdatePrivateMessageContent",
            "RevokePrivateMessage",
            "UpdateGroupMessageContent",
            "RevokeGroupMessage",
            "GetPrivateHistory",
            "GetGroupHistory",
            "UpsertPrivateReadState",
            "UpsertGroupReadState",
        )
        relation_mutation_tokens = (
            "AddFriendApply",
            "AuthFriendApply",
            "AddFriend",
            "DeleteFriend",
            "CreateGroup",
            "InviteGroupMember",
            "ApplyJoinGroup",
            "ReviewGroupApply",
            "UpdateGroupAnnouncement",
            "UpdateGroupIcon",
            "SetGroupAdmin",
            "MuteGroupMember",
            "KickGroupMember",
            "QuitGroup",
            "DissolveGroup",
            "UpsertDialogDraft",
            "UpsertDialogMuteState",
            "UpsertDialogPinned",
        )

        assert_no_tokens(relation_paths, concrete_manager_tokens + concrete_repo_tokens + message_persistence_tokens)
        assert_no_tokens(message_paths, concrete_manager_tokens + concrete_repo_tokens + ("LogicSystem&", "_logic"))
        assert_no_tokens(delivery_paths, concrete_manager_tokens + concrete_repo_tokens + message_persistence_tokens)
        assert_no_tokens(delivery_paths, relation_mutation_tokens)
        assert_no_tokens(async_paths, concrete_manager_tokens + concrete_repo_tokens)

        relation_source = "\n".join(path.read_text(encoding="utf-8") for path in relation_paths)
        for token in (
            "IRelationRepository",
            "IRelationBootstrapCache",
            "IDeliveryGateway",
            "IDeliveryTaskPublisher",
            "IEventPublisher",
        ):
            self.assertIn(token, relation_source)

        message_source = "\n".join(path.read_text(encoding="utf-8") for path in message_paths)
        for token in ("IMessageRepository", "IDeliveryGateway", "IEventPublisher"):
            self.assertIn(token, message_source)

        delivery_source = "\n".join(path.read_text(encoding="utf-8") for path in delivery_paths)
        for token in ("IDeliveryGateway", "IDeliveryTaskPublisher", "IOnlineRouteStore", "ISessionRegistry"):
            self.assertIn(token, delivery_source)

        async_source = "\n".join(path.read_text(encoding="utf-8") for path in async_paths)
        for token in (
            "IMessageRepository",
            "IRelationRepository",
            "ISessionRegistry",
            "IOnlineRouteStore",
            "IRelationBootstrapCache",
        ):
            self.assertIn(token, async_source)

    def test_message_service_backend_config_defaults_to_inprocess(self):
        config_port_path = CHATSERVER_DIR / "domain" / "ports" / "IMessageServiceConfig.hpp"
        config_adapter_header_path = CHATSERVER_DIR / "config" / "MessageServiceConfig.hpp"
        config_adapter_source_path = CHATSERVER_DIR / "config" / "MessageServiceConfig.cpp"
        for path, label in (
            (config_port_path, "message service config port header"),
            (config_adapter_header_path, "message service config adapter header"),
            (config_adapter_source_path, "message service config adapter source"),
        ):
            self.assertTrue(path.is_file(), f"{label} is missing")

        config_port = config_port_path.read_text(encoding="utf-8")
        self.assertIn("class IMessageServiceConfig", config_port)
        self.assertIn("virtual std::string MessageServiceBackend() const = 0", config_port)
        self.assertIn("virtual std::string MessageServiceEndpoint() const = 0", config_port)

        config_adapter_header = config_adapter_header_path.read_text(encoding="utf-8")
        self.assertIn("ports/IMessageServiceConfig.hpp", config_adapter_header)
        self.assertRegex(config_adapter_header, r"class\s+MessageServiceConfig\s*:\s*public\s+IMessageServiceConfig")
        self.assertIn("std::string MessageServiceEndpoint() const override", config_adapter_header)

        config_adapter_source = config_adapter_source_path.read_text(encoding="utf-8")
        self.assertIn('#include "ConfigMgr.hpp"', config_adapter_source)
        self.assertIn('GetValue("MessageService", "Backend")', config_adapter_source)
        self.assertIn('GetValue("MessageService", "Endpoint")', config_adapter_source)
        self.assertIn('return "inprocess"', config_adapter_source)
        self.assertIn("memochat::chat::config::modules::LowerAsciiInPlace", config_adapter_source)

        cmake = read_cmake()
        self.assertIn("config/MessageServiceConfig.cpp", cmake_set_values(cmake, "CHATSERVER_CONFIG_SOURCES"))

        composition_header = runtime_composition_header()
        self.assertIn("class IMessageServiceConfig;", composition_header)
        self.assertIn("std::unique_ptr<IMessageServiceConfig> _message_service_config", composition_header)
        self.assertNotIn("std::unique_ptr<IMessageServiceConfig> _message_service_config", logic_header_source())

        composition_source = runtime_composition_source()
        self.assertIn('#include "MessageServiceConfig.hpp"', composition_source)
        self.assertIn("_message_service_config = std::make_unique<MessageServiceConfig>()", composition_source)
        self.assertIn("CreatePrivateMessageService(*_message_service_config", composition_source)
        self.assertIn("CreateGroupMessageService(*_message_service_config", composition_source)

        for config_name in (
            "config.ini",
            "chatserver1.ini",
            "chatserver2.ini",
        ):
            config_source = (CHATSERVER_DIR / config_name).read_text(encoding="utf-8-sig")
            assert_ini_section_has(
                self,
                config_source,
                "MessageService",
                "Backend=grpc",
                "Endpoint=127.0.0.1:50092",
            )
        worker_config_source = (CHATSERVER_DIR / "chatdeliveryworker1.ini").read_text(encoding="utf-8-sig")
        assert_ini_section_has(self, worker_config_source, "MessageService", "Backend=inprocess")

    def test_remote_relation_and_message_backend_contract_proto_is_explicit_and_defaulted_for_chatserver(self):
        internal_proto_path = COMMON_PROTO_DIR / "chat_internal.proto"
        self.assertTrue(internal_proto_path.is_file(), "chat internal service proto is missing")

        internal_proto = internal_proto_path.read_text(encoding="utf-8")
        self.assertIn('syntax = "proto3";', internal_proto)
        self.assertIn("package chatinternal;", internal_proto)
        self.assertIn("message SessionContext", internal_proto)
        self.assertIn("message JsonPayloadRequest", internal_proto)
        self.assertIn("message JsonPayloadResponse", internal_proto)
        self.assertIn("message BootstrapRequest", internal_proto)
        self.assertIn("message BootstrapResponse", internal_proto)
        self.assertIn("message DeliveryCommand", internal_proto)
        self.assertIn("message EventCommand", internal_proto)
        self.assertIn("service ChatRelationInternalService", internal_proto)
        self.assertIn("rpc AppendRelationBootstrap", internal_proto)
        self.assertIn("rpc BuildDialogList", internal_proto)
        for method in (
            "SearchUser",
            "AddFriendApply",
            "AuthFriendApply",
            "DeleteFriend",
            "GetDialogList",
            "SyncDraft",
            "PinDialog",
        ):
            self.assertIn(f"rpc {method}", internal_proto)

        default_smoke = (
            REPO_ROOT / "tools" / "scripts" / "status" / "smoke_chatserver_default_grpc_runtime.sh"
        ).read_text(encoding="utf-8")
        for token in (
            "ChatServer default config",
            "ChatRelationQueryService",
            "ChatRelationServiceWorker",
            "ChatMessageService",
            "Endpoint=127.0.0.1:50090",
            "Endpoint=127.0.0.1:50091",
            "Endpoint=127.0.0.1:50092",
        ):
            self.assertIn(token, default_smoke)

        self.assertIn("service ChatPrivateMessageInternalService", internal_proto)
        for method in (
            "SendTextChatMessage",
            "ForwardPrivateMessage",
            "PrivateReadAck",
            "EditPrivateMessage",
            "RevokePrivateMessage",
            "PrivateHistory",
        ):
            self.assertIn(f"rpc {method}", internal_proto)

        self.assertIn("service ChatGroupMessageInternalService", internal_proto)
        self.assertIn("rpc BuildGroupList", internal_proto)
        for method in (
            "CreateGroup",
            "GetGroupList",
            "InviteGroupMember",
            "ApplyJoinGroup",
            "ReviewGroupApply",
            "GroupChatMessage",
            "GroupHistory",
            "EditGroupMessage",
            "RevokeGroupMessage",
            "ForwardGroupMessage",
            "GroupReadAck",
            "UpdateGroupAnnouncement",
            "UpdateGroupIcon",
            "SetGroupAdmin",
            "MuteGroupMember",
            "KickGroupMember",
            "QuitGroup",
            "DissolveGroup",
        ):
            self.assertIn(f"rpc {method}", internal_proto)

        cmake = (SERVER_CORE_DIR / "CMakeLists.txt").read_text(encoding="utf-8")
        for token in (
            "MEMOCHAT_PROTO_CHAT_INTERNAL_SRC",
            "common/proto/chat_internal.proto",
            "MEMOCHAT_PROTO_CHAT_INTERNAL_CC",
            "MEMOCHAT_PROTO_CHAT_INTERNAL_H",
            "MEMOCHAT_GRPC_CHAT_INTERNAL_CC",
            "MEMOCHAT_GRPC_CHAT_INTERNAL_H",
        ):
            self.assertIn(token, cmake)
        self.assertIn("${MEMOCHAT_PROTO_CHAT_INTERNAL_SRC}", cmake)
        self.assertIn("${MEMOCHAT_PROTO_CHAT_INTERNAL_CC}", cmake)
        self.assertIn("${MEMOCHAT_GRPC_CHAT_INTERNAL_CC}", cmake)

        chatserver_cmake = read_cmake()
        self.assertNotIn("add_executable(ChatRelationService\n", chatserver_cmake)

        relation_factory = (DOMAIN_RELATION_DIR / "RelationServiceFactory.cpp").read_text(encoding="utf-8")
        message_factory = (DOMAIN_MESSAGE_DIR / "MessageServiceFactory.cpp").read_text(encoding="utf-8")
        self.assertIn("memochat::chat::factory::modules::IsRemoteBackend", relation_factory)
        self.assertIn("std::make_unique<RelationGrpcServiceAdapter>", relation_factory)
        self.assertNotIn("remote_backend_not_implemented", relation_factory)
        self.assertIn("memochat::chat::factory::modules::IsRemoteBackend", message_factory)
        self.assertIn("std::make_unique<MessageGrpcServiceAdapter>", message_factory)
        self.assertNotIn("remote_backend_not_implemented", message_factory)

    def test_chat_internal_proto_has_cpp_contract_tests(self):
        proto_test_dir = REPO_ROOT / "tests" / "cpp" / "apps" / "server" / "core" / "common" / "proto"
        cmake_path = proto_test_dir / "CMakeLists.txt"
        test_path = proto_test_dir / "ChatInternalProtoTest.cpp"
        self.assertTrue(test_path.is_file(), "chat internal proto C++ contract test is missing")

        cmake = cmake_path.read_text(encoding="utf-8")
        self.assertIn("ChatInternalProtoTest.cpp", cmake)

        test_source = test_path.read_text(encoding="utf-8")
        self.assertIn('#include "common/proto/chat_internal.pb.h"', test_source)
        self.assertIn('#include "common/proto/chat_internal.grpc.pb.h"', test_source)
        for token in (
            "ChatInternalProtoTest",
            "JsonPayloadRequestRoundTripKeepsSessionContext",
            "JsonPayloadResponseCarriesDeliveryAndEventCommands",
            "BootstrapRoundTripKeepsJsonPayload",
            "GrpcDescriptorsExposeInternalServiceContracts",
            "ChatRelationInternalService",
            "ChatPrivateMessageInternalService",
            "ChatGroupMessageInternalService",
        ):
            self.assertIn(token, test_source)

    def test_chat_relation_service_does_not_access_concrete_manager_singletons(self):
        checked_paths = (DOMAIN_RELATION_DIR / "ChatRelationService.cpp",)
        forbidden_tokens = (
            '#include "PostgresMgr.hpp"',
            '#include "MongoMgr.hpp"',
            '#include "RedisMgr.hpp"',
            '#include "ConfigMgr.hpp"',
            '#include "UserMgr.hpp"',
            "PostgresMgr::GetInstance()",
            "MongoMgr::GetInstance()",
            "RedisMgr::GetInstance()",
            "ConfigMgr::Inst()",
            "UserMgr::GetInstance()",
        )
        violations = []
        for path in checked_paths:
            source = path.read_text(encoding="utf-8")
            for token in forbidden_tokens:
                if token in source:
                    violations.append(f"{path.relative_to(CHATSERVER_DIR)} contains {token}")
        self.assertEqual([], violations)

    def test_group_message_service_uses_relation_repository_for_group_management(self):
        port_source = (CHATSERVER_DIR / "domain" / "ports" / "IRelationRepository.hpp").read_text(encoding="utf-8")
        for method in (
            "GetUserByUid",
            "GetGroupById",
            "GetPendingGroupApplyForReviewer",
            "GetGroupIdByCode",
            "GetGroupMemberList",
            "CreateGroup",
            "InviteGroupMember",
            "ApplyJoinGroup",
            "ReviewGroupApply",
            "GetUserRoleInGroup",
            "UpdateGroupAnnouncement",
            "UpdateGroupIcon",
            "SetGroupAdmin",
            "MuteGroupMember",
            "KickGroupMember",
            "QuitGroup",
            "DissolveGroup",
        ):
            self.assertIn(method, port_source)

        adapter_header = (CHATSERVER_DIR / "persistence" / "ChatRelationRepository.hpp").read_text(encoding="utf-8")
        adapter_source = (CHATSERVER_DIR / "persistence" / "ChatRelationRepository.cpp").read_text(encoding="utf-8")
        for method in (
            "GetUserByUid",
            "GetGroupById",
            "GetPendingGroupApplyForReviewer",
            "GetGroupIdByCode",
            "GetGroupMemberList",
            "CreateGroup",
            "InviteGroupMember",
            "ApplyJoinGroup",
            "ReviewGroupApply",
            "GetUserRoleInGroup",
            "UpdateGroupAnnouncement",
            "UpdateGroupIcon",
            "SetGroupAdmin",
            "MuteGroupMember",
            "KickGroupMember",
            "QuitGroup",
            "DissolveGroup",
        ):
            self.assertIn(method, adapter_header)
            self.assertIn(f"ChatRelationRepository::{method}", adapter_source)

        group_header = (DOMAIN_MESSAGE_DIR / "GroupMessageService.hpp").read_text(encoding="utf-8")
        self.assertIn("ports/IRelationRepository.hpp", group_header)
        self.assertIn("IRelationRepository* relation_repository", group_header)
        self.assertIn("IRelationRepository* _relation_repository", group_header)
        self.assertIn("GroupManagementService", group_header)
        self.assertIn("GroupMembershipService", group_header)
        self.assertIn("GroupMessageHistoryService", group_header)
        self.assertIn("GroupMessageWorkflow", group_header)

        group_source = (DOMAIN_MESSAGE_DIR / "GroupMessageService.cpp").read_text(encoding="utf-8")
        group_management_source = (DOMAIN_MESSAGE_DIR / "GroupManagementService.cpp").read_text(encoding="utf-8")
        group_membership_source = (DOMAIN_MESSAGE_DIR / "GroupMembershipService.cpp").read_text(encoding="utf-8")
        group_history_source = (DOMAIN_MESSAGE_DIR / "GroupMessageHistoryService.cpp").read_text(encoding="utf-8")
        group_workflow_source = (DOMAIN_MESSAGE_DIR / "GroupMessageWorkflow.cpp").read_text(encoding="utf-8")
        self.assertNotIn('#include "PostgresMgr.hpp"', group_source)
        self.assertNotIn("PostgresMgr::GetInstance()", group_source)
        self.assertNotIn('#include "PostgresMgr.hpp"', group_management_source)
        self.assertNotIn("PostgresMgr::GetInstance()", group_management_source)
        self.assertNotIn('#include "PostgresMgr.hpp"', group_membership_source)
        self.assertNotIn("PostgresMgr::GetInstance()", group_membership_source)
        self.assertNotIn('#include "PostgresMgr.hpp"', group_history_source)
        self.assertNotIn("PostgresMgr::GetInstance()", group_history_source)
        self.assertNotIn('#include "PostgresMgr.hpp"', group_workflow_source)
        self.assertNotIn("PostgresMgr::GetInstance()", group_workflow_source)
        for method in (
            "GetUserGroupList",
            "GetPendingGroupApplyForReviewer",
            "GetGroupById",
            "GetUserByUid",
            "GetUidByUserId",
            "IsPrivateFriend",
            "CreateGroup",
            "InviteGroupMember",
            "GetGroupIdByCode",
            "ApplyJoinGroup",
            "GetGroupMemberList",
            "ReviewGroupApply",
            "GetUserRoleInGroup",
            "IsGroupMember",
        ):
            owner_source = (
                group_workflow_source if method in ("GetUserRoleInGroup", "IsGroupMember") else group_membership_source
            )
            self.assertRegex(owner_source, rf"_relation_repository\s*->\s*{re.escape(method)}")
        self.assertRegex(group_history_source, r"_relation_repository\s*->\s*IsGroupMember")
        self.assertRegex(group_history_source, r"_relation_repository\s*->\s*GetGroupMemberList")
        self.assertRegex(group_history_source, r"_relation_repository\s*->\s*GetGroupById")
        self.assertRegex(group_history_source, r"_relation_repository\s*->\s*GetUserByUid")
        for method in (
            "GroupChatMessage",
            "EditGroupMessage",
            "RevokeGroupMessage",
            "ForwardGroupMessage",
        ):
            self.assertIn(f"_workflow_service->{method}(request)", group_source)
        for method in ("GroupHistory", "GroupReadAck"):
            self.assertIn(f"_history_service->{method}(request)", group_source)
        for method in (
            "UpdateGroupAnnouncement",
            "UpdateGroupIcon",
            "SetGroupAdmin",
            "MuteGroupMember",
            "KickGroupMember",
            "QuitGroup",
            "DissolveGroup",
        ):
            self.assertRegex(group_management_source, rf"_relation_repository\s*->\s*{re.escape(method)}")
            self.assertIn(f"_management_service->{method}(request)", group_source)

        composition_source = runtime_composition_source()
        constructor_block = text_between(
            composition_source,
            "_group_message_service =",
            "_chat_relation_service = CreateRelationService",
        )
        self.assertIn("_message_repository.get()", constructor_block)
        self.assertIn("_relation_repository.get()", constructor_block)

    def test_logicsystem_tcp_dispatch_uses_service_registrars_without_legacy_handler_fallbacks(self):
        logic_source = (DOMAIN_ORCHESTRATION_DIR / "LogicSystem.cpp").read_text(encoding="utf-8")
        logic_header = (DOMAIN_ORCHESTRATION_DIR / "LogicSystem.hpp").read_text(encoding="utf-8")
        registrar_source = (DOMAIN_ORCHESTRATION_DIR / "ChatHandlerRegistrars.cpp").read_text(encoding="utf-8")

        composition_source = runtime_composition_source()
        self.assertIn("_chat_session_service = std::make_unique<ChatSessionService>", composition_source)
        self.assertIn("_chat_relation_service = CreateRelationService(*_relation_service_config", composition_source)
        self.assertIn("_private_message_service = CreatePrivateMessageService", composition_source)
        self.assertIn("*_message_service_config", composition_source)
        self.assertIn("_group_message_service =", composition_source)
        self.assertIn("_delivery_runtime->Start();", composition_source)
        self.assertIn("_composition = std::make_unique<ChatRuntimeComposition>(*this)", logic_source)
        self.assertNotIn("_chat_session_service = std::make_unique<ChatSessionService>", logic_source)

        for registrar in (
            "ChatSessionServiceRegistrar().Register(*_composition, _fun_callbacks)",
            "ChatRelationServiceRegistrar().Register(*_composition, _fun_callbacks)",
            "PrivateMessageServiceRegistrar().Register(*_composition, _fun_callbacks)",
            "GroupMessageServiceRegistrar().Register(*_composition, _fun_callbacks)",
            "AsyncEventDispatcherRegistrar().Register(*_composition, _fun_callbacks)",
        ):
            self.assertIn(registrar, logic_source)
        self.assertNotIn("Register(*this, _fun_callbacks)", logic_source)
        self.assertNotIn("logic._", registrar_source)
        self.assertIn("&composition.SessionService()", registrar_source)
        self.assertIn("&composition.RelationSessionService()", registrar_source)
        self.assertIn("&composition.PrivateMessageService()", registrar_source)
        self.assertIn("&composition.GroupMessageService()", registrar_source)

        for service_bind in (
            "&ChatSessionService::HandleLogin",
            "&ChatSessionService::HandleRelationBootstrap",
            "&ChatSessionService::HandleHeartbeat",
            "&IRelationSessionService::HandleSearchUser",
            "&IPrivateMessageService::HandleTextChatMessage",
            "&IGroupMessageService::HandleGroupChatMessage",
        ):
            self.assertIn(service_bind, registrar_source)

        legacy_handler_names = (
            "LoginHandler",
            "GetRelationBootstrapHandler",
            "SearchInfo",
            "AddFriendApply",
            "AuthFriendApply",
            "DealChatTextMsg",
            "HeartBeatHandler",
            "GetDialogListHandler",
            "SyncDraftHandler",
            "PinDialogHandler",
            "ForwardGroupMsgHandler",
            "ForwardPrivateMsgHandler",
            "GroupReadAckHandler",
            "PrivateReadAckHandler",
            "CreateGroupHandler",
            "GetGroupListHandler",
            "InviteGroupMemberHandler",
            "ApplyJoinGroupHandler",
            "ReviewGroupApplyHandler",
            "DealGroupChatMsg",
            "GroupHistoryHandler",
            "EditPrivateMsgHandler",
            "RevokePrivateMsgHandler",
            "EditGroupMsgHandler",
            "RevokeGroupMsgHandler",
            "PrivateHistoryHandler",
            "UpdateGroupAnnouncementHandler",
            "UpdateGroupIconHandler",
            "SetGroupAdminHandler",
            "MuteGroupMemberHandler",
            "KickGroupMemberHandler",
            "QuitGroupHandler",
            "DissolveGroupHandler",
        )
        for handler_name in legacy_handler_names:
            self.assertNotIn(f"LogicSystem::{handler_name}", logic_source)
            self.assertNotRegex(logic_header, rf"\b{re.escape(handler_name)}\s*\(")
            self.assertNotIn(f"&LogicSystem::{handler_name}", registrar_source)

        self.assertNotIn("CHATSERVER_LEGACY_LOGICSYSTEM_HANDLERS_BEGIN", logic_source)
        self.assertNotIn("CHATSERVER_LEGACY_LOGICSYSTEM_HANDLERS_END", logic_source)
        self.assertNotIn("CHATSERVER_LEGACY_LOGICSYSTEM_HELPERS_BEGIN", logic_source)
        self.assertNotIn("CHATSERVER_LEGACY_LOGICSYSTEM_HELPERS_END", logic_source)

        for helper_name in (
            "PushGroupPayload",
            "BuildGroupListJson",
            "BuildDialogListJson",
            "isPureDigit",
            "GetUserByUid",
            "GetUserByName",
            "GetBaseInfo",
            "GetFriendApplyInfo",
            "GetFriendList",
            "HandlePrivateAsyncEvent",
            "HandleGroupAsyncEvent",
            "NotifyMessageStatus",
        ):
            self.assertNotRegex(logic_header, rf"\b{re.escape(helper_name)}\s*\(")

        for stale_logic_method in (
            "LogicSystem::HandlePrivateAsyncEvent",
            "LogicSystem::HandleGroupAsyncEvent",
            "LogicSystem::NotifyMessageStatus",
        ):
            self.assertNotIn(stale_logic_method, logic_source)

        for stale_include in (
            '#include "StatusGrpcClient.hpp"',
            '#include "MongoMgr.hpp"',
            '#include "RedisMgr.hpp"',
            '#include "ChatGrpcClient.hpp"',
            '#include "DistLock.hpp"',
            '#include "cluster/ChatClusterDiscovery.hpp"',
            '#include "auth/ChatLoginTicket.hpp"',
        ):
            self.assertNotIn(stale_include, logic_source)

        self.assertIn("SetWorkerConfig", logic_source)
        self.assertIn("s_configured_worker_count", logic_source)
        self.assertIn("BindTcpTraceContext", logic_source)

    def test_runtime_role_controls_ingress_and_worker_lifecycle(self):
        runtime_source = (CHATSERVER_DIR / "app" / "ChatRuntime.cpp").read_text(encoding="utf-8")
        current_role_block = text_between(runtime_source, "ChatNodeRole CurrentRole()", "bool IsIngressEnabled()")
        self.assertIn('role == "ingress"', current_role_block)
        self.assertIn("return ChatNodeRole::Ingress", current_role_block)
        self.assertIn('role == "worker"', current_role_block)
        self.assertIn("return ChatNodeRole::Worker", current_role_block)
        self.assertIn('role == "hybrid"', current_role_block)
        self.assertIn("return ChatNodeRole::Hybrid", current_role_block)
        self.assertTrue(
            current_role_block.rstrip().endswith("return ChatNodeRole::Hybrid;\n}"),
            "empty or unsupported Runtime.Role should default to hybrid",
        )

        ingress_block = text_between(runtime_source, "bool IsIngressEnabled()", "bool IsWorkerEnabled()")
        self.assertIn("role == ChatNodeRole::Ingress || role == ChatNodeRole::Hybrid", ingress_block)
        worker_block = text_between(runtime_source, "bool IsWorkerEnabled()", "bool FeatureEnabled")
        self.assertIn("role == ChatNodeRole::Worker || role == ChatNodeRole::Hybrid", worker_block)

        app_source = (CHATSERVER_DIR / "app" / "ChatServer.cpp").read_text(encoding="utf-8")
        self.assertIn("const bool ingress_enabled = memochat::chatruntime::IsIngressEnabled();", app_source)
        self.assertIn(
            "if (ingress_enabled)\n        {\n            CleanupTrackedOnlineState(server_name);", app_source
        )
        self.assertIn(
            "if (ingress_enabled)\n                {\n                    CleanupTrackedOnlineState(server_name);",
            app_source,
        )
        self.assertIn("if (ingress_enabled)\n        {\n            ingress_coordinator =", app_source)

        composition_source = runtime_composition_source()
        self.assertIn(
            "if (_delivery_runtime && memochat::chatruntime::IsWorkerEnabled())\n    {\n        _delivery_runtime->Start();",
            composition_source,
        )
        self.assertIn("_composition->StartDeliveryRuntimeIfEnabled();", logic_source())
        self.assertNotIn("_delivery_runtime->Start();\n}", logic_source())

        for config_name in (
            "config.ini",
            "chatserver1.ini",
            "chatserver2.ini",
        ):
            config_source = (CHATSERVER_DIR / config_name).read_text(encoding="utf-8-sig")
            self.assertIn("[Runtime]", config_source)
            self.assertIn("Role=hybrid", config_source)


if __name__ == "__main__":
    unittest.main()
