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


def composition_port_binder_text() -> str:
    return "\n".join(read(path) for path in sorted((APP / "composition").glob("*PortBinder.cpp")))


def app_chat_binding_text() -> str:
    return (
        "\n".join(read(path) for path in sorted((APP / "controller").glob("AppChat*Binding.cpp")))
        + "\n"
        + read(APP / "controller/AppControllerChatFeatureBinding.cpp")
    )


def qml_sources() -> dict[str, str]:
    roots = (FEATURES, QML_ROOT)
    sources: dict[str, str] = {}
    for root in roots:
        for path in root.rglob("*"):
            if path.suffix in {".qml", ".js"}:
                sources[str(path.relative_to(CLIENT))] = read(path)
    return sources


def appcontroller_sections() -> tuple[str, str, str, str]:
    header = read(APP / "controller/AppController.h")
    public = header[header.index("public:") : header.index("signals:")]
    private_start = header.index("private:")
    private_slots_start = header.find("private slots:")
    if private_slots_start == -1:
        signals = header[header.index("signals:") : private_start]
        private_slots = ""
    else:
        signals = header[header.index("signals:") : private_slots_start]
        private_slots = header[private_slots_start:private_start]
    private = header[header.index("private:") :]
    return public, signals, private_slots, private


class AppControllerFacadePruningContractTests(unittest.TestCase):
    def test_appcontroller_has_no_qml_metadata_after_direct_facades(self):
        header = read(APP / "controller/AppController.h")
        public, _, _, _ = appcontroller_sections()

        self.assertNotIn("Q_PROPERTY(", header)
        self.assertNotIn("Q_INVOKABLE", header)

        kept_cpp_surface = (
            "explicit AppController(QObject* parent = nullptr);",
            "~AppController();",
            "MomentsModel* momentsModel() const;",
            "MomentsController* momentsController() const;",
            "AgentController* agentController() const;",
            "AgentMessageModel* agentMessageModel() const;",
            "PetController* petController() const;",
            "R18Controller* r18Controller() const;",
            "ShellViewModel* shellViewModel();",
            "AuthViewModel* authViewModel();",
            "ChatViewModel* chatViewModel();",
            "ContactController* contactController();",
            "GroupController* groupController();",
            "ProfileController* profileController();",
            "CallController* callController();",
        )
        for token in kept_cpp_surface:
            with self.subTest(token=token):
                self.assertIn(token, public)

        removed_alias_getters = (
            "ShellViewModel* shell();",
            "AuthViewModel* auth();",
            "ChatViewModel* chat();",
            "ContactController* contact();",
            "GroupController* group();",
            "ProfileController* profile();",
            "CallController* call();",
        )
        for token in removed_alias_getters:
            with self.subTest(token=token):
                self.assertNotIn(token, header)

    def test_appcontroller_public_surface_excludes_legacy_state_and_command_facades(self):
        public, _, _, _ = appcontroller_sections()

        legacy_public_tokens = (
            "Page page() const;",
            "QString tipText() const;",
            "bool busy() const;",
            "int registerCountdown() const;",
            "ChatTab chatTab() const;",
            "ContactPane contactPane() const;",
            "QString currentUserName() const;",
            "QString currentContactName() const;",
            "QString currentChatPeerName() const;",
            "bool searchPending() const;",
            "bool chatLoadingMore() const;",
            "QString currentDraftText() const;",
            "QVariantList currentPendingAttachments() const;",
            "bool contactsReady() const;",
            "bool groupsReady() const;",
            "bool chatShellBusy() const;",
            "void switchToLogin();",
            "void switchToRegister();",
            "void switchToReset();",
            "void switchChatTab(int tab);",
            "void ensureContactsInitialized();",
            "void ensureGroupsInitialized();",
            "void ensureApplyInitialized();",
            "void ensureChatListInitialized();",
            "void clearTip();",
            "void selectChatIndex(int index);",
            "void selectGroupIndex(int index);",
            "void selectDialogByUid(int uid);",
            "void selectContactIndex(int index);",
            "QVariantMap contactProfileByUid(int uid) const;",
            "void deleteFriend(int uid);",
            "void loadMoreChats();",
            "void loadMoreContacts();",
            "void sendTextMessage(const QString& text);",
            "void sendCurrentComposerPayload(const QString& text);",
            "void sendImageMessage();",
            "void sendFileMessage();",
            "void openExternalResource(const QString& url);",
            "void searchUser(const QString& uidText);",
            "void requestAddFriend(int uid, const QString& bakName, const QVariantList& labels = QVariantList());",
            "void approveFriend(int uid, const QString& backName, const QVariantList& labels = QVariantList());",
            "void refreshGroupList();",
            "void createGroup(const QString& name, const QVariantList& memberUserIdList = QVariantList());",
            "void sendGroupTextMessage(const QString& text);",
            "void updateCurrentDraft(const QString& text);",
            "void toggleCurrentDialogPinned();",
            "void toggleCurrentDialogMuted();",
            "void toggleDialogPinnedByUid(int dialogUid);",
            "void toggleDialogMutedByUid(int dialogUid);",
            "void markDialogReadByUid(int dialogUid);",
            "void clearDialogDraftByUid(int dialogUid);",
            "QString loginCredentialCacheJson() const;",
            "void login(const QString& email, const QString& password);",
            "void beginPostLoginBootstrap();",
            "void requestRegisterCode(const QString& email);",
            "void registerUser(const QString& user,",
            "void requestResetCode(const QString& email);",
            "void resetPassword(const QString& user, const QString& email, const QString& password,",
        )
        for token in legacy_public_tokens:
            with self.subTest(token=token):
                self.assertNotIn(token, public)

    def test_appcontroller_private_surface_excludes_dialog_command_wrappers(self):
        header = read(APP / "controller/AppController.h")
        controller_sources = "\n".join(read(path) for path in sorted((APP / "controller").glob("AppController*.cpp")))

        self.assertFalse((APP / "controller/AppControllerDialogCommands.cpp").exists())

        removed_wrappers = (
            "updateCurrentDraft",
            "toggleCurrentDialogPinned",
            "toggleCurrentDialogMuted",
            "toggleDialogPinnedByUid",
            "toggleDialogMutedByUid",
            "markDialogReadByUid",
            "clearDialogDraftByUid",
        )
        for name in removed_wrappers:
            with self.subTest(name=name):
                self.assertNotRegex(header, rf"\bvoid\s+{name}\s*\(")
                self.assertNotRegex(controller_sources, rf"\bAppController::{name}\s*\(")

    def test_migrated_feature_methods_are_private_cpp_signal_targets(self):
        header = read(APP / "controller/AppController.h")
        public, _, _, private = appcontroller_sections()

        plain_methods = (
            "void ensureContactsInitialized();",
            "void ensureGroupsInitialized();",
            "void ensureApplyInitialized();",
            "void selectGroupIndex(int index);",
            "void jumpChatWithCurrentContact();",
            "void refreshGroupList();",
        )
        for token in plain_methods:
            with self.subTest(token=token):
                self.assertIn(token, header)
                self.assertIn(token, private)
                self.assertNotIn(token, public)
                self.assertNotIn(f"Q_INVOKABLE {token}", header)

        removed_group_conversation_helpers = (
            "void editGroupMessage(const QString& msgId, const QString& text);",
            "void revokeGroupMessage(const QString& msgId);",
            "void forwardGroupMessage(const QString& msgId);",
            "void loadGroupHistory();",
        )
        for token in removed_group_conversation_helpers:
            with self.subTest(removed_group_conversation_helper=token):
                self.assertNotIn(token, header)

    def test_appcontroller_private_surface_excludes_media_pass_through_wrappers(self):
        header = read(APP / "controller/AppController.h")
        controller = (
            read(APP / "controller/AppController.cpp")
            + "\n"
            + composition_port_binder_text()
            + "\n"
            + app_chat_binding_text()
        )

        removed_wrappers = (
            "sendTextMessage",
            "sendCurrentComposerPayload",
            "sendImageMessage",
            "sendFileMessage",
            "removePendingAttachment",
            "clearPendingAttachments",
        )
        for name in removed_wrappers:
            with self.subTest(name=name):
                self.assertNotRegex(header, rf"\bvoid\s+{name}\s*\(")

        self.assertIn("_media_coordinator->sendTextMessage(text)", controller)
        self.assertIn("_media_coordinator->sendCurrentComposerPayload(text)", controller)
        self.assertIn("_media_coordinator->sendImageMessage()", controller)
        self.assertIn("_media_coordinator->sendFileMessage()", controller)
        self.assertIn("_media_coordinator->removePendingAttachment(attachmentId)", controller)
        self.assertIn("_media_coordinator->clearPendingAttachments()", controller)

    def test_composition_no_longer_exposes_raw_appcontroller(self):
        composition_header = read(APP / "composition/AppComposition.h")
        composition_source = read(APP / "composition/AppComposition.cpp")

        self.assertNotIn("AppController& controller();", composition_header)
        self.assertNotIn("const AppController& controller() const;", composition_header)
        self.assertNotIn("AppController& AppComposition::controller()", composition_source)
        self.assertNotIn("const AppController& AppComposition::controller() const", composition_source)

    def test_feature_ports_replace_direct_appcontroller_signal_targets(self):
        controller = normalized(read(APP / "controller/AppController.cpp"))
        group_feature_binder = normalized(read(APP / "composition/AppGroupFeaturePortBinder.cpp"))
        profile_feature_binder = normalized(read(APP / "composition/AppProfileFeaturePortBinder.cpp"))
        call_binder = normalized(read(APP / "composition/AppCallPortBinder.cpp"))
        port_binder = group_feature_binder + " " + profile_feature_binder + " " + call_binder

        self.assertIn("_features.profileController.setCommandPort(ProfileCommandPort{", profile_feature_binder)
        self.assertIn("_features.callController.setCommandPort(CallCommandPort{", call_binder)
        self.assertIn("_features.groupController.setCommandPort", group_feature_binder)
        self.assertIn("GroupCommandPort{", group_feature_binder)
        self.assertNotIn("_features.profileController.setCommandPort(ProfileCommandPort{", controller)
        self.assertNotIn("_features.callController.setCommandPort(CallCommandPort{", controller)
        self.assertNotIn("_features.groupController.setCommandPort", controller)
        self.assertNotIn("CallController::startVoiceChatRequested", controller + port_binder)
        self.assertNotIn("CallController::toggleCallCameraRequested", controller + port_binder)
        self.assertNotIn("MessageMutationCommandService::run", controller)
        self.assertNotIn("MessageMutationCommandRequest", controller)
        self.assertNotIn("MessageMutationCommandDependencies", controller)
        self.assertIn("_features.chatFeatureController.editCurrentMessage(msgId, text);", port_binder)
        self.assertIn("_features.chatFeatureController.revokeCurrentMessage(msgId);", port_binder)
        self.assertIn("_features.chatFeatureController.forwardCurrentMessage(msgId);", port_binder)
        self.assertIn("_features.chatFeatureController.requestCurrentGroupHistory();", port_binder)
        self.assertNotIn(
            "_features.chatFeatureController.requestGroupHistory(request, groupConversationDependencies(), port);",
            controller,
        )
        self.assertNotIn("editGroupMessage(msgId", controller)
        self.assertNotIn("revokeGroupMessage(msgId", controller)
        self.assertNotIn("forwardGroupMessage(msgId", controller)
        self.assertNotIn("loadGroupHistory();", controller)
        self.assertNotIn("GroupController::editGroupMessageRequested", controller)
        self.assertNotIn("GroupController::revokeGroupMessageRequested", controller)
        self.assertNotIn("GroupController::forwardGroupMessageRequested", controller)
        self.assertNotIn("GroupController::loadGroupHistoryRequested", controller)
        self.assertNotIn("ProfileController::saveProfileRequested", controller)
        self.assertNotIn("ProfileController::chooseAvatarRequested", controller)

    def test_engine_removes_legacy_livekit_context_after_call_facade(self):
        composition = read(APP / "composition/AppComposition.cpp")
        engine = read(APP / "bootstrap/MainQmlEngineSetup.cpp")

        self.assertIn('context.setContextProperty("call", call())', composition)
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
