import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CHAT_FEATURE_VIEW = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/chat/view"
CHAT_LEFT_PANEL = CHAT_FEATURE_VIEW / "ChatLeftPanel.qml"
CHAT_CONVERSATION = CHAT_FEATURE_VIEW / "ChatConversationPane.qml"
CHAT_MESSAGE_LIST_VIEW = CHAT_FEATURE_VIEW / "conversation/ChatMessageListView.qml"
CHAT_MESSAGE_DELEGATE = CHAT_FEATURE_VIEW / "conversation/ChatMessageDelegate.qml"
CHAT_MESSAGE_BUBBLE = CHAT_FEATURE_VIEW / "conversation/ChatMessageBubble.qml"
CHAT_MESSAGE_BODY_LOADER = CHAT_FEATURE_VIEW / "conversation/ChatMessageBodyLoader.qml"
CHAT_MESSAGE_META = CHAT_FEATURE_VIEW / "conversation/ChatMessageMeta.qml"
MOMENTS_FEED = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/moments/view/MomentsFeedPane.qml"
MOMENTS_DELEGATE = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/moments/view/MomentsDelegate.qml"
MAIN_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/app/Main.qml"
LINUX_MAIN_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/linux/Main.qml"
LOGIN_TOP_BAR_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/auth/view/components/LoginTopBar.qml"
FEATURE_AUTH_VIEW = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/auth/view"
MAIN_CPP = REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/bootstrap/main.cpp"
SESSION_AUTH_LOGIN_RESPONSE = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/session/SessionAuthCoordinatorLoginResponse.cpp"
)
APP_SESSION_LOGOUT = REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/session/AppSessionCoordinatorLogout.cpp"
APP_CONTROLLER_NAVIGATION = REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/controller/AppControllerNavigation.cpp"
APP_CONTROLLER_CPP = REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/controller/AppController.cpp"
APP_SESSION_AUTH_PORT_BINDER = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/composition/AppSessionAuthPortBinder.cpp"
)
APP_SESSION_LOGOUT_PORT_BINDER = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/composition/AppSessionLogoutPortBinder.cpp"
)
APP_SIGNAL_BINDER = REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/composition/AppSignalBinder.cpp"
APP_CHAT_TRANSPORT_SIGNAL_BINDER = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/composition/AppChatTransportSignalBinder.cpp"
)
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
        self.assertIn("reuseItems: true", message_list_source)
        message_list_block = message_list_source[
            message_list_source.index("id: messageList") : message_list_source.index("delegate: ChatMessageDelegate")
        ]
        self.assertNotIn("WheelHandler", message_list_block)

    def test_chat_message_delegate_is_split_for_reuse_safe_rows(self):
        delegate = CHAT_MESSAGE_DELEGATE.read_text(encoding="utf-8")
        message_list_source = CHAT_MESSAGE_LIST_VIEW.read_text(encoding="utf-8")

        self.assertIn("reuseItems: true", message_list_source)
        self.assertIn("ChatMessageBubble", delegate)
        self.assertIn("ChatMessageMeta", delegate)
        self.assertIn("function resetTransientState()", delegate)
        self.assertIn("onRowIdentityChanged: resetTransientState()", delegate)
        self.assertIn("actionMenu.close()", delegate)
        for token in ("ChatMessageTextBody", "ChatMessageImageBody", "ChatMessageFileBody", "GlassButton"):
            with self.subTest(token=token):
                self.assertNotIn(token, delegate)

    def test_moments_feed_has_scrollable_stable_delegate_height(self):
        feed = MOMENTS_FEED.read_text(encoding="utf-8")
        delegate = MOMENTS_DELEGATE.read_text(encoding="utf-8")

        self.assertIn("interactive: contentHeight > height", feed)
        self.assertIn("boundsBehavior: Flickable.StopAtBounds", feed)
        self.assertIn("ScrollBar.vertical: GlassScrollBar", feed)
        self.assertIn("height: item ? item.implicitHeight : 0", feed)
        self.assertIn("height: implicitHeight", delegate)

    def test_chat_bubbles_bind_actual_geometry(self):
        delegate = CHAT_MESSAGE_DELEGATE.read_text(encoding="utf-8")
        bubble = CHAT_MESSAGE_BUBBLE.read_text(encoding="utf-8")
        body_loader = CHAT_MESSAGE_BODY_LOADER.read_text(encoding="utf-8")
        meta = CHAT_MESSAGE_META.read_text(encoding="utf-8")

        self.assertIn("readonly property real bubblePreferredWidth", delegate)
        self.assertIn("id: textMeasure", delegate)
        self.assertIn("id: fileMeasure", delegate)
        self.assertIn("id: translationTextMeasure", delegate)
        self.assertIn("readonly property real bubbleX", delegate)
        self.assertIn("readonly property real bubbleY", delegate)
        self.assertIn("bubbleWidth: root.bubblePreferredWidth", delegate)
        self.assertNotIn("ChatMessageTextBody", delegate)
        self.assertNotIn("ChatMessageFileBody", delegate)
        self.assertNotIn("ChatMessageImageBody", delegate)

        delegate_bubble_block = delegate[delegate.index("ChatMessageBubble {") : delegate.index("ChatMessageMeta {")]
        self.assertIn("x: root.bubbleX", delegate_bubble_block)
        self.assertIn("y: root.bubbleY", delegate_bubble_block)
        self.assertNotIn("anchors.left", delegate_bubble_block)
        self.assertNotIn("anchors.right", delegate_bubble_block)

        self.assertIn("width: root.bubbleWidth", bubble)
        self.assertIn("height: implicitHeight", bubble)
        self.assertIn("implicitHeight: bubbleColumn.implicitHeight + root.verticalPadding * 2", bubble)
        self.assertNotIn("bubbleColumn.implicitWidth", bubble)

        self.assertIn("sourceComponent: bodyComponent()", body_loader)
        self.assertIn("ChatMessageTextBody", body_loader)
        self.assertIn("ChatMessageImageBody", body_loader)
        self.assertIn("ChatMessageFileBody", body_loader)
        self.assertIn("width: implicitWidth", body_loader)
        self.assertIn("height: implicitHeight", body_loader)

        self.assertIn("id: translationBubble", meta)
        self.assertIn("width: root.translationPreferredWidth", meta)
        self.assertIn("height: implicitHeight", meta)
        self.assertIn("ChatMessageStatusBadge", meta)

    def test_login_and_chat_use_mutually_exclusive_top_level_windows(self):
        for path in (MAIN_QML, LINUX_MAIN_QML):
            qml = path.read_text(encoding="utf-8")
            sync_windows = extract_cpp_function(qml, "function syncWindowsByPage")

            self.assertIn("property var loginWindowRef: null", qml)
            self.assertIn("property var chatWindowRef: null", qml)
            self.assertIn("property int windowSwitchToken: 0", qml)
            self.assertIn("function ensureLoginWindow()", qml)
            self.assertIn("function ensureChatWindow()", qml)
            self.assertIn("function destroyLoginWindow()", qml)
            self.assertIn("function destroyChatWindow()", qml)
            self.assertIn("function showLoginWindow()", qml)
            self.assertIn("function showChatWindow()", qml)
            self.assertIn("function syncWindowsByPage()", qml)
            self.assertIn("destroyLoginWindow()", sync_windows)
            self.assertIn("destroyChatWindow()", sync_windows)
            self.assertIn("scheduleWindowHandoff", sync_windows)
            self.assertNotIn("Qt.callLater(function() {", sync_windows)
            self.assertNotIn("showChatWindow()", sync_windows)
            self.assertNotIn("showLoginWindow()", sync_windows)
            self.assertLess(sync_windows.index("destroyLoginWindow()"), sync_windows.index("scheduleWindowHandoff"))
            self.assertLess(
                sync_windows.index("destroyChatWindow()"),
                sync_windows.rindex("scheduleWindowHandoff"),
            )
            self.assertIn("visible: shell.page === ShellViewModel.LoginPage", qml)
            self.assertIn("visible: shell.page === ShellViewModel.RegisterPage", qml)
            self.assertIn("visible: shell.page === ShellViewModel.ResetPage", qml)
            self.assertIn("Component {\n        id: chatWindowComponent", qml)
            self.assertIn("sourceComponent: chatShellPageComponent", qml)
            self.assertIn("onClosing: Qt.quit()", qml)
            self.assertNotIn("StackLayout", qml)
            self.assertNotIn("shell.page === ShellViewModel.ChatPage ? 3 : 0", qml)
            self.assertNotIn("property var appWindowRef: null", qml)
            self.assertNotIn("function ensureAppWindow()", qml)
            self.assertNotIn("function configureAppWindowForPage(win)", qml)
            self.assertNotIn("property int displayedPage", qml)
            self.assertNotIn("active: root.chatPageActive", qml)
            self.assertIn("function scheduleWindowHandoff", qml)
            self.assertNotIn("Window.window.startSystemMove()", qml)

        login_top_bar = LOGIN_TOP_BAR_QML.read_text(encoding="utf-8")
        login_page = (FEATURE_AUTH_VIEW / "LoginPage.qml").read_text(encoding="utf-8")
        self.assertIn("onPressed:", login_top_bar)
        self.assertIn("signal dragMoveRequested()", login_top_bar)
        self.assertIn("onDragMoveRequested:", login_page)
        self.assertIn("Window.window.startSystemMove()", login_page)

        main_cpp = MAIN_CPP.read_text(encoding="utf-8")
        self.assertIn("setQuitOnLastWindowClosed(false)", main_cpp)

    def test_window_switch_retires_old_window_before_next_show(self):
        for path in (MAIN_QML, LINUX_MAIN_QML):
            qml = path.read_text(encoding="utf-8")
            self.assertIn("function retireWindow", qml)
            self.assertIn("function destroyLoginWindow", qml)
            self.assertIn("function destroyChatWindow", qml)
            retire_window = extract_cpp_function(qml, "function retireWindow")
            destroy_login = extract_cpp_function(qml, "function destroyLoginWindow")
            destroy_chat = extract_cpp_function(qml, "function destroyChatWindow")
            sync_windows = extract_cpp_function(qml, "function syncWindowsByPage")

            self.assertIn("win.opacity = 0", retire_window)
            self.assertIn("win.visible = false", retire_window)
            self.assertIn("win.hide()", retire_window)
            self.assertIn("win.destroy()", retire_window)
            self.assertIn("retireWindow(win)", destroy_login)
            self.assertIn("retireWindow(win)", destroy_chat)
            self.assertIn("return true", destroy_login)
            self.assertIn("return true", destroy_chat)
            self.assertLess(destroy_login.index("root.loginWindowRef = null"), destroy_login.index("retireWindow(win)"))
            self.assertLess(destroy_chat.index("root.chatWindowRef = null"), destroy_chat.index("retireWindow(win)"))
            self.assertIn("const token = ++windowSwitchToken", sync_windows)
            self.assertIn("scheduleWindowHandoff", sync_windows)
            self.assertLess(sync_windows.index("destroyLoginWindow()"), sync_windows.index("scheduleWindowHandoff"))

    def test_window_switch_waits_for_retired_native_surface_before_next_show(self):
        for path in (MAIN_QML, LINUX_MAIN_QML):
            qml = path.read_text(encoding="utf-8")
            retire_window = extract_cpp_function(qml, "function retireWindow")
            destroy_login = extract_cpp_function(qml, "function destroyLoginWindow")
            destroy_chat = extract_cpp_function(qml, "function destroyChatWindow")
            self.assertIn("function scheduleWindowHandoff", qml)
            self.assertIn("function continueWindowHandoff", qml)
            schedule_handoff = extract_cpp_function(qml, "function scheduleWindowHandoff")
            continue_handoff = extract_cpp_function(qml, "function continueWindowHandoff")
            show_target = extract_cpp_function(qml, "function showWindowForHandoffTarget")
            sync_windows = extract_cpp_function(qml, "function syncWindowsByPage")

            self.assertIn("property bool handoffRetiredWindowPending: false", qml)
            self.assertIn("property int handoffMinimumPasses", qml)
            self.assertIn("property int handoffPasses: 0", qml)
            self.assertIn("property int handoffTargetPage", qml)
            self.assertIn("Timer {\n        id: windowHandoffTimer", qml)
            self.assertIn("onTriggered: root.continueWindowHandoff()", qml)

            self.assertIn("win.retiring = true", retire_window)
            self.assertIn("win.opacity = 0", retire_window)
            self.assertIn("win.visible = false", retire_window)
            self.assertIn("win.hide()", retire_window)
            self.assertIn("win.destroy()", retire_window)

            self.assertIn("return true", destroy_login)
            self.assertIn("return true", destroy_chat)
            self.assertIn("return false", destroy_login)
            self.assertIn("return false", destroy_chat)

            self.assertIn("root.handoffRetiredWindowPending = retiredWindowPending", schedule_handoff)
            self.assertIn("root.handoffPasses = 0", schedule_handoff)
            self.assertNotIn("retiredWindow.hide()", schedule_handoff)
            self.assertIn("windowHandoffTimer.restart()", schedule_handoff)
            self.assertIn("root.handoffRetiredWindowPending", continue_handoff)
            self.assertIn("root.handoffPasses < root.handoffMinimumPasses", continue_handoff)
            self.assertIn("showWindowForHandoffTarget(root.handoffTargetPage)", continue_handoff)
            self.assertIn("showChatWindow()", show_target)
            self.assertIn("showLoginWindow()", show_target)

            self.assertIn("const retiredWindowPending = destroyLoginWindow()", sync_windows)
            self.assertIn("scheduleWindowHandoff(retiredWindowPending, token, ShellViewModel.ChatPage)", sync_windows)
            self.assertIn("const retiredWindowPending = destroyChatWindow()", sync_windows)
            self.assertIn("scheduleWindowHandoff(retiredWindowPending, token, shell.page)", sync_windows)
            self.assertNotIn("Qt.callLater(function() {", sync_windows)

    def test_separate_login_and_chat_windows_use_their_own_size_constraints(self):
        for path in (MAIN_QML, LINUX_MAIN_QML):
            qml = path.read_text(encoding="utf-8")
            self.assertIn("function configureLoginWindow", qml)
            self.assertIn("function configureChatWindow", qml)
            configure_login = extract_cpp_function(qml, "function configureLoginWindow")
            configure_chat = extract_cpp_function(qml, "function configureChatWindow")

            self.assertIn("win.minimumWidth = root.loginWindowSize.width", configure_login)
            self.assertIn("win.minimumHeight = root.loginWindowSize.height", configure_login)
            self.assertIn("win.maximumWidth = root.loginWindowSize.width", configure_login)
            self.assertIn("win.maximumHeight = root.loginWindowSize.height", configure_login)
            self.assertIn("win.width = root.loginWindowSize.width", configure_login)
            self.assertIn("win.height = root.loginWindowSize.height", configure_login)

            self.assertIn("win.maximumWidth = root.unboundedWindowSize.width", configure_chat)
            self.assertIn("win.maximumHeight = root.unboundedWindowSize.height", configure_chat)
            self.assertIn("win.minimumWidth = root.chatWindowMinimumSize.width", configure_chat)
            self.assertIn("win.minimumHeight = root.chatWindowMinimumSize.height", configure_chat)
            self.assertIn("win.width = root.chatWindowSize.width", configure_chat)
            self.assertIn("win.height = root.chatWindowSize.height", configure_chat)
            self.assertIn("width: root.loginWindowSize.width", qml)
            self.assertIn("height: root.loginWindowSize.height", qml)
            self.assertIn("minimumWidth: root.loginWindowSize.width", qml)
            self.assertIn("minimumHeight: root.loginWindowSize.height", qml)
            self.assertIn("maximumWidth: root.loginWindowSize.width", qml)
            self.assertIn("maximumHeight: root.loginWindowSize.height", qml)
            self.assertIn("width: root.chatWindowSize.width", qml)
            self.assertIn("height: root.chatWindowSize.height", qml)
            self.assertIn("minimumWidth: root.chatWindowMinimumSize.width", qml)
            self.assertIn("minimumHeight: root.chatWindowMinimumSize.height", qml)

    def test_login_and_logout_switch_pages_before_transport_teardown(self):
        source = SESSION_AUTH_LOGIN_RESPONSE.read_text(encoding="utf-8")
        app_controller = APP_CONTROLLER_CPP.read_text(encoding="utf-8")
        login_port_binder = APP_SESSION_AUTH_PORT_BINDER.read_text(encoding="utf-8")
        logout_port_binder = APP_SESSION_LOGOUT_PORT_BINDER.read_text(encoding="utf-8")

        login_block = extract_cpp_function(source, "void SessionAuthCoordinator::onLoginHttpFinished")
        self.assertIn("_port.applyLoginSuccess(server_info, obj);", login_block)
        self.assertNotIn("setPage(AppController::ChatPage);", login_block)
        self.assertIn("setPage(_constants.chatPage);", login_port_binder)
        self.assertNotIn("setPage(AppController::ChatPage);", login_port_binder)
        self.assertNotIn("setPage(AppController::ChatPage);", app_controller)
        self.assertLess(
            login_port_binder.index("setPage(_constants.chatPage);"),
            login_port_binder.index("_gateway.chatTransport()->connectToServer(serverInfo);"),
        )

        navigation_source = APP_CONTROLLER_NAVIGATION.read_text(encoding="utf-8")
        logout_block_start = navigation_source.index("void AppController::switchToLogin()")
        logout_block_end = navigation_source.index("void AppController::switchToRegister()")
        logout_block = navigation_source[logout_block_start:logout_block_end]
        self.assertIn("setPage(LoginPage);", logout_block)
        self.assertLess(
            logout_block.index("setPage(LoginPage);"),
            logout_block.index("_session_coordinator->resetForLogout();"),
        )
        reset_block = extract_cpp_function(
            APP_SESSION_LOGOUT.read_text(encoding="utf-8"), "void AppSessionCoordinator::resetForLogout"
        )
        self.assertIn("invokeIfSet(_logout_port.closeNetworkResources);", reset_block)
        self.assertIn("if (const auto transport = _gateway.chatTransport())", logout_port_binder)
        self.assertIn("transport->CloseConnection();", logout_port_binder)
        self.assertNotIn("_gateway.chatTransport()->CloseConnection();", app_controller)

    def test_stale_account_switch_disconnect_is_ignored_before_chat_page_reconnect(self):
        app_controller = APP_CONTROLLER_CPP.read_text(encoding="utf-8")
        signal_binder = APP_CHAT_TRANSPORT_SIGNAL_BINDER.read_text(encoding="utf-8")
        self.assertIn("_chat_connection_coordinator->onConnectionClosed();", signal_binder)
        self.assertNotIn("_chat_connection_coordinator->onConnectionClosed();", app_controller)
        self.assertNotIn("&AppController::onConnectionClosed", app_controller + signal_binder)

        source = APP_CHAT_CONNECTION_COORDINATOR.read_text(encoding="utf-8")
        body = extract_cpp_function(source, "void AppChatConnectionCoordinator::onConnectionClosed")

        ignore_index = body.index("if (snapshot.ignoreNextLoginDisconnect)")
        page_branch_index = body.index("if (!snapshot.isChatPage)")
        chat_reconnect_index = body.index("if (_port.callVisible())")

        self.assertLess(ignore_index, page_branch_index)
        self.assertLess(ignore_index, chat_reconnect_index)
        ignored_block = body[ignore_index:page_branch_index]
        self.assertIn("_port.setIgnoreNextLoginDisconnect(false);", ignored_block)
        self.assertIn("resetReconnectState();", ignored_block)
        self.assertIn("resetHeartbeatTracking();", ignored_block)
        self.assertIn("return;", ignored_block)
        self.assertNotIn("_app.", body)


if __name__ == "__main__":
    unittest.main()
