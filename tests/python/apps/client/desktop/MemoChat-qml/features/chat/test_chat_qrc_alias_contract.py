import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CLIENT = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
CHAT_QRC = CLIENT / "features/chat/resources/chat.qrc"
GROUP_QRC = CLIENT / "features/group/resources/group.qrc"
CONTACT_QRC = CLIENT / "features/contact/resources/contact.qrc"
PROFILE_QRC = CLIENT / "features/profile/resources/profile.qrc"
SETTINGS_QRC = CLIENT / "features/settings/resources/settings.qrc"
SHELL_QRC = CLIENT / "resources/qrc/qml-shell.qrc"
GLOBAL_CHAT_QRC = CLIENT / "resources/qrc/qml-chat.qrc"
RESOURCES = CLIENT / "resources/resources.cmake"

CONVERSATION_FEATURE_FILES = (
    "ChatComposerBar.qml",
    "ChatMessageListView.qml",
    "ChatMessageDelegate.qml",
    "ChatMessageActionMenu.qml",
    "ChatMessageStatusBadge.qml",
    "ChatMessageTextBody.qml",
    "ChatMessageImageBody.qml",
    "ChatMessageFileBody.qml",
    "ChatComposerReplyBar.qml",
    "ChatComposerAttachmentStrip.qml",
    "ChatConversationHeader.qml",
    "ChatSmartActionPopups.qml",
    "ChatSmartSummaryPopup.qml",
    "ChatSmartTranslatePopup.qml",
    "ComposerIconButton.qml",
    "MessageAvatar.qml",
)

GROUP_FEATURE_FILES = (
    "CreateGroupDialog.qml",
    "GroupInfoPane.qml",
    "GroupManagePane.qml",
    "GroupManagementPanel.qml",
    "GroupApplyReviewPane.qml",
)

SIDEBAR_FEATURE_FILES = (
    "ChatSideBar.qml",
    "ChatLeftHeader.qml",
    "ChatDialogRow.qml",
    "ChatFindFriendPopup.qml",
    "ChatJoinGroupPopup.qml",
)

CHAT_ROOT_FEATURE_FILES = (
    "ChatAgentSidePane.qml",
    "ChatLive2DEntryPane.qml",
    "ChatModalLayer.qml",
    "ChatNormalFace.qml",
    "ChatLeftPanel.qml",
    "ChatConversationPane.qml",
    "ChatShellContent.qml",
)

CONTACT_ROOT_FEATURE_FILES = ("ContactPane.qml",)

CONTACT_FEATURE_FILES = (
    "AddFriendDialog.qml",
    "ApplyRequestDelegate.qml",
    "ApplyRequestList.qml",
    "AuthFriendDialog.qml",
    "FriendInfoPane.qml",
    "ContactActionIconButton.qml",
    "ContactListPane.qml",
    "TagEditor.qml",
)

PROFILE_FEATURE_FILES = ("ProfileCenterPane.qml",)

SETTINGS_ROOT_FEATURE_FILES = (
    "MorePane.qml",
    "SettingsPane.qml",
)

SETTINGS_FEATURE_FILES = (
    "SettingsAvatarCard.qml",
    "SettingsProfileForm.qml",
)


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8") if path.exists() else ""


class ChatQrcAliasContractTests(unittest.TestCase):
    def qrc_text(self) -> str:
        qrc_paths = (SHELL_QRC, CHAT_QRC, GROUP_QRC, CONTACT_QRC, PROFILE_QRC, SETTINGS_QRC)
        return "\n".join(read(path) for path in qrc_paths if path.exists())

    def test_chat_qrc_keeps_feature_aliases(self):
        qrc = read(CHAT_QRC)
        expected_feature_aliases = (
            *(
                (f'alias="features/chat/view/{file_name}">../view/{file_name}</file>')
                for file_name in CHAT_ROOT_FEATURE_FILES
            ),
            *(
                (f'alias="features/chat/view/conversation/{file_name}">../view/conversation/{file_name}</file>')
                for file_name in CONVERSATION_FEATURE_FILES
            ),
            *(
                (f'alias="features/group/view/{file_name}">../view/{file_name}</file>')
                for file_name in GROUP_FEATURE_FILES
            ),
            *(
                (f'alias="features/chat/view/sidebar/{file_name}">../view/sidebar/{file_name}</file>')
                for file_name in SIDEBAR_FEATURE_FILES
            ),
            *(
                (f'alias="features/contact/view/{file_name}">../view/{file_name}</file>')
                for file_name in CONTACT_ROOT_FEATURE_FILES
            ),
            *(
                (f'alias="features/contact/view/{file_name}">../view/{file_name}</file>')
                for file_name in CONTACT_FEATURE_FILES
            ),
            *(
                (f'alias="features/profile/view/{file_name}">../view/{file_name}</file>')
                for file_name in PROFILE_FEATURE_FILES
            ),
            *(
                (f'alias="features/settings/view/{file_name}">../view/{file_name}</file>')
                for file_name in SETTINGS_ROOT_FEATURE_FILES
            ),
            *(
                (f'alias="features/settings/view/{file_name}">../view/{file_name}</file>')
                for file_name in SETTINGS_FEATURE_FILES
            ),
        )
        for alias in expected_feature_aliases:
            with self.subTest(alias=alias):
                self.assertIn(alias, self.qrc_text())

    def test_legacy_business_chat_aliases_are_removed(self):
        self.assertFalse(GLOBAL_CHAT_QRC.exists())
        qrc = self.qrc_text()
        removed_shell_aliases = (
            'alias="qml/app/ChatShellContent.qml">../../features/chat/view/ChatShellContent.qml</file>',
            'alias="qml/ChatShellContent.qml">../../features/chat/view/ChatShellContent.qml</file>',
        )
        for alias in removed_shell_aliases:
            with self.subTest(removed_shell_alias=alias):
                self.assertNotIn(alias, qrc)

        removed_aliases = (
            *(
                (f'alias="qml/chat/{file_name}"')
                for file_name in CHAT_ROOT_FEATURE_FILES
                if file_name != "ChatShellContent.qml"
            ),
            *((f'alias="qml/chat/conversation/{file_name}"') for file_name in CONVERSATION_FEATURE_FILES),
            *((f'alias="qml/chat/group/{file_name}"') for file_name in GROUP_FEATURE_FILES),
            *((f'alias="qml/chat/{file_name}"') for file_name in SIDEBAR_FEATURE_FILES),
            *((f'alias="qml/chat/{file_name}"') for file_name in CONTACT_ROOT_FEATURE_FILES),
            *((f'alias="qml/chat/contact/{file_name}"') for file_name in CONTACT_FEATURE_FILES),
            *((f'alias="qml/chat/{file_name}"') for file_name in PROFILE_FEATURE_FILES),
            *((f'alias="qml/chat/{file_name}"') for file_name in SETTINGS_ROOT_FEATURE_FILES),
            *((f'alias="qml/chat/settings/{file_name}"') for file_name in SETTINGS_FEATURE_FILES),
        )
        for alias in removed_aliases:
            with self.subTest(alias=alias):
                self.assertNotIn(alias, qrc)

    def test_chat_qrc_is_in_feature_resource_manifest(self):
        resources = read(RESOURCES)
        chat_manifest = read(CLIENT / "features/chat/sources.cmake")
        group_manifest = read(CLIENT / "features/group/sources.cmake")
        contact_manifest = read(CLIENT / "features/contact/sources.cmake")
        profile_manifest = read(CLIENT / "features/profile/sources.cmake")
        settings_manifest = read(CLIENT / "features/settings/sources.cmake")
        feature_manifest = read(CLIENT / "features/sources.cmake")
        self.assertIn("features/chat/resources/chat.qrc", chat_manifest)
        self.assertIn("features/group/resources/group.qrc", group_manifest)
        self.assertIn("include(${CMAKE_CURRENT_LIST_DIR}/group/sources.cmake)", feature_manifest)
        self.assertIn("features/contact/resources/contact.qrc", contact_manifest)
        self.assertIn("features/profile/resources/profile.qrc", profile_manifest)
        self.assertIn("features/settings/resources/settings.qrc", settings_manifest)
        self.assertIn("include(${CMAKE_CURRENT_LIST_DIR}/settings/sources.cmake)", feature_manifest)
        self.assertIn("${MEMOCHAT_QML_FEATURE_RESOURCES}", resources)
        self.assertNotIn("resources/qrc/qml-chat.qrc", resources)


if __name__ == "__main__":
    unittest.main()
