import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
LOGIN_TOP_BAR = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/auth/view/components/LoginTopBar.qml"
APP_CORE_QRC = REPO_ROOT / "apps/client/desktop/MemoChat-qml/resources/qrc/app-core.qrc"
SHARED_MAIN_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/app/Main.qml"
LINUX_MAIN_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/linux/Main.qml"
ICON_PATH_UTILS = REPO_ROOT / "apps/client/desktop/MemoChat-qml/shared/utils/IconPathUtils.h"
APP_CONTROLLER = REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/controller/AppController.cpp"
APP_CONTROLLER_USER_STATE = REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/controller/AppControllerUserState.h"
APP_CONTROLLER_NAVIGATION = REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/controller/AppControllerNavigation.cpp"
APP_CONTROLLER_PRIVATE_HISTORY = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/controller/AppControllerPrivateHistory.cpp"
)
APP_CONTROLLER_PRIVATE_SELECTION = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/controller/AppControllerPrivateSelection.cpp"
)
APP_CONTROLLER_PROFILE_STATE = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/controller/AppControllerProfileState.cpp"
)
SESSION_CHAT_ENTRY = REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/session/SessionChatEntryCoordinator.cpp"
AUTH_CONTROLLER = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/auth/AuthController.cpp"
FEATURE_AUTH_VIEW = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/auth/view"


class LoginAvatarResourceContractTests(unittest.TestCase):
    def test_login_top_bar_exposes_long_press_window_move(self):
        source = LOGIN_TOP_BAR.read_text(encoding="utf-8")
        login_page = (FEATURE_AUTH_VIEW / "LoginPage.qml").read_text(encoding="utf-8")

        self.assertIn("import QtQuick.Window 2.15", source)
        self.assertIn("MouseArea", source)
        self.assertIn("onPressed:", source)
        self.assertIn("signal dragMoveRequested()", source)
        self.assertIn("root.dragMoveRequested()", source)
        self.assertIn("onDragMoveRequested:", login_page)
        self.assertIn("Window.window.startSystemMove()", login_page)
        self.assertIn("settingsClicked()", source)

    def test_default_avatar_is_registered_as_png_backed_resource(self):
        source = APP_CORE_QRC.read_text(encoding="utf-8")

        self.assertIn('<file alias="res/head_1.png">../icons/head_1.png</file>', source)
        self.assertIn('<file alias="res/head_1.jpg">../icons/head_1.png</file>', source)

        icon_utils = ICON_PATH_UTILS.read_text(encoding="utf-8")
        self.assertIn('QStringLiteral("qrc:/res/head_1.png")', icon_utils)
        self.assertIn('if (icon.compare(QStringLiteral("head_1.jpg"), Qt::CaseInsensitive) == 0)', icon_utils)

        state_header = APP_CONTROLLER_USER_STATE.read_text(encoding="utf-8")
        self.assertIn('QString icon = QStringLiteral("qrc:/res/head_1.png");', state_header)
        self.assertIn('QString peerIcon = QStringLiteral("qrc:/res/head_1.png");', state_header)

        profile_state = APP_CONTROLLER_PROFILE_STATE.read_text(encoding="utf-8")
        self.assertIn('QStringLiteral("qrc:/res/head_1.png")', profile_state)

        auth = AUTH_CONTROLLER.read_text(encoding="utf-8")
        self.assertIn('payload["icon"] = ":/res/head_1.png";', auth)

    def test_cpp_default_user_avatar_fallbacks_use_png_canonical_resource(self):
        for path in (
            APP_CONTROLLER_NAVIGATION,
            SESSION_CHAT_ENTRY,
            APP_CONTROLLER_PRIVATE_HISTORY,
            APP_CONTROLLER_PRIVATE_SELECTION,
        ):
            with self.subTest(path=path.name):
                source = path.read_text(encoding="utf-8")
                self.assertNotIn('QStringLiteral("qrc:/res/head_1.jpg")', source)
                self.assertNotIn('"qrc:/res/head_1.jpg"', source)
                self.assertIn("qrc:/res/head_1.png", source)

    def test_login_windows_no_longer_depend_on_immediate_drag_handler(self):
        for path in (SHARED_MAIN_QML, LINUX_MAIN_QML):
            source = path.read_text(encoding="utf-8")
            self.assertNotIn("DragHandler {", source)


if __name__ == "__main__":
    unittest.main()
