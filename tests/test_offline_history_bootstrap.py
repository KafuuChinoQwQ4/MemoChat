import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
APP_CONTROLLER_PRIVATE_EVENTS = REPO_ROOT / "apps/client/desktop/MemoChat-qml/AppControllerPrivateEvents.cpp"
APP_CONTROLLER_GROUP_EVENTS = REPO_ROOT / "apps/client/desktop/MemoChat-qml/AppControllerGroupEvents.cpp"
APP_CONTROLLER = REPO_ROOT / "apps/client/desktop/MemoChat-qml/AppController.cpp"
CHATSERVER_CONFIGS = [
    REPO_ROOT / f"apps/server/core/ChatServer/chatserver{i}.ini"
    for i in range(1, 7)
]


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
                return source[start:index + 1]
    raise AssertionError(f"Function body not found for {signature}")


class OfflineHistoryBootstrapTests(unittest.TestCase):
    def test_chat_login_offline_push_is_enabled_in_runtime_configs(self):
        for path in CHATSERVER_CONFIGS:
            with self.subTest(path=path.name):
                source = path.read_text(encoding="utf-8")
                self.assertIn("chat_login_offline_push=true", source)
                self.assertNotIn("chat_login_offline_push=false", source)

    def test_bootstrap_dialog_fetches_unread_private_and_group_history(self):
        source = APP_CONTROLLER_PRIVATE_EVENTS.read_text(encoding="utf-8")
        body = extract_function(source, "void AppController::onDialogListRsp")

        bootstrap_start = body.index("if (bootstrappingDialog)")
        bootstrap = body[bootstrap_start:]
        self.assertIn("requestPrivateHistory(dialog->_uid, 0, QString());", bootstrap)
        self.assertIn("requestGroupHistoryForBootstrap", bootstrap)
        self.assertNotIn("if (_private_history_loading) {\n        return;\n    }", bootstrap)

    def test_group_history_response_uses_response_group_id(self):
        source = APP_CONTROLLER_GROUP_EVENTS.read_text(encoding="utf-8")
        body = extract_function(source, "void AppController::onGroupRsp")
        history_start = body.index("case ID_GROUP_HISTORY_RSP: {")
        next_case = body.index("case ID_EDIT_GROUP_MSG_RSP:", history_start)
        history_case = body[history_start:next_case]
        self.assertIn('const qint64 groupId = payload.value("groupid").toVariant().toLongLong();', history_case)
        self.assertIn("_gateway.userMgr()->GetGroupById(groupId)", history_case)
        self.assertNotIn("_gateway.userMgr()->GetGroupById(_current_group_id)", history_case)

    def test_private_bootstrap_history_does_not_clear_current_pending_state(self):
        source = APP_CONTROLLER_PRIVATE_EVENTS.read_text(encoding="utf-8")
        body = extract_function(source, "void AppController::onPrivateHistoryRsp")
        self.assertIn("const bool isPendingRequest = peerUid == _private_history_pending_peer_uid;", body)
        self.assertIn("if (isPendingRequest) {\n        setPrivateHistoryLoading(false);\n    }", body)
        self.assertIn("if (isPendingRequest) {\n        _private_history_pending_before_ts = 0;", body)

    def test_group_dialog_bootstrap_registers_group_placeholder(self):
        source = APP_CONTROLLER_PRIVATE_EVENTS.read_text(encoding="utf-8")
        body = extract_function(source, "void AppController::onDialogListRsp")
        group_branch_start = body.index('if (dialogType == "group")')
        next_block = body.index("if (merged.empty()", group_branch_start)
        group_branch = body[group_branch_start:next_block]
        self.assertIn("std::make_shared<GroupInfoData>()", group_branch)
        self.assertIn("_gateway.userMgr()->UpsertGroup(groupInfo);", group_branch)
        self.assertIn("_group_list_model.upsertFriend(item);", group_branch)

    def test_select_dialog_by_group_uid_falls_back_without_group_list_row(self):
        source = APP_CONTROLLER.read_text(encoding="utf-8")
        body = extract_function(source, "void AppController::selectDialogByUid")
        self.assertIn("ConversationSyncService::groupIdForDialogUid(_group_uid_map, uid)", body)
        self.assertIn("selectGroupByDialogUid(uid, groupId)", body)

    def test_select_group_index_derives_group_id_when_uid_map_was_refreshed(self):
        source = APP_CONTROLLER.read_text(encoding="utf-8")
        body = extract_function(source, "void AppController::selectGroupIndex")
        self.assertIn("ConversationSyncService::groupIdForDialogUid(_group_uid_map, pseudoUid)", body)
        self.assertNotIn("_group_uid_map.value(pseudoUid, 0)", body)

    def test_group_history_request_is_not_blocked_by_private_history_loading(self):
        source = APP_CONTROLLER.read_text(encoding="utf-8")
        body = extract_function(source, "void AppController::loadGroupHistory")
        self.assertNotIn("if (_private_history_loading) {\n        return;\n    }", body)
        self.assertIn("_group_history_loading", body)

    def test_group_bootstrap_history_is_not_blocked_by_private_history_loading(self):
        source = APP_CONTROLLER.read_text(encoding="utf-8")
        body = extract_function(source, "void AppController::requestGroupHistoryForBootstrap")
        self.assertNotIn("groupId == _current_group_id && _private_history_loading", body)
        self.assertIn("_group_history_loading", body)

    def test_group_dialog_fallback_creates_placeholder_before_history_request(self):
        source = APP_CONTROLLER.read_text(encoding="utf-8")
        body = extract_function(source, "void AppController::selectGroupByDialogUid")
        placeholder_pos = body.index("_gateway.userMgr()->UpsertGroup(groupInfo);")
        request_pos = body.index("loadGroupHistory();")
        self.assertLess(placeholder_pos, request_pos)

    def test_group_list_refresh_preserves_cached_group_messages(self):
        source = (REPO_ROOT / "apps/client/desktop/MemoChatShared/usermgr.cpp").read_text(encoding="utf-8")
        body = extract_function(source, "void UserMgr::SetGroupList")
        self.assertIn("previousGroups", body)
        self.assertIn("info->_chat_msgs = existing->_chat_msgs;", body)
        self.assertIn("if (info->_last_msg.isEmpty())", body)


if __name__ == "__main__":
    unittest.main()
