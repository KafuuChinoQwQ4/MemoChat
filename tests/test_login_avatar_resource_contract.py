import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
LOGIN_TOP_BAR = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/components/LoginTopBar.qml"
SHARED_QRC = REPO_ROOT / "apps/client/desktop/MemoChatShared/rc.qrc"
SHARED_MAIN_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/Main.qml"
LINUX_MAIN_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/linux/Main.qml"
ICON_PATH_UTILS = REPO_ROOT / "apps/client/desktop/MemoChat-qml/IconPathUtils.h"
APP_CONTROLLER = REPO_ROOT / "apps/client/desktop/MemoChat-qml/AppController.cpp"
APP_CONTROLLER_STATE = REPO_ROOT / "apps/client/desktop/MemoChat-qml/AppControllerState.cpp"
AUTH_CONTROLLER = REPO_ROOT / "apps/client/desktop/MemoChat-qml/AuthController.cpp"


class LoginAvatarResourceContractTests(unittest.TestCase):
    def test_login_top_bar_exposes_long_press_window_move(self):
        source = LOGIN_TOP_BAR.read_text(encoding="utf-8")
        login_page = (REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/LoginPage.qml").read_text(encoding="utf-8")

        self.assertIn("import QtQuick.Window 2.15", source)
        self.assertIn("MouseArea", source)
        self.assertIn("onPressAndHold", source)
        self.assertIn("signal dragMoveRequested()", source)
        self.assertIn("root.dragMoveRequested()", source)
        self.assertIn("onDragMoveRequested:", login_page)
        self.assertIn("Window.window.startSystemMove()", login_page)
        self.assertIn("settingsClicked()", source)

    def test_default_avatar_is_registered_as_png_backed_resource(self):
        source = SHARED_QRC.read_text(encoding="utf-8")

        self.assertIn("<file>res/head_1.png</file>", source)
        self.assertIn('<file alias="res/head_1.jpg">res/head_1.png</file>', source)

        icon_utils = ICON_PATH_UTILS.read_text(encoding="utf-8")
        self.assertIn('QStringLiteral("qrc:/res/head_1.png")', icon_utils)
        self.assertIn('if (icon.compare(QStringLiteral("head_1.jpg"), Qt::CaseInsensitive) == 0)', icon_utils)

        controller = APP_CONTROLLER.read_text(encoding="utf-8")
        self.assertIn('_current_user_icon("qrc:/res/head_1.png")', controller)
        self.assertIn('_current_contact_icon("qrc:/res/head_1.png")', controller)
        self.assertIn('_current_chat_peer_icon("qrc:/res/head_1.png")', controller)

        state = APP_CONTROLLER_STATE.read_text(encoding="utf-8")
        self.assertIn('QStringLiteral("qrc:/res/head_1.png")', state)

        auth = AUTH_CONTROLLER.read_text(encoding="utf-8")
        self.assertIn('payload["icon"] = ":/res/head_1.png";', auth)

    def test_login_windows_no_longer_depend_on_immediate_drag_handler(self):
        for path in (SHARED_MAIN_QML, LINUX_MAIN_QML):
            source = path.read_text(encoding="utf-8")
            self.assertNotIn("DragHandler {", source)


if __name__ == "__main__":
    unittest.main()
