import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
APP_CONTROLLER_DIALOG_LIST_PORTS = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/controller/AppControllerDialogListPorts.cpp"
)
APP_CONTROLLER_PRIVATE_HISTORY_RESPONSES = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/events/AppChatDispatcherEventRouter.cpp"
)
APP_CONTROLLER_GROUP_EVENTS = REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/controller/AppControllerGroupEvents.cpp"
APP_CONTROLLER_GROUP_RESPONSES = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/controller/AppControllerGroupResponses.cpp"
)
APP_CONTROLLER_GROUP_HISTORY_RESPONSES = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/controller/AppControllerGroupHistoryResponses.cpp"
)
APP_CONTROLLER_DIALOG_SELECTION = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/controller/AppControllerDialogSelection.cpp"
)
APP_CONTROLLER_GROUP_SELECTION = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/controller/AppControllerGroupSelection.cpp"
)
APP_CONTROLLER_GROUP_COMMANDS = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/controller/AppControllerGroupCommands.cpp"
)
APP_CONTROLLER_CPP = REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/controller/AppController.cpp"
APP_GROUP_FEATURE_PORT_BINDER = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/composition/AppGroupFeaturePortBinder.cpp"
)
APP_CONTROLLER_H = REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/controller/AppController.h"
CHAT_DIALOG_LIST_RESPONSE_SERVICE = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/chat/services/ChatDialogListResponseService.cpp"
)
CHAT_DIALOG_SELECTION_SERVICE = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/chat/services/ChatDialogSelectionService.cpp"
)
PRIVATE_HISTORY_SERVICE = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/chat/services/PrivateChatHistoryService.cpp"
)
PRIVATE_HISTORY_REQUEST_SERVICE = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/chat/services/PrivateChatHistoryRequestService.cpp"
)
CHAT_FEATURE_PRIVATE_HISTORY = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/chat/controller/ChatFeatureControllerPrivateHistory.cpp"
)
USER_MGR_GROUP_LIST = REPO_ROOT / "apps/client/desktop/MemoChat-qml/core/session/UserMgrGroupList.cpp"
# Cluster reduced 6→2 nodes (chatserver1/2 only); iterate the real topology.
CHATSERVER_CONFIGS = [REPO_ROOT / f"apps/server/core/ChatServer/chatserver{i}.ini" for i in range(1, 3)]


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


def compact_ws(source: str) -> str:
    return " ".join(source.split())


class OfflineHistoryBootstrapTests(unittest.TestCase):
    def test_chat_login_offline_push_is_enabled_in_runtime_configs(self):
        for path in CHATSERVER_CONFIGS:
            with self.subTest(path=path.name):
                source = path.read_text(encoding="utf-8")
                self.assertIn("chat_login_offline_push=true", source)
                self.assertNotIn("chat_login_offline_push=false", source)

    def test_bootstrap_dialog_fetches_unread_private_and_group_history(self):
        app_port = extract_function(
            APP_CONTROLLER_DIALOG_LIST_PORTS.read_text(encoding="utf-8"),
            "ChatDialogListAppPort AppController::dialogListAppPort",
        )
        router = extract_function(
            APP_CONTROLLER_PRIVATE_HISTORY_RESPONSES.read_text(encoding="utf-8"),
            "void AppChatDispatcherEventRouter::onDialogListRsp",
        )
        source = CHAT_DIALOG_LIST_RESPONSE_SERVICE.read_text(encoding="utf-8")
        body = extract_function(source, "ChatDialogListResponseEffect ChatDialogListResponseService::reduce")
        apply = extract_function(source, "void ChatDialogListResponseService::apply")

        bootstrap_start = body.index("if (context.bootstrappingDialog)")
        bootstrap = body[bootstrap_start:]
        self.assertIn("effect.currentPrivateHistoryUid = dialog->_uid;", bootstrap)
        self.assertIn("effect.bootstrapGroupHistoryIds.push_back(groupId);", bootstrap)
        self.assertIn("callIfPresent(appPort.requestCurrentPrivateHistory, effect.currentPrivateHistoryUid);", apply)
        self.assertIn("callIfPresent(appPort.requestGroupHistoryForBootstrap, groupId);", apply)
        self.assertIn("port.requestCurrentPrivateHistory", app_port)
        self.assertIn("requestPrivateHistory(peerUid, 0, QString());", app_port)
        self.assertIn("port.requestGroupHistoryForBootstrap", app_port)
        self.assertIn("requestGroupHistoryForBootstrap(groupId);", app_port)
        self.assertIn("_chat_controller.handleDialogListResponse", router)
        self.assertNotIn("if (_private_history_loading) {\n        return;\n    }", bootstrap)

    def test_group_history_response_uses_response_group_id(self):
        source = APP_CONTROLLER_GROUP_HISTORY_RESPONSES.read_text(encoding="utf-8")
        history_case = extract_function(source, "void AppController::handleGroupHistoryRsp")
        self.assertIn("_features.chatFeatureController.handleGroupHistoryResponse", history_case)
        self.assertIn("result.groupId", history_case)
        self.assertIn("result.currentGroupId", history_case)
        self.assertIn("result.cachedMessageCount", history_case)
        self.assertNotIn('payload.value("groupid").toVariant().toLongLong()', history_case)
        self.assertNotIn("_gateway.userMgr()->GetGroupById(result.groupId)", history_case)
        self.assertNotIn("_gateway.userMgr()->GetGroupById(_current_group_id)", history_case)

    def test_private_bootstrap_history_does_not_clear_current_pending_state(self):
        source = APP_CONTROLLER_PRIVATE_HISTORY_RESPONSES.read_text(encoding="utf-8")
        body = extract_function(source, "void AppChatDispatcherEventRouter::onPrivateHistoryRsp")
        service = PRIVATE_HISTORY_SERVICE.read_text(encoding="utf-8")
        request_service = PRIVATE_HISTORY_REQUEST_SERVICE.read_text(encoding="utf-8")
        feature = CHAT_FEATURE_PRIVATE_HISTORY.read_text(encoding="utf-8")
        compact = compact_ws(service)

        self.assertIn(
            "_chat_controller.handlePrivateHistoryResponse(request, _make_private_history_response_dependencies());",
            body,
        )
        self.assertIn("dependencies.historyState = &_privateHistoryState;", feature)
        self.assertIn("bool isPendingResponse(int peerUid, const PrivateHistoryRequestState& state)", service)
        self.assertIn("return state.pendingPeerUid > 0 && peerUid == state.pendingPeerUid;", service)
        self.assertIn(
            "if (result.pendingMatched && dependencies.setLoading) { dependencies.setLoading(false); }", compact
        )
        self.assertIn("if (result.pendingMatched && state) { clearPending(*state); }", compact)
        self.assertIn("dependencies.historyState->pendingPeerUid = request.peerUid;", request_service)

    def test_group_dialog_bootstrap_registers_group_placeholder(self):
        source = CHAT_DIALOG_LIST_RESPONSE_SERVICE.read_text(encoding="utf-8")
        body = extract_function(source, "ChatDialogListResponseEffect ChatDialogListResponseService::reduce")
        group_branch_start = body.index('if (dialogType == QStringLiteral("group"))')
        next_block = body.index("DialogListService::appendMissingPrivateDialogs", group_branch_start)
        group_branch = body[group_branch_start:next_block]
        self.assertIn("std::make_shared<GroupInfoData>()", group_branch)
        self.assertIn("effect.upsertGroups.push_back(groupInfo);", group_branch)
        self.assertIn("merged.push_back(item);", group_branch)
        self.assertNotIn("effect.upsertGroupDialogs.push_back(item);", group_branch)
        self.assertIn("effect.upsertGroupDialogs.push_back(dialog);", body)

    def test_select_dialog_by_group_uid_falls_back_without_group_list_row(self):
        adapter = extract_function(
            APP_CONTROLLER_DIALOG_SELECTION.read_text(encoding="utf-8"), "void AppController::selectDialogByUid"
        )
        source = CHAT_DIALOG_SELECTION_SERVICE.read_text(encoding="utf-8")
        body = extract_function(source, "void ChatDialogSelectionService::selectDialogByUid")
        self.assertIn("groupIdForDialogUid(port, dialogUid)", body)
        self.assertIn("selectGroupByDialogUid(dialogUid, groupId, port)", body)
        self.assertIn("_features.chatFeatureController.selectDialogByUid(uid);", adapter)
        self.assertNotIn("port.groupDialogUidMap", body)

    def test_select_group_index_derives_group_id_when_uid_map_was_refreshed(self):
        source = CHAT_DIALOG_SELECTION_SERVICE.read_text(encoding="utf-8")
        body = extract_function(source, "void ChatDialogSelectionService::selectGroupByIndex")
        self.assertIn("groupIdForDialogUid(port, pseudoUid)", body)
        self.assertNotIn("_group_uid_map.value(pseudoUid, 0)", body)
        self.assertNotIn("port.groupDialogUidMap", body)

    def test_group_history_request_is_not_blocked_by_private_history_loading(self):
        group_commands = APP_CONTROLLER_GROUP_COMMANDS.read_text(encoding="utf-8")
        app_controller = APP_CONTROLLER_CPP.read_text(encoding="utf-8")
        port_binder = APP_GROUP_FEATURE_PORT_BINDER.read_text(encoding="utf-8")
        app_header = APP_CONTROLLER_H.read_text(encoding="utf-8")
        body = port_binder.split("_features.groupController.setCommandPort(GroupCommandPort{", 1)[1].split("});", 1)[0]
        self.assertNotIn("void loadGroupHistory();", app_header)
        self.assertNotIn("void AppController::loadGroupHistory", group_commands)
        self.assertNotIn("_features.groupController.setCommandPort(GroupCommandPort{", app_controller)
        self.assertNotIn("if (_private_history_loading) {\n        return;\n    }", body)
        self.assertIn("_features.chatFeatureController.requestCurrentGroupHistory()", body)
        self.assertNotIn("GroupHistoryRequestCommand request;", body)
        self.assertNotIn("_features.chatFeatureController.requestGroupHistory(request", body)
        self.assertNotIn("_shell_state.loadingState().groupHistoryLoading = true", body)

    def test_group_bootstrap_history_is_not_blocked_by_private_history_loading(self):
        source = APP_CONTROLLER_GROUP_COMMANDS.read_text(encoding="utf-8")
        body = extract_function(source, "void AppController::requestGroupHistoryForBootstrap")
        self.assertNotIn("groupId == _current_group_id && _private_history_loading", body)
        self.assertIn("_features.chatFeatureController.requestGroupHistoryForBootstrap(", body)
        self.assertNotIn("_shell_state.loadingState().groupHistoryLoading = true", body)

    def test_group_dialog_fallback_creates_placeholder_before_history_request(self):
        source = CHAT_DIALOG_SELECTION_SERVICE.read_text(encoding="utf-8")
        body = extract_function(source, "void ChatDialogSelectionService::selectGroupByDialogUid")
        placeholder_pos = body.index("port.upsertGroup(groupInfo);")
        request_pos = body.index("openCurrentGroupConversation(groupId, port);")
        self.assertLess(placeholder_pos, request_pos)

    def test_group_list_refresh_preserves_cached_group_messages(self):
        source = USER_MGR_GROUP_LIST.read_text(encoding="utf-8")
        body = extract_function(source, "void UserMgr::SetGroupList")
        self.assertIn("previousGroups", body)
        self.assertIn("info->_chat_msgs = existing->_chat_msgs;", body)
        self.assertIn("if (info->_last_msg.isEmpty())", body)


if __name__ == "__main__":
    unittest.main()
