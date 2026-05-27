import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
CLIENT_DIR = REPO_ROOT / "apps/client/desktop/MemoChat-qml"

PET_ASSET_SETTINGS_H = CLIENT_DIR / "features/pet/PetAssetSettings.h"
PET_ASSET_SETTINGS_CPP = CLIENT_DIR / "features/pet/PetAssetSettings.cpp"
PET_ASSET_SETTINGS_PRIVATE_H = CLIENT_DIR / "features/pet/PetAssetSettingsPrivate.h"
PET_ASSET_SETTINGS_AVATAR_CPP = CLIENT_DIR / "features/pet/PetAssetSettingsAvatar.cpp"
PET_ASSET_SETTINGS_PERSISTENCE_CPP = CLIENT_DIR / "features/pet/PetAssetSettingsPersistence.cpp"
PET_ASSET_SETTINGS_STATE_CPP = CLIENT_DIR / "features/pet/PetAssetSettingsState.cpp"
LIVE2D_AVATAR_OPENGL_RENDERER_H = CLIENT_DIR / "live2d/Live2DAvatarOpenGLRenderer.h"
LIVE2D_AVATAR_OPENGL_RENDERER_CPP = CLIENT_DIR / "live2d/Live2DAvatarOpenGLRenderer.cpp"
PET_CHAT_WINDOW_QML = CLIENT_DIR / "qml/pet/PetChatWindow.qml"
PET_WINDOW_QML = CLIENT_DIR / "qml/pet/PetWindow.qml"
MAIN_QML = CLIENT_DIR / "qml/Main.qml"
LINUX_MAIN_QML = CLIENT_DIR / "qml/linux/Main.qml"
CHARACTER_PANE_QML = CLIENT_DIR / "qml/pet/Live2DCharacterPane.qml"
MAIN_CPP = CLIENT_DIR / "app/main.cpp"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def pet_asset_settings_source() -> str:
    return "\n".join(
        read(path)
        for path in (
            PET_ASSET_SETTINGS_CPP,
            PET_ASSET_SETTINGS_PRIVATE_H,
            PET_ASSET_SETTINGS_AVATAR_CPP,
            PET_ASSET_SETTINGS_PERSISTENCE_CPP,
            PET_ASSET_SETTINGS_STATE_CPP,
        )
    )


class PetLive2DAvatarContractTests(unittest.TestCase):
    def test_pet_asset_settings_derives_cached_live2d_avatar_from_model_package(self):
        header = read(PET_ASSET_SETTINGS_H)
        source = pet_asset_settings_source()
        renderer_header = read(LIVE2D_AVATAR_OPENGL_RENDERER_H)
        renderer_source = read(LIVE2D_AVATAR_OPENGL_RENDERER_CPP)
        main_source = read(MAIN_CPP)

        self.assertIn("live2dAvatarUrl", header)
        self.assertIn("resolveLive2DAvatarUrl", header)
        self.assertIn("resolveLive2DAvatarUrl", source)
        self.assertIn("Live2DAvatarOpenGLRenderer", source)
        self.assertIn("renderedLive2DAvatar", source)
        self.assertIn("modelTexturePaths", source)
        self.assertIn("packageImageCandidates", source)
        self.assertIn("cropHeadAvatar", source)
        self.assertIn("pet/live2d-avatars", source)
        self.assertIn("memochat-live2d-avatar-opengl-v3", source)
        self.assertIn("avatar_gl_", source)
        self.assertIn("FileReferences", source)
        self.assertIn("Textures", source)
        self.assertIn("QUrl::fromLocalFile", source)
        self.assertIn("renderAvatar", renderer_header)
        self.assertIn("QImage Live2DAvatarOpenGLRenderer::renderAvatar", renderer_source)
        self.assertIn("cropAvatarFrame", renderer_source)
        self.assertIn("globalShareContext", renderer_source)
        self.assertIn("AA_ShareOpenGLContexts", main_source)

    def test_pet_chat_window_uses_real_self_avatar_and_live2d_avatar_with_fixed_sides(self):
        chat = read(PET_CHAT_WINDOW_QML)
        window = read(PET_WINDOW_QML)
        main = read(MAIN_QML)
        linux_main = read(LINUX_MAIN_QML)

        self.assertIn('property string selfAvatar: "qrc:/res/head_1.jpg"', chat)
        self.assertIn("property string live2dAvatarSource", chat)
        self.assertIn("function isOutgoingMessage", chat)
        self.assertIn("function refreshLive2DAvatar", chat)
        self.assertIn("resolveLive2DAvatarUrl", chat)
        self.assertIn("readonly property bool outgoingMessage: root.isOutgoingMessage(outgoing)", chat)
        self.assertIn("outgoing: messageDelegateRoot.outgoingMessage", chat)
        self.assertIn("root.messageSenderName(messageDelegateRoot.outgoingMessage)", chat)
        self.assertIn("showOutgoingSenderName: true", chat)
        self.assertIn("function avatarForMessage", chat)
        self.assertIn("return outgoing ? root.effectiveSelfAvatar() : root.effectiveLive2DAvatar()", chat)
        self.assertIn("source: root.live2dAvatarSource", chat)
        self.assertNotIn('avatarSource: model.outgoing ? "qrc:/res/head_1.jpg"', chat)

        self.assertIn('property string selfAvatar: "qrc:/res/head_1.jpg"', window)
        self.assertIn('"selfAvatar": root.selfAvatar', window)
        self.assertIn("petChatWindowRef.selfAvatar = root.selfAvatar", window)
        for source in (main, linux_main):
            self.assertIn('"selfAvatar": controller.currentUserIcon', source)
            self.assertIn("petWindowRef.selfAvatar = controller.currentUserIcon", source)
            self.assertIn("function onCurrentUserChanged()", source)

    def test_live2d_character_pane_uses_package_avatar_in_initialization_ui(self):
        pane = read(CHARACTER_PANE_QML)

        self.assertIn("property string characterAvatarSource", pane)
        self.assertIn("function refreshCharacterAvatar", pane)
        self.assertIn("resolveLive2DAvatarUrl(root.modelJson, root.modelRoot)", pane)
        self.assertIn("onModelRootChanged: root.refreshCharacterAvatar()", pane)
        self.assertIn("onModelJsonChanged: root.refreshCharacterAvatar()", pane)
        self.assertIn("imageSource: root.characterAvatarSource", pane)
        self.assertIn("source: avatarPreview.imageSource", pane)


if __name__ == "__main__":
    unittest.main()
