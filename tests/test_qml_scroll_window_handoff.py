import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
CHAT_LEFT_PANEL = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/chat/ChatLeftPanel.qml"
CHAT_CONVERSATION = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/chat/ChatConversationPane.qml"
CHAT_MESSAGE_LIST_VIEW = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/chat/conversation/ChatMessageListView.qml"
CHAT_MESSAGE_DELEGATE = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/chat/conversation/ChatMessageDelegate.qml"
MOMENTS_FEED = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/moments/MomentsFeedPane.qml"
MOMENTS_DELEGATE = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/moments/MomentsDelegate.qml"
MAIN_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/app/Main.qml"
LINUX_MAIN_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/linux/Main.qml"
LOGIN_TOP_BAR_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/components/LoginTopBar.qml"
MAIN_CPP = REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/bootstrap/main.cpp"
SESSION_AUTH_LOGIN_RESPONSE = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/session/SessionAuthCoordinatorLoginResponse.cpp"
)
APP_CONTROLLER_NAVIGATION = REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/controller/AppControllerNavigation.cpp"
APP_CONTROLLER_CPP = REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/controller/AppController.cpp"
APP_CHAT_CONNECTION_COORDINATOR = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/connection/AppChatConnectionCoordinator.cpp"
)


def extract_cpp_function(source: str, signature: str) -> str:
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


class QmlScrollWindowHandoffTests(unittest.TestCase):
    def test_chat_lists_use_native_listview_scrolling(self):
        left_panel = CHAT_LEFT_PANEL.read_text(encoding="utf-8")
        conversation = CHAT_CONVERSATION.read_text(encoding="utf-8")
        message_list_source = CHAT_MESSAGE_LIST_VIEW.read_text(encoding="utf-8")

        self.assertIn("interactive: contentHeight > height", left_panel)
        self.assertIn("boundsBehavior: Flickable.StopAtBounds", left_panel)
        self.assertNotIn("interactive: false", left_panel)
        self.assertNotIn("WheelHandler", left_panel)

        self.assertIn("ChatMessageListView", conversation)
        self.assertIn("interactive: contentHeight > height", message_list_source)
        self.assertIn("boundsBehavior: Flickable.StopAtBounds", message_list_source)
        message_list_block = message_list_source[
            message_list_source.index("id: messageList") : message_list_source.index("delegate: ChatMessageDelegate")
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

    def test_login_and_chat_use_single_top_level_window_with_exclusive_loaders(self):
        for path in (MAIN_QML, LINUX_MAIN_QML):
            qml = path.read_text(encoding="utf-8")

            self.assertIn("property var appWindowRef: null", qml)
            self.assertIn("function ensureAppWindow()", qml)
            self.assertIn("function configureAppWindowForPage(win)", qml)
            self.assertIn("function syncWindowsByPage()", qml)
            self.assertIn("configureAppWindowForPage(win)", qml)
            self.assertIn("showWindowCentered(win)", qml)
            self.assertIn("visible: controller.page === AppController.LoginPage", qml)
            self.assertIn("visible: controller.page === AppController.RegisterPage", qml)
            self.assertIn("visible: controller.page === AppController.ResetPage", qml)
            self.assertIn("active: root.chatPageActive", qml)
            self.assertIn("sourceComponent: chatShellPageComponent", qml)
            self.assertIn("onClosing: Qt.quit()", qml)
            self.assertNotIn("StackLayout", qml)
            self.assertNotIn("controller.page === AppController.ChatPage ? 3 : 0", qml)
            self.assertNotIn("loginWindowRef", qml)
            self.assertNotIn("chatWindowRef", qml)
            self.assertNotIn("function destroyLoginWindow()", qml)
            self.assertNotIn("function destroyChatWindow()", qml)
            self.assertNotIn("windowHandoffToken", qml)
            self.assertNotIn("scheduleWindowHandoff", qml)
            self.assertNotIn("Window.window.startSystemMove()", qml)

        login_top_bar = LOGIN_TOP_BAR_QML.read_text(encoding="utf-8")
        login_page = (REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/auth/LoginPage.qml").read_text(encoding="utf-8")
        self.assertIn("onPressed:", login_top_bar)
        self.assertIn("signal dragMoveRequested()", login_top_bar)
        self.assertIn("onDragMoveRequested:", login_page)
        self.assertIn("Window.window.startSystemMove()", login_page)

        main_cpp = MAIN_CPP.read_text(encoding="utf-8")
        self.assertIn("setQuitOnLastWindowClosed(false)", main_cpp)

    def test_page_size_handoff_releases_login_constraints_before_chat_resize(self):
        for path in (MAIN_QML, LINUX_MAIN_QML):
            qml = path.read_text(encoding="utf-8")
            configure_window = extract_cpp_function(qml, "function configureAppWindowForPage")
            sync_windows = extract_cpp_function(qml, "function syncWindowsByPage")

            self.assertIn("const size = targetWindowSize()", configure_window)
            self.assertIn(
                "const minimumSize = loginMode ? root.loginWindowSize : root.chatWindowMinimumSize", configure_window
            )
            self.assertIn(
                "const maximumSize = loginMode ? root.loginWindowSize : root.unboundedWindowSize", configure_window
            )
            self.assertIn("win.maximumWidth = root.unboundedWindowSize.width", configure_window)
            self.assertIn("win.maximumHeight = root.unboundedWindowSize.height", configure_window)
            self.assertLess(
                configure_window.index("win.maximumWidth = root.unboundedWindowSize.width"),
                configure_window.index("win.minimumWidth = minimumSize.width"),
            )
            self.assertLess(
                configure_window.index("win.maximumHeight = root.unboundedWindowSize.height"),
                configure_window.index("win.minimumHeight = minimumSize.height"),
            )
            self.assertLess(
                configure_window.index("win.maximumWidth = root.unboundedWindowSize.width"),
                configure_window.index("win.width = size.width"),
            )
            self.assertLess(
                configure_window.index("win.maximumHeight = root.unboundedWindowSize.height"),
                configure_window.index("win.height = size.height"),
            )
            self.assertIn("win.maximumWidth = maximumSize.width", configure_window)
            self.assertIn("win.maximumHeight = maximumSize.height", configure_window)

            self.assertIn("const targetSize = targetWindowSize()", sync_windows)
            self.assertIn("AppWindowRuntime.shouldHideResize(win, targetSize.width, targetSize.height)", sync_windows)
            self.assertLess(
                sync_windows.index("win.visible = false"),
                sync_windows.index("configureAppWindowForPage(win)"),
            )
            self.assertIn("width: root.loginWindowSize.width", qml)
            self.assertIn("height: root.loginWindowSize.height", qml)
            self.assertIn("minimumWidth: root.loginWindowSize.width", qml)
            self.assertIn("minimumHeight: root.loginWindowSize.height", qml)
            self.assertIn("maximumWidth: root.loginWindowSize.width", qml)
            self.assertIn("maximumHeight: root.loginWindowSize.height", qml)

    def test_login_and_logout_switch_pages_before_transport_teardown(self):
        source = SESSION_AUTH_LOGIN_RESPONSE.read_text(encoding="utf-8")

        login_block = extract_cpp_function(source, "void SessionAuthCoordinator::onLoginHttpFinished")
        self.assertIn("setPage(AppController::ChatPage);", login_block)
        self.assertLess(
            login_block.index("setPage(AppController::ChatPage);"),
            login_block.index("_gateway.chatTransport()->connectToServer(server_info);"),
        )

        navigation_source = APP_CONTROLLER_NAVIGATION.read_text(encoding="utf-8")
        logout_block_start = navigation_source.index("void AppController::switchToLogin()")
        logout_block_end = navigation_source.index("void AppController::switchToRegister()")
        logout_block = navigation_source[logout_block_start:logout_block_end]
        self.assertIn("setPage(LoginPage);", logout_block)
        self.assertLess(
            logout_block.index("setPage(LoginPage);"),
            logout_block.index("_gateway.chatTransport()->CloseConnection();"),
        )

    def test_stale_account_switch_disconnect_is_ignored_before_chat_page_reconnect(self):
        app_controller = APP_CONTROLLER_CPP.read_text(encoding="utf-8")
        self.assertIn("_chat_connection_coordinator->onConnectionClosed();", app_controller)
        self.assertNotIn("&AppController::onConnectionClosed", app_controller)

        source = APP_CHAT_CONNECTION_COORDINATOR.read_text(encoding="utf-8")
        body = extract_cpp_function(source, "void AppChatConnectionCoordinator::onConnectionClosed")

        ignore_index = body.index("if (_app._chat_recovery_state.ignoreNextLoginDisconnect)")
        page_branch_index = body.index("if (_app._page != AppController::ChatPage)")
        chat_reconnect_index = body.index("if (_app._call_session_model.visible())")

        self.assertLess(ignore_index, page_branch_index)
        self.assertLess(ignore_index, chat_reconnect_index)
        ignored_block = body[ignore_index:page_branch_index]
        self.assertIn("_app._chat_recovery_state.ignoreNextLoginDisconnect = false;", ignored_block)
        self.assertIn("resetReconnectState();", ignored_block)
        self.assertIn("resetHeartbeatTracking();", ignored_block)
        self.assertIn("return;", ignored_block)


if __name__ == "__main__":
    unittest.main()
