import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CLIENT = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
FEATURE_VIEW = CLIENT / "features/chat/view"
GROUP_VIEW = CLIENT / "features/group/view"
LEGACY_APP = CLIENT / "qml/app"
LEGACY_CHAT = CLIENT / "qml/chat"
CHAT_QRC = CLIENT / "features/chat/resources/chat.qrc"
GROUP_QRC = CLIENT / "features/group/resources/group.qrc"
CONTACT_QRC = CLIENT / "features/contact/resources/contact.qrc"
PROFILE_QRC = CLIENT / "features/profile/resources/profile.qrc"
SETTINGS_QRC = CLIENT / "features/settings/resources/settings.qrc"
SHELL_QRC = CLIENT / "resources/qrc/qml-shell.qrc"
GLOBAL_CHAT_QRC = CLIENT / "resources/qrc/qml-chat.qrc"

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


def read_existing(path: Path) -> str:
    return path.read_text(encoding="utf-8") if path.exists() else ""


class ChatQmlPhysicalLayoutContractTests(unittest.TestCase):
    def test_chat_qml_lives_under_feature_view(self):
        expected = (
            *(FEATURE_VIEW / file_name for file_name in CHAT_ROOT_FEATURE_FILES),
            *(FEATURE_VIEW / "conversation" / file_name for file_name in CONVERSATION_FEATURE_FILES),
            *(FEATURE_VIEW / "sidebar" / file_name for file_name in SIDEBAR_FEATURE_FILES),
            *(GROUP_VIEW / file_name for file_name in GROUP_FEATURE_FILES),
            *(CLIENT / "features/contact/view" / file_name for file_name in CONTACT_ROOT_FEATURE_FILES),
            *(CLIENT / "features/contact/view" / file_name for file_name in CONTACT_FEATURE_FILES),
            *(CLIENT / "features/profile/view" / file_name for file_name in PROFILE_FEATURE_FILES),
            *(CLIENT / "features/settings/view" / file_name for file_name in SETTINGS_ROOT_FEATURE_FILES),
            *(CLIENT / "features/settings/view" / file_name for file_name in SETTINGS_FEATURE_FILES),
        )
        for path in expected:
            with self.subTest(path=path.relative_to(CLIENT)):
                self.assertTrue(path.is_file())

    def test_legacy_physical_chat_files_are_removed(self):
        removed = (
            LEGACY_APP / "ChatShellContent.qml",
            *(LEGACY_CHAT / file_name for file_name in CHAT_ROOT_FEATURE_FILES if file_name != "ChatShellContent.qml"),
            *(LEGACY_CHAT / "conversation" / file_name for file_name in CONVERSATION_FEATURE_FILES),
            *(LEGACY_CHAT / "group" / file_name for file_name in GROUP_FEATURE_FILES),
            *(LEGACY_CHAT / file_name for file_name in SIDEBAR_FEATURE_FILES),
            *(LEGACY_CHAT / file_name for file_name in CONTACT_ROOT_FEATURE_FILES),
            *(LEGACY_CHAT / "contact" / file_name for file_name in CONTACT_FEATURE_FILES),
            *(LEGACY_CHAT / file_name for file_name in PROFILE_FEATURE_FILES),
            *(LEGACY_CHAT / file_name for file_name in SETTINGS_ROOT_FEATURE_FILES),
            *(LEGACY_CHAT / "settings" / file_name for file_name in SETTINGS_FEATURE_FILES),
        )
        for path in removed:
            with self.subTest(path=path.relative_to(CLIENT)):
                self.assertFalse(path.exists())

    def test_feature_aliases_point_at_feature_local_files(self):
        qrc = "\n".join(read_existing(path) for path in (CHAT_QRC, GROUP_QRC, CONTACT_QRC, PROFILE_QRC, SETTINGS_QRC))
        expected_paths = (
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
        for token in expected_paths:
            with self.subTest(token=token):
                self.assertIn(token, qrc)

    def test_legacy_business_aliases_are_removed_after_feature_import_migration(self):
        self.assertFalse(GLOBAL_CHAT_QRC.exists())
        qrc = SHELL_QRC.read_text(encoding="utf-8")
        removed_shell_aliases = (
            'alias="qml/app/ChatShellContent.qml">../../features/chat/view/ChatShellContent.qml</file>',
            'alias="qml/ChatShellContent.qml">../../features/chat/view/ChatShellContent.qml</file>',
        )
        for token in removed_shell_aliases:
            with self.subTest(removed_shell_alias=token):
                self.assertNotIn(token, qrc)

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
        for token in removed_aliases:
            with self.subTest(token=token):
                self.assertNotIn(token, qrc)


if __name__ == "__main__":
    unittest.main()
