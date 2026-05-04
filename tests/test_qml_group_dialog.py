import re
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
CREATE_GROUP_DIALOG = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/chat/group/CreateGroupDialog.qml"
CHAT_SHELL_PAGE = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/ChatShellPage.qml"
CHAT_LEFT_PANEL = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/chat/ChatLeftPanel.qml"
GROUP_MANAGEMENT_PANEL = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/chat/group/GroupManagementPanel.qml"
GROUP_APPLY_REVIEW_PANE = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/chat/group/GroupApplyReviewPane.qml"
CHAT_CONVERSATION_PANE = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/chat/ChatConversationPane.qml"
APP_CONTROLLER_GROUP_EVENTS = REPO_ROOT / "apps/client/desktop/MemoChat-qml/AppControllerGroupEvents.cpp"


class CreateGroupDialogQmlTests(unittest.TestCase):
    def test_create_group_dialog_selects_friend_model_user_ids(self):
        dialog = CREATE_GROUP_DIALOG.read_text(encoding="utf-8")
        shell = CHAT_SHELL_PAGE.read_text(encoding="utf-8")

        self.assertIn("property var friendModel", dialog)
        self.assertIn("ListView", dialog)
        self.assertIn("model: root.friendModel", dialog)
        self.assertIn("selectedUserIds", dialog)
        self.assertRegex(dialog, re.compile(r"selectedUserIds\[[^\]]*userId[^\]]*\]"))
        self.assertIn("friendModel: controller.contactListModel", shell)

    def test_create_group_dialog_keeps_manual_user_id_fallback(self):
        dialog = CREATE_GROUP_DIALOG.read_text(encoding="utf-8")

        self.assertIn("manualMembersInput", dialog)
        self.assertIn("u123456789", dialog)

    def test_join_group_request_lives_in_plus_menu(self):
        left_panel = CHAT_LEFT_PANEL.read_text(encoding="utf-8")
        shell = CHAT_SHELL_PAGE.read_text(encoding="utf-8")

        self.assertIn("signal applyJoinGroupRequested(string groupCode, string reason)", left_panel)
        self.assertIn('text: "加入群聊"', left_panel)
        self.assertIn('text: "创建群聊"', left_panel)
        self.assertIn("joinGroupDialog.openFresh()", left_panel)
        self.assertIn("root.applyJoinGroupRequested(joinGroupCodeInput.text.trim(), joinGroupReasonInput.text)", left_panel)
        self.assertIn("onApplyJoinGroupRequested: function(groupCode, reason) { controller.applyJoinGroup(groupCode, reason) }", shell)

    def test_group_management_only_reviews_current_group_applications(self):
        panel = GROUP_MANAGEMENT_PANEL.read_text(encoding="utf-8")
        info = (REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/chat/group/GroupInfoPane.qml").read_text(encoding="utf-8")
        review = GROUP_APPLY_REVIEW_PANE.read_text(encoding="utf-8")

        self.assertNotIn("signal applyJoinRequested", panel)
        self.assertNotIn("提交入群申请", review)
        self.assertNotIn("groupIdInput", review)
        self.assertIn('text: "入群审核"', review)
        self.assertIn("root.reviewRequested(parseInt(applyIdInput.text), true)", review)
        self.assertIn('text: "解散群聊"', info)
        self.assertIn('currentGroupRole >= 3', info)
        self.assertIn('群主可解散群聊', info)

    def test_group_ack_upserts_payload_before_accepting_state(self):
        source = APP_CONTROLLER_GROUP_EVENTS.read_text(encoding="utf-8")
        branch_start = source.index("case ID_GROUP_CHAT_MSG_RSP:\n    case ID_FORWARD_GROUP_MSG_RSP:")
        branch_end = source.index("case ID_SYNC_DRAFT_RSP:", branch_start)
        branch = source[branch_start:branch_end]

        self.assertIn("MessagePayloadService::buildGroupAckMessage", branch)
        self.assertIn("_message_model.upsertMessage(correctedMsg", branch)
        self.assertNotRegex(
            branch,
            re.compile(r'if\s*\(\s*ackStatus\s*==\s*QStringLiteral\("accepted"\).*?break;', re.S),
        )

    def test_chat_page_initializes_groups_for_all_sessions(self):
        left_panel = CHAT_LEFT_PANEL.read_text(encoding="utf-8")
        shell = CHAT_SHELL_PAGE.read_text(encoding="utf-8")

        self.assertIn("function ensureCurrentSessionSource()", left_panel)
        self.assertIn("controller.ensureGroupsInitialized()", left_panel)
        self.assertIn("Component.onCompleted: root.ensureCurrentSessionSource()", left_panel)
        self.assertIn("root.ensureCurrentSessionSource()", left_panel)
        self.assertIn("controller.ensureGroupsInitialized()", shell)

    def test_group_header_actions_wrap_and_group_info_uses_implicit_height(self):
        conversation = CHAT_CONVERSATION_PANE.read_text(encoding="utf-8")
        panel = GROUP_MANAGEMENT_PANEL.read_text(encoding="utf-8")

        self.assertNotIn("id: headerActionFlow", conversation)
        self.assertIn("id: groupSettingsButton", conversation)
        self.assertIn("Item { Layout.fillWidth: true }", conversation)
        self.assertIn("Layout.preferredHeight: 46", conversation)
        self.assertNotIn("Layout.leftMargin: 8", conversation)
        self.assertIn("height: implicitHeight", panel)
        self.assertNotIn("height: 284", panel)

    def test_chat_left_panel_uses_plus_menu_and_all_sessions_only(self):
        left_panel = CHAT_LEFT_PANEL.read_text(encoding="utf-8")

        self.assertIn("id: addMenuButton", left_panel)
        self.assertIn("id: addMenuPopup", left_panel)
        self.assertIn("contentItem: GlassSurface", left_panel)
        self.assertIn('ToolTip.text: "添加"', left_panel)
        self.assertIn('text: "创建群聊"', left_panel)
        self.assertIn('text: "添加好友"', left_panel)
        self.assertIn('text: "加入群聊"', left_panel)
        self.assertNotIn("Layout.preferredHeight: 60", left_panel)
        self.assertNotIn("sourceComponent: placeholderPaneComponent", left_panel)
        self.assertNotIn("id: placeholderPaneComponent", left_panel)
        self.assertNotIn('placeholderText: "搜索用户ID（u#########）"', left_panel)
        self.assertNotIn('root.sessionFilter = 1', left_panel)
        self.assertNotIn('root.sessionFilter = 2', left_panel)
        self.assertIn("function currentSessionModel() { return dialogModel }", left_panel)

    def test_group_conversation_header_uses_single_settings_entry(self):
        conversation = CHAT_CONVERSATION_PANE.read_text(encoding="utf-8")

        header_start = conversation.index("id: groupSettingsButton")
        header_end = conversation.index("Rectangle {\n            Layout.fillWidth: true\n            Layout.fillHeight: true", header_start)
        header = conversation[header_start:header_end]

        self.assertIn('text: "..."', header)
        self.assertIn('ToolTip.text: "群设置"', header)
        self.assertIn("root.openGroupManageRequested()", header)
        self.assertNotIn('text: root.currentDialogPinned ? "取消置顶" : "置顶"', header)
        self.assertNotIn('text: root.currentDialogMuted ? "取消静音" : "静音"', header)
        self.assertNotIn('text: "群管理"', header)

    def test_group_settings_contains_dialog_pin_and_mute(self):
        panel = GROUP_MANAGEMENT_PANEL.read_text(encoding="utf-8")
        info = (REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/chat/group/GroupInfoPane.qml").read_text(encoding="utf-8")
        shell = CHAT_SHELL_PAGE.read_text(encoding="utf-8")

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
        self.assertIn("currentDialogPinned: controller.currentDialogPinned", shell)
        self.assertIn("currentDialogMuted: controller.currentDialogMuted", shell)


if __name__ == "__main__":
    unittest.main()
