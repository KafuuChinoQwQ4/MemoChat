import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
CHAT_LEFT_PANEL = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/chat/ChatLeftPanel.qml"
CHAT_CONVERSATION = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/chat/ChatConversationPane.qml"
CHAT_MESSAGE_DELEGATE = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/chat/conversation/ChatMessageDelegate.qml"
MOMENTS_FEED = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/moments/MomentsFeedPane.qml"
MOMENTS_DELEGATE = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/moments/MomentsDelegate.qml"
MAIN_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/Main.qml"
LINUX_MAIN_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/linux/Main.qml"
APP_CONTROLLER_SESSION = REPO_ROOT / "apps/client/desktop/MemoChat-qml/AppControllerSession.cpp"
APP_CONTROLLER_NAVIGATION = REPO_ROOT / "apps/client/desktop/MemoChat-qml/AppControllerNavigation.cpp"


class QmlScrollWindowHandoffTests(unittest.TestCase):
    def test_chat_lists_use_native_listview_scrolling(self):
        left_panel = CHAT_LEFT_PANEL.read_text(encoding="utf-8")
        conversation = CHAT_CONVERSATION.read_text(encoding="utf-8")

        self.assertIn("interactive: contentHeight > height", left_panel)
        self.assertIn("boundsBehavior: Flickable.StopAtBounds", left_panel)
        self.assertNotIn("interactive: false", left_panel)
        self.assertNotIn("WheelHandler", left_panel)

        self.assertIn("interactive: contentHeight > height", conversation)
        self.assertIn("boundsBehavior: Flickable.StopAtBounds", conversation)
        message_list_block = conversation[
            conversation.index("id: messageList"):
            conversation.index("delegate: ChatMessageDelegate")
        ]
        self.assertNotIn("WheelHandler", message_list_block)

    def test_moments_feed_has_scrollable_stable_delegate_height(self):
        feed = MOMENTS_FEED.read_text(encoding="utf-8")
        delegate = MOMENTS_DELEGATE.read_text(encoding="utf-8")

        self.assertIn("interactive: contentHeight > height", feed)
        self.assertIn("boundsBehavior: Flickable.StopAtBounds", feed)
        self.assertIn("ScrollBar.vertical: GlassScrollBar", feed)
        self.assertIn("height: item ? item.height : 0", feed)
        self.assertIn("height: implicitHeight", delegate)

    def test_chat_bubbles_bind_actual_geometry(self):
        delegate = CHAT_MESSAGE_DELEGATE.read_text(encoding="utf-8")

        bubble_start = delegate.index("id: bubble")
        translation_start = delegate.index("id: translationBubble")
        file_start = delegate.index("id: fileComp")
        call_start = delegate.index("id: callComp")

        self.assertIn("id: textMeasure", delegate)
        self.assertIn("id: fileMeasure", delegate)
        self.assertIn("id: translationTextMeasure", delegate)
        self.assertIn("readonly property real bubblePreferredWidth", delegate)
        self.assertIn("width: root.bubblePreferredWidth", delegate[bubble_start:translation_start])
        self.assertNotIn("bubbleColumn.implicitWidth", delegate)
        self.assertIn("height: implicitHeight", delegate[bubble_start:translation_start])
        self.assertIn("width: root.translationPreferredWidth", delegate[translation_start:file_start])
        self.assertIn("height: implicitHeight", delegate[translation_start:file_start])
        self.assertIn("width: implicitWidth", delegate[file_start:call_start])
        self.assertIn("height: implicitHeight", delegate[file_start:call_start])
        self.assertIn("width: implicitWidth", delegate[call_start:])
        self.assertIn("height: implicitHeight", delegate[call_start:])

    def test_login_chat_uses_single_stack_window(self):
        for path in (MAIN_QML, LINUX_MAIN_QML):
            qml = path.read_text(encoding="utf-8")

            self.assertIn("readonly property bool chatPageActive", qml)
            self.assertIn("root.contentItem.enabled = true", qml)
            self.assertIn("root.opacity = 1.0", qml)
            self.assertIn("function showChatPage()", qml)
            self.assertIn("controller.page === AppController.ChatPage ? 3 : 0", qml)
            self.assertIn("active: controller.page === AppController.ChatPage", qml)
            self.assertIn("sourceComponent: chatShellPageComponent", qml)
            self.assertIn("enabled: !root.chatPageActive", qml)
            self.assertNotIn("chatWindowRef", qml)
            self.assertNotIn("chatWindowComponent", qml)
            self.assertNotIn("windowHandoffToken", qml)
            self.assertNotIn("scheduleWindowHandoff", qml)

    def test_login_and_logout_switch_pages_before_transport_teardown(self):
        source = APP_CONTROLLER_SESSION.read_text(encoding="utf-8")

        login_block_start = source.index("void AppController::onLoginHttpFinished")
        login_block_end = source.index("void AppController::onSwitchToChat")
        login_block = source[login_block_start:login_block_end]
        self.assertIn("setPage(ChatPage);", login_block)
        self.assertLess(login_block.index("setPage(ChatPage);"),
                        login_block.index("_gateway.chatTransport()->connectToServer(server_info);"))

        navigation_source = APP_CONTROLLER_NAVIGATION.read_text(encoding="utf-8")
        logout_block_start = navigation_source.index("void AppController::switchToLogin()")
        logout_block_end = navigation_source.index("void AppController::switchToRegister()")
        logout_block = navigation_source[logout_block_start:logout_block_end]
        self.assertIn("setPage(LoginPage);", logout_block)
        self.assertLess(logout_block.index("setPage(LoginPage);"),
                        logout_block.index("_gateway.chatTransport()->CloseConnection();"))


if __name__ == "__main__":
    unittest.main()
