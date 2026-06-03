import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CLIENT = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
GROUP = CLIENT / "features/group"
APP = CLIENT / "app"
CHAT_VIEW = CLIENT / "features/chat/view"
SHELL_PAGE = CLIENT / "qml/app/ChatShellPage.qml"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def normalized(source: str) -> str:
    return " ".join(source.split())


class GroupFacadeContractTests(unittest.TestCase):
    def test_group_controller_exposes_qml_state_and_actions(self):
        header = read(GROUP / "controller/GroupController.h")
        compact_header = normalized(header)

        expected_tokens = (
            "class GroupController : public QObject",
            "Q_OBJECT",
            "Q_PROPERTY(FriendListModel* groupListModel READ groupListModel NOTIFY modelChanged)",
            "Q_PROPERTY(bool hasCurrentGroup READ hasCurrentGroup NOTIFY currentGroupChanged)",
            "Q_PROPERTY(int currentGroupRole READ currentGroupRole NOTIFY currentGroupChanged)",
            "Q_PROPERTY(QString currentGroupName READ currentGroupName NOTIFY currentGroupChanged)",
            "Q_PROPERTY(QString currentGroupCode READ currentGroupCode NOTIFY currentGroupChanged)",
            "Q_PROPERTY(bool currentGroupCanChangeInfo READ currentGroupCanChangeInfo NOTIFY currentGroupChanged)",
            "Q_PROPERTY(bool currentGroupCanDeleteMessages READ currentGroupCanDeleteMessages NOTIFY currentGroupChanged)",
            "Q_PROPERTY(bool currentGroupCanInviteUsers READ currentGroupCanInviteUsers NOTIFY currentGroupChanged)",
            "Q_PROPERTY(bool currentGroupCanManageAdmins READ currentGroupCanManageAdmins NOTIFY currentGroupChanged)",
            "Q_PROPERTY(bool currentGroupCanPinMessages READ currentGroupCanPinMessages NOTIFY currentGroupChanged)",
            "Q_PROPERTY(bool currentGroupCanBanUsers READ currentGroupCanBanUsers NOTIFY currentGroupChanged)",
            "Q_PROPERTY(bool currentGroupCanManageTopics READ currentGroupCanManageTopics NOTIFY currentGroupChanged)",
            "Q_PROPERTY(QString groupStatusText READ groupStatusText NOTIFY groupStatusChanged)",
            "Q_PROPERTY(bool groupStatusError READ groupStatusError NOTIFY groupStatusChanged)",
            "Q_PROPERTY(bool groupsReady READ groupsReady NOTIFY groupsReadyChanged)",
            "Q_INVOKABLE void ensureGroupsInitialized();",
            "Q_INVOKABLE void selectGroupIndex(int index);",
            "Q_INVOKABLE void refreshGroupList();",
            "Q_INVOKABLE void createGroup(const QString& name, const QVariantList& memberUserIdList = QVariantList());",
            "Q_INVOKABLE void inviteGroupMember(const QString& userId, const QString& reason = QString());",
            "Q_INVOKABLE void applyJoinGroup(const QString& groupCode, const QString& reason = QString());",
            "Q_INVOKABLE void reviewGroupApply(qint64 applyId, bool agree);",
            "Q_INVOKABLE void sendGroupTextMessage(const QString& text);",
            "Q_INVOKABLE void sendGroupImageMessage();",
            "Q_INVOKABLE void sendGroupFileMessage();",
            "Q_INVOKABLE void editGroupMessage(const QString& msgId, const QString& text);",
            "Q_INVOKABLE void revokeGroupMessage(const QString& msgId);",
            "Q_INVOKABLE void forwardGroupMessage(const QString& msgId);",
            "Q_INVOKABLE void loadGroupHistory();",
            "Q_INVOKABLE void updateGroupAnnouncement(const QString& announcement);",
            "Q_INVOKABLE void updateGroupIcon(int source = 0);",
            "Q_INVOKABLE void setGroupAdmin(const QString& userId, bool isAdmin, qint64 permissionBits = 0);",
            "Q_INVOKABLE void muteGroupMember(const QString& userId, int muteSeconds);",
            "Q_INVOKABLE void kickGroupMember(const QString& userId);",
            "Q_INVOKABLE void quitCurrentGroup();",
            "Q_INVOKABLE void dissolveCurrentGroup();",
            "Q_INVOKABLE void clearGroupStatus();",
            "void syncModel(FriendListModel* groupListModel);",
            "void syncCurrentGroup(bool hasCurrentGroup,",
            "void syncGroupStatus(const QString& text, bool isError);",
            "void syncGroupsReady(bool ready);",
            "void ensureGroupsInitializedRequested();",
            "void refreshGroupListRequested();",
            "void createGroupRequested(const QString& name, const QVariantList& memberUserIdList);",
            "void applyJoinGroupRequested(const QString& groupCode, const QString& reason);",
            "void updateGroupAnnouncementRequested(const QString& announcement);",
            "void setGroupAdminRequested(const QString& userId, bool isAdmin, qint64 permissionBits);",
            "void dissolveCurrentGroupRequested();",
        )
        for token in expected_tokens:
            with self.subTest(token=token):
                self.assertIn(token, compact_header if "\n" not in token else header)

    def test_group_feature_is_registered_in_cmake(self):
        feature_sources = read(CLIENT / "features/sources.cmake")
        group_sources = read(GROUP / "sources.cmake")
        cmake = read(CLIENT / "CMakeLists.txt")

        self.assertIn("include(${CMAKE_CURRENT_LIST_DIR}/group/sources.cmake)", feature_sources)
        self.assertIn("features/group/controller/GroupController.cpp", group_sources)
        self.assertIn("features/group/controller/GroupController.h", group_sources)
        self.assertIn("${MEMOCHAT_QML_FEATURE_DIR}/group", cmake)
        self.assertIn("${MEMOCHAT_QML_FEATURE_DIR}/group/controller", cmake)

    def test_appcontroller_exposes_additive_group_surface(self):
        header = read(APP / "controller/AppController.h")
        controller = read(APP / "controller/AppController.cpp")
        model_props = read(APP / "controller/AppControllerModelProperties.cpp")
        status_state = read(APP / "controller/AppControllerStatusState.cpp")
        bootstrap_state = read(APP / "controller/AppControllerBootstrapState.cpp")
        dialog_state = read(APP / "controller/AppControllerDialogRuntimeState.cpp")
        models = read(APP / "controller/AppControllerModels.cpp")
        engine = read(APP / "bootstrap/MainQmlEngineSetup.cpp")

        self.assertNotIn("Q_PROPERTY(GroupController* group READ group CONSTANT)", header)
        self.assertIn("GroupController* group();", header)
        self.assertIn("GroupController* groupController();", header)
        self.assertIn("GroupController _group_controller;", header)
        self.assertIn("void syncGroupControllerState();", header)

        self.assertIn("GroupController* AppController::group()", model_props)
        self.assertIn("GroupController* AppController::groupController()", model_props)
        self.assertIn("return &_group_controller;", model_props)

        self.assertIn(
            "connect(&_group_controller, &GroupController::refreshGroupListRequested, this, &AppController::refreshGroupList);",
            controller,
        )
        self.assertIn(
            "connect(&_group_controller, &GroupController::createGroupRequested, this, &AppController::createGroup);",
            controller,
        )
        self.assertIn(
            "connect(&_group_controller, &GroupController::updateGroupAnnouncementRequested, this, "
            "&AppController::updateGroupAnnouncement);",
            normalized(controller),
        )
        self.assertIn("void AppController::syncGroupControllerState()", controller)
        self.assertIn("_group_controller.syncModel(&_group_list_model);", controller)
        self.assertIn("_group_controller.syncCurrentGroup(hasCurrentGroup(),", controller)
        self.assertIn(
            "_group_controller.syncGroupStatus(_feature_status_state.groupText, _feature_status_state.groupError);",
            controller,
        )
        self.assertIn("_group_controller.syncGroupsReady(_bootstrap_state.groupsReady);", controller)

        self.assertIn("syncGroupControllerState();", status_state)
        self.assertIn("syncGroupControllerState();", bootstrap_state)
        self.assertIn("syncGroupControllerState();", dialog_state)
        self.assertIn("syncGroupControllerState();", models)
        self.assertIn('setContextProperty("group", controller.groupController())', engine)

    def test_group_qml_uses_group_facade_for_left_panel_and_modal(self):
        normal_face = read(CHAT_VIEW / "ChatNormalFace.qml")
        shell_page = read(SHELL_PAGE)

        expected_tokens = (
            "groupsReady: group.groupsReady",
            "groupStatusText: group.groupStatusText",
            "groupStatusError: group.groupStatusError",
            "onApplyJoinGroupRequested: function(groupCode, reason) { group.applyJoinGroup(groupCode, reason) }",
            "onCreateGroupSubmitted: function(name, memberUserIds) { group.createGroup(name, memberUserIds) }",
            "friendModel: contact.contactListModel",
        )
        combined = normal_face + shell_page
        for token in expected_tokens:
            with self.subTest(token=token):
                self.assertIn(token, combined)

    def test_group_qml_uses_group_facade_for_management_panel(self):
        qml = read(CHAT_VIEW / "ChatShellContent.qml")

        expected_tokens = (
            "groupName: group.currentGroupName",
            "groupCode: group.currentGroupCode",
            "canUpdateIcon: group.currentGroupCanChangeInfo",
            "canUpdateAnnouncement: group.currentGroupCanChangeInfo",
            "canDeleteMessages: group.currentGroupCanDeleteMessages",
            "canInviteUsers: group.currentGroupCanInviteUsers",
            "canManageAdmins: group.currentGroupCanManageAdmins",
            "canPinMessages: group.currentGroupCanPinMessages",
            "canBanUsers: group.currentGroupCanBanUsers",
            "canManageTopics: group.currentGroupCanManageTopics",
            "statusText: group.groupStatusText",
            "statusError: group.groupStatusError",
            "onRefreshRequested: group.refreshGroupList()",
            "onLoadHistoryRequested: group.loadGroupHistory()",
            "onUpdateAnnouncementRequested: function(announcement) { group.updateGroupAnnouncement(announcement) }",
            "onUpdateGroupIconRequested: function(source) { group.updateGroupIcon(source) }",
            "group.quitCurrentGroup()",
            "group.dissolveCurrentGroup()",
            "onInviteRequested: function(userId, reason) { group.inviteGroupMember(userId, reason) }",
            "onSetAdminRequested: function(userId, isAdmin, permissionBits) { group.setGroupAdmin(userId, isAdmin, permissionBits) }",
            "onMuteRequested: function(userId, muteSeconds) { group.muteGroupMember(userId, muteSeconds) }",
            "onKickRequested: function(userId) { group.kickGroupMember(userId) }",
            "onReviewRequested: function(applyId, agree) { group.reviewGroupApply(applyId, agree) }",
            "onForwardMessage: function(msgId) { group.forwardGroupMessage(msgId) }",
            "onRevokeMessage: function(msgId) { group.revokeGroupMessage(msgId) }",
            "onEditMessage: function(msgId, text) { group.editGroupMessage(msgId, text) }",
        )
        for token in expected_tokens:
            with self.subTest(token=token):
                self.assertIn(token, qml)

    def test_migrated_group_qml_avoids_old_controller_group_surface(self):
        qml_sources = {
            "ChatNormalFace.qml": read(CHAT_VIEW / "ChatNormalFace.qml"),
            "ChatShellContent.qml": read(CHAT_VIEW / "ChatShellContent.qml"),
            "ChatShellPage.qml": read(SHELL_PAGE),
        }

        forbidden_tokens = (
            "controller.groupsReady",
            "controller.groupStatusText",
            "controller.groupStatusError",
            "controller.currentGroupName",
            "controller.currentGroupCode",
            "controller.currentGroupCanChangeInfo",
            "controller.currentGroupCanDeleteMessages",
            "controller.currentGroupCanInviteUsers",
            "controller.currentGroupCanManageAdmins",
            "controller.currentGroupCanPinMessages",
            "controller.currentGroupCanBanUsers",
            "controller.currentGroupCanManageTopics",
            "controller.refreshGroupList()",
            "controller.createGroup(",
            "controller.inviteGroupMember(",
            "controller.applyJoinGroup(",
            "controller.reviewGroupApply(",
            "controller.loadGroupHistory()",
            "controller.updateGroupAnnouncement(",
            "controller.updateGroupIcon(",
            "controller.setGroupAdmin(",
            "controller.muteGroupMember(",
            "controller.kickGroupMember(",
            "controller.quitCurrentGroup()",
            "controller.dissolveCurrentGroup()",
            "controller.clearGroupStatus()",
            "controller.editGroupMessage(",
            "controller.revokeGroupMessage(",
            "controller.forwardGroupMessage(",
        )
        for file_name, source in qml_sources.items():
            compact_source = normalized(source)
            for token in forbidden_tokens:
                with self.subTest(file=file_name, token=token):
                    self.assertNotIn(token, compact_source)

    def test_appcontroller_prunes_legacy_group_qml_surface_but_keeps_cpp_targets(self):
        header = read(APP / "controller/AppController.h")

        legacy_qml_tokens = (
            "Q_INVOKABLE void ensureGroupsInitialized();",
            "Q_INVOKABLE void selectGroupIndex(int index);",
            "Q_INVOKABLE void refreshGroupList();",
            "Q_INVOKABLE void createGroup(const QString& name, const QVariantList& memberUserIdList = QVariantList());",
            "Q_INVOKABLE void inviteGroupMember(const QString& userId, const QString& reason = QString());",
            "Q_INVOKABLE void applyJoinGroup(const QString& groupCode, const QString& reason = QString());",
            "Q_INVOKABLE void reviewGroupApply(qint64 applyId, bool agree);",
            "Q_INVOKABLE void sendGroupTextMessage(const QString& text);",
            "Q_INVOKABLE void sendGroupImageMessage();",
            "Q_INVOKABLE void sendGroupFileMessage();",
            "Q_INVOKABLE void editGroupMessage(const QString& msgId, const QString& text);",
            "Q_INVOKABLE void revokeGroupMessage(const QString& msgId);",
            "Q_INVOKABLE void forwardGroupMessage(const QString& msgId);",
            "Q_INVOKABLE void loadGroupHistory();",
            "Q_INVOKABLE void updateGroupAnnouncement(const QString& announcement);",
            "Q_INVOKABLE void updateGroupIcon(int source = 0);",
            "Q_INVOKABLE void setGroupAdmin(const QString& userId, bool isAdmin, qint64 permissionBits = 0);",
            "Q_INVOKABLE void muteGroupMember(const QString& userId, int muteSeconds);",
            "Q_INVOKABLE void kickGroupMember(const QString& userId);",
            "Q_INVOKABLE void quitCurrentGroup();",
            "Q_INVOKABLE void dissolveCurrentGroup();",
            "Q_INVOKABLE void clearGroupStatus();",
            "Q_PROPERTY(FriendListModel* groupListModel READ groupListModel CONSTANT)",
            "Q_PROPERTY(QString groupStatusText READ groupStatusText NOTIFY groupStatusChanged)",
            "Q_PROPERTY(bool groupStatusError READ groupStatusError NOTIFY groupStatusChanged)",
        )
        for token in legacy_qml_tokens:
            with self.subTest(token=token):
                self.assertNotIn(token, header)

        cpp_targets = (
            "void ensureGroupsInitialized();",
            "void selectGroupIndex(int index);",
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
            "FriendListModel* groupListModel();",
            "QString groupStatusText() const;",
            "bool groupStatusError() const;",
        )
        for token in cpp_targets:
            with self.subTest(token=token):
                self.assertIn(token, header)


if __name__ == "__main__":
    unittest.main()
