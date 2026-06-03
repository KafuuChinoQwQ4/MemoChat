import re
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CLIENT = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
APP = CLIENT / "app"
FEATURES = CLIENT / "features"
QML_ROOT = CLIENT / "qml"
CHAT_VIEW = FEATURES / "chat/view"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def normalized(source: str) -> str:
    return " ".join(source.split())


def qml_sources() -> dict[str, str]:
    roots = (FEATURES, QML_ROOT)
    sources: dict[str, str] = {}
    for root in roots:
        for path in root.rglob("*"):
            if path.suffix in {".qml", ".js"}:
                sources[str(path.relative_to(CLIENT))] = read(path)
    return sources


class AppControllerFacadePruningContractTests(unittest.TestCase):
    def test_appcontroller_has_no_qml_metadata_after_direct_facades(self):
        header = read(APP / "controller/AppController.h")

        self.assertNotIn("Q_PROPERTY(", header)
        self.assertNotIn("Q_INVOKABLE", header)

        kept_cpp_surface = (
            "ContactController* contact();",
            "ContactController* contactController();",
            "GroupController* group();",
            "GroupController* groupController();",
            "ProfileController* profile();",
            "ProfileController* profileController();",
            "CallController* call();",
            "CallController* callController();",
        )
        for token in kept_cpp_surface:
            with self.subTest(token=token):
                self.assertIn(token, header)

    def test_migrated_feature_methods_remain_as_plain_cpp_signal_targets(self):
        header = read(APP / "controller/AppController.h")

        plain_methods = (
            "void ensureContactsInitialized();",
            "void ensureGroupsInitialized();",
            "void ensureApplyInitialized();",
            "void selectGroupIndex(int index);",
            "void selectContactIndex(int index);",
            "void deleteFriend(int uid);",
            "void showApplyRequests();",
            "void jumpChatWithCurrentContact();",
            "void loadMoreContacts();",
            "void searchUser(const QString& uidText);",
            "void clearSearchState();",
            "void requestAddFriend(int uid, const QString& bakName, const QVariantList& labels = QVariantList());",
            "void approveFriend(int uid, const QString& backName, const QVariantList& labels = QVariantList());",
            "void clearAuthStatus();",
            "void startVoiceChat();",
            "void startVideoChat();",
            "void acceptIncomingCall();",
            "void rejectIncomingCall();",
            "void endCurrentCall();",
            "void toggleCallMuted();",
            "void toggleCallCamera();",
            "void chooseAvatar(int source = 0);",
            "void saveProfile(const QString& nick, const QString& desc);",
            "void clearSettingsStatus();",
            "void refreshGroupList();",
            "void createGroup(const QString& name, const QVariantList& memberUserIdList = QVariantList());",
            "void inviteGroupMember(const QString& userId, const QString& reason = QString());",
            "void applyJoinGroup(const QString& groupCode, const QString& reason = QString());",
            "void reviewGroupApply(qint64 applyId, bool agree);",
            "void sendGroupTextMessage(const QString& text);",
            "void sendGroupImageMessage();",
            "void sendGroupFileMessage();",
            "void editGroupMessage(const QString& msgId, const QString& text);",
            "void revokeGroupMessage(const QString& msgId);",
            "void forwardGroupMessage(const QString& msgId);",
            "void loadGroupHistory();",
            "void updateGroupAnnouncement(const QString& announcement);",
            "void updateGroupIcon(int source = 0);",
            "void setGroupAdmin(const QString& userId, bool isAdmin, qint64 permissionBits = 0);",
            "void muteGroupMember(const QString& userId, int muteSeconds);",
            "void kickGroupMember(const QString& userId);",
            "void quitCurrentGroup();",
            "void dissolveCurrentGroup();",
            "void clearGroupStatus();",
        )
        for token in plain_methods:
            with self.subTest(token=token):
                self.assertIn(token, header)
                self.assertNotIn(f"Q_INVOKABLE {token}", header)

    def test_facade_signal_connections_still_target_appcontroller_methods(self):
        controller = normalized(read(APP / "controller/AppController.cpp"))

        expected_connections = (
            "connect(&_contact_controller, &ContactController::searchUserRequested, this, &AppController::searchUser);",
            "connect(&_contact_controller, &ContactController::approveFriendRequested, this, &AppController::approveFriend);",
            "connect(&_group_controller, &GroupController::editGroupMessageRequested, this, &AppController::editGroupMessage);",
            "connect(&_group_controller, &GroupController::revokeGroupMessageRequested, this, &AppController::revokeGroupMessage);",
            "connect(&_group_controller, &GroupController::forwardGroupMessageRequested, this, &AppController::forwardGroupMessage);",
            "connect(&_profile_controller, &ProfileController::saveProfileRequested, this, &AppController::saveProfile);",
            "connect(&_call_controller, &CallController::startVoiceChatRequested, this, &AppController::startVoiceChat);",
            "connect(&_call_controller, &CallController::toggleCallCameraRequested, this, &AppController::toggleCallCamera);",
        )
        for token in expected_connections:
            with self.subTest(token=token):
                self.assertIn(token, controller)

    def test_engine_removes_legacy_livekit_context_after_call_facade(self):
        engine = read(APP / "bootstrap/MainQmlEngineSetup.cpp")

        self.assertIn('setContextProperty("call", controller.callController())', engine)
        self.assertNotIn('setContextProperty("livekitBridge"', engine)

    def test_qml_uses_feature_facades_instead_of_old_appcontroller_feature_surface(self):
        forbidden_pattern = re.compile(
            r"controller\."
            r"(settingsStatusText|settingsStatusError|chooseAvatar|saveProfile|clearSettingsStatus|"
            r"contactPane|currentContact|contactListModel|searchResultModel|applyRequestModel|"
            r"searchPending|searchStatusText|searchStatusError|hasPendingApply|contactLoadingMore|"
            r"canLoadMoreContacts|authStatusText|authStatusError|ensureContactsInitialized|"
            r"ensureApplyInitialized|selectContactIndex|searchUser|clearSearchState|requestAddFriend|"
            r"approveFriend|deleteFriend|showApplyRequests|jumpChatWithCurrentContact|loadMoreContacts|"
            r"clearAuthStatus|groupListModel|hasCurrentGroup|currentGroup|groupStatusText|"
            r"groupStatusError|groupsReady|ensureGroupsInitialized|selectGroupIndex|refreshGroupList|"
            r"createGroup|inviteGroupMember|applyJoinGroup|reviewGroupApply|sendGroupTextMessage|"
            r"sendGroupImageMessage|sendGroupFileMessage|editGroupMessage|revokeGroupMessage|"
            r"forwardGroupMessage|loadGroupHistory|updateGroupAnnouncement|updateGroupIcon|"
            r"setGroupAdmin|muteGroupMember|kickGroupMember|quitCurrentGroup|dissolveCurrentGroup|"
            r"clearGroupStatus|callSession|livekitBridge|startVoiceChat|startVideoChat|"
            r"acceptIncomingCall|rejectIncomingCall|endCurrentCall|toggleCallMuted|toggleCallCamera)"
        )

        failures = []
        for name, source in qml_sources().items():
            for match in forbidden_pattern.finditer(source):
                failures.append(f"{name}: {match.group(0)}")

        self.assertEqual([], failures)

        shell_content = read(CHAT_VIEW / "ChatShellContent.qml")
        self.assertIn("onForwardMessage: function(msgId) { group.forwardGroupMessage(msgId) }", shell_content)
        self.assertIn("onRevokeMessage: function(msgId) { group.revokeGroupMessage(msgId) }", shell_content)
        self.assertIn("onEditMessage: function(msgId, text) { group.editGroupMessage(msgId, text) }", shell_content)


if __name__ == "__main__":
    unittest.main()
