import unittest

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
APP = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
HEADER = APP / "app/controller/AppController.h"
SOURCES = APP / "app/sources.cmake"
FEATURE_SOURCES = APP / "features/chat/sources.cmake"
BUFFER = APP / "app/controller/AppControllerIncomingBuffer.cpp"
PRIVATE = APP / "app/controller/AppControllerPrivateEvents.cpp"
GROUP = APP / "app/controller/AppControllerGroupEvents.cpp"
CHAT_DISPATCHER_ROUTER = APP / "app/events/AppChatDispatcherEventRouter.cpp"
FACTORY_H = APP / "app/controller/IncomingMessageRouterFactory.h"
FACTORY_CPP = APP / "app/controller/IncomingMessageRouterFactory.cpp"
DIALOG_PORTS = APP / "app/controller/AppControllerDialogListPorts.cpp"
NAV = APP / "app/controller/AppControllerNavigation.cpp"
SESSION_LOGOUT = APP / "app/session/AppSessionCoordinatorLogout.cpp"
CHAT_ENTRY = APP / "app/session/SessionChatEntryCoordinator.cpp"
RELATION = APP / "app/session/SessionRelationBootstrap.cpp"
APP_COORDINATORS = APP / "app/coordinators/AppCoordinators.h"
ROUTER_H = APP / "features/chat/services/IncomingMessageRouter.h"
ROUTER_CPP = APP / "features/chat/services/IncomingMessageRouter.cpp"
CHAT_FEATURE_H = APP / "features/chat/controller/ChatFeatureController.h"
CHAT_FEATURE_INCOMING = APP / "features/chat/controller/ChatFeatureControllerIncoming.cpp"
PRIVATE_EVENT_SERVICE = APP / "features/chat/services/PrivateChatEventService.cpp"
GROUP_CONVERSATION_SERVICE = APP / "features/chat/services/GroupConversationService.cpp"


def read(path):
    return path.read_text(encoding="utf-8")


def private_chat_event_service_source() -> str:
    service_dir = APP / "features/chat/services"
    files = (
        "PrivateChatEventServiceInternal.h",
        "PrivateChatEventService.cpp",
        "PrivateChatReadStatusService.cpp",
        "PrivateChatResponseService.cpp",
    )
    return "\n".join(read(service_dir / name) for name in files)


def group_conversation_service_source() -> str:
    service_dir = APP / "features/chat/services"
    files = (
        "GroupConversationServiceInternal.h",
        "GroupConversationService.cpp",
        "GroupConversationAckService.cpp",
        "GroupConversationHistoryService.cpp",
        "GroupConversationIncomingService.cpp",
        "GroupConversationMutationService.cpp",
    )
    return "\n".join(read(service_dir / name) for name in files)


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


def extract_struct(source: str, struct_name: str) -> str:
    start = source.index(f"struct {struct_name}")
    end = source.index("\n};", start)
    return source[start : end + 3]


class IncomingMessageBufferContractTests(unittest.TestCase):
    def test_incoming_buffer_state_is_chat_owned(self):
        header = HEADER.read_text(encoding="utf-8")
        app_sources = SOURCES.read_text(encoding="utf-8")
        feature_sources = FEATURE_SOURCES.read_text(encoding="utf-8")
        router_h = ROUTER_H.read_text(encoding="utf-8")
        chat_feature_h = CHAT_FEATURE_H.read_text(encoding="utf-8")

        self.assertNotIn("AppIncomingMessageBufferState _incoming_buffer_state;", header)
        for declaration in (
            "bool shouldBufferIncomingMessages() const;",
            "bool bufferIncomingPrivateMessage(std::shared_ptr<TextChatMsg> msg);",
            "bool bufferIncomingGroupMessage(std::shared_ptr<GroupChatMsg> msg);",
            "void flushPendingIncomingMessages();",
            "void clearPendingIncomingMessages();",
        ):
            self.assertNotIn(declaration, header)

        for declaration in (
            "IncomingMessageRouterReadiness incomingMessageReadiness() const;",
            "IncomingMessageRouterDispatch incomingMessageDispatch();",
        ):
            self.assertNotIn(declaration, header)

        for declaration in (
            "memochat::app::IncomingMessageRouterReadinessInputs incomingMessageReadinessInputs() const;",
            "memochat::app::IncomingMessageRouterDispatchActions incomingMessageDispatchActions();",
            "void flushIncomingMessageRouter();",
            "void clearIncomingMessageRouter();",
            "void applyTextChatMsg(std::shared_ptr<TextChatMsg> msg);",
            "void applyGroupChatMsg(std::shared_ptr<GroupChatMsg> msg);",
        ):
            self.assertIn(declaration, header)
        for declaration in (
            "void onTextChatMsg(std::shared_ptr<TextChatMsg> msg);",
            "void onPrivateMsgChanged(QJsonObject payload);",
            "void onPrivateReadAck(QJsonObject payload);",
        ):
            self.assertNotIn(declaration, header)

        self.assertIn("class IncomingMessageRouter", router_h)
        self.assertIn("QVector<PendingIncomingMessage> _messages", router_h)
        self.assertIn("QSet<QString> _keys", router_h)
        self.assertIn("qint64 _nextSequence = 0", router_h)
        self.assertIn("static constexpr qsizetype maxMessages = 1000", router_h)
        self.assertIn("IncomingMessageRouter _incomingMessageRouter;", chat_feature_h)
        self.assertIn("app/controller/AppControllerIncomingBuffer.cpp", app_sources)
        self.assertIn("app/controller/IncomingMessageRouterFactory.cpp", app_sources)
        self.assertIn("app/controller/IncomingMessageRouterFactory.h", app_sources)
        self.assertIn("features/chat/services/IncomingMessageRouter.cpp", feature_sources)
        self.assertIn("features/chat/controller/ChatFeatureControllerIncoming.cpp", feature_sources)

    def test_incoming_router_factory_maps_app_inputs_without_appcontroller_handle(self):
        factory_header = FACTORY_H.read_text(encoding="utf-8")
        factory_cpp = FACTORY_CPP.read_text(encoding="utf-8")
        factory_sources = factory_header + "\n" + factory_cpp
        buffer_source = BUFFER.read_text(encoding="utf-8")

        self.assertIn("struct IncomingMessageRouterReadinessInputs", factory_header)
        self.assertIn("struct IncomingMessageRouterDispatchActions", factory_header)
        self.assertIn(
            "IncomingMessageRouterReadiness makeIncomingMessageRouterReadiness(IncomingMessageRouterReadinessInputs inputs);",
            factory_header,
        )
        self.assertIn(
            "IncomingMessageRouterDispatch makeIncomingMessageRouterDispatch(IncomingMessageRouterDispatchActions actions);",
            factory_header,
        )
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

        self.assertIn("readiness.onChatPage = inputs.onChatPage;", factory_cpp)
        self.assertIn("readiness.dialogsReady = inputs.dialogsReady;", factory_cpp)
        self.assertIn("readiness.chatListInitialized = inputs.chatListInitialized;", factory_cpp)
        self.assertIn("dispatch.applyPrivateMessage = std::move(actions.applyPrivateMessage);", factory_cpp)
        self.assertIn("dispatch.applyGroupMessage = std::move(actions.applyGroupMessage);", factory_cpp)
        self.assertNotIn("IncomingMessageRouterReadiness AppController::incomingMessageReadiness", buffer_source)
        self.assertNotIn("IncomingMessageRouterDispatch AppController::incomingMessageDispatch", buffer_source)
        self.assertIn(
            "memochat::app::IncomingMessageRouterReadinessInputs AppController::incomingMessageReadinessInputs() const",
            buffer_source,
        )
        self.assertIn(
            "memochat::app::IncomingMessageRouterDispatchActions AppController::incomingMessageDispatchActions()",
            buffer_source,
        )

    def test_slots_route_through_chat_owned_router_without_recursion(self):
        private = PRIVATE.read_text(encoding="utf-8")
        group = GROUP.read_text(encoding="utf-8")
        chat_dispatcher_router = CHAT_DISPATCHER_ROUTER.read_text(encoding="utf-8")
        private_slot = extract_function(chat_dispatcher_router, "void AppChatDispatcherEventRouter::onTextChatMsg")
        private_apply = extract_function(private, "void AppController::applyTextChatMsg")
        group_slot = extract_function(chat_dispatcher_router, "void AppChatDispatcherEventRouter::onGroupChatMsg")
        group_apply = extract_function(group, "void AppController::applyGroupChatMsg")

        self.assertIn("_chat_controller.routePrivateIncomingMessage", private_slot)
        self.assertIn("_make_incoming_message_readiness()", private_slot)
        self.assertIn("_make_incoming_message_dispatch()", private_slot)
        self.assertNotIn("bufferIncomingPrivateMessage", private_slot)
        self.assertIn("_chat_controller.routeGroupIncomingMessage", group_slot)
        self.assertIn("_make_incoming_message_readiness()", group_slot)
        self.assertIn("_make_incoming_message_dispatch()", group_slot)
        self.assertNotIn("bufferIncomingGroupMessage", group_slot)
        self.assertIn("_features.chatFeatureController.handlePrivateIncomingMessage", private_apply)
        self.assertIn("_features.chatFeatureController.handleGroupIncomingMessage", group_apply)
        self.assertNotIn("routePrivateIncomingMessage", private_apply)
        self.assertNotIn("routeGroupIncomingMessage", group_apply)

        private_service = private_chat_event_service_source()
        group_service = group_conversation_service_source()
        self.assertIn(
            "dependencies.appendFriendMessages(result.peerUid, request.message->_chat_msgs);", private_service
        )
        self.assertIn("dependencies.upsertGroupMessage(result.groupId, textMsg);", group_service)

    def test_group_apply_creates_placeholder_before_upserting_message(self):
        group_apply = extract_function(GROUP.read_text(encoding="utf-8"), "void AppController::applyGroupChatMsg")
        service = group_conversation_service_source()
        ensure_group = extract_function(
            service,
            "std::shared_ptr<GroupInfoData> ensureGroup",
        )
        handle_incoming = extract_function(
            service,
            "GroupIncomingResult GroupConversationService::handleIncomingMessage",
        )

        self.assertIn("_features.chatFeatureController.handleGroupIncomingMessage", group_apply)
        self.assertIn("std::make_shared<GroupInfoData>()", ensure_group)
        self.assertIn('groupInfo->_name = QStringLiteral("群聊%1").arg(groupId);', ensure_group)
        self.assertNotIn('? QString("群聊%1").arg(msg->_group_id) : msg->_from_name', ensure_group)
        self.assertIn("ConversationSyncService::resolveGroupDialogUid", service)
        self.assertIn("dependencies.upsertGroup(groupInfo);", ensure_group)
        self.assertIn("dependencies.groupListModel->upsertFriend(item);", ensure_group)
        self.assertIn("dependencies.dialogListModel->upsertFriend(item);", ensure_group)
        self.assertLess(
            handle_incoming.index("ensureGroup(dependencies"),
            handle_incoming.index("dependencies.upsertGroupMessage(result.groupId, textMsg);"),
        )

    def test_flush_waits_for_dialog_bootstrap_and_is_triggered_after_readiness(self):
        buffer_source = BUFFER.read_text(encoding="utf-8")
        router_source = ROUTER_CPP.read_text(encoding="utf-8")
        dialog_ports = DIALOG_PORTS.read_text(encoding="utf-8")
        nav = NAV.read_text(encoding="utf-8")
        relation = RELATION.read_text(encoding="utf-8")
        chat_entry = CHAT_ENTRY.read_text(encoding="utf-8")

        readiness = extract_function(
            buffer_source,
            "memochat::app::IncomingMessageRouterReadinessInputs AppController::incomingMessageReadinessInputs",
        )
        self.assertIn("page() == ChatPage", readiness)
        self.assertIn("_shell_state.bootstrapState().dialogsReady", readiness)
        self.assertIn("_shell_state.bootstrapState().chatListInitialized", readiness)
        flush_body = extract_function(buffer_source, "void AppController::flushIncomingMessageRouter")
        self.assertIn("makeIncomingMessageRouterReadiness(incomingMessageReadinessInputs())", flush_body)
        self.assertIn("makeIncomingMessageRouterDispatch(incomingMessageDispatchActions())", flush_body)
        self.assertIn("std::stable_sort", router_source)
        self.assertIn("_messages.size() >= maxMessages", router_source)
        self.assertIn("_keys.contains(pending.key)", router_source)
        self.assertNotIn("_incoming_buffer_state", buffer_source)
        self.assertNotIn("std::stable_sort", buffer_source)

        dialog_rsp = extract_function(
            CHAT_DISPATCHER_ROUTER.read_text(encoding="utf-8"), "void AppChatDispatcherEventRouter::onDialogListRsp"
        )
        dialog_port = extract_function(dialog_ports, "ChatDialogListAppPort AppController::dialogListAppPort")
        dialog_service = (APP / "features/chat/services/ChatDialogListResponseService.cpp").read_text(encoding="utf-8")
        feature_dialog_list = (APP / "features/chat/controller/ChatFeatureControllerDialogList.cpp").read_text(
            encoding="utf-8"
        )
        apply_effect = extract_function(dialog_service, "void ChatDialogListResponseService::apply")
        self.assertIn("_chat_controller.handleDialogListResponse", dialog_rsp)
        self.assertIn("ChatDialogListResponseService::handle", feature_dialog_list)
        self.assertIn("port.setChatListInitialized", dialog_port)
        self.assertIn("port.flushIncomingMessages", dialog_port)
        self.assertNotIn("if (effect.", dialog_rsp)
        self.assertIn("callIfPresent(appPort.setChatListInitialized, true);", apply_effect)
        self.assertIn("callIfPresent(appPort.flushIncomingMessages);", apply_effect)
        self.assertGreater(
            apply_effect.rindex("callIfPresent(appPort.flushIncomingMessages);"),
            apply_effect.index("if (effect.syncCurrentDialogDraft)"),
        )

        ensure_chat = extract_function(nav, "void AppController::ensureChatListInitialized")
        feature_controller = (APP / "features/chat/controller/ChatFeatureController.cpp").read_text(encoding="utf-8")
        feature_ensure = extract_function(
            feature_controller, "ChatListPagingResult ChatFeatureController::ensureChatListInitialized"
        )
        self.assertIn("_features.chatFeatureController.ensureChatListInitialized();", ensure_chat)
        self.assertNotIn("flushIncomingMessageRouter();", ensure_chat)
        self.assertIn("_chatListPagingPort.flushIncomingMessages();", feature_ensure)
        self.assertGreater(
            feature_ensure.rindex("_chatListPagingPort.flushIncomingMessages();"),
            feature_ensure.index("_chatListPagingPort.setInitialized(true);"),
        )

        relation_updated = extract_function(relation, "void SessionRelationBootstrap::onRelationBootstrapUpdated")
        self.assertIn("_port.flushIncomingMessageRouter();", relation_updated)
        self.assertNotIn("_app.flushIncomingMessageRouter();", relation_updated)

        switch_chat = extract_function(chat_entry, "void SessionChatEntryCoordinator::onSwitchToChat")
        self.assertIn("_port.resetChatEntryRuntime();", switch_chat)
        self.assertNotIn("_app.clearIncomingMessageRouter();", switch_chat)
        feature_controller = (APP / "features/chat/controller/ChatFeatureController.cpp").read_text(encoding="utf-8")
        reset_post_login = extract_function(feature_controller, "void ChatFeatureController::resetForPostLoginEntry")
        self.assertIn("clearIncomingMessages();", reset_post_login)

        switch_login = extract_function(nav, "void AppController::switchToLogin")
        self.assertIn("_session_coordinator->resetForLogout();", switch_login)
        logout = extract_function(
            SESSION_LOGOUT.read_text(encoding="utf-8"), "void AppSessionCoordinator::resetForLogout"
        )
        self.assertIn("invokeIfSet(_logout_port.resetFeatureRuntimeForLogout);", logout)

    def test_bootstrap_ports_keep_delayed_bootstrap_and_relation_update_contracts(self):
        coordinators = APP_COORDINATORS.read_text(encoding="utf-8")
        chat_entry = CHAT_ENTRY.read_text(encoding="utf-8")
        relation = RELATION.read_text(encoding="utf-8")
        post_login_port = extract_struct(coordinators, "PostLoginBootstrapPort")
        relation_port = extract_struct(coordinators, "RelationBootstrapPort")
        run_bootstrap = extract_function(chat_entry, "void SessionChatEntryCoordinator::runPostLoginBootstrap")
        relation_updated = extract_function(relation, "void SessionRelationBootstrap::onRelationBootstrapUpdated")

        for field in (
            "bootstrapDialogs",
            "ensureApplyInitialized",
            "requestRelationBootstrap",
            "startHeartbeatTimer",
            "sendHeartbeatNow",
            "ensureChatListInitialized",
            "resetChatEntryRuntime",
        ):
            with self.subTest(post_login_port=field):
                self.assertIn(field, post_login_port)

        for call in (
            "_port.bootstrapDialogs();",
            "_port.ensureApplyInitialized();",
            "_port.requestRelationBootstrap();",
            "_port.startHeartbeatTimer",
            "_port.sendHeartbeatNow();",
            "_port.ensureChatListInitialized();",
        ):
            with self.subTest(run_bootstrap=call):
                self.assertIn(call, run_bootstrap)

        for field in (
            "ensureChatListInitialized",
            "friendListSnapshot",
            "upsertChatListFriend",
            "nextContactPage",
            "setContacts",
            "upsertContact",
            "markContactPageLoaded",
            "refreshContactLoadMoreState",
            "setContactsReady",
            "refreshApplySnapshot",
            "refreshDialogModelIncremental",
            "flushIncomingMessageRouter",
            "setCurrentChatPeerName",
            "setCurrentChatPeerIcon",
        ):
            with self.subTest(relation_port=field):
                self.assertIn(field, relation_port)
        self.assertNotIn("notifyPendingApplyChanged", relation_port)

        self.assertIn("_port.ensureChatListInitialized();", relation_updated)
        self.assertIn("_port.refreshApplySnapshot();", relation_updated)
        self.assertIn("_port.refreshDialogModelIncremental();", relation_updated)
        self.assertIn("_port.flushIncomingMessageRouter();", relation_updated)
        self.assertNotIn("_port.notifyPendingApplyChanged();", relation_updated)
        self.assertNotIn("_port.refreshApplyModel();", relation_updated)
        self.assertNotIn("_port.setApplyReady(true);", relation_updated)

    def test_chat_feature_controller_owns_incoming_router_entrypoints(self):
        feature_h = CHAT_FEATURE_H.read_text(encoding="utf-8")
        feature_incoming = CHAT_FEATURE_INCOMING.read_text(encoding="utf-8")

        for entrypoint in (
            "routePrivateIncomingMessage",
            "routeGroupIncomingMessage",
            "flushIncomingMessages",
            "clearIncomingMessages",
            "pendingIncomingMessageCount",
        ):
            with self.subTest(entrypoint=entrypoint):
                self.assertIn(entrypoint, feature_h)
                self.assertIn(f"ChatFeatureController::{entrypoint}", feature_incoming)


if __name__ == "__main__":
    unittest.main()
