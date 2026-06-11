import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CLIENT = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
GROUP = CLIENT / "features/group"
APP = CLIENT / "app"
APP_FEATURE_REGISTRY_H = APP / "composition/AppFeatureRegistry.h"
APP_GROUP_FEATURE_PORT_BINDER = APP / "composition/AppGroupFeaturePortBinder.cpp"
CHAT_VIEW = CLIENT / "features/chat/view"
SHELL_PAGE = CLIENT / "qml/app/ChatShellPage.qml"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def normalized(source: str) -> str:
    return " ".join(source.split())


def extract_function(source: str, signature: str) -> str:
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
            "Q_INVOKABLE void requestDialogList();",
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
            "void setGroups(const std::vector<std::shared_ptr<FriendInfo>>& groups);",
            "void setGroupsFromSnapshots(const std::vector<std::shared_ptr<GroupInfoData>>& groups, QMap<int, qint64>& groupDialogUidMap);",
            "void appendGroups(const std::vector<std::shared_ptr<FriendInfo>>& groups);",
            "void upsertGroup(const std::shared_ptr<FriendInfo>& group);",
            "void removeGroupById(int groupDialogUid);",
            "void setCurrentGroup(const GroupInfoData& group);",
            "void clearCurrentGroup();",
            "void syncGroupStatus(const QString& text, bool isError);",
            "void syncGroupsReady(bool ready);",
            "void setCommandPort(GroupCommandPort port);",
            "struct GroupCommandPort",
            "std::function<void(int)> selectGroupIndex;",
            "std::function<void(const QString&, const QString&)> editMessage;",
            "std::function<void(const QString&)> revokeMessage;",
            "std::function<void(const QString&)> forwardMessage;",
            "std::function<void()> loadHistory;",
        )
        for token in expected_tokens:
            with self.subTest(token=token):
                self.assertIn(token, compact_header if "\n" not in token else header)

        forbidden_requested_signals = (
            "ensureGroupsInitializedRequested",
            "selectGroupIndexRequested",
            "refreshGroupListRequested",
            "requestDialogListRequested",
            "editGroupMessageRequested",
            "revokeGroupMessageRequested",
            "forwardGroupMessageRequested",
            "loadGroupHistoryRequested",
            "updateGroupIconRequested",
            "clearGroupStatusRequested",
        )
        for token in forbidden_requested_signals:
            with self.subTest(forbidden_requested_signal=token):
                self.assertNotIn(token, header)
                self.assertNotIn(token, read(GROUP / "controller/GroupController.cpp"))

    def test_group_feature_is_registered_in_cmake(self):
        feature_sources = read(CLIENT / "features/sources.cmake")
        group_sources = read(GROUP / "sources.cmake")
        cmake = read(CLIENT / "CMakeLists.txt")

        self.assertIn("include(${CMAKE_CURRENT_LIST_DIR}/group/sources.cmake)", feature_sources)
        self.assertIn("features/group/controller/GroupController.cpp", group_sources)
        self.assertIn("features/group/controller/GroupController.h", group_sources)
        self.assertIn("features/group/controller/GroupManagementEventService.cpp", group_sources)
        self.assertIn("features/group/controller/GroupManagementEventService.h", group_sources)
        self.assertIn("features/group/controller/GroupManagementResponseService.cpp", group_sources)
        self.assertIn("features/group/controller/GroupManagementResponseService.h", group_sources)
        self.assertIn("features/group/controller/GroupResponseErrorService.cpp", group_sources)
        self.assertIn("features/group/controller/GroupResponseErrorService.h", group_sources)
        self.assertIn("${MEMOCHAT_QML_FEATURE_DIR}/group", cmake)
        self.assertIn("${MEMOCHAT_QML_FEATURE_DIR}/group/controller", cmake)

    def test_appcontroller_exposes_additive_group_composition_surface(self):
        header = read(APP / "controller/AppController.h")
        registry = read(APP_FEATURE_REGISTRY_H)
        controller = read(APP / "controller/AppController.cpp")
        port_binder = read(APP_GROUP_FEATURE_PORT_BINDER)
        model_props = read(APP / "controller/AppControllerModelProperties.cpp")
        status_state = read(APP / "controller/AppControllerStatusState.cpp")
        bootstrap_state = read(APP / "controller/AppControllerBootstrapState.cpp")
        dialog_state = read(APP / "controller/AppControllerDialogRuntimeState.cpp")
        models = read(APP / "controller/AppControllerModels.cpp")
        group_header = read(GROUP / "controller/GroupController.h")
        composition = read(APP / "composition/AppComposition.cpp")
        public = header[header.index("public:") : header.index("signals:")]
        private = header[header.index("private:") :]

        self.assertNotIn("Q_PROPERTY(GroupController* group READ group CONSTANT)", header)
        self.assertNotIn("GroupController* group();", header)
        self.assertIn("GroupController* groupController();", public)
        self.assertIn("AppFeatureRegistry _features", header)
        self.assertIn("GroupController groupController;", registry)
        self.assertNotIn("AppGroupDialogState _group_dialog_state;", header)
        self.assertNotIn("AppControllerGroupState.h", header)
        self.assertIn("void syncGroupControllerState();", private)

        self.assertNotIn("GroupController* AppController::group()", model_props)
        self.assertIn("GroupController* AppController::groupController()", model_props)
        self.assertIn("return &_features.groupController;", model_props)
        self.assertNotIn("FriendListModel* AppController::groupListModel()", model_props)

        self.assertIn("_features.groupController.setCommandPort", port_binder)
        self.assertIn("GroupCommandPort{", port_binder)
        self.assertNotIn("_features.groupController.setCommandPort", controller)
        self.assertIn("std::function<std::vector<std::shared_ptr<FriendInfo>>()> friendListSnapshot;", group_header)
        self.assertIn("bool isFriendUserId(const QString& userId) const;", group_header)
        self.assertIn("if (!isFriendUserId(trimmedUserId))", read(GROUP / "controller/GroupController.cpp"))
        self.assertNotIn("std::function<bool(const QString&)> isFriendUserId;", group_header)
        group_port = port_binder.split("_features.groupController.setCommandPort(GroupCommandPort{", 1)[1].split(
            "});", 1
        )[0]
        self.assertIn("GetFriendListSnapshot();", group_port)
        self.assertNotIn("CheckFriendById", group_port)
        combined_app_binding = controller + port_binder
        self.assertNotIn("GroupController::createGroupRequested", combined_app_binding)
        self.assertNotIn("GroupController::reviewGroupApplyRequested", combined_app_binding)
        self.assertNotIn("GroupController::sendGroupTextMessageRequested", combined_app_binding)
        self.assertNotIn("GroupController::updateGroupAnnouncementRequested", combined_app_binding)
        self.assertNotIn("GroupController::setGroupAdminRequested", combined_app_binding)
        self.assertNotIn("GroupController::quitCurrentGroupRequested", combined_app_binding)
        self.assertNotIn("GroupController::ensureGroupsInitializedRequested", combined_app_binding)
        self.assertNotIn("GroupController::selectGroupIndexRequested", combined_app_binding)
        self.assertNotIn("GroupController::editGroupMessageRequested", combined_app_binding)
        self.assertNotIn("GroupController::loadGroupHistoryRequested", combined_app_binding)
        self.assertNotIn("class GroupCoordinator", read(APP / "coordinators/AppCoordinators.h"))
        self.assertNotIn("friend class GroupCoordinator", header)
        self.assertIn("void AppController::syncGroupControllerState()", controller)
        self.assertNotIn("_features.groupController.syncModel(&_group_list_model);", controller)
        self.assertNotIn("_features.groupController.syncCurrentGroup(hasCurrentGroup(),", controller)
        self.assertIn(
            "_features.groupController.syncGroupsReady(_shell_state.bootstrapState().groupsReady);", controller
        )
        self.assertIn("_features.groupController.setGroupsFromSnapshots(", models)
        self.assertNotIn("_features.groupController.setGroups(converted);", models)
        self.assertNotIn("std::make_shared<FriendInfo>", models)
        self.assertNotIn("ConversationSyncService::resolveGroupDialogUid", models)
        group_controller = read(GROUP / "controller/GroupController.cpp")
        set_from_snapshots = extract_function(
            group_controller,
            "void GroupController::setGroupsFromSnapshots",
        )
        self.assertIn("GroupController::setGroupsFromSnapshots", set_from_snapshots)
        self.assertIn("groupDialogUidMap", set_from_snapshots)
        self.assertIn("if (!group || group->_group_id <= 0)", set_from_snapshots)
        self.assertIn(
            "ConversationSyncService::resolveGroupDialogUid(groupDialogUidMap, group->_group_id)",
            normalized(set_from_snapshots),
        )
        self.assertIn('QStringLiteral("qrc:/res/chat_icon.png")', set_from_snapshots)
        self.assertIn("std::make_shared<FriendInfo>", set_from_snapshots)
        self.assertIn("group->_announcement", set_from_snapshots)
        self.assertIn("group->_last_msg", set_from_snapshots)
        self.assertIn("setGroups(converted);", set_from_snapshots)
        self.assertNotIn("_group_list_model", header)
        self.assertNotIn("_group_list_model.setFriends", models)

        self.assertIn("_features.groupController.syncGroupStatus(text, isError);", status_state)
        self.assertIn("syncGroupControllerState();", bootstrap_state)
        self.assertIn(
            "_features.groupController.setCurrentGroup(groupId, role, normalizedName, normalizedCode, permissionBits);",
            dialog_state,
        )
        self.assertIn('context.setContextProperty("group", group())', composition)

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
            "Q_INVOKABLE void requestDialogList();",
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
            "FriendListModel* groupListModel();",
            "QString groupStatusText() const;",
            "bool groupStatusError() const;",
        )
        for token in legacy_qml_tokens:
            with self.subTest(token=token):
                self.assertNotIn(token, header)

        cpp_targets = (
            "void ensureGroupsInitialized();",
            "void selectGroupIndex(int index);",
            "void refreshGroupList();",
        )
        for token in cpp_targets:
            with self.subTest(token=token):
                self.assertIn(token, header)
                self.assertIn(token, header[header.index("private:") :])
                self.assertNotIn(token, header[header.index("public:") : header.index("signals:")])

        removed_cpp_targets = (
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
        for token in removed_cpp_targets:
            with self.subTest(removed_token=token):
                self.assertNotIn(token, header)

    def test_group_management_events_and_responses_are_group_owned(self):
        events = read(APP / "controller/AppControllerGroupEvents.cpp")
        chat_dispatcher_router = read(APP / "events/AppChatDispatcherEventRouter.cpp")
        management = read(APP / "controller/AppControllerGroupManagementResponses.cpp")
        errors = read(APP / "controller/AppControllerGroupResponseErrors.cpp")
        event_service = read(GROUP / "controller/GroupManagementEventService.cpp")
        event_header = read(GROUP / "controller/GroupManagementEventService.h")
        response_service = read(GROUP / "controller/GroupManagementResponseService.cpp")
        response_header = read(GROUP / "controller/GroupManagementResponseService.h")
        effect_ports_header = read(GROUP / "controller/GroupManagementEffectPorts.h")
        app_header = read(APP / "controller/AppController.h")
        app_cmake = read(APP / "sources.cmake")
        factory_header_path = APP / "controller/GroupManagementEffectPortFactory.h"
        factory_cpp_path = APP / "controller/GroupManagementEffectPortFactory.cpp"

        self.assertTrue(factory_header_path.exists())
        self.assertTrue(factory_cpp_path.exists())
        factory_header = read(factory_header_path)
        factory_cpp = read(factory_cpp_path)
        factory_sources = factory_header + factory_cpp

        for reducer in (
            "reduceGroupListUpdated",
            "reduceGroupInvite",
            "reduceGroupApply",
            "reduceGroupMemberChanged",
        ):
            with self.subTest(event_reducer=reducer):
                self.assertIn(f"GroupManagementEventService::{reducer}", chat_dispatcher_router)
                self.assertIn(reducer, event_header)
                self.assertIn(f"GroupManagementEffect GroupManagementEventService::{reducer}", event_service)

        self.assertIn("GroupManagementResponseService::reduceSuccess", management)
        self.assertIn("GroupResponseErrorService::reduce", errors)
        self.assertIn("GroupResponseErrorTarget::ManagementEffect", errors)
        self.assertNotIn("GroupManagementResponseService::reduceError", errors)
        self.assertIn("static bool isManagementResponse(int reqId);", response_header)
        self.assertIn("enum class GroupResponseErrorTarget", read(GROUP / "controller/GroupResponseErrorService.h"))
        self.assertIn("GroupManagementResponseEffect", response_header)
        self.assertNotIn("GroupManagementEffectApplier.h", management)
        self.assertNotIn("GroupManagementEffectApplier::apply", events + chat_dispatcher_router + management + errors)
        self.assertIn("_group_controller.applyGroupManagementEffect", chat_dispatcher_router)
        self.assertIn("_features.groupController.applyGroupManagementResponseEffect", management + errors)
        self.assertNotIn("void applyGroupManagementEffect", app_header)
        self.assertNotIn("void applyGroupManagementResponseEffect", app_header)
        self.assertNotIn("groupManagementEventEffectPort", app_header[app_header.index("private:") :])
        self.assertNotIn("groupManagementResponseEffectPort", app_header[app_header.index("private:") :])
        self.assertNotIn(
            "AppController::groupManagementEventEffectPort", events + chat_dispatcher_router + management + errors
        )
        self.assertNotIn(
            "AppController::groupManagementResponseEffectPort", events + chat_dispatcher_router + management + errors
        )

        self.assertIn("app/controller/GroupManagementEffectPortFactory.cpp", app_cmake)
        self.assertIn("app/controller/GroupManagementEffectPortFactory.h", app_cmake)
        self.assertIn("GroupManagementEffectPorts.h", factory_header)
        self.assertIn("struct GroupManagementEventEffectActions", factory_header)
        self.assertIn("struct GroupManagementResponseEffectActions", factory_header)
        self.assertIn("makeGroupManagementEventEffectPort(GroupManagementEventEffectActions actions)", factory_header)
        self.assertIn(
            "makeGroupManagementResponseEffectPort(GroupManagementResponseEffectActions actions)", factory_header
        )
        self.assertIn("makeGroupManagementEventEffectPort(GroupManagementEventEffectActions actions)", factory_cpp)
        self.assertIn(
            "makeGroupManagementResponseEffectPort(GroupManagementResponseEffectActions actions)", factory_cpp
        )
        self.assertIn("memochat::group::GroupManagementEventEffectPort", factory_sources)
        self.assertIn("memochat::group::GroupManagementResponseEffectPort", factory_sources)
        self.assertNotIn("AppController", factory_sources)
        self.assertNotIn("appController", factory_sources)

        for event_port_callback in (
            "removeGroup",
            "clearGroupConversation",
            "refreshGroupModel",
            "refreshDialogModel",
            "requestDialogList",
            "selectCurrentGroup",
            "clearCurrentGroup",
            "setCurrentChatPeerIcon",
            "openGroupConversation",
            "syncCurrentDialogDraft",
            "loadCurrentChatMessages",
            "clearMessageModel",
            "resetCurrentChatProjection",
        ):
            with self.subTest(event_port_callback=event_port_callback):
                self.assertIn(event_port_callback, factory_header)
                self.assertRegex(factory_cpp, rf"(?:\bport\.|\.){event_port_callback}\s*=")

        for response_port_callback in (
            "removeGroup",
            "clearGroupConversation",
            "refreshGroupModel",
            "refreshDialogModel",
            "selectDialogByUid",
            "setCurrentChatPeerIcon",
        ):
            with self.subTest(response_port_callback=response_port_callback):
                self.assertIn(response_port_callback, factory_header)
                self.assertRegex(factory_cpp, rf"(?:\bport\.|\.){response_port_callback}\s*=")

        for service_source in (event_service + event_header, response_service + response_header):
            with self.subTest(service_contract="no_appcontroller_dependency"):
                self.assertNotIn("AppController", service_source)
                self.assertNotIn("appController", service_source)

        self.assertIn("struct GroupManagementEventEffectPort", effect_ports_header)
        self.assertIn("struct GroupManagementResponseEffectPort", effect_ports_header)

        for group_owned_status in (
            "收到群邀请",
            "收到入群申请",
            "群事件：",
            "群聊创建成功",
            "群头像已更新",
            "群聊已解散",
            "已退出当前群聊",
        ):
            with self.subTest(group_owned_status=group_owned_status):
                self.assertIn(group_owned_status, event_service + response_service)
                self.assertNotIn(group_owned_status, events + management + errors)

        self.assertNotIn("QJsonDocument(", events + management + errors)

        group_controller = read(GROUP / "controller/GroupController.cpp")
        group_payloads = read(GROUP / "controller/GroupRequestPayloads.cpp")
        group_header = read(GROUP / "controller/GroupController.h")
        self.assertIn("void applyGroupManagementEffect(", group_header)
        self.assertIn("void applyGroupManagementResponseEffect(", group_header)
        self.assertIn("GroupController::applyGroupManagementEffect", group_controller)
        self.assertIn("GroupController::applyGroupManagementResponseEffect", group_controller)
        self.assertIn("GroupManagementEffectApplier::apply(effect, port);", group_controller)
        self.assertNotIn("AppController", group_header + group_controller)
        self.assertIn("void GroupController::updateGroupIcon", group_controller)
        self.assertIn("pickGroupIcon", group_controller)
        self.assertIn("uploadGroupIcon", group_controller)
        self.assertIn("QFutureWatcher<UploadedMediaInfo>", group_controller)
        self.assertIn("QtConcurrent::run", group_controller)
        self.assertIn("ID_UPDATE_GROUP_ICON_REQ", group_controller)
        self.assertIn("buildUpdateGroupIconPayload", group_payloads)
        self.assertNotIn("void updateGroupIcon(int source = 0);", app_header)


if __name__ == "__main__":
    unittest.main()
