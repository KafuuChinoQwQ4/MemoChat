import re
import unittest

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CREATE_GROUP_DIALOG = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/chat/group/CreateGroupDialog.qml"
CHAT_SHELL_PAGE = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/app/ChatShellPage.qml"
CHAT_SHELL_CONTENT = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/app/ChatShellContent.qml"
CHAT_NORMAL_FACE = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/chat/ChatNormalFace.qml"
CHAT_MODAL_LAYER = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/chat/ChatModalLayer.qml"
CHAT_LEFT_PANEL = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/chat/ChatLeftPanel.qml"
CHAT_LEFT_HEADER = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/chat/ChatLeftHeader.qml"
CHAT_JOIN_GROUP_POPUP = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/chat/ChatJoinGroupPopup.qml"
CHAT_CONVERSATION_HEADER = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/chat/conversation/ChatConversationHeader.qml"
)
GROUP_MANAGEMENT_PANEL = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/chat/group/GroupManagementPanel.qml"
GROUP_APPLY_REVIEW_PANE = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/chat/group/GroupApplyReviewPane.qml"
CHAT_CONVERSATION_PANE = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/chat/ChatConversationPane.qml"
APP_CONTROLLER_GROUP_EVENTS = REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/controller/AppControllerGroupEvents.cpp"
APP_CONTROLLER_GROUP_RESPONSES = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/controller/AppControllerGroupResponses.cpp"
)
APP_CONTROLLER_GROUP_MESSAGE_RESPONSES = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/controller/AppControllerGroupMessageResponses.cpp"
)
APP_CONTROLLER_DIALOG_META_RESPONSES = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/controller/AppControllerDialogMetaResponses.cpp"
)


class CreateGroupDialogQmlTests(unittest.TestCase):
    def test_create_group_dialog_selects_friend_model_user_ids(self):
        dialog = CREATE_GROUP_DIALOG.read_text(encoding="utf-8")
        shell = CHAT_SHELL_PAGE.read_text(encoding="utf-8")
        shell_content = CHAT_SHELL_CONTENT.read_text(encoding="utf-8")
        modal_layer = CHAT_MODAL_LAYER.read_text(encoding="utf-8")

        self.assertIn("property var friendModel", dialog)
        self.assertIn("ListView", dialog)
        self.assertIn("model: root.friendModel", dialog)
        self.assertIn("selectedUserIds", dialog)
        self.assertRegex(dialog, re.compile(r"selectedUserIds\[[^\]]*userId[^\]]*\]"))
        self.assertIn("friendModel: controller.contactListModel", shell + shell_content + modal_layer)

    def test_create_group_dialog_keeps_manual_user_id_fallback(self):
        dialog = CREATE_GROUP_DIALOG.read_text(encoding="utf-8")

        self.assertIn("manualMembersInput", dialog)
        self.assertIn("u123456789", dialog)

    def test_join_group_request_lives_in_plus_menu(self):
        left_panel = CHAT_LEFT_PANEL.read_text(encoding="utf-8")
        left_header = CHAT_LEFT_HEADER.read_text(encoding="utf-8")
        join_group_popup = CHAT_JOIN_GROUP_POPUP.read_text(encoding="utf-8")
        shell = CHAT_SHELL_PAGE.read_text(encoding="utf-8")
        normal_face = CHAT_NORMAL_FACE.read_text(encoding="utf-8")

        self.assertIn("signal applyJoinGroupRequested(string groupCode, string reason)", left_panel)
        self.assertIn("ChatJoinGroupPopup", left_panel)
        self.assertIn('text: "加入群聊"', left_header)
        self.assertIn('text: "创建群聊"', left_header)
        self.assertIn("joinGroupDialog.openFresh()", left_panel)
        self.assertIn(
            "root.applyJoinGroupRequested(joinGroupCodeInput.text.trim(), joinGroupReasonInput.text)", join_group_popup
        )
        self.assertIn("root.applyJoinGroupRequested(groupCode, reason)", left_panel)
        self.assertIn(
            "onApplyJoinGroupRequested: function(groupCode, reason) { controller.applyJoinGroup(groupCode, reason) }",
            shell + normal_face,
        )

    def test_group_management_only_reviews_current_group_applications(self):
        panel = GROUP_MANAGEMENT_PANEL.read_text(encoding="utf-8")
        info = (REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/chat/group/GroupInfoPane.qml").read_text(
            encoding="utf-8"
        )
        review = GROUP_APPLY_REVIEW_PANE.read_text(encoding="utf-8")

        self.assertNotIn("signal applyJoinRequested", panel)
        self.assertNotIn("提交入群申请", review)
        self.assertNotIn("groupIdInput", review)
        self.assertIn('text: "入群审核"', review)
        self.assertIn("root.reviewRequested(parseInt(applyIdInput.text), true)", review)
        self.assertIn('text: "解散群聊"', info)
        self.assertIn("currentGroupRole >= 3", info)
        self.assertIn("群主可解散群聊", info)

    def test_group_ack_upserts_payload_before_accepting_state(self):
        branch = APP_CONTROLLER_GROUP_MESSAGE_RESPONSES.read_text(encoding="utf-8")

        self.assertIn("void AppController::handleGroupMessageAckRsp", branch)
        self.assertIn("MessagePayloadService::buildGroupAckMessage", branch)
        self.assertIn("_message_model.upsertMessage(correctedMsg", branch)
        self.assertNotRegex(
            branch,
            re.compile(r'if\s*\(\s*ackStatus\s*==\s*QStringLiteral\("accepted"\).*?break;', re.S),
        )

    def test_dialog_meta_responses_keep_draft_and_pin_persistence(self):
        source = APP_CONTROLLER_DIALOG_META_RESPONSES.read_text(encoding="utf-8")

        self.assertIn("void AppController::handleDialogMetaRsp", source)
        self.assertIn("ID_SYNC_DRAFT_RSP", source)
        self.assertIn("ID_PIN_DIALOG_RSP", source)
        self.assertIn("applyDraftToDialogModel(dialogUid, draftText)", source)
        self.assertIn("saveDraftStore(ownerUid)", source)

    def test_chat_page_initializes_groups_for_all_sessions(self):
        left_panel = CHAT_LEFT_PANEL.read_text(encoding="utf-8")
        shell = CHAT_SHELL_PAGE.read_text(encoding="utf-8")
        normal_face = CHAT_NORMAL_FACE.read_text(encoding="utf-8")

        self.assertIn("function ensureCurrentSessionSource()", left_panel)
        self.assertIn("controller.ensureGroupsInitialized()", left_panel)
        self.assertRegex(
            left_panel,
            re.compile(r"Component\.onCompleted:\s*\{[^}]*root\.ensureCurrentSessionSource\(\)", re.S),
        )
        self.assertIn("root.ensureCurrentSessionSource()", left_panel)
        self.assertIn("controller.ensureGroupsInitialized()", shell + normal_face)

    def test_group_header_actions_wrap_and_group_info_uses_implicit_height(self):
        conversation = CHAT_CONVERSATION_PANE.read_text(encoding="utf-8")
        header = CHAT_CONVERSATION_HEADER.read_text(encoding="utf-8")
        panel = GROUP_MANAGEMENT_PANEL.read_text(encoding="utf-8")

        self.assertNotIn("id: headerActionFlow", conversation + header)
        self.assertIn("id: groupSettingsButton", header)
        self.assertIn("Item { Layout.fillWidth: true }", header)
        self.assertIn("Layout.preferredHeight: 46", conversation)
        self.assertNotIn("Layout.leftMargin: 8", conversation + header)
        self.assertIn("height: implicitHeight", panel)
        self.assertNotIn("height: 284", panel)

    def test_chat_left_panel_uses_plus_menu_and_all_sessions_only(self):
        left_panel = CHAT_LEFT_PANEL.read_text(encoding="utf-8")
        left_header = CHAT_LEFT_HEADER.read_text(encoding="utf-8")
        combined_left = left_panel + left_header

        self.assertIn("id: addMenuButton", left_header)
        self.assertIn("id: addMenuPopup", left_header)
        self.assertIn("contentItem: GlassSurface", left_header)
        self.assertIn('ToolTip.text: "添加"', left_header)
        self.assertIn('text: "创建群聊"', left_header)
        self.assertIn('text: "添加好友"', left_header)
        self.assertIn('text: "加入群聊"', left_header)
        self.assertNotIn("Layout.preferredHeight: 60", combined_left)
        self.assertNotIn("sourceComponent: placeholderPaneComponent", combined_left)
        self.assertNotIn("id: placeholderPaneComponent", combined_left)
        self.assertNotIn('placeholderText: "搜索用户ID（u#########）"', combined_left)
        self.assertNotIn("root.sessionFilter = 1", combined_left)
        self.assertNotIn("root.sessionFilter = 2", combined_left)
        self.assertIn("function currentSessionModel() { return dialogModel }", left_panel)

    def test_group_conversation_header_uses_single_settings_entry(self):
        header = CHAT_CONVERSATION_HEADER.read_text(encoding="utf-8")

        self.assertIn('text: "..."', header)
        self.assertIn('ToolTip.text: "群设置"', header)
        self.assertIn("root.openGroupManageRequested()", header)
        self.assertNotIn('text: root.currentDialogPinned ? "取消置顶" : "置顶"', header)
        self.assertNotIn('text: root.currentDialogMuted ? "取消静音" : "静音"', header)
        self.assertNotIn('text: "群管理"', header)

    def test_group_settings_contains_dialog_pin_and_mute(self):
        panel = GROUP_MANAGEMENT_PANEL.read_text(encoding="utf-8")
        info = (REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/chat/group/GroupInfoPane.qml").read_text(
            encoding="utf-8"
        )
        shell = CHAT_SHELL_PAGE.read_text(encoding="utf-8")
        shell_content = CHAT_SHELL_CONTENT.read_text(encoding="utf-8")
        normal_face = CHAT_NORMAL_FACE.read_text(encoding="utf-8")
        shell_sources = shell + shell_content + normal_face

        self.assertIn("property bool currentDialogPinned", panel)
        self.assertIn("property bool currentDialogMuted", panel)
        self.assertIn("signal toggleDialogPinned()", panel)
        self.assertIn("signal toggleDialogMuted()", panel)
        self.assertIn("currentDialogPinned: root.currentDialogPinned", panel)
        self.assertIn("currentDialogMuted: root.currentDialogMuted", panel)
        self.assertIn("onToggleDialogPinned: root.toggleDialogPinned()", panel)
        self.assertIn("onToggleDialogMuted: root.toggleDialogMuted()", panel)
        self.assertIn('text: root.currentDialogPinned ? "取消置顶" : "置顶会话"', info)
        self.assertIn('text: root.currentDialogMuted ? "取消静音" : "静音会话"', info)
        self.assertIn("currentDialogPinned: controller.currentDialogPinned", shell_sources)
        self.assertIn("currentDialogMuted: controller.currentDialogMuted", shell_sources)


if __name__ == "__main__":
    unittest.main()
