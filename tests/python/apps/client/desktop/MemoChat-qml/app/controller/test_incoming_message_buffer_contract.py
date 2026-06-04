import unittest

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
APP = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
HEADER = APP / "app/controller/AppController.h"
STATE = APP / "app/controller/AppControllerDialogStateData.h"
SOURCES = APP / "app/sources.cmake"
BUFFER = APP / "app/controller/AppControllerIncomingBuffer.cpp"
PRIVATE = APP / "app/controller/AppControllerPrivateEvents.cpp"
GROUP = APP / "app/controller/AppControllerGroupEvents.cpp"
DIALOGS = APP / "app/controller/AppControllerDialogListEvents.cpp"
NAV = APP / "app/controller/AppControllerNavigation.cpp"
CHAT_ENTRY = APP / "app/session/SessionChatEntryCoordinator.cpp"
RELATION = APP / "app/session/SessionRelationBootstrap.cpp"


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


class IncomingMessageBufferContractTests(unittest.TestCase):
    def test_buffer_state_and_controller_methods_are_declared(self):
        header = HEADER.read_text(encoding="utf-8")
        state = STATE.read_text(encoding="utf-8")
        sources = SOURCES.read_text(encoding="utf-8")

        self.assertIn("struct PendingIncomingMessage", state)
        self.assertIn("struct AppIncomingMessageBufferState", state)
        self.assertIn("QVector<PendingIncomingMessage> messages", state)
        self.assertIn("QSet<QString> keys", state)
        self.assertIn("qint64 nextSequence = 0", state)
        self.assertIn("static constexpr qsizetype maxMessages = 1000", state)
        self.assertIn("AppIncomingMessageBufferState _incoming_buffer_state;", header)
        for declaration in (
            "bool shouldBufferIncomingMessages() const;",
            "bool bufferIncomingPrivateMessage(std::shared_ptr<TextChatMsg> msg);",
            "bool bufferIncomingGroupMessage(std::shared_ptr<GroupChatMsg> msg);",
            "void flushPendingIncomingMessages();",
            "void clearPendingIncomingMessages();",
            "void applyTextChatMsg(std::shared_ptr<TextChatMsg> msg);",
            "void applyGroupChatMsg(std::shared_ptr<GroupChatMsg> msg);",
        ):
            self.assertIn(declaration, header)
        self.assertIn("app/controller/AppControllerIncomingBuffer.cpp", sources)

    def test_slots_buffer_before_applying_without_recursion(self):
        private = PRIVATE.read_text(encoding="utf-8")
        group = GROUP.read_text(encoding="utf-8")
        private_slot = extract_function(private, "void AppController::onTextChatMsg")
        private_apply = extract_function(private, "void AppController::applyTextChatMsg")
        group_slot = extract_function(group, "void AppController::onGroupChatMsg")
        group_apply = extract_function(group, "void AppController::applyGroupChatMsg")

        self.assertIn("if (bufferIncomingPrivateMessage(msg))", private_slot)
        self.assertIn("applyTextChatMsg(msg);", private_slot)
        self.assertIn("if (bufferIncomingGroupMessage(msg))", group_slot)
        self.assertIn("applyGroupChatMsg(msg);", group_slot)
        self.assertIn("_gateway.userMgr()->AppendFriendChatMsg", private_apply)
        self.assertIn("_gateway.userMgr()->UpsertGroupChatMsg", group_apply)
        self.assertNotIn("bufferIncomingPrivateMessage", private_apply)
        self.assertNotIn("bufferIncomingGroupMessage", group_apply)

    def test_group_apply_creates_placeholder_before_upserting_message(self):
        group = GROUP.read_text(encoding="utf-8")
        group_apply = extract_function(group, "void AppController::applyGroupChatMsg")

        self.assertIn("std::make_shared<GroupInfoData>()", group_apply)
        self.assertIn('const QString groupName = QString("群聊%1").arg(msg->_group_id);', group_apply)
        self.assertNotIn('? QString("群聊%1").arg(msg->_group_id) : msg->_from_name', group_apply)
        self.assertIn("ConversationSyncService::resolveGroupDialogUid", group_apply)
        self.assertIn("_gateway.userMgr()->UpsertGroup(groupInfo);", group_apply)
        self.assertIn("_group_list_model.upsertFriend", group_apply)
        self.assertIn("_dialog_list_model.upsertFriend", group_apply)
        self.assertLess(
            group_apply.index("_gateway.userMgr()->UpsertGroup(groupInfo);"),
            group_apply.index("_gateway.userMgr()->UpsertGroupChatMsg"),
        )

    def test_flush_waits_for_dialog_bootstrap_and_is_triggered_after_readiness(self):
        buffer_source = BUFFER.read_text(encoding="utf-8")
        dialogs = DIALOGS.read_text(encoding="utf-8")
        nav = NAV.read_text(encoding="utf-8")
        relation = RELATION.read_text(encoding="utf-8")
        chat_entry = CHAT_ENTRY.read_text(encoding="utf-8")

        should_buffer = extract_function(buffer_source, "bool AppController::shouldBufferIncomingMessages")
        self.assertIn("_page == ChatPage", should_buffer)
        self.assertIn("!_bootstrap_state.dialogsReady || !_bootstrap_state.chatListInitialized", should_buffer)
        self.assertIn("std::stable_sort", buffer_source)
        self.assertIn("applyTextChatMsg", buffer_source)
        self.assertIn("applyGroupChatMsg", buffer_source)
        self.assertIn("_incoming_buffer_state.maxMessages", buffer_source)
        self.assertIn("_incoming_buffer_state.keys", buffer_source)

        dialog_rsp = extract_function(dialogs, "void AppController::onDialogListRsp")
        self.assertIn("flushPendingIncomingMessages();", dialog_rsp)
        self.assertIn("_bootstrap_state.chatListInitialized = true;", dialog_rsp)
        self.assertLess(
            dialog_rsp.index("_bootstrap_state.chatListInitialized = true;"),
            dialog_rsp.index('if (!payload.contains("dialogs"))'),
        )
        self.assertGreater(
            dialog_rsp.rindex("flushPendingIncomingMessages();"), dialog_rsp.index("syncCurrentDialogDraft();")
        )

        ensure_chat = extract_function(nav, "void AppController::ensureChatListInitialized")
        self.assertIn("flushPendingIncomingMessages();", ensure_chat)
        self.assertGreater(
            ensure_chat.rindex("flushPendingIncomingMessages();"),
            ensure_chat.index("_bootstrap_state.chatListInitialized = true;"),
        )

        relation_updated = extract_function(relation, "void SessionRelationBootstrap::onRelationBootstrapUpdated")
        self.assertIn("_app.flushPendingIncomingMessages();", relation_updated)

        switch_chat = extract_function(chat_entry, "void SessionChatEntryCoordinator::onSwitchToChat")
        self.assertIn("_app.clearPendingIncomingMessages();", switch_chat)

        switch_login = extract_function(nav, "void AppController::switchToLogin")
        self.assertIn("clearPendingIncomingMessages();", switch_login)


if __name__ == "__main__":
    unittest.main()
