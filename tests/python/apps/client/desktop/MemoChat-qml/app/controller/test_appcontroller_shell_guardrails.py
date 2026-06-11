import re
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CLIENT = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
APP = CLIENT / "app"
FEATURES = CLIENT / "features"
QML_ROOT = CLIENT / "qml"
APP_CONTROLLER_H = APP / "controller/AppController.h"
APP_CONTROLLER_CPP = APP / "controller/AppController.cpp"
APP_FEATURE_REGISTRY_H = APP / "composition/AppFeatureRegistry.h"
APP_PORT_REGISTRY_H = APP / "composition/AppPortRegistry.h"
APP_PORT_BINDER = APP / "composition/AppPortBinder.cpp"
APP_PORT_BINDER_FILES = (
    APP / "composition/AppPortBinder.cpp",
    APP / "composition/AppSessionPortBinder.cpp",
    APP / "composition/AppSessionAuthPortBinder.cpp",
    APP / "composition/AppPostLoginBootstrapPortBinder.cpp",
    APP / "composition/AppRelationBootstrapPortBinder.cpp",
    APP / "composition/AppRegisterCountdownPortBinder.cpp",
    APP / "composition/AppSessionLogoutPortBinder.cpp",
    APP / "composition/AppMediaPortBinder.cpp",
    APP / "composition/AppCallPortBinder.cpp",
    APP / "composition/AppFeaturePortBinder.cpp",
    APP / "composition/AppAuthFeaturePortBinder.cpp",
    APP / "composition/AppContactFeaturePortBinder.cpp",
    APP / "composition/AppGroupFeaturePortBinder.cpp",
    APP / "composition/AppProfileFeaturePortBinder.cpp",
    APP / "composition/AppConnectionPortBinder.cpp",
)
APP_FEATURE_PORT_BINDER = APP / "composition/AppFeaturePortBinder.cpp"
APP_GROUP_FEATURE_PORT_BINDER = APP / "composition/AppGroupFeaturePortBinder.cpp"
APP_SIGNAL_BINDER = APP / "composition/AppSignalBinder.cpp"
APP_SIGNAL_BINDER_FILES = (
    APP / "composition/AppSignalBinder.cpp",
    APP / "composition/AppHttpSignalBinder.cpp",
    APP / "composition/AppChatTransportSignalBinder.cpp",
    APP / "composition/AppChatDispatcherSignalBinder.cpp",
    APP / "composition/AppCallSignalBinder.cpp",
    APP / "composition/AppShellSignalBinder.cpp",
    APP / "composition/AppTimerSignalBinder.cpp",
)
APP_CONTROLLER_CHAT_BINDING = APP / "controller/AppControllerChatFeatureBinding.cpp"
APP_CHAT_BINDING_FILES = (
    APP / "controller/AppControllerChatFeatureBinding.cpp",
    APP / "controller/AppChatProjectionBinding.cpp",
    APP / "controller/AppChatSendBinding.cpp",
    APP / "controller/AppChatDialogBinding.cpp",
    APP / "controller/AppChatHistoryBinding.cpp",
    APP / "controller/AppChatGroupBinding.cpp",
    APP / "controller/AppChatMediaBinding.cpp",
    APP / "controller/AppChatReadMutationBinding.cpp",
)
APP_CONTROLLER_GROUP_COMMANDS = APP / "controller/AppControllerGroupCommands.cpp"
APP_COORDINATORS_H = APP / "coordinators/AppCoordinators.h"
APP_CONTROLLER_DIR = APP / "controller"
CHAT_EVENT_FACTORY_H = APP_CONTROLLER_DIR / "ChatEventDependenciesFactory.h"
CHAT_EVENT_FACTORY_CPP = APP_CONTROLLER_DIR / "ChatEventDependenciesFactory.cpp"
CONTACT_EVENT_FACTORY_H = APP_CONTROLLER_DIR / "ContactEventDependenciesFactory.h"
CONTACT_EVENT_FACTORY_CPP = APP_CONTROLLER_DIR / "ContactEventDependenciesFactory.cpp"
PRIVATE_HISTORY_FACTORY_H = APP_CONTROLLER_DIR / "PrivateHistoryDependenciesFactory.h"
PRIVATE_HISTORY_FACTORY_CPP = APP_CONTROLLER_DIR / "PrivateHistoryDependenciesFactory.cpp"
MEDIA_RUNNER_CPP = APP / "coordinators/MediaPendingAttachmentRunner.cpp"
MEDIA_RUNNER_H = APP / "coordinators/MediaPendingAttachmentRunner.h"
CHAT_FEATURE_SOURCES_CMAKE = FEATURES / "chat/sources.cmake"
CHAT_CONTROLLER_GTEST_CMAKE = (
    REPO_ROOT / "tests/cpp/apps/client/desktop/MemoChat-qml/features/chat/controller/CMakeLists.txt"
)
SESSION_DIR = APP / "session"
COORDINATORS_DIR = APP / "coordinators"

CPP_SUFFIXES = {".cpp", ".h", ".hpp", ".cxx", ".cc"}
CPP_IMPLEMENTATION_SUFFIXES = {".cpp", ".cxx", ".cc"}
QML_SUFFIXES = {".qml", ".js"}


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def normalized(source: str) -> str:
    return " ".join(source.split())


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


def qml_cmake_text() -> str:
    manifests = (
        CLIENT / "CMakeLists.txt",
        CLIENT / "app/sources.cmake",
        CLIENT / "features/sources.cmake",
        CLIENT / "features/contact/sources.cmake",
        CLIENT / "features/group/sources.cmake",
        CLIENT / "features/profile/sources.cmake",
    )
    return "\n".join(read(path) for path in manifests if path.exists())


def composition_port_binder_text() -> str:
    return "\n".join(read(path) for path in APP_PORT_BINDER_FILES)


def app_chat_binding_text() -> str:
    return "\n".join(read(path) for path in APP_CHAT_BINDING_FILES)


def app_signal_binding_text() -> str:
    return "\n".join(read(path) for path in APP_SIGNAL_BINDER_FILES)


def app_controller_sources_with(*tokens: str) -> set[str]:
    hits: set[str] = set()
    for pattern in ("AppController*.cpp", "AppChat*Binding.cpp"):
        for path in sorted(APP_CONTROLLER_DIR.glob(pattern)):
            source = read(path)
            if any(token in source for token in tokens):
                hits.add(path.name)
    return hits


def app_controller_cpp_sources_text() -> str:
    sources = []
    for pattern in ("AppController*.cpp", "AppChat*Binding.cpp"):
        for path in sorted(APP_CONTROLLER_DIR.glob(pattern)):
            sources.append(read(path))
    return "\n".join(sources)


def app_controller_binding_orchestrator() -> str:
    return read(APP_CONTROLLER_CHAT_BINDING)


def app_chat_binding_file_by_name(name: str) -> str:
    return read(APP_CONTROLLER_DIR / name)


def chat_dialog_runtime_source() -> str:
    controller_dir = FEATURES / "chat/controller"
    files = (
        "ChatFeatureControllerDialogRuntimeInternal.h",
        "ChatFeatureControllerDialogRuntime.cpp",
        "ChatFeatureControllerDialogDecorations.cpp",
        "ChatFeatureControllerDialogDraftCommands.cpp",
        "ChatFeatureControllerDialogReadCommands.cpp",
        "ChatFeatureControllerDialogMeta.cpp",
        "ChatFeatureControllerPrivateConversation.cpp",
    )
    return "\n".join(read(controller_dir / name) for name in files)


def group_conversation_service_source() -> str:
    service_dir = FEATURES / "chat/services"
    files = (
        "GroupConversationServiceInternal.h",
        "GroupConversationService.cpp",
        "GroupConversationAckService.cpp",
        "GroupConversationHistoryService.cpp",
        "GroupConversationIncomingService.cpp",
        "GroupConversationMutationService.cpp",
    )
    return "\n".join(read(service_dir / name) for name in files)


def private_chat_event_service_source() -> str:
    service_dir = FEATURES / "chat/services"
    files = (
        "PrivateChatEventServiceInternal.h",
        "PrivateChatEventService.cpp",
        "PrivateChatReadStatusService.cpp",
        "PrivateChatResponseService.cpp",
    )
    return "\n".join(read(service_dir / name) for name in files)


def _legacy_app_controller_sources_with(*tokens: str) -> set[str]:
    hits: set[str] = set()
    for path in sorted(APP_CONTROLLER_DIR.glob("AppController*.cpp")):
        source = read(path)
        if any(token in source for token in tokens):
            hits.add(path.name)
    return hits


def app_reset_owner_sources_with(*tokens: str) -> set[str]:
    hits: set[str] = set()
    roots = (APP_CONTROLLER_DIR, APP / "composition", SESSION_DIR, COORDINATORS_DIR)
    for root in roots:
        for path in sorted(root.rglob("*")):
            if path.suffix not in CPP_IMPLEMENTATION_SUFFIXES:
                continue
            source = read(path)
            if any(token in source for token in tokens):
                hits.add(path.relative_to(APP).as_posix())
    return hits


class AppControllerShellGuardrailTests(unittest.TestCase):
    def test_chat_dialog_runtime_shards_are_registered_in_product_and_gtest_cmake(self):
        feature_cmake = read(CHAT_FEATURE_SOURCES_CMAKE)
        gtest_cmake = read(CHAT_CONTROLLER_GTEST_CMAKE)
        runtime_shards = (
            "features/chat/controller/ChatFeatureControllerDialogDecorations.cpp",
            "features/chat/controller/ChatFeatureControllerDialogDraftCommands.cpp",
            "features/chat/controller/ChatFeatureControllerDialogList.cpp",
            "features/chat/controller/ChatFeatureControllerDialogMeta.cpp",
            "features/chat/controller/ChatFeatureControllerDialogReadCommands.cpp",
            "features/chat/controller/ChatFeatureControllerDialogRuntime.cpp",
            "features/chat/controller/ChatFeatureControllerPrivateConversation.cpp",
        )
        for shard in runtime_shards:
            with self.subTest(shard=shard):
                self.assertIn(shard, feature_cmake)
                self.assertIn(
                    "${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/" + shard,
                    gtest_cmake,
                )

    def test_group_conversation_shards_are_registered_in_product_and_gtest_cmake(self):
        feature_cmake = read(CHAT_FEATURE_SOURCES_CMAKE)
        gtest_cmake = read(CHAT_CONTROLLER_GTEST_CMAKE)
        service_shards = (
            "features/chat/services/GroupConversationAckService.cpp",
            "features/chat/services/GroupConversationHistoryService.cpp",
            "features/chat/services/GroupConversationIncomingService.cpp",
            "features/chat/services/GroupConversationMutationService.cpp",
            "features/chat/services/GroupConversationService.cpp",
        )
        for shard in service_shards:
            with self.subTest(shard=shard):
                self.assertIn(shard, feature_cmake)
                self.assertIn(
                    "${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/" + shard,
                    gtest_cmake,
                )

    def test_private_chat_event_shards_are_registered_in_product_and_gtest_cmake(self):
        feature_cmake = read(CHAT_FEATURE_SOURCES_CMAKE)
        controller_gtest_cmake = read(CHAT_CONTROLLER_GTEST_CMAKE)
        service_gtest_cmake = read(
            REPO_ROOT / "tests/cpp/apps/client/desktop/MemoChat-qml/features/chat/services/CMakeLists.txt"
        )
        service_shards = (
            "features/chat/services/PrivateChatEventService.cpp",
            "features/chat/services/PrivateChatReadStatusService.cpp",
            "features/chat/services/PrivateChatResponseService.cpp",
        )
        for shard in service_shards:
            with self.subTest(shard=shard):
                self.assertIn(shard, feature_cmake)
                self.assertIn(
                    "${MEMOCHAT_REPO_ROOT}/apps/client/desktop/MemoChat-qml/" + shard,
                    controller_gtest_cmake,
                )
                self.assertIn(
                    "${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/" + shard,
                    service_gtest_cmake,
                )

    def test_features_cpp_do_not_depend_on_root_appcontroller(self):
        forbidden_patterns = {
            "AppController include": re.compile(r'#\s*include\s*[<"][^>"]*AppController(?:\.h)?[>"]'),
            "AppController forward declaration": re.compile(r"\bclass\s+AppController\b"),
            "AppController pointer or reference": re.compile(r"\bAppController\s*[*&]"),
            "AppController qualified method": re.compile(r"\bAppController::"),
            "broad AppControllerPort": re.compile(r"\bAppControllerPort\b"),
            "broad AppContext": re.compile(r"\bAppContext\b"),
            "broad ControllerContext": re.compile(r"\bControllerContext\b"),
            "broad state bag": re.compile(r"\b(?:AppStateBag|StateBag)\b"),
            "camel-case appController handle": re.compile(r"\bappController\b"),
        }

        failures: list[str] = []
        for path in sorted(FEATURES.rglob("*")):
            if path.suffix not in CPP_SUFFIXES:
                continue
            source = read(path)
            rel_path = path.relative_to(CLIENT).as_posix()
            for label, pattern in forbidden_patterns.items():
                if pattern.search(source):
                    failures.append(f"{rel_path}: {label}")

        self.assertEqual([], failures)

    def test_qml_and_js_do_not_reference_root_appcontroller(self):
        forbidden_patterns = {
            "AppController type": re.compile(r"\bAppController\b"),
            "camel-case appController handle": re.compile(r"\bappController\b"),
        }

        failures: list[str] = []
        for root in (FEATURES, QML_ROOT):
            for path in sorted(root.rglob("*")):
                if path.suffix not in QML_SUFFIXES:
                    continue
                source = read(path)
                rel_path = path.relative_to(CLIENT).as_posix()
                for label, pattern in forbidden_patterns.items():
                    if pattern.search(source):
                        failures.append(f"{rel_path}: {label}")

        self.assertEqual([], failures)

        composition = read(APP / "composition/AppComposition.cpp")
        engine_setup = read(APP / "bootstrap/MainQmlEngineSetup.cpp")
        type_registry = read(APP / "bootstrap/MainQmlTypeRegistry.cpp")
        self.assertNotIn('setContextProperty("controller"', composition)
        self.assertNotIn('setContextProperty("controller"', engine_setup)
        self.assertNotIn("qmlRegisterUncreatableType<AppController>", type_registry)

    def test_appcontroller_header_has_no_qml_facade_metadata_or_public_slots(self):
        header = read(APP_CONTROLLER_H)

        self.assertNotIn("Q_PROPERTY(", header)
        self.assertNotIn("Q_INVOKABLE", header)
        self.assertIsNone(re.search(r"^\s*public\s+slots\s*:", header, re.MULTILINE))
        self.assertIsNone(re.search(r"\bAppControllerPort\b", header))
        self.assertNotIn("AppContext", header)
        self.assertNotIn("ControllerContext", header)
        self.assertNotIn("AppStateBag", header)

    def test_app_adapters_do_not_reintroduce_private_appcontroller_bridge(self):
        roots = (
            APP / "coordinators",
            APP / "session",
            APP / "connection",
        )

        failures: list[str] = []
        for root in roots:
            for path in sorted(root.rglob("*")):
                if path.suffix not in CPP_SUFFIXES:
                    continue
                source = read(path)
                rel_path = path.relative_to(CLIENT).as_posix()
                if "_app." in source:
                    failures.append(f"{rel_path}: _app private bridge")
                if "AppController&" in source or "AppController*" in source:
                    failures.append(f"{rel_path}: broad AppController handle")

        self.assertEqual([], failures)

    def test_direct_requested_connections_to_appcontroller_are_explicitly_allowlisted(self):
        controller = " ".join(read(APP / "composition/AppShellSignalBinder.cpp").split())
        constructor = " ".join(read(APP_CONTROLLER_CPP).split())
        pattern = re.compile(
            r"connect\(&(_(?:\w+)|_features\.\w+),\s*&(\w+)::(\w+Requested),\s*this,\s*&AppController::(\w+)\);"
        )
        actual = {(member, owner, signal, target) for member, owner, signal, target in pattern.findall(controller)}
        allowed = {
            ("_features.shellViewModel", "ShellViewModel", "switchToLoginRequested", "switchToLogin"),
            ("_features.shellViewModel", "ShellViewModel", "switchToRegisterRequested", "switchToRegister"),
            ("_features.shellViewModel", "ShellViewModel", "switchToResetRequested", "switchToReset"),
            ("_features.shellViewModel", "ShellViewModel", "switchChatTabRequested", "switchChatTab"),
            (
                "_features.shellViewModel",
                "ShellViewModel",
                "beginPostLoginBootstrapRequested",
                "beginPostLoginBootstrap",
            ),
            ("_features.shellViewModel", "ShellViewModel", "openExternalResourceRequested", "openExternalResource"),
        }

        self.assertEqual(sorted(allowed), sorted(actual))
        self.assertIn("bindAppControllerSignals();", constructor)
        self.assertNotRegex(constructor, pattern)

    def test_appcontroller_constructor_does_not_reown_signal_wiring(self):
        app_controller = read(APP_CONTROLLER_CPP)
        signal_binder = read(APP_SIGNAL_BINDER)
        all_signal_binders = app_signal_binding_text()

        self.assertNotRegex(app_controller, r"\bconnect\s*\(")
        self.assertIn("void AppController::bindAppControllerSignals()", signal_binder)
        self.assertNotRegex(signal_binder, r"\bconnect\s*\(")
        self.assertRegex(all_signal_binders, r"\bconnect\s*\(")
        for helper in (
            "bindAppHttpSignals();",
            "bindAppChatTransportSignals();",
            "bindAppChatDispatcherSignals();",
            "bindAppCallSignals();",
            "bindAppShellSignals();",
            "bindAppTimerSignals();",
        ):
            with self.subTest(signal_helper=helper):
                self.assertIn(helper, signal_binder)
        self.assertNotIn("bindAppFeatureFacadeSignals", signal_binder + read(APP_CONTROLLER_H))
        self.assertNotIn("bindAppChatProjectionSignals", signal_binder + read(APP_CONTROLLER_H))

    def test_appcontroller_constructor_does_not_reown_port_assembly(self):
        app_controller = read(APP_CONTROLLER_CPP)
        port_binder = read(APP_PORT_BINDER)
        all_port_binders = composition_port_binder_text()

        migrated_tokens = (
            "_features.authViewModel.setCommandPort(",
            "_features.callController.setCommandPort(",
            "_features.contactController.setBootstrapPort(",
            "_features.contactController.setApplyBootstrapPort(",
            "_features.contactController.setCommandPort(",
            "_features.groupController.setCommandPort(",
            "_features.profileController.setCommandPort(",
            "_features.profileController.setStatePort(",
            "std::make_unique<AppChatConnectionCoordinator>(",
            "std::make_unique<AppSessionCoordinator>(",
            "std::make_unique<MediaCoordinator>(",
            "std::make_unique<CallCoordinator>(",
            "SessionAuthPort{",
            "PostLoginBootstrapPort port;",
            "RelationBootstrapPort{",
            "RegisterCountdownPort{",
            "SessionLogoutPort port;",
            "MediaSendPort{",
            "CallShellPort{",
            "CallCommandPort{",
            "ChatConnectionPort{",
            "AuthCommandPort{",
            "ContactBootstrapPort{",
            "ContactApplyBootstrapPort{",
            "ContactCommandPort{",
            "GroupCommandPort{",
            "ProfileCommandPort{",
            "ProfileStatePort{",
        )

        self.assertIn("bindAppControllerPorts();", app_controller)
        self.assertIn("void AppController::bindAppControllerPorts()", port_binder)
        app_sources = read(APP / "sources.cmake")
        self.assertIn("app/composition/AppPortRegistry.cpp", app_sources)
        self.assertIn("app/composition/AppPortRegistry.h", app_sources)
        for split_binder in (
            "app/composition/AppFeaturePortBinder.cpp",
            "app/composition/AppAuthFeaturePortBinder.cpp",
            "app/composition/AppContactFeaturePortBinder.cpp",
            "app/composition/AppGroupFeaturePortBinder.cpp",
            "app/composition/AppProfileFeaturePortBinder.cpp",
        ):
            with self.subTest(split_binder=split_binder):
                self.assertIn(split_binder, app_sources)
        for helper in (
            "_port_registry->bindSessionPorts();",
            "_port_registry->bindMediaPorts();",
            "_port_registry->bindCallPorts();",
            "_port_registry->bindFeaturePorts();",
            "_port_registry->bindConnectionPorts();",
        ):
            with self.subTest(helper=helper):
                self.assertIn(helper, port_binder)
        for forbidden_helper in (
            "bindAppSessionPorts",
            "bindAppMediaPorts",
            "bindAppCallPorts",
            "bindAppFeaturePorts",
            "bindAppAuthFeaturePorts",
            "bindAppContactFeaturePorts",
            "bindAppGroupFeaturePorts",
            "bindAppProfileFeaturePorts",
            "bindAppConnectionPorts",
        ):
            with self.subTest(forbidden_appcontroller_port_helper=forbidden_helper):
                self.assertNotIn(forbidden_helper, read(APP_CONTROLLER_H))
        feature_orchestrator = read(APP_FEATURE_PORT_BINDER)
        app_header = read(APP_CONTROLLER_H)
        port_registry = read(APP_PORT_REGISTRY_H)
        self.assertIn("_port_registry->bindSessionPorts();", port_binder)
        self.assertIn("void AppPortRegistry::bindSessionPorts()", all_port_binders)
        self.assertIn("_port_registry->bindFeaturePorts();", port_binder)
        self.assertIn("void AppPortRegistry::bindFeaturePorts()", feature_orchestrator)
        self.assertIn("makeSessionAuthPort()", all_port_binders)
        for maker in (
            "makeSessionAuthPort",
            "makePostLoginBootstrapPort",
            "makeRelationBootstrapPort",
            "makeRegisterCountdownPort",
            "makeSessionLogoutPort",
        ):
            with self.subTest(app_port_registry_maker=maker):
                self.assertIn(f"{maker}();", port_registry)
                self.assertNotIn(f"makeApp{maker.removeprefix('make')}(", app_header)
        for helper in (
            "bindAuthFeaturePorts();",
            "bindContactFeaturePorts();",
            "bindGroupFeaturePorts();",
            "bindProfileFeaturePorts();",
        ):
            with self.subTest(feature_helper=helper):
                self.assertIn(helper, feature_orchestrator)
        for forbidden in (
            "setCommandPort(",
            "setStatePort(",
            "setBootstrapPort(",
            "_gateway",
            "_media_coordinator",
            "_session_coordinator",
            "_features.chatFeatureController",
            "AuthCommandPort{",
            "ContactBootstrapPort{",
            "ContactApplyBootstrapPort{",
            "ContactCommandPort{",
            "GroupCommandPort{",
            "ProfileCommandPort{",
            "ProfileStatePort{",
        ):
            with self.subTest(feature_orchestrator_forbidden=forbidden):
                self.assertNotIn(forbidden, feature_orchestrator)
        for token in migrated_tokens:
            with self.subTest(token=token):
                self.assertNotIn(token, app_controller)
                self.assertIn(token, all_port_binders)

        for token in migrated_tokens:
            with self.subTest(orchestrator_forbidden_token=token):
                self.assertNotIn(token, port_binder)

    def test_contact_group_profile_coordinators_stay_deleted(self):
        qml_cmake = qml_cmake_text()
        coordinators = read(APP_COORDINATORS_H)
        app_header = read(APP_CONTROLLER_H)

        legacy_coordinators = (
            "ContactCoordinatorShell",
            "GroupCoordinator",
            "ProfileCoordinator",
        )
        legacy_paths = (
            "app/coordinators/ContactCoordinatorShell.cpp",
            "app/coordinators/GroupCoordinator.cpp",
            "app/coordinators/ProfileCoordinator.cpp",
        )

        for name in legacy_coordinators:
            with self.subTest(coordinator=name):
                self.assertNotIn(f"class {name}", coordinators)
                self.assertNotIn(f"friend class {name};", app_header)
                self.assertNotIn(f"std::unique_ptr<{name}>", app_header)

        for rel_path in legacy_paths:
            with self.subTest(path=rel_path):
                self.assertFalse((CLIENT / rel_path).exists())
                self.assertNotIn(rel_path, qml_cmake)

    def test_appcontroller_has_no_private_friend_bridge(self):
        header = read(APP_CONTROLLER_H)
        friends = set(re.findall(r"\bfriend\s+class\s+(\w+)\s*;", header))

        self.assertEqual(set(), friends)
        for forbidden in (
            "AppPortRegistry",
            "SessionAuthCoordinator",
            "SessionChatEntryCoordinator",
            "SessionRelationBootstrap",
            "RegisterCountdownController",
            "AppChatConnectionCoordinator",
            "MediaCoordinator",
            "CallCoordinator",
            "ContactCoordinatorShell",
            "GroupCoordinator",
            "ProfileCoordinator",
        ):
            with self.subTest(forbidden_friend=forbidden):
                self.assertNotIn(f"friend class {forbidden};", header)

    def test_app_port_registry_uses_explicit_context_instead_of_root_controller(self):
        header = read(APP_PORT_REGISTRY_H)
        source = read(APP / "composition/AppPortRegistry.cpp")
        registry_owned_port_binders = "\n".join(read(path) for path in APP_PORT_BINDER_FILES if path != APP_PORT_BINDER)

        for contract_type in (
            "struct AppPortRegistryRefs",
            "struct AppPortRegistryConstants",
            "struct AppPortRegistryQueries",
            "struct AppPortRegistryActions",
            "struct AppPortRegistryContext",
            "explicit AppPortRegistry(AppPortRegistryContext context);",
        ):
            with self.subTest(contract_type=contract_type):
                self.assertIn(contract_type, header)
        self.assertNotIn("struct AppPortRegistryEvents", header)

        forbidden_root_controller_tokens = (
            '#include "AppController.h"',
            "#include <AppController.h>",
            "class AppController;",
            "AppController&",
            "AppController*",
            "AppPortRegistry(AppController",
            "_controller",
        )
        for token in forbidden_root_controller_tokens:
            with self.subTest(registry_forbidden_token=token):
                self.assertNotIn(token, header)
                self.assertNotIn(token, source)
                self.assertNotIn(token, registry_owned_port_binders)

        for enum_token in (
            "AppController::LoginPage",
            "AppController::ChatPage",
            "AppController::ApplyRequestPane",
        ):
            with self.subTest(registry_enum_token=enum_token):
                self.assertNotIn(enum_token, source)
                self.assertNotIn(enum_token, registry_owned_port_binders)

    def test_appcontroller_feature_payload_debt_is_explicitly_allowlisted(self):
        actual = app_controller_sources_with("QJsonDocument(", "QJsonObject")
        allowed = {
            "AppController.cpp",
            "AppControllerDialogListPorts.cpp",
            "AppControllerDialogMetaResponses.cpp",
            "AppControllerGroupEvents.cpp",
            "AppControllerGroupHistoryResponses.cpp",
            "AppControllerGroupManagementResponses.cpp",
            "AppControllerGroupMessageResponses.cpp",
            "AppControllerGroupResponseErrors.cpp",
            "AppControllerGroupResponses.cpp",
            "AppControllerPrivateMessageResponses.cpp",
            "AppControllerProfileState.cpp",
        }

        self.assertEqual([], sorted(actual - allowed))

    def test_appcontroller_does_not_call_feature_request_payload_builders(self):
        app_sources = "\n".join(
            read(path) for path in sorted(APP_CONTROLLER_DIR.glob("AppController*")) if path.suffix in CPP_SUFFIXES
        )
        forbidden_patterns = {
            "feature request payload include": re.compile(
                r'#\s*include\s*[<"](?:Call|Contact|Group|Profile)RequestPayloads\.h[>"]'
            ),
            "feature request payload builder call": re.compile(
                r"\bmemochat::(?:call_request_payload|contact_payload|group_payload|profile_payload)"
                r"::build[A-Za-z0-9_]*Payload\b"
            ),
            "app-layer request payload builder": re.compile(
                r"\b(?:QByteArray|QJsonObject)\s+(?:AppController::)?build[A-Za-z0-9_]*Payload\s*\("
            ),
        }

        failures: list[str] = []
        for label, pattern in forbidden_patterns.items():
            if pattern.search(app_sources):
                failures.append(label)

        self.assertEqual([], failures)

    def test_chat_message_model_and_cache_stores_are_chat_owned(self):
        app_sources = app_controller_cpp_sources_text()
        app_header = read(APP_CONTROLLER_H)
        chat_header = read(CLIENT / "features/chat/controller/ChatFeatureController.h")
        chat_source = read(CLIENT / "features/chat/controller/ChatFeatureController.cpp")

        for forbidden in (
            "_message_model",
            "_private_cache_store",
            "_group_cache_store",
            "ChatMessageModel _message_model",
            "PrivateChatCacheStore _private_cache_store",
            "GroupChatCacheStore _group_cache_store",
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, app_header)
                self.assertNotIn(forbidden, app_sources)

        for expected in (
            "ChatMessageModel _messageModel;",
            "PrivateChatCacheStore _privateCacheStore;",
            "GroupChatCacheStore _groupCacheStore;",
            "ChatMessageModel& messageModel();",
            "void openCacheStoresForUser(int uid);",
            "void closeCacheStores();",
        ):
            with self.subTest(expected=expected):
                self.assertIn(expected, chat_header)
        self.assertIn("ChatFeatureController::openCacheStoresForUser", chat_source)
        self.assertIn("ChatFeatureController::closeCacheStores", chat_source)

    def test_chat_feature_binding_orchestrator_stays_thin(self):
        orchestrator = app_controller_binding_orchestrator()
        app_controller = read(APP_CONTROLLER_CPP)
        app_cmake = read(APP / "sources.cmake")

        self.assertLessEqual(len(orchestrator.splitlines()), 30)
        self.assertIn("void AppController::bindChatFeatureController()", orchestrator)
        for helper in (
            "bindChatFeatureProjectionPort();",
            "bindChatFeatureDialogPorts();",
            "bindChatFeatureHistoryPorts();",
            "bindChatFeatureGroupPorts();",
            "bindChatFeatureMediaPorts();",
            "bindChatFeatureSendPorts();",
            "bindChatFeatureReadMutationPorts();",
            "_features.chatFeatureController.bindRequests();",
        ):
            with self.subTest(helper=helper):
                self.assertIn(helper, orchestrator)

        for forbidden in (
            "ChatCurrentSendPort",
            "ChatCurrentGroupHistoryPort",
            "ChatUploadedAttachmentDispatchPort",
            "ChatDialogCommandPort",
            "ChatReadAckPort",
            "ChatMessageMutationPort",
            "_features.chatFeatureController.set",
            "slot_send_data",
            "_gateway",
            "_media_coordinator",
            "_call_coordinator",
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, orchestrator)

        for forbidden in (
            "ChatCurrentSendPort",
            "ChatCurrentGroupHistoryPort",
            "ChatUploadedAttachmentDispatchPort",
            "ChatDialogCommandPort",
            "ChatReadAckPort",
            "ChatMessageMutationPort",
            "_features.chatFeatureController.set",
            "slot_send_data",
        ):
            with self.subTest(app_controller_forbidden=forbidden):
                self.assertNotIn(forbidden, app_controller)

        for path in APP_CHAT_BINDING_FILES:
            with self.subTest(chat_binding_source=path.name):
                self.assertIn(path.relative_to(CLIENT).as_posix(), app_cmake)
                self.assertLessEqual(len(read(path).splitlines()), 180)

        self.assertNotRegex(app_cmake, r"\b(?:file\s*\(\s*GLOB|GLOB_RECURSE|aux_source_directory)\b")

        selection_adapter = read(APP_CONTROLLER_DIR / "AppControllerDialogSelection.cpp")
        app_header = read(APP_CONTROLLER_H)
        app_cmake = read(APP / "sources.cmake")
        factory_header_path = APP_CONTROLLER_DIR / "ChatDialogSelectionPortFactory.h"
        factory_cpp_path = APP_CONTROLLER_DIR / "ChatDialogSelectionPortFactory.cpp"
        private_selection_adapter = read(APP_CONTROLLER_DIR / "AppControllerPrivateSelection.cpp")
        group_selection_adapter = read(APP_CONTROLLER_DIR / "AppControllerGroupSelection.cpp")
        dialog_list_adapter = read(APP_CONTROLLER_DIR / "AppControllerDialogListPorts.cpp")
        dialog_list_router = read(APP / "events/AppChatDispatcherEventRouter.cpp")
        app_controller = read(APP_CONTROLLER_DIR / "AppController.cpp") + "\n" + app_chat_binding_text()
        app_controller_shell = read(APP_CONTROLLER_DIR / "AppController.cpp")
        chat_header = read(FEATURES / "chat/controller/ChatFeatureController.h")
        feature_controller = read(FEATURES / "chat/controller/ChatFeatureController.cpp")
        feature_controller_private_send = read(FEATURES / "chat/controller/ChatFeatureControllerPrivateSend.cpp")
        feature_controller_private_history = read(FEATURES / "chat/controller/ChatFeatureControllerPrivateHistory.cpp")
        feature_controller_dialog_runtime = chat_dialog_runtime_source()

        self.assertTrue(factory_header_path.exists())
        self.assertTrue(factory_cpp_path.exists())
        factory_header = read(factory_header_path)
        factory_cpp = read(factory_cpp_path)
        factory_sources = factory_header + "\n" + factory_cpp

        self.assertIn("app/controller/ChatDialogSelectionPortFactory.cpp", app_cmake)
        self.assertIn("app/controller/ChatDialogSelectionPortFactory.h", app_cmake)
        self.assertIn("namespace memochat::app", factory_sources)
        self.assertIn("struct ChatDialogSelectionActions", factory_header)
        self.assertIn(
            "ChatDialogSelectionPort makeChatDialogSelectionPort(ChatDialogSelectionActions actions);", factory_header
        )
        self.assertIn(
            "ChatDialogSelectionPort makeChatDialogSelectionPort(ChatDialogSelectionActions actions)", factory_cpp
        )
        self.assertNotIn("AppController", factory_sources)
        self.assertNotIn("ChatDialogSelectionPort chatDialogSelectionPort();", app_header)
        self.assertNotIn("ChatDialogSelectionPort AppController::chatDialogSelectionPort", selection_adapter)
        self.assertIn(
            "memochat::app::ChatDialogSelectionActions AppController::chatDialogSelectionActions",
            selection_adapter,
        )
        self.assertIn("_features.chatFeatureController.selectDialogByUid(uid);", selection_adapter)
        self.assertNotIn("setDialogSelectionPort(chatDialogSelectionPort())", app_controller)
        self.assertIn(
            "memochat::app::makeChatDialogSelectionPort(chatDialogSelectionActions())",
            app_controller,
        )

        for field in (
            "groupDialogs",
            "ensureGroupsInitialized",
            "friendById",
            "addFriend",
            "upsertContact",
            "groupById",
            "upsertGroup",
            "dialogDecorationState",
            "setPendingReplyContext",
            "setCurrentPrivatePeerUid",
            "setCurrentGroup",
            "setCurrentChatPeerName",
            "setCurrentChatPeerIcon",
            "clearMessageModel",
            "clearPrivateHistoryState",
            "resetGroupConversationState",
            "setGroupHistoryLoading",
            "setPrivateHistoryLoading",
            "setCanLoadMorePrivateHistory",
            "removeMentionForDialog",
            "emitCurrentDialogUidChangedIfNeeded",
            "loadCurrentPrivateMessages",
            "syncCurrentDialogDraft",
            "openGroupConversation",
            "sendGroupReadAck",
            "loadGroupHistory",
        ):
            with self.subTest(dialog_selection_action=field):
                self.assertIn(f"std::function", factory_header)
                self.assertIn(f"port.{field} = std::move(actions.{field});", factory_cpp)
                self.assertIn(f"actions.{field}", selection_adapter)

        self.assertIn("ChatFeatureController::selectDialogByUid", feature_controller)
        self.assertIn("_dialogSelectionPort.chatListModel = &_chatListModel;", feature_controller)
        self.assertIn("_dialogSelectionPort.dialogListModel = &_dialogListModel;", feature_controller)
        self.assertIn("dependencies.chatListModel = &_chatListModel;", feature_controller_private_send)
        self.assertIn("dependencies.dialogListModel = &_dialogListModel;", feature_controller_private_send)
        self.assertIn("dependencies.chatListModel = &_chatListModel;", feature_controller_private_history)
        self.assertIn("dependencies.dialogListModel = &_dialogListModel;", feature_controller_private_history)
        self.assertIn("dependencies.dialogListModel = &_dialogListModel;", feature_controller_dialog_runtime)
        self.assertIn("dependencies.privateChatListModel = &_chatListModel;", feature_controller_dialog_runtime)
        self.assertIn(
            "dependencies.readAckDispatchPort.dispatchPayload ? dependencies.readAckDispatchPort : _readAckPort.dispatch",
            feature_controller_dialog_runtime,
        )
        self.assertIn(
            "sendPrivateReadAck(readAckRequest, readAckPort)",
            feature_controller_dialog_runtime,
        )
        self.assertIn("sendGroupReadAck(readAckRequest, readAckPort)", feature_controller_dialog_runtime)
        self.assertIn(
            "dependencies.ensureFriend = [&controller, &dependencies](int peerUid)", feature_controller_private_history
        )
        self.assertIn("dependencies.addFriend(placeholder);", feature_controller_private_history)
        self.assertIn("dependencies.upsertContact(placeholder);", feature_controller_private_history)
        feature_controller_group = read(FEATURES / "chat/controller/ChatFeatureControllerGroupConversation.cpp")
        self.assertIn("dependencies.dialogListModel = &controller.dialogListModel();", feature_controller_group)
        self.assertIn("controller.clearGroupUnreadAndMention(dialogListModel, dialogUid);", feature_controller_group)
        self.assertIn(
            "controller.incrementGroupUnreadAndMention(dialogListModel, dialogUid, mentioned);",
            feature_controller_group,
        )
        self.assertIn("return controller.dialogDecorationState();", feature_controller_group)
        self.assertIn("ChatDialogSelectionService::selectDialogByUid", feature_controller)
        self.assertNotIn("ChatDialogSelectionService::selectDialogByUid", selection_adapter)
        self.assertNotIn("port.chatListModel = &_features.chatFeatureController.chatListModel();", selection_adapter)
        self.assertNotIn(
            "port.dialogListModel = &_features.chatFeatureController.dialogListModel();", selection_adapter
        )
        self.assertNotIn("port.groupListModel =", selection_adapter)
        self.assertNotIn("port.groupDialogUidMap =", selection_adapter)
        self.assertIn("actions.groupDialogs.groupDialogByIndex =", selection_adapter)
        self.assertIn("actions.groupDialogs.groupIdForDialogUid =", selection_adapter)
        self.assertIn("port.groupDialogs = std::move(actions.groupDialogs);", factory_cpp)
        self.assertIn("return groupIdForDialogUid(dialogUid);", selection_adapter)
        for source_name, source in (
            ("message dispatch", read(APP_CONTROLLER_DIR / "AppControllerMessageDispatch.cpp")),
            ("private events", read(APP_CONTROLLER_DIR / "AppControllerPrivateEvents.cpp")),
            ("dialog state", read(APP_CONTROLLER_DIR / "AppControllerDialogState.cpp")),
        ):
            with self.subTest(source=source_name):
                self.assertNotIn(
                    "dependencies.chatListModel = &_features.chatFeatureController.chatListModel();", source
                )
                self.assertNotIn(
                    "dependencies.dialogListModel = &_features.chatFeatureController.dialogListModel();", source
                )
                self.assertNotIn(
                    "dependencies.privateChatListModel = &_features.chatFeatureController.chatListModel();",
                    source,
                )
        app_chat_sources = app_controller_cpp_sources_text()
        private_events_source = read(APP_CONTROLLER_DIR / "AppControllerPrivateEvents.cpp")
        self.assertIn("memochat::app::makePrivateChatEventDependencies(", app_chat_sources)
        self.assertIn("_features.contactController.upsertContact(std::move(authInfo));", app_chat_sources)
        self.assertNotIn("PrivateChatEventActions AppController::privateChatEventActions", app_chat_sources)
        self.assertNotIn("actions.addFriend =", private_events_source)
        self.assertNotIn("actions.upsertContact =", private_events_source)
        self.assertNotIn("actions.ensureFriend =", private_events_source)
        self.assertNotIn("buildDialogPlaceholder", private_events_source)
        self.assertNotIn("_features.chatFeatureController.chatListModel().upsertFriend", app_chat_sources)
        self.assertNotIn("_features.chatFeatureController.dialogListModel().upsertFriend", app_chat_sources)
        for forbidden in (
            "AppendFriendChatMsg",
            "UpdatePrivateChatMsgContent",
            "MarkPrivateOutgoingReadUntil",
            "UpdatePrivateChatMsgState",
        ):
            with self.subTest(private_event_mutation=forbidden):
                self.assertNotIn(forbidden, app_chat_sources)
        self.assertIn("memochat::app::makeGroupConversationDependencies(", app_chat_sources)
        self.assertIn("controller.groupConversationDependencies(", read(CHAT_EVENT_FACTORY_CPP))
        self.assertNotIn("GroupConversationActions AppController::groupConversationActions", app_chat_sources)
        self.assertNotIn("actions.dialogUidMap =", app_chat_sources)
        self.assertNotIn("_group_dialog_state", app_chat_sources)
        self.assertNotIn("actions.groupListModel =", app_chat_sources)
        self.assertNotIn(
            "dependencies.dialogListModel = &_features.chatFeatureController.dialogListModel();", app_chat_sources
        )
        self.assertNotIn("dependencies.dispatchGroupReadAckPayload", app_chat_sources)
        self.assertNotIn("ID_GROUP_READ_ACK_REQ", app_chat_sources)
        self.assertNotIn("dependencies.clearGroupUnreadAndMention =", app_chat_sources)
        self.assertNotIn("dependencies.incrementGroupUnreadAndMention =", app_chat_sources)
        self.assertNotIn("dependencies.dialogDecorationState =", app_chat_sources)
        for forbidden in (
            "UpsertGroupChatMsg",
            "UpdateGroupChatMsgState",
            "UpdateGroupChatMsgContent",
            "MarkGroupOutgoingReadUntil",
        ):
            with self.subTest(group_event_mutation=forbidden):
                self.assertNotIn(forbidden, app_chat_sources)
        self.assertNotIn("std::make_shared<GroupInfoData>()", selection_adapter)
        selection_service = read(FEATURES / "chat/services/ChatDialogSelectionService.cpp")
        self.assertIn("port.loadGroupHistory();", selection_service)
        self.assertIn("_features.chatFeatureController.selectPrivateByIndex(index);", private_selection_adapter)
        self.assertIn("_features.chatFeatureController.selectPrivateByUid(uid);", private_selection_adapter)
        self.assertIn("_features.chatFeatureController.selectGroupByIndex(index);", group_selection_adapter)
        self.assertIn(
            "_features.chatFeatureController.selectGroupByDialogUid(dialogUid, groupId);", group_selection_adapter
        )
        self.assertNotIn("ChatDialogSelectionService::selectPrivateByIndex", private_selection_adapter)
        self.assertNotIn("ChatDialogSelectionService::selectPrivateByUid", private_selection_adapter)
        self.assertNotIn("ChatDialogSelectionService::selectGroupByIndex", group_selection_adapter)
        self.assertNotIn("ChatDialogSelectionService::selectGroupByDialogUid", group_selection_adapter)
        self.assertIn("_features.groupController.loadGroupHistory();", selection_adapter)
        self.assertIn("ChatDialogListAppPort port;", dialog_list_adapter)
        self.assertIn("_chat_controller.handleDialogListResponse", dialog_list_router)
        self.assertIn("port.selectFirstChat = [this]()", dialog_list_adapter)
        self.assertIn("selectChatIndex(0);", dialog_list_adapter)
        self.assertNotIn("_features.chatFeatureController.chatListModel().count()", dialog_list_adapter)
        self.assertNotIn("_features.chatViewModel.sync", app_controller_shell)
        self.assertNotIn("ChatViewProjectionState state;", app_controller_shell)
        self.assertIn("_features.chatFeatureController.syncViewModelState();", app_controller_shell)
        self.assertIn("ChatViewProjectionPort projectionPort;", app_chat_binding_text())
        self.assertIn("projectionPort.snapshot = [this]()", app_chat_binding_text())
        self.assertIn(
            "_features.chatFeatureController.setViewProjectionPort(std::move(projectionPort));", app_chat_binding_text()
        )
        self.assertIn("struct ChatViewProjectionState", chat_header)
        self.assertIn("struct ChatViewProjectionPort", chat_header)
        self.assertIn("void setViewProjectionPort(ChatViewProjectionPort port);", chat_header)
        self.assertIn("void syncViewModelState();", chat_header)
        self.assertIn("void syncViewModelState(const ChatViewProjectionState& state);", chat_header)
        self.assertIn("_viewModel.syncDialogListModel(&_dialogListModel);", feature_controller)
        self.assertIn("_viewModel.syncMessageModel(&_messageModel);", feature_controller)
        self.assertIn("ChatFeatureController::syncViewModelState", feature_controller)
        self.assertIn(
            "_viewModel.syncMediaUpload(state.mediaUploadInProgress, state.mediaUploadProgressText);",
            feature_controller,
        )
        self.assertNotIn("ChatDialogListResponsePort port;", dialog_list_adapter)
        self.assertNotIn("ChatDialogListResponseService::handle", dialog_list_adapter)
        self.assertNotIn("dialogDecorationStateWithoutServerMeta", dialog_list_adapter)
        self.assertNotIn("context.existingDialogs", dialog_list_adapter)
        self.assertNotIn("resetServerDialogMeta", dialog_list_adapter)
        self.assertNotIn("seedServerDialogMeta", dialog_list_adapter)
        self.assertNotIn("replaceDialogList =", dialog_list_adapter)
        self.assertNotIn("clearUnreadDialog =", dialog_list_adapter)
        self.assertNotIn("clearMentionDialog =", dialog_list_adapter)
        self.assertNotIn("ChatDialogListResponseService::reduce", dialog_list_adapter)
        self.assertNotIn("effect.upsertGroups", dialog_list_adapter)
        self.assertNotIn("effect.upsertChatDialogs", dialog_list_adapter)
        self.assertNotIn("effect.upsertGroupDialogs", dialog_list_adapter)
        self.assertNotIn("effect.bootstrapPrivateHistoryUids", dialog_list_adapter)
        self.assertNotIn("effect.bootstrapGroupHistoryIds", dialog_list_adapter)
        dialog_list_service = read(FEATURES / "chat/services/ChatDialogListResponseService.cpp")
        feature_controller_dialog_list = read(FEATURES / "chat/controller/ChatFeatureControllerDialogList.cpp")
        self.assertIn("void ChatDialogListResponseService::apply", dialog_list_service)
        self.assertIn("ChatDialogListResponseService::handle", dialog_list_service)
        self.assertIn("for (const auto& group : effect.upsertGroups)", dialog_list_service)
        self.assertIn("for (const int peerUid : effect.bootstrapPrivateHistoryUids)", dialog_list_service)
        self.assertIn("ChatFeatureController::handleDialogListResponse", feature_controller_dialog_list)
        self.assertIn(
            "context.decorationState = dialogDecorationStateWithoutServerMeta();", feature_controller_dialog_list
        )
        self.assertIn("context.existingDialogs.push_back(_dialogListModel.get(i));", feature_controller_dialog_list)
        self.assertIn("featurePort.replaceDialogList =", feature_controller_dialog_list)
        self.assertIn("ChatDialogListAppPort guardedAppPort = appPort;", feature_controller_dialog_list)
        self.assertIn("if (_chatListModel.count() <= 0)", feature_controller_dialog_list)
        self.assertIn("selectFirstChat();", feature_controller_dialog_list)
        self.assertIn("ChatDialogListResponseService::handle", feature_controller_dialog_list)
        for adapter_name, adapter in (
            ("dialog selection", selection_adapter),
            ("private selection", private_selection_adapter),
            ("group selection", group_selection_adapter),
        ):
            with self.subTest(adapter=adapter_name):
                self.assertNotIn("DialogListService::buildDialogEntry", adapter)
                self.assertNotIn("DialogListService::buildPlaceholderAuthInfo", adapter)
                self.assertNotIn("ConversationSyncService::groupIdForDialogUid", adapter)

    def test_contact_event_dependency_factory_is_app_layer_without_appcontroller_handle(self):
        app_cmake = read(APP / "sources.cmake")

        self.assertTrue(CONTACT_EVENT_FACTORY_H.exists())
        self.assertTrue(CONTACT_EVENT_FACTORY_CPP.exists())
        self.assertIn("app/controller/ContactEventDependenciesFactory.cpp", app_cmake)
        self.assertIn("app/controller/ContactEventDependenciesFactory.h", app_cmake)

        factory_header = read(CONTACT_EVENT_FACTORY_H)
        factory_cpp = read(CONTACT_EVENT_FACTORY_CPP)
        factory_sources = factory_header + "\n" + factory_cpp

        self.assertIn("namespace memochat::app", factory_sources)
        self.assertIn("struct ContactEventActions", factory_header)
        self.assertIn(
            "ContactEventDependencies makeContactEventDependencies(ContactEventActions actions);", factory_header
        )
        self.assertIn("ContactEventDependencies makeContactEventDependencies(ContactEventActions actions)", factory_cpp)
        for forbidden in (
            '#include "AppController.h"',
            "#include <AppController.h>",
            "AppController",
            "AppController::",
            "AppController&",
            "AppController*",
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, factory_sources)

        for field in (
            "addAuthFriendToStore",
            "addAuthRspToStore",
            "removeFriendFromStore",
            "currentUser",
            "isFriend",
            "alreadyApplied",
            "markApplyApprovedInStore",
            "addApplyToStore",
            "upsertChatContactFromAuthInfo",
            "upsertChatContactFromAuthRsp",
            "removeChatContact",
            "openPrivateChat",
            "switchToChatTab",
            "refreshDialogModel",
            "refreshChatLoadMoreState",
            "refreshContactLoadMoreState",
        ):
            with self.subTest(contact_event_action=field):
                self.assertIn(f"std::function", factory_header)
                self.assertIn(f"dependencies.{field} = std::move(actions.{field});", factory_cpp)
        for forbidden in (
            "removeChatFriend",
            "removeDialogFriend",
            "removeDialogDecoration",
            "clearSelectedPrivateChat",
        ):
            with self.subTest(forbidden_contact_chat_detail=forbidden):
                self.assertNotIn(forbidden, factory_sources)
        app_projection_sources = "\n".join(
            read(path)
            for root in (APP_CONTROLLER_DIR, APP / "composition", APP / "coordinators")
            for path in sorted(root.rglob("*"))
            if path.suffix in CPP_SUFFIXES
        )
        self.assertNotIn("_features.chatFeatureController.chatListModel().upsertFriend", app_projection_sources)

    def test_appcontroller_session_state_debt_is_explicitly_allowlisted(self):
        actual = app_controller_sources_with("_pending_login_state", "_chat_endpoint_state", "_chat_recovery_state")
        allowed: set[str] = set()

        self.assertEqual(sorted(allowed), sorted(actual))

    def test_appcontroller_queue_and_incoming_buffer_debt_is_explicitly_allowlisted(self):
        old_upload_runner = app_controller_sources_with("processPendingAttachmentQueue")
        runner_adapter = app_controller_sources_with("startPendingAttachmentRunner")
        incoming_buffer = app_controller_sources_with(
            "AppIncomingMessageBufferState",
            "_incoming_buffer_state",
            "bufferIncoming",
            "flushPendingIncomingMessages",
        )

        self.assertEqual([], sorted(old_upload_runner))
        self.assertEqual(
            [],
            sorted(
                runner_adapter
                - {"AppController.cpp", "AppControllerMediaUploadQueue.cpp", "AppControllerPendingAttachments.cpp"}
            ),
        )
        self.assertEqual(
            [],
            sorted(incoming_buffer),
        )

    def test_dialog_model_refresh_delegates_entry_reducer_to_chat_feature(self):
        dialogs = read(APP_CONTROLLER_DIR / "AppControllerDialogModels.cpp")
        chat_feature_header = read(FEATURES / "chat/controller/ChatFeatureController.h")
        chat_feature_dialog_list = read(FEATURES / "chat/controller/ChatFeatureControllerDialogList.cpp")
        dialog_service = read(FEATURES / "chat/services/DialogListService.cpp")

        self.assertIn("_features.chatFeatureController.replaceDialogListFromSnapshots", dialogs)
        self.assertIn("_features.chatFeatureController.upsertDialogListFromSnapshots", dialogs)
        for forbidden in (
            "DialogEntrySeed",
            "DialogListService::buildDialogEntry",
            "DialogListService::sortDialogs",
            "ConversationSyncService::resolveGroupDialogUid",
            "_features.chatFeatureController.dialogListModel().upsertBatch",
            "_features.chatFeatureController.chatListModel().upsertFriend",
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, dialogs)

        self.assertIn("replaceDialogListFromSnapshots", chat_feature_header)
        self.assertIn("upsertDialogListFromSnapshots", chat_feature_header)
        self.assertIn("ChatFeatureController::replaceDialogListFromSnapshots", chat_feature_dialog_list)
        self.assertIn("ChatFeatureController::upsertDialogListFromSnapshots", chat_feature_dialog_list)
        self.assertIn("DialogListService::buildSortedSnapshotDialogs", chat_feature_dialog_list)
        self.assertIn("DialogListService::buildSortedSnapshotDialogs", dialog_service)

    def test_group_model_refresh_delegates_snapshot_reducer_to_group_feature(self):
        models = read(APP_CONTROLLER_DIR / "AppControllerModels.cpp")
        refresh_group_model = extract_function(models, "void AppController::refreshGroupModel")
        group_header = read(FEATURES / "group/controller/GroupController.h")
        group_controller = read(FEATURES / "group/controller/GroupController.cpp")
        set_from_snapshots = extract_function(group_controller, "void GroupController::setGroupsFromSnapshots")

        chat_feature_header = read(FEATURES / "chat/controller/ChatFeatureController.h")
        chat_feature_dialog_list = read(FEATURES / "chat/controller/ChatFeatureControllerDialogList.cpp")

        self.assertIn("_features.chatFeatureController.resetGroupDialogIdentityMap();", refresh_group_model)
        self.assertIn("_gateway.userMgr()->GetGroupListSnapshot()", refresh_group_model)
        self.assertIn("_features.groupController.setGroupsFromSnapshots(", refresh_group_model)
        self.assertIn("_features.chatFeatureController.groupDialogUidMap()", refresh_group_model)
        self.assertNotIn("_group_dialog_state", refresh_group_model)
        for forbidden in (
            "FriendInfo",
            "std::vector<std::shared_ptr<FriendInfo>>",
            "std::make_shared<FriendInfo>",
            "new FriendInfo",
            "ConversationSyncService",
            "resolveGroupDialogUid",
            "qrc:/res/chat_icon.png",
            "_group_id",
            "_icon",
            "_announcement",
            "_last_msg",
            "reserve(",
            "push_back(",
            "_features.groupController.setGroups(",
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, refresh_group_model)
                self.assertNotIn(forbidden, models)

        self.assertIn("setGroupsFromSnapshots", group_header)
        self.assertIn("GroupController::setGroupsFromSnapshots", set_from_snapshots)
        self.assertIn("ConversationSyncService::resolveGroupDialogUid", set_from_snapshots)
        self.assertIn("std::make_shared<FriendInfo>", set_from_snapshots)
        self.assertIn('QStringLiteral("qrc:/res/chat_icon.png")', set_from_snapshots)
        self.assertIn("setGroups(converted);", set_from_snapshots)
        self.assertIn("QMap<int, qint64> _groupDialogUidMap;", chat_feature_header)
        self.assertIn("ChatFeatureController::resetGroupDialogIdentityMap", chat_feature_dialog_list)
        self.assertIn("ChatFeatureController::resolveGroupDialogUid", chat_feature_dialog_list)

    def test_logout_reset_debt_is_bound_to_app_owned_reset_sources(self):
        reset_sources = app_reset_owner_sources_with(
            "clearSessionForLogout",
            "ResetSession()",
            "resetContactFeature()",
            "resetGroupFeature()",
            "_logout_port.resetFeatureRuntimeForLogout",
            "resetCallSurface()",
            "resetRuntimeForLogout()",
            "resetModelsForLogout()",
            "CloseConnection()",
            "clearConnectionCache()",
        )
        allowed_reset_sources = {
            "composition/AppConnectionPortBinder.cpp",
            "composition/AppPostLoginBootstrapPortBinder.cpp",
            "composition/AppSessionAuthPortBinder.cpp",
            "composition/AppSessionLogoutPortBinder.cpp",
            "controller/AppController.cpp",
            "controller/AppControllerIncomingBuffer.cpp",
            "controller/AppControllerMediaUploadQueue.cpp",
            "coordinators/CallCoordinator.cpp",
            "coordinators/CallCoordinatorCommands.cpp",
            "session/AppSessionCoordinatorConnectionState.cpp",
            "session/AppSessionCoordinatorLogout.cpp",
            "session/SessionChatEntryCoordinator.cpp",
        }

        self.assertEqual([], sorted(reset_sources - allowed_reset_sources))

        reset_owner_text = "\n".join(
            read(APP / rel_path) for rel_path in sorted(reset_sources) if (APP / rel_path).exists()
        )
        for token in (
            "clearSessionForLogout",
            "ResetSession()",
            "resetContactFeature()",
            "resetGroupFeature()",
            "invokeIfSet(_logout_port.resetFeatureRuntimeForLogout)",
            "_features.chatFeatureController.resetModelsForLogout();",
            "_features.chatFeatureController.resetRuntimeForLogout();",
            "resetCallSurface()",
        ):
            with self.subTest(reset_token=token):
                self.assertIn(token, reset_owner_text)

    def test_appcontroller_media_upload_queue_is_runner_adapter_only(self):
        upload_queue = read(APP_CONTROLLER_DIR / "AppControllerMediaUploadQueue.cpp")
        header = read(APP_CONTROLLER_H)
        runner = read(MEDIA_RUNNER_CPP)
        runner_header = read(MEDIA_RUNNER_H)

        self.assertIn("void AppController::startPendingAttachmentRunner()", upload_queue)
        self.assertIn("MediaPendingAttachmentRunnerPort{", upload_queue)
        self.assertIn("new MediaPendingAttachmentRunner", upload_queue)
        self.assertIn("runner->start()", upload_queue)
        self.assertIn("void startPendingAttachmentRunner();", header)
        self.assertIn(
            "_features.chatFeatureController.dispatchUploadedAttachment(attachment, uploaded, destination)",
            upload_queue,
        )
        self.assertNotIn("processPendingAttachmentQueue", upload_queue + header)
        self.assertNotIn("UploadedAttachmentDispatchPort", upload_queue)
        self.assertNotIn("dispatchPrivatePacket", upload_queue)
        self.assertNotIn("dispatchGroupPayload", upload_queue)
        self.assertNotIn("slot_send_data", upload_queue)

        for runner_loop_token in (
            "QFutureWatcher<MediaUploadResult>",
            "QtConcurrent::run",
            "Qt::QueuedConnection",
            "handleUploadFinished",
            "processNextAsync",
            "advanceQueue",
            "resetQueue",
        ):
            with self.subTest(runner_loop_token=runner_loop_token):
                self.assertIn(runner_loop_token, runner + runner_header)
                self.assertNotIn(runner_loop_token, upload_queue)

        self.assertNotIn("AppController", runner + runner_header)

    def test_appcontroller_group_management_callbacks_delegate_to_group_services(self):
        events = read(APP_CONTROLLER_DIR / "AppControllerGroupEvents.cpp")
        chat_dispatcher_router = read(APP / "events/AppChatDispatcherEventRouter.cpp")
        management = read(APP_CONTROLLER_DIR / "AppControllerGroupManagementResponses.cpp")
        errors = read(APP_CONTROLLER_DIR / "AppControllerGroupResponseErrors.cpp")
        app_header = read(APP_CONTROLLER_H)
        app_cmake = read(APP / "sources.cmake")
        factory_header_path = APP_CONTROLLER_DIR / "GroupManagementEffectPortFactory.h"
        factory_cpp_path = APP_CONTROLLER_DIR / "GroupManagementEffectPortFactory.cpp"
        event_service = read(FEATURES / "group/controller/GroupManagementEventService.cpp")
        response_service = read(FEATURES / "group/controller/GroupManagementResponseService.cpp")
        group_controller_header = read(FEATURES / "group/controller/GroupController.h")
        group_controller = read(FEATURES / "group/controller/GroupController.cpp")

        self.assertTrue(factory_header_path.exists())
        self.assertTrue(factory_cpp_path.exists())
        factory_header = read(factory_header_path)
        factory_cpp = read(factory_cpp_path)
        factory_sources = factory_header + factory_cpp

        for reducer in (
            "GroupManagementEventService::reduceGroupListUpdated",
            "GroupManagementEventService::reduceGroupInvite",
            "GroupManagementEventService::reduceGroupApply",
            "GroupManagementEventService::reduceGroupMemberChanged",
        ):
            with self.subTest(event_reducer=reducer):
                self.assertIn(reducer, chat_dispatcher_router)

        self.assertIn("GroupManagementResponseService::reduceSuccess", management)
        self.assertIn("GroupResponseErrorService::reduce", errors)
        self.assertIn("GroupResponseErrorTarget::ManagementEffect", errors)
        self.assertNotIn("GroupManagementResponseService::reduceError", errors)
        self.assertNotIn("GroupManagementEffectApplier.h", management)
        self.assertNotIn("GroupManagementEffectApplier::apply", events + chat_dispatcher_router + management + errors)
        self.assertNotIn("void applyGroupManagementEffect", app_header)
        self.assertNotIn("void applyGroupManagementResponseEffect", app_header)
        self.assertIn("_group_controller.applyGroupManagementEffect", chat_dispatcher_router)
        self.assertIn("_features.groupController.applyGroupManagementResponseEffect", management + errors)
        self.assertNotIn("groupManagementEventEffectPort", app_header[app_header.index("private:") :])
        self.assertNotIn("groupManagementResponseEffectPort", app_header[app_header.index("private:") :])
        self.assertNotIn(
            "AppController::groupManagementEventEffectPort", events + chat_dispatcher_router + management + errors
        )
        self.assertNotIn(
            "AppController::groupManagementResponseEffectPort", events + chat_dispatcher_router + management + errors
        )
        self.assertIn("GroupManagementEffectPortFactory.h", read(APP / "events/AppChatDispatcherEventRouter.h"))
        self.assertIn("GroupManagementEffectPortFactory.h", management)
        self.assertIn("GroupManagementEffectPortFactory.h", errors)
        self.assertIn("app/controller/GroupManagementEffectPortFactory.cpp", app_cmake)
        self.assertIn("app/controller/GroupManagementEffectPortFactory.h", app_cmake)
        self.assertIn("GroupManagementEffectPorts.h", factory_header)
        self.assertIn("struct GroupManagementEventEffectActions", factory_header)
        self.assertIn("struct GroupManagementResponseEffectActions", factory_header)
        self.assertIn("makeGroupManagementEventEffectPort(GroupManagementEventEffectActions actions)", factory_header)
        self.assertIn(
            "makeGroupManagementResponseEffectPort(GroupManagementResponseEffectActions actions)", factory_header
        )
        self.assertIn("makeGroupManagementEventEffectPort(GroupManagementEventEffectActions actions)", factory_cpp)
        self.assertIn(
            "makeGroupManagementResponseEffectPort(GroupManagementResponseEffectActions actions)", factory_cpp
        )
        self.assertIn("memochat::group::GroupManagementEventEffectPort", factory_sources)
        self.assertIn("memochat::group::GroupManagementResponseEffectPort", factory_sources)
        self.assertNotIn("AppController", factory_sources)
        self.assertNotIn("appController", factory_sources)

        for event_port_callback in (
            "removeGroup",
            "clearGroupConversation",
            "refreshGroupModel",
            "refreshDialogModel",
            "requestDialogList",
            "selectCurrentGroup",
            "clearCurrentGroup",
            "setCurrentChatPeerIcon",
            "openGroupConversation",
            "syncCurrentDialogDraft",
            "loadCurrentChatMessages",
            "clearMessageModel",
            "resetCurrentChatProjection",
        ):
            with self.subTest(event_port_callback=event_port_callback):
                self.assertIn(event_port_callback, factory_header)
                self.assertRegex(factory_cpp, rf"(?:\bport\.|\.){event_port_callback}\s*=")

        for response_port_callback in (
            "removeGroup",
            "clearGroupConversation",
            "refreshGroupModel",
            "refreshDialogModel",
            "selectDialogByUid",
            "setCurrentChatPeerIcon",
        ):
            with self.subTest(response_port_callback=response_port_callback):
                self.assertIn(response_port_callback, factory_header)
                self.assertRegex(factory_cpp, rf"(?:\bport\.|\.){response_port_callback}\s*=")

        self.assertIn("void applyGroupManagementEffect(", group_controller_header)
        self.assertIn("void applyGroupManagementResponseEffect(", group_controller_header)
        self.assertIn("GroupController::applyGroupManagementEffect", group_controller)
        self.assertIn("GroupController::applyGroupManagementResponseEffect", group_controller)
        self.assertIn("GroupManagementEffectApplier::apply(effect, port);", group_controller)
        self.assertNotIn("AppController", group_controller_header + group_controller)
        self.assertFalse((APP / "composition/AppFeatureFacadeSignalBinder.cpp").exists())
        self.assertIn("Connections {", read(QML_ROOT / "app/ChatShellPage.qml"))
        self.assertIn("target: group", read(QML_ROOT / "app/ChatShellPage.qml"))
        self.assertNotIn("connect(&_features.groupController, &GroupController::groupCreated", read(APP_CONTROLLER_CPP))
        self.assertNotIn("QJsonDocument(", events + chat_dispatcher_router + management)

        for service_source in (event_service, response_service):
            with self.subTest(service="group_management"):
                self.assertNotIn("AppController", service_source)
                self.assertIn("Effect", service_source)

        for old_inline_policy in (
            "收到群邀请",
            "收到入群申请",
            "群事件：",
            "群聊创建成功",
            "邀请成员成功",
            "群聊已解散",
            "已退出当前群聊",
            "QJsonDocument(",
        ):
            with self.subTest(old_inline_policy=old_inline_policy):
                self.assertNotIn(old_inline_policy, events + chat_dispatcher_router + management)

    def test_request_dialog_list_adapter_delegates_to_group_controller(self):
        source = read(APP_CONTROLLER_GROUP_COMMANDS)
        body = extract_function(source, "void AppController::requestDialogList")
        compact_body = " ".join(body.split())

        self.assertEqual(
            "void AppController::requestDialogList() { _features.groupController.requestDialogList(); }",
            compact_body,
        )
        self.assertNotIn("ID_GET_DIALOG_LIST_REQ", body)
        self.assertNotIn("buildRequestDialogListPayload", body)
        self.assertNotIn("slot_send_data", body)

        group_header = read(FEATURES / "group/controller/GroupController.h")
        group_controller = read(FEATURES / "group/controller/GroupController.cpp")
        group_body = extract_function(group_controller, "void GroupController::requestDialogList")
        self.assertIn("Q_INVOKABLE void requestDialogList();", group_header)
        self.assertIn("memochat::group_payload::buildRequestDialogListPayload", group_body)
        self.assertIn("ReqId::ID_GET_DIALOG_LIST_REQ", group_body)

        self.assertFalse((APP / "controller/AppControllerGroupPayloads.cpp").exists())
        self.assertFalse((APP / "controller/AppControllerGroupPayloads.h").exists())
        self.assertIn("buildRequestDialogListPayload", group_controller)
        self.assertNotIn("buildRequestDialogListPayload", source)

    def test_group_history_request_adapters_delegate_policy_to_chat_feature(self):
        group_commands = read(APP_CONTROLLER_GROUP_COMMANDS)
        app_controller = read(APP_CONTROLLER_CPP)
        port_binder = read(APP_GROUP_FEATURE_PORT_BINDER)
        app_header = read(APP_CONTROLLER_H)
        group_port = port_binder.split("_features.groupController.setCommandPort(GroupCommandPort{", 1)[1].split(
            "});", 1
        )[0]
        bootstrap = extract_function(group_commands, "void AppController::requestGroupHistoryForBootstrap")
        chat_header = read(FEATURES / "chat/controller/ChatFeatureController.h")
        chat_group = read(FEATURES / "chat/controller/ChatFeatureControllerGroupConversation.cpp")
        group_binding = app_chat_binding_file_by_name("AppChatGroupBinding.cpp")

        self.assertNotIn("void loadGroupHistory();", app_header)
        self.assertNotIn("void AppController::loadGroupHistory", group_commands)
        self.assertNotIn("_features.groupController.setCommandPort(GroupCommandPort{", app_controller)
        self.assertIn("_features.chatFeatureController.requestCurrentGroupHistory()", group_port)
        self.assertIn("GetFriendListSnapshot();", group_port)
        self.assertNotIn("CheckFriendById", group_port)
        self.assertNotIn("one->_user_id", group_port)
        self.assertIn("_features.chatFeatureController.requestGroupHistoryForBootstrap(", bootstrap)
        self.assertIn("struct ChatCurrentGroupHistoryPort", chat_header)
        self.assertIn("void setCurrentGroupHistoryPort(ChatCurrentGroupHistoryPort port);", chat_header)
        self.assertIn("ChatFeatureController::requestCurrentGroupHistory()", chat_group)
        self.assertIn("GroupHistoryRequestCommand request;", chat_group)
        self.assertIn("ChatCurrentGroupHistoryPort currentGroupHistoryPort;", group_binding)
        self.assertIn("currentGroupHistoryPort.snapshot = [this]()", group_binding)
        self.assertIn(
            "_features.chatFeatureController.setCurrentGroupHistoryPort(std::move(currentGroupHistoryPort));",
            group_binding,
        )

        for name, body in (
            ("loadHistory adapter", group_port),
            ("requestGroupHistoryForBootstrap", bootstrap),
        ):
            with self.subTest(adapter=name):
                self.assertNotIn("GroupHistoryBuildRequest", body)
                self.assertNotIn("buildGroupHistoryRequest", body)
                self.assertNotIn("GroupConversationService::buildHistoryRequest", body)
                self.assertNotIn("_shell_state.loadingState().groupHistoryLoading = true", body)

        self.assertNotIn("GroupHistoryRequestCommand request;", group_port)
        self.assertNotIn("_features.chatFeatureController.requestGroupHistory(request", group_port)
        self.assertNotRegex(group_port, r"if\s*\([^)]*_shell_state\.loadingState\(\)\.groupHistoryLoading[^)]*\)\s*\{")
        self.assertNotRegex(bootstrap, r"if\s*\([^)]*_shell_state\.loadingState\(\)\.groupHistoryLoading[^)]*\)\s*\{")

    def test_chat_event_dependency_factory_maps_app_actions_without_appcontroller_handle(self):
        app_cmake = read(APP / "sources.cmake")
        app_header = read(APP_CONTROLLER_H)
        app_sources = (
            app_header
            + "\n"
            + "\n".join(
                read(path) for path in sorted(APP_CONTROLLER_DIR.glob("AppController*")) if path.suffix in CPP_SUFFIXES
            )
        )
        source = read(APP_CONTROLLER_DIR / "AppControllerPrivateEvents.cpp")
        factory_header = read(CHAT_EVENT_FACTORY_H)
        factory_cpp = read(CHAT_EVENT_FACTORY_CPP)
        factory_sources = factory_header + "\n" + factory_cpp

        self.assertTrue(CHAT_EVENT_FACTORY_H.exists())
        self.assertTrue(CHAT_EVENT_FACTORY_CPP.exists())
        self.assertIn("app/controller/ChatEventDependenciesFactory.cpp", app_cmake)
        self.assertIn("app/controller/ChatEventDependenciesFactory.h", app_cmake)
        self.assertIn("makePrivateChatEventDependencies(", factory_sources)
        self.assertIn("makeGroupConversationDependencies(", factory_sources)
        self.assertIn("ChatFeatureController& controller", factory_header)
        self.assertIn("std::shared_ptr<UserMgr> userMgr", factory_header)
        self.assertIn("FriendListModel* groupListModel", factory_header)
        self.assertIn("controller.privateEventDependencies(", factory_cpp)
        self.assertIn("controller.groupConversationDependencies(", factory_cpp)
        self.assertNotIn("struct PrivateChatEventActions", factory_header)
        self.assertNotIn("struct GroupConversationActions", factory_header)
        self.assertNotIn("PrivateChatEventActions actions", factory_sources)
        self.assertNotIn("GroupConversationActions actions", factory_sources)
        for forbidden in (
            '#include "AppController.h"',
            "#include <AppController.h>",
            "AppController",
            "AppController::",
            "AppController&",
            "AppController*",
            "appController",
        ):
            with self.subTest(factory_forbidden=forbidden):
                self.assertNotIn(forbidden, factory_sources)

        for old_builder in (
            "PrivateChatEventDependencies AppController::privateChatEventDependencies()",
            "GroupConversationDependencies AppController::groupConversationDependencies()",
        ):
            with self.subTest(old_builder=old_builder):
                self.assertNotIn(old_builder, app_sources)

        for action_builder in (
            "memochat::app::PrivateChatEventActions privateChatEventActions();",
            "memochat::app::GroupConversationActions groupConversationActions();",
            "memochat::app::PrivateChatEventActions AppController::privateChatEventActions()",
            "memochat::app::GroupConversationActions AppController::groupConversationActions()",
        ):
            with self.subTest(action_builder=action_builder):
                self.assertNotIn(action_builder, app_sources)

        self.assertIn(
            "memochat::app::makePrivateChatEventDependencies(\n            _features.chatFeatureController",
            app_sources,
        )
        self.assertIn(
            "memochat::app::makeGroupConversationDependencies(\n                _features.chatFeatureController",
            app_sources,
        )
        self.assertIn("_features.contactController.upsertContact(std::move(authInfo));", app_sources)
        self.assertIn("transport->slot_send_data(static_cast<ReqId>(", app_sources)
        self.assertNotIn("ID_PRIVATE_HISTORY_REQ", source)
        self.assertNotIn("ID_GROUP_CHAT_MSG_REQ", app_sources)
        self.assertNotIn("ID_GROUP_HISTORY_REQ", app_sources)
        for forbidden in (
            "AppendFriendChatMsg",
            "UpdatePrivateChatMsgContent",
            "MarkPrivateOutgoingReadUntil",
            "UpdatePrivateChatMsgState",
            "UpsertGroupChatMsg",
            "UpdateGroupChatMsgState",
            "UpdateGroupChatMsgContent",
            "MarkGroupOutgoingReadUntil",
        ):
            with self.subTest(appcontroller_chat_mutation=forbidden):
                self.assertNotIn(forbidden, source)

        feature_private = read(FEATURES / "chat/controller/ChatFeatureControllerPrivateHistory.cpp")
        feature_group = read(FEATURES / "chat/controller/ChatFeatureControllerGroupConversation.cpp")
        self.assertIn("ChatFeatureController::privateEventDependencies", feature_private)
        self.assertIn("ChatFeatureController::groupConversationDependencies", feature_group)

        app_controller_call_sources = "\n".join(
            read(path) for path in sorted(APP_CONTROLLER_DIR.glob("AppController*.cpp"))
        )
        self.assertNotIn(
            "memochat::app::makePrivateChatEventDependencies(privateChatEventActions())", app_controller_call_sources
        )
        self.assertNotIn(
            "memochat::app::makeGroupConversationDependencies(groupConversationActions())", app_controller_call_sources
        )
        self.assertNotIn("privateChatEventActions()", app_controller_call_sources)
        self.assertNotIn("groupConversationActions()", app_controller_call_sources)
        self.assertNotIn("privateChatEventDependencies()", app_controller_call_sources)
        self.assertNotIn("groupConversationDependencies()", app_controller_call_sources)

    def test_group_conversation_adapters_use_generic_request_dispatch(self):
        app_sources = app_controller_cpp_sources_text()
        factory_sources = read(CHAT_EVENT_FACTORY_H) + "\n" + read(CHAT_EVENT_FACTORY_CPP)

        self.assertIn("controller.groupConversationDependencies(", factory_sources)
        self.assertIn("std::function<void(int, const QByteArray&)> dispatchPayload", factory_sources)
        self.assertIn("transport->slot_send_data(static_cast<ReqId>(", app_sources)
        self.assertNotIn("dispatchGroupChatPayload", app_sources + factory_sources)
        self.assertNotIn("dispatchGroupHistoryPayload", app_sources + factory_sources)
        self.assertNotIn("ID_GROUP_CHAT_MSG_REQ", app_sources + factory_sources)
        self.assertNotIn("ID_GROUP_HISTORY_REQ", app_sources + factory_sources)
        self.assertNotIn("groupConversationActions()", app_sources)

        service_header = read(FEATURES / "chat/services/GroupConversationService.h")
        service = group_conversation_service_source()
        self.assertIn("std::function<void(int, const QByteArray&)> dispatchPayload;", service_header)
        self.assertNotIn("dispatchGroupChatPayload", service_header + service)
        self.assertNotIn("dispatchGroupHistoryPayload", service_header + service)
        self.assertIn("constexpr int kGroupChatRequestId = 1044;", service)
        self.assertIn("constexpr int kGroupHistoryRequestId = 1047;", service)

    def test_current_message_dispatch_delegates_send_request_construction_to_chat_feature(self):
        dispatch = read(APP_CONTROLLER_DIR / "AppControllerMessageDispatch.cpp")
        chat_header = read(FEATURES / "chat/controller/ChatFeatureController.h")
        private_send = read(FEATURES / "chat/controller/ChatFeatureControllerPrivateSend.cpp")
        group_send = read(FEATURES / "chat/controller/ChatFeatureControllerGroupConversation.cpp")
        app_controller = read(APP_CONTROLLER_CPP) + "\n" + app_chat_binding_text()
        app_header = read(APP_CONTROLLER_H)
        registry_header = read(APP_PORT_REGISTRY_H)
        media_binder = read(APP / "composition/AppMediaPortBinder.cpp")

        self.assertIn("bool AppController::isChatTransportReady() const", dispatch)
        self.assertNotIn("bool AppController::dispatchChatContent", dispatch + app_header)
        self.assertNotIn("bool AppController::dispatchGroupChatContent", dispatch + app_header)
        self.assertNotIn("dispatchChatContent", registry_header)
        self.assertNotIn("dispatchGroupChatContent", registry_header)
        self.assertIn("struct ChatCurrentSendSnapshot", chat_header)
        self.assertIn("struct ChatCurrentSendPort", chat_header)
        self.assertIn("struct ChatContentDispatchPort", chat_header)
        self.assertIn("void setCurrentSendPort(ChatCurrentSendPort port);", chat_header)
        self.assertIn("void setContentDispatchPort(ChatContentDispatchPort port);", chat_header)
        self.assertIn("sendCurrentPrivateText(const QString& content, const QString& previewText)", chat_header)
        self.assertIn("sendCurrentGroupText(const QString& content, const QString& previewText)", chat_header)
        self.assertIn("dispatchCurrentPrivateContent(const QString& content, const QString& previewText)", chat_header)
        self.assertIn("dispatchCurrentGroupContent(const QString& content, const QString& previewText)", chat_header)
        self.assertIn("ChatFeatureController::sendCurrentPrivateText", private_send)
        self.assertIn("ChatFeatureController::dispatchCurrentPrivateContent", private_send)
        self.assertIn("sendCurrentPrivateText(content, previewText)", private_send)
        self.assertIn("PrivateChatSendRequest request;", private_send)
        self.assertIn("ChatFeatureController::sendCurrentGroupText", group_send)
        self.assertIn("ChatFeatureController::dispatchCurrentGroupContent", group_send)
        self.assertIn("sendCurrentGroupText(content, previewText)", group_send)
        self.assertIn("GroupSendRequest request;", group_send)
        send_binding = app_chat_binding_file_by_name("AppChatSendBinding.cpp")
        self.assertIn("ChatCurrentSendPort currentSendPort;", send_binding)
        self.assertIn("currentSendPort.snapshot = [this]()", send_binding)
        self.assertIn("ChatContentDispatchPort contentDispatchPort;", send_binding)
        self.assertIn("_features.chatFeatureController.setCurrentSendPort(std::move(currentSendPort));", app_controller)
        self.assertIn(
            "_features.chatFeatureController.setContentDispatchPort(std::move(contentDispatchPort));", app_controller
        )
        self.assertIn("_features.chatFeatureController.dispatchCurrentPrivateContent(content, preview)", send_binding)
        self.assertIn("_features.chatFeatureController.dispatchCurrentGroupContent(content, preview)", send_binding)
        self.assertIn("_features.chatFeatureController.dispatchCurrentPrivateContent(content, preview)", media_binder)
        self.assertIn("_features.chatFeatureController.dispatchCurrentGroupContent(content, preview)", media_binder)
        self.assertNotIn("void AppController::bindChatFeatureController()", read(APP_CONTROLLER_CPP))
        self.assertIn("void AppController::bindChatFeatureController()", read(APP_CONTROLLER_CHAT_BINDING))
        for leaked_builder in (
            "bindChatPrivateSendHooks",
            "buildChatCurrentSendPort",
            "buildChatUploadedAttachmentDispatchPort",
            "buildChatDialogNavigationPort",
            "buildChatGroupConversationPort",
            "buildChatPrivateCommandPort",
            "buildChatPendingAttachmentPort",
            "buildChatDialogRuntimePort",
            "buildChatShellIntentPort",
        ):
            with self.subTest(leaked_builder=leaked_builder):
                self.assertNotIn(leaked_builder, app_header)

        for forbidden in (
            "PrivateChatSendRequest",
            "PrivateChatSendDependencies",
            "GroupSendRequest",
            "ChatCurrentPrivateSendCommand",
            "ChatCurrentGroupSendCommand",
            "sendPrivateText(",
            "sendGroupText(",
            "replyToMsgId",
            "_private_cache_store",
            "groupConversationDependencies()",
            "groupConversationContext(",
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, dispatch + read(APP_CONTROLLER_CPP))

    def test_group_history_response_adapter_delegates_effects_to_chat_feature(self):
        source = read(APP_CONTROLLER_DIR / "AppControllerGroupHistoryResponses.cpp")
        body = extract_function(source, "void AppController::handleGroupHistoryRsp")

        self.assertIn("GroupHistoryResponsePort port;", body)
        self.assertIn("_features.chatFeatureController.handleGroupHistoryResponse(", body)
        self.assertIn(
            "memochat::app::makeGroupConversationDependencies(",
            body,
        )
        self.assertIn("_features.chatFeatureController,", body)
        self.assertIn("_gateway.userMgr(),", body)
        self.assertIn("_features.groupController.groupListModel(),", body)
        self.assertIn(
            "transport->slot_send_data(static_cast<ReqId>(reqId), payload);",
            normalized(body),
        )
        self.assertIn("port.setGroupHistoryLoading", body)
        self.assertIn("port.setPrivateHistoryLoading", body)
        self.assertIn("port.setCanLoadMorePrivateHistory", body)
        self.assertIn("port.sendGroupReadAck", body)
        self.assertIn("port.setGroupStatus", body)

        self.assertNotIn("_shell_state.loadingState().groupHistoryLoading = false", body)
        self.assertNotIn("setPrivateHistoryLoading(false)", body)
        self.assertNotIn("setCanLoadMorePrivateHistory(result.currentDialog && result.canLoadMore)", body)
        self.assertNotIn("if (result.currentDialog && result.success)", body)
        self.assertNotIn("sendGroupReadAck(groupId, result.requestedReadAckTs)", body)
        self.assertNotIn('setGroupStatus("历史消息已加载", false)', body)

    def test_current_history_load_more_adapter_delegates_policy_to_chat_feature(self):
        source = read(APP_CONTROLLER_DIR / "AppControllerPagination.cpp")
        binding = app_chat_binding_file_by_name("AppChatHistoryBinding.cpp")
        app_header = read(APP_CONTROLLER_H)
        chat_header = read(FEATURES / "chat/controller/ChatFeatureController.h")
        feature_private_history = read(FEATURES / "chat/controller/ChatFeatureControllerPrivateHistory.cpp")

        self.assertNotIn("void AppController::loadMorePrivateHistory", source + app_header)
        self.assertIn("ChatCurrentHistoryPort currentHistoryPort;", binding)
        self.assertIn("currentHistoryPort.snapshot = [this]()", binding)
        self.assertIn("ChatHistoryLoadMoreRequest request;", binding)
        self.assertIn("currentHistoryPort.privateDependencies = [this]()", binding)
        self.assertIn("currentHistoryPort.groupDependencies = [this]()", binding)
        self.assertIn("currentHistoryPort.groupRequestPort.setGroupHistoryLoading", binding)
        self.assertIn("_features.chatFeatureController.setCurrentHistoryPort(std::move(currentHistoryPort));", binding)
        self.assertIn("struct ChatCurrentHistoryPort", chat_header)
        self.assertIn("void setCurrentHistoryPort(ChatCurrentHistoryPort port);", chat_header)
        self.assertIn("ChatHistoryLoadMoreResult loadMoreCurrentHistory();", chat_header)
        self.assertIn("ChatFeatureController::loadMoreCurrentHistory()", feature_private_history)

        self.assertNotRegex(source, r"if\s*\([^)]*_shell_state\.loadingState\(\)\.privateHistoryLoading[^)]*\)\s*\{")
        self.assertNotRegex(
            source, r"if\s*\([^)]*_shell_state\.loadingState\(\)\.canLoadMorePrivateHistory[^)]*\)\s*\{"
        )
        self.assertNotIn("if (currentGroupId() > 0)", source)
        self.assertNotIn("loadGroupHistory();", source)
        self.assertNotIn("PrivateHistoryLoadMoreRequest", source)
        self.assertNotIn("buildPrivateHistoryLoadMoreRequest", source)

    def test_chat_list_paging_adapters_delegate_policy_to_chat_feature(self):
        navigation = read(APP_CONTROLLER_DIR / "AppControllerNavigation.cpp")
        pagination = read(APP_CONTROLLER_DIR / "AppControllerPagination.cpp")
        binding = app_chat_binding_file_by_name("AppChatDialogBinding.cpp")
        feature_header = read(FEATURES / "chat/controller/ChatFeatureController.h")
        feature_controller = read(FEATURES / "chat/controller/ChatFeatureController.cpp")

        ensure_body = extract_function(navigation, "void AppController::ensureChatListInitialized")
        load_more_body = extract_function(pagination, "void AppController::loadMoreChats")

        self.assertIn("_features.chatFeatureController.ensureChatListInitialized();", ensure_body)
        self.assertIn("_features.chatFeatureController.loadMoreChats();", load_more_body)
        self.assertIn("struct ChatListPagingPort", feature_header)
        self.assertIn("void setChatListPagingPort(ChatListPagingPort port);", feature_header)
        self.assertIn("ChatListPagingResult ensureChatListInitialized();", feature_header)
        self.assertIn("ChatListPagingResult loadMoreChats();", feature_header)
        self.assertIn("ChatListPagingPort listPagingPort;", binding)
        self.assertIn("listPagingPort.snapshot = [this]()", binding)
        self.assertIn("_features.chatFeatureController.setChatListPagingPort(std::move(listPagingPort));", binding)
        self.assertIn("ChatFeatureController::ensureChatListInitialized", feature_controller)
        self.assertIn("ChatFeatureController::loadMoreChats", feature_controller)

        for body_name, body in (("ensure", ensure_body), ("load_more", load_more_body)):
            with self.subTest(body=body_name):
                self.assertNotIn("GetChatListPerPage", body)
                self.assertNotIn("UpdateChatLoadedCount", body)
                self.assertNotIn("IsLoadChatFin", body)
                self.assertNotIn("chatListModel().clear", body)
                self.assertNotIn("chatListModel().setFriends", body)
                self.assertNotIn("chatListModel().appendFriends", body)
                self.assertNotIn("refreshChatLoadMoreState();", body)
                self.assertNotIn("flushIncomingMessageRouter();", body)

    def test_group_conversation_clear_adapter_delegates_policy_to_chat_feature(self):
        dialog_state = read(APP_CONTROLLER_DIR / "AppControllerDialogState.cpp")
        body = extract_function(dialog_state, "void AppController::clearCurrentGroupConversation")
        feature_header = read(FEATURES / "chat/controller/ChatFeatureController.h")
        feature_group = read(FEATURES / "chat/controller/ChatFeatureControllerGroupConversation.cpp")

        self.assertIn("ChatGroupConversationClearRequest request;", body)
        self.assertIn("ChatGroupConversationClearPort port;", body)
        self.assertIn("_features.chatFeatureController.clearGroupConversation(request, port);", body)
        self.assertIn("port.removeGroupByDialogUid = [this](int dialogUid)", body)
        self.assertIn("port.removeGroupDialogMapping = [this](int dialogUid)", body)
        self.assertIn("port.clearMessageModel = [this]()", body)
        self.assertIn("port.resetCurrentPeer = [this]()", body)
        self.assertIn("struct ChatGroupConversationClearPort", feature_header)
        self.assertIn("ChatFeatureController::clearGroupConversation", feature_group)

        for forbidden in (
            "const qint64 targetGroupId",
            "_features.chatFeatureController.removeMentionForDialog",
            "_features.chatFeatureController.dialogListModel().clearMention",
            "_features.chatFeatureController.dialogListModel().removeByUid",
            "resetGroupConversationFeatureState();",
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, body)

    def test_private_history_dependency_factory_maps_app_actions_without_appcontroller_handle(self):
        app_cmake = read(APP / "sources.cmake")
        app_header = read(APP_CONTROLLER_H)
        app_sources = (
            app_header
            + "\n"
            + "\n".join(
                read(path) for path in sorted(APP_CONTROLLER_DIR.glob("AppController*")) if path.suffix in CPP_SUFFIXES
            )
        )
        events = read(APP_CONTROLLER_DIR / "AppControllerPrivateEvents.cpp")
        history = read(APP_CONTROLLER_DIR / "AppControllerPrivateHistory.cpp")
        pagination = read(APP_CONTROLLER_DIR / "AppControllerPagination.cpp")
        feature_cmake = read(FEATURES / "chat/sources.cmake")
        feature_private_history = read(FEATURES / "chat/controller/ChatFeatureControllerPrivateHistory.cpp")
        request_service_header = read(FEATURES / "chat/services/PrivateChatHistoryRequestService.h")
        request_service = read(FEATURES / "chat/services/PrivateChatHistoryRequestService.cpp")

        self.assertTrue(PRIVATE_HISTORY_FACTORY_H.exists())
        self.assertTrue(PRIVATE_HISTORY_FACTORY_CPP.exists())
        self.assertIn("app/controller/PrivateHistoryDependenciesFactory.cpp", app_cmake)
        self.assertIn("app/controller/PrivateHistoryDependenciesFactory.h", app_cmake)

        factory_header = read(PRIVATE_HISTORY_FACTORY_H)
        factory_cpp = read(PRIVATE_HISTORY_FACTORY_CPP)
        factory_sources = factory_header + "\n" + factory_cpp

        self.assertIn("namespace memochat::app", factory_sources)
        self.assertIn("ChatFeatureController& controller", factory_header)
        self.assertIn("std::shared_ptr<UserMgr> userMgr", factory_header)
        self.assertIn("makePrivateHistoryLoadCurrentDependencies(", factory_sources)
        self.assertIn("makePrivateHistoryRequestBuildDependencies(", factory_sources)
        self.assertIn("makePrivateHistoryLoadMoreDependencies(", factory_sources)
        self.assertIn("controller.privateHistoryLoadCurrentDependencies(", factory_cpp)
        self.assertIn("dependencies.messageModel = &controller.messageModel();", factory_cpp)
        self.assertNotIn("struct PrivateHistoryLoadCurrentActions", factory_header)
        self.assertNotIn("struct PrivateHistoryRequestBuildActions", factory_header)
        self.assertNotIn("struct PrivateHistoryLoadMoreActions", factory_header)
        self.assertNotIn("PrivateHistoryLoadCurrentActions actions", factory_sources)
        self.assertNotIn("PrivateHistoryRequestBuildActions actions", factory_sources)
        self.assertNotIn("PrivateHistoryLoadMoreActions actions", factory_sources)

        for forbidden in (
            '#include "AppController.h"',
            "#include <AppController.h>",
            "AppController",
            "AppController::",
            "AppController&",
            "AppController*",
            "appController",
        ):
            with self.subTest(factory_forbidden=forbidden):
                self.assertNotIn(forbidden, factory_sources)

        self.assertIn("std::function<void(const QString&)> setCurrentPeerName", factory_header)
        self.assertIn("std::function<void(const QString&)> setCurrentPeerIcon", factory_header)
        self.assertIn("std::function<void(bool)> setLoading", factory_header)
        self.assertIn("std::function<void(bool)> setCanLoadMore", factory_header)
        self.assertIn("std::function<void(int, qint64)> requestReadAck", factory_header)
        self.assertIn("dependencies.dispatchPayload = std::move(dispatchPayload);", factory_cpp)

        for old_builder in (
            "PrivateHistoryLoadCurrentDependencies AppController::privateHistoryLoadCurrentDependencies(",
            "PrivateHistoryRequestBuildDependencies AppController::privateHistoryRequestBuildDependencies(",
            "PrivateHistoryLoadMoreDependencies AppController::privateHistoryLoadMoreDependencies(",
        ):
            with self.subTest(old_builder=old_builder):
                self.assertNotIn(old_builder, app_sources)

        for action_builder in (
            "memochat::app::PrivateHistoryLoadCurrentActions privateHistoryLoadCurrentActions();",
            "memochat::app::PrivateHistoryRequestBuildActions privateHistoryRequestBuildActions();",
            "memochat::app::PrivateHistoryLoadMoreActions privateHistoryLoadMoreActions();",
            "memochat::app::PrivateHistoryLoadCurrentActions AppController::privateHistoryLoadCurrentActions()",
            "memochat::app::PrivateHistoryRequestBuildActions AppController::privateHistoryRequestBuildActions()",
            "memochat::app::PrivateHistoryLoadMoreActions AppController::privateHistoryLoadMoreActions()",
        ):
            with self.subTest(action_builder=action_builder):
                self.assertNotIn(action_builder, app_sources)

        load_current_body = extract_function(history, "void AppController::loadCurrentChatMessages")
        request_body = extract_function(history, "void AppController::requestPrivateHistory")
        bootstrap_body = extract_function(history, "void AppController::requestPrivateHistoryForBootstrap")
        chat_dialog_binding = app_chat_binding_file_by_name("AppChatHistoryBinding.cpp")
        self.assertIn(
            "memochat::app::makePrivateHistoryLoadCurrentDependencies(\n            _features.chatFeatureController",
            load_current_body,
        )
        self.assertIn(
            "memochat::app::makePrivateHistoryRequestBuildDependencies(",
            request_body,
        )
        self.assertIn(
            "memochat::app::makePrivateHistoryLoadMoreDependencies(\n            _features.chatFeatureController",
            chat_dialog_binding,
        )
        self.assertNotIn("privateHistoryRequestBuildActions()", bootstrap_body)
        self.assertIn("dependencies.dispatchPayload = [this](int reqId, const QByteArray& payload)", bootstrap_body)
        self.assertIn("transport->slot_send_data(static_cast<ReqId>(reqId), payload);", bootstrap_body)
        self.assertNotIn("ID_PRIVATE_HISTORY_REQ", events + history + pagination)

        self.assertIn("features/chat/controller/ChatFeatureControllerPrivateHistory.cpp", feature_cmake)
        self.assertIn("features/chat/services/PrivateChatHistoryRequestService.cpp", feature_cmake)
        self.assertIn("features/chat/services/PrivateChatHistoryRequestService.h", feature_cmake)
        self.assertIn("ChatFeatureController::loadCurrentPrivateHistory", feature_private_history)
        self.assertIn("ChatFeatureController::buildPrivateHistoryRequest", feature_private_history)
        self.assertIn("ChatFeatureController::loadMoreCurrentHistory", feature_private_history)
        self.assertIn("PrivateChatHistoryRequestService::loadCurrent(request, dependencies)", feature_private_history)
        self.assertIn("PrivateChatHistoryRequestService::buildRequest(request, dependencies)", feature_private_history)
        self.assertIn(
            "PrivateChatHistoryRequestService::buildLoadMoreRequest(request, dependencies)", feature_private_history
        )
        self.assertIn("std::function<void(int, const QByteArray&)> dispatchPayload;", request_service_header)
        self.assertIn("constexpr int kPrivateHistoryRequestId = 1059;", request_service)
        self.assertIn(
            "dispatchIfAvailable(kPrivateHistoryRequestId, result.compactPayload",
            normalized(request_service),
        )
        self.assertNotIn("PrivateChatHistoryRequestService::", events + history + pagination)

    def test_read_ack_send_adapters_delegate_payload_policy_to_chat_feature(self):
        app_header = read(APP_CONTROLLER_H)
        app_sources = app_controller_cpp_sources_text()
        binding = app_chat_binding_file_by_name("AppChatReadMutationBinding.cpp")
        read_ack_header = read(FEATURES / "chat/controller/ChatReadAckCommand.h")
        feature_header = read(FEATURES / "chat/controller/ChatFeatureController.h")
        feature_private = read(FEATURES / "chat/controller/ChatFeatureControllerPrivateHistory.cpp")
        private_event_service = private_chat_event_service_source()
        feature_group = read(FEATURES / "chat/controller/ChatFeatureControllerGroupConversation.cpp")

        self.assertFalse((APP_CONTROLLER_DIR / "AppControllerReadAcks.cpp").exists())
        self.assertNotIn("sendGroupReadAck(qint64 groupId", app_header)
        self.assertNotIn("sendPrivateReadAck(int peerUid", app_header)
        self.assertIn("struct ChatReadAckPort", read_ack_header)
        self.assertIn("void setReadAckPort(ChatReadAckPort port);", feature_header)
        self.assertIn("sendPrivateReadAckForPeer(int peerUid, qint64 readTs = 0)", feature_header)
        self.assertIn("sendGroupReadAckForGroup(qint64 groupId, qint64 readTs = 0)", feature_header)
        self.assertIn("ChatReadAckPort readAckPort;", binding)
        self.assertIn("readAckPort.selfUid = [this]()", binding)
        self.assertIn("readAckPort.dispatch.dispatchPayload = [this](int reqId, const QByteArray& payload)", binding)
        self.assertIn("_features.chatFeatureController.setReadAckPort(std::move(readAckPort));", binding)
        self.assertIn("transport->slot_send_data(static_cast<ReqId>(reqId), payload);", binding)
        self.assertIn("_features.chatFeatureController.sendPrivateReadAckForPeer(peerUid, readTs)", app_sources)
        self.assertIn("_features.chatFeatureController.sendGroupReadAckForGroup(groupId, readTs)", app_sources)

        for forbidden in (
            "buildReadAckPayload",
            "ID_GROUP_READ_ACK_REQ",
            "ID_PRIVATE_READ_ACK_REQ",
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, app_sources)

        self.assertIn("PrivateChatEventService::buildReadAckPayload", private_event_service)
        self.assertIn("GroupConversationService::buildReadAckPayload", feature_group)
        self.assertIn("constexpr int kPrivateReadAckRequestId = 1076;", feature_private)
        self.assertIn("constexpr int kGroupReadAckRequestId = 1071;", feature_group)

    def test_mark_dialog_read_adapter_uses_generic_read_ack_dispatch_port(self):
        binding = app_chat_binding_text()
        app_header = read(APP_CONTROLLER_H)
        dialog_state = read(APP_CONTROLLER_DIR / "AppControllerDialogState.cpp")

        self.assertFalse((APP_CONTROLLER_DIR / "AppControllerDialogCommands.cpp").exists())
        self.assertNotIn("markDialogReadDependencies", app_header + binding)
        self.assertNotIn("void markDialogReadByUid(int dialogUid);", app_header)
        self.assertNotIn("latestPrivatePeerCreatedAt", app_header + dialog_state + binding)
        self.assertNotIn("latestGroupCreatedAt", app_header + dialog_state + binding)
        self.assertIn("ChatDialogCommandPort commandPort;", binding)
        self.assertIn("commandPort.privatePeerById = [this](int peerUid)", binding)
        self.assertIn("return _gateway.userMgr()->GetFriendById(peerUid);", binding)
        self.assertIn("commandPort.groupById = [this](qint64 groupId)", binding)
        self.assertIn("return _gateway.userMgr()->GetGroupById(groupId);", binding)
        self.assertIn("commandPort.groupListModel = _features.groupController.groupListModel();", binding)
        self.assertIn("commandPort.dispatchPayload = [this](int reqId, const QByteArray& payload)", binding)
        self.assertIn("_features.chatFeatureController.setDialogCommandPort(std::move(commandPort));", binding)
        self.assertIn("_features.chatFeatureController.setReadAckPort(std::move(readAckPort));", binding)

        runtime_header = read(FEATURES / "chat/controller/ChatDialogRuntimeState.h")
        feature_header = read(FEATURES / "chat/controller/ChatFeatureController.h")
        runtime = chat_dialog_runtime_source()
        feature_bindings = read(FEATURES / "chat/controller/ChatFeatureController.cpp")
        self.assertIn("ChatReadAckDispatchPort readAckDispatchPort;", runtime_header)
        self.assertIn("FriendListModel* groupListModel = nullptr;", runtime_header)
        self.assertIn("ChatMarkDialogReadResult markDialogReadByUid(int dialogUid);", feature_header)
        self.assertIn("markDialogReadByUid(dialogUid);", feature_bindings)
        self.assertIn("markReadDependenciesFromPort", runtime)
        self.assertIn("dependencies.readAckDispatchPort = readAckPort.dispatch;", runtime)
        self.assertIn("qint64 latestPrivatePeerCreatedAt(const ChatDialogCommandPort& port, int peerUid)", runtime)
        self.assertIn("qint64 latestGroupCreatedAt(const ChatDialogCommandPort& port, qint64 groupId)", runtime)
        self.assertIn("port.privatePeerById(peerUid)", runtime)
        self.assertIn("port.groupById(groupId)", runtime)
        self.assertIn(
            "request.latestPrivatePeerTs = latestPrivatePeerCreatedAt(_dialogCommandPort, request.target.peerUid)",
            runtime,
        )
        self.assertIn(
            "request.latestGroupTs = latestGroupCreatedAt(_dialogCommandPort, request.target.groupId)",
            runtime,
        )
        self.assertNotIn("dispatchPrivateReadAckPayload", runtime_header + runtime)
        self.assertNotIn("dispatchGroupReadAckPayload", runtime_header + runtime)
        self.assertIn("sendPrivateReadAck(readAckRequest, readAckPort)", runtime)
        self.assertIn("sendGroupReadAck(readAckRequest, readAckPort)", runtime)

    def test_dialog_command_adapter_uses_generic_request_dispatch(self):
        binding = app_chat_binding_file_by_name("AppChatDialogBinding.cpp")
        app_header = read(APP_CONTROLLER_H)

        self.assertFalse((APP_CONTROLLER_DIR / "AppControllerDialogCommands.cpp").exists())
        self.assertNotIn("dialogCommandDependencies", app_header + binding)
        self.assertIn("ChatDialogCommandPort commandPort;", binding)
        self.assertIn("ChatDialogCommandSnapshot snapshot;", binding)
        self.assertIn("commandPort.targetForDialogUid = [this](int dialogUid)", binding)
        self.assertIn("commandPort.dispatchPayload = [this](int reqId, const QByteArray& payload)", binding)
        self.assertIn("commandPort.savePersistentDialogStore = [this](int ownerUid)", binding)
        self.assertIn("commandPort.refreshDialogModel = [this]()", binding)
        self.assertIn("_features.chatFeatureController.setDialogCommandPort(std::move(commandPort));", binding)
        self.assertIn("transport->slot_send_data(static_cast<ReqId>(reqId), payload);", binding)

        runtime_header = read(FEATURES / "chat/controller/ChatDialogRuntimeState.h")
        feature_header = read(FEATURES / "chat/controller/ChatFeatureController.h")
        feature_bindings = read(FEATURES / "chat/controller/ChatFeatureController.cpp")
        runtime = chat_dialog_runtime_source()
        self.assertIn("struct ChatDialogCommandSnapshot", runtime_header)
        self.assertIn("struct ChatDialogCommandPort", runtime_header)
        self.assertIn("std::function<void(int, const QByteArray&)> dispatchPayload;", runtime_header)
        self.assertIn("void setDialogCommandPort(ChatDialogCommandPort port);", feature_header)
        self.assertIn("ChatDialogCommandResult updateCurrentDraft(const QString& text);", feature_header)
        self.assertIn("ChatDialogCommandResult toggleCurrentDialogPinned();", feature_header)
        self.assertIn("ChatDialogCommandResult toggleCurrentDialogMuted();", feature_header)
        self.assertIn("ChatDialogCommandResult clearDialogDraftByUid(int dialogUid);", feature_header)
        self.assertIn("updateCurrentDraft(text);", feature_bindings)
        self.assertIn("toggleCurrentDialogPinned();", feature_bindings)
        self.assertIn("toggleCurrentDialogMuted();", feature_bindings)
        self.assertIn("toggleDialogPinnedByUid(dialogUid);", feature_bindings)
        self.assertIn("toggleDialogMutedByUid(dialogUid);", feature_bindings)
        self.assertIn("clearDialogDraftByUid(dialogUid);", feature_bindings)
        self.assertIn("commandDependenciesFromPort", runtime)
        self.assertIn("return updateDialogDraft(request, commandDependenciesFromPort(_dialogCommandPort));", runtime)
        self.assertIn("return toggleDialogPinned(request, commandDependenciesFromPort(_dialogCommandPort));", runtime)
        self.assertIn("return toggleDialogMuted(request, commandDependenciesFromPort(_dialogCommandPort));", runtime)
        self.assertNotIn("dispatchDraftSyncPayload", runtime_header + runtime)
        self.assertNotIn("dispatchPinDialogPayload", runtime_header + runtime)
        self.assertIn("constexpr int kSyncDraftRequestId = 1072;", runtime)
        self.assertIn("constexpr int kPinDialogRequestId = 1074;", runtime)
        self.assertIn("dispatchDialogPayload(kSyncDraftRequestId, result.compactPayload, dependencies)", runtime)
        self.assertIn("dispatchDialogPayload(kPinDialogRequestId, result.compactPayload, dependencies)", runtime)

    def test_dialog_meta_response_adapter_delegates_effects_to_chat_feature(self):
        source = read(APP_CONTROLLER_DIR / "AppControllerDialogMetaResponses.cpp")
        body = extract_function(source, "void AppController::handleDialogMetaRsp")

        self.assertIn("ChatDialogMetaResponseRequest request;", body)
        self.assertIn("ChatDialogMetaResponseDependencies dependencies;", body)
        self.assertIn("_features.chatFeatureController.handleDialogMetaResponse(request, dependencies);", body)
        for forbidden in (
            "ID_SYNC_DRAFT_RSP",
            "ID_PIN_DIALOG_RSP",
            "applyRemoteDraftMeta",
            "applyRemotePinnedMeta",
            "setDialogMeta",
            "applyDraftToDialogModel",
            'payload.value("draft_text")',
            'payload.value("mute_state")',
            'payload.value("pinned_rank")',
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, body)

        runtime_header = read(FEATURES / "chat/controller/ChatDialogRuntimeState.h")
        runtime = chat_dialog_runtime_source()
        self.assertIn("struct ChatDialogMetaResponseRequest", runtime_header)
        self.assertIn("struct ChatDialogMetaResponseDependencies", runtime_header)
        self.assertIn("ChatFeatureController::handleDialogMetaResponse", runtime)
        self.assertIn("constexpr int kSyncDraftResponseId = 1073;", runtime)
        self.assertIn("constexpr int kPinDialogResponseId = 1075;", runtime)
        self.assertIn("applyDraftToDialogModel(*dependencies.dialogListModel", runtime)

    def test_app_layer_does_not_directly_mutate_chat_dialog_models_for_logout_or_drafts(self):
        app_sources = "\n".join(
            read(path)
            for root in (APP_CONTROLLER_DIR, APP / "composition", APP / "coordinators", SESSION_DIR)
            for path in sorted(root.rglob("*"))
            if path.suffix in CPP_SUFFIXES
        )
        feature_header = read(FEATURES / "chat/controller/ChatFeatureController.h")
        feature_controller = read(FEATURES / "chat/controller/ChatFeatureController.cpp")

        self.assertIn("void clearConversationLists();", feature_header)
        self.assertIn("void resetModelsForLogout();", feature_header)
        self.assertIn("void resetRuntimeForLogout();", feature_header)
        self.assertIn("ChatFeatureController::clearConversationLists", feature_controller)
        self.assertIn("ChatFeatureController::resetModelsForLogout", feature_controller)
        self.assertIn("ChatFeatureController::resetRuntimeForLogout", feature_controller)
        self.assertIn("_features.chatFeatureController.resetModelsForLogout();", app_sources)
        self.assertIn("_features.chatFeatureController.resetRuntimeForLogout();", app_sources)
        for forbidden in (
            "_features.chatFeatureController.chatListModel().clear",
            "_features.chatFeatureController.dialogListModel().clear",
            "_features.chatFeatureController.dialogListModel().setDialogMeta",
            "_features.chatFeatureController.dialogListModel().get",
            "_features.chatFeatureController.dialogListModel().indexOfUid",
            "clearChatListModel",
            "clearDialogListModel",
            "void AppController::applyDraftToDialogModel",
            "void applyDraftToDialogModel(int dialogUid",
        ):
            with self.subTest(forbidden_app_chat_model_mutation=forbidden):
                self.assertNotIn(forbidden, app_sources)

    def test_protocol_demux_routes_are_feature_owned(self):
        group_response_handlers = read(APP_CONTROLLER_DIR / "AppControllerGroupResponses.cpp")
        message_status = read(APP / "events/AppChatDispatcherEventRouter.cpp")
        chat_dispatcher_router = read(APP / "events/AppChatDispatcherEventRouter.cpp")
        group_events = read(APP_CONTROLLER_DIR / "AppControllerGroupEvents.cpp")
        router_header = read(FEATURES / "chat/services/ChatProtocolRouter.h")
        router_source = read(FEATURES / "chat/services/ChatProtocolRouter.cpp")
        chat_manifest = read(FEATURES / "chat/sources.cmake")

        self.assertIn("features/chat/services/ChatProtocolRouter.cpp", chat_manifest)
        self.assertIn("features/chat/services/ChatProtocolRouter.h", chat_manifest)
        self.assertIn("class ChatProtocolRouter", router_header)
        self.assertIn("enum class GroupResponseRoute", router_header)
        self.assertIn("enum class MessageStatusRoute", router_header)
        self.assertIn("enum class GroupMemberEventRoute", router_header)
        self.assertIn("ChatProtocolRouter::routeGroupResponse", router_source)
        self.assertIn("ChatProtocolRouter::routeMessageStatus", router_source)
        self.assertIn("ChatProtocolRouter::routeGroupMemberEvent", router_source)

        group_rsp_body = extract_function(chat_dispatcher_router, "void AppChatDispatcherEventRouter::onGroupRsp")
        status_body = extract_function(message_status, "void AppChatDispatcherEventRouter::onMessageStatus")
        group_event_body = extract_function(
            chat_dispatcher_router, "void AppChatDispatcherEventRouter::onGroupMemberChanged"
        )

        self.assertIn("ChatProtocolRouter::routeGroupResponse(reqId)", group_rsp_body)
        self.assertIn("GroupResponseRoute::Management", group_rsp_body)
        self.assertIn("_group_response_handlers.handleManagement", group_rsp_body)
        self.assertIn("handlers.handleManagement", group_response_handlers)
        self.assertIn("ChatProtocolRouter::routeMessageStatus(payload)", status_body)
        self.assertIn("MessageStatusRoute::GroupMessageStatus", status_body)
        self.assertIn("_chat_controller.handleGroupMessageStatus", status_body)
        self.assertIn("_chat_controller.handlePrivateMessageStatus", status_body)
        self.assertIn("ChatProtocolRouter::routeGroupMemberEvent(payload)", group_event_body)
        self.assertIn("GroupMemberEventRoute::GroupReadAck", group_event_body)
        self.assertIn("GroupMemberEventRoute::GroupMessageMutation", group_event_body)
        self.assertIn("_chat_controller.handleGroupReadAckEvent", group_event_body)
        self.assertIn("_chat_controller.handleGroupMessageMutation", group_event_body)
        self.assertIn("GroupManagementEventService::reduceGroupMemberChanged", group_event_body)
        self.assertIn("_group_controller.applyGroupManagementEffect", group_event_body)
        self.assertIn(
            "makeGroupManagementEventEffectPort(_make_group_management_event_effect_actions())", group_event_body
        )

        for forbidden in (
            "case ID_CREATE_GROUP_RSP",
            "case ID_GROUP_HISTORY_RSP",
            "case ID_EDIT_GROUP_MSG_RSP",
            "case ID_EDIT_PRIVATE_MSG_RSP",
            "case ID_FORWARD_PRIVATE_MSG_RSP",
            "case ID_GROUP_CHAT_MSG_RSP",
            "case ID_SYNC_DRAFT_RSP",
            "case ID_PIN_DIALOG_RSP",
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, group_rsp_body)

        for forbidden in (
            'payload.value("scope")',
            'payload.value("client_msg_id")',
            'payload.value("groupid")',
            'payload.value(QStringLiteral("scope"))',
            'payload.value(QStringLiteral("client_msg_id"))',
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, status_body)

        for forbidden in (
            'event == "group_read_ack"',
            'event == "group_msg_edited"',
            'event == "group_msg_revoked"',
            "const QString event =",
            'payload.value("groupid")',
            'payload.value("fromuid")',
            'payload.value(QStringLiteral("groupid"))',
            'payload.value(QStringLiteral("fromuid"))',
            'payload.value(QStringLiteral("event"))',
            "request.event =",
            "readerUid ==",
            "groupId <= 0",
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, group_event_body)

        group_service = group_conversation_service_source()
        read_ack_body = extract_function(
            group_service, "GroupReadAckResult GroupConversationService::handleReadAckEvent"
        )
        mutation_body = extract_function(
            group_service, "GroupMutationResult GroupConversationService::handleMessageMutation"
        )
        self.assertIn('request.payload.value(QStringLiteral("groupid"))', read_ack_body)
        self.assertIn('request.payload.value(QStringLiteral("fromuid"))', read_ack_body)
        self.assertIn("result.readerUid == request.context.selfUid", read_ack_body)
        self.assertIn('request.payload.value(QStringLiteral("event")).toString()', mutation_body)


if __name__ == "__main__":
    unittest.main()
