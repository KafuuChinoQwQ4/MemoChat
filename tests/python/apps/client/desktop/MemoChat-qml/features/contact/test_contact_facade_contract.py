import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CLIENT = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
CONTACT = CLIENT / "features/contact"
APP = CLIENT / "app"
APP_FEATURE_REGISTRY_H = APP / "composition/AppFeatureRegistry.h"
APP_CONTACT_FEATURE_PORT_BINDER = APP / "composition/AppContactFeaturePortBinder.cpp"
APP_CONTROLLER = APP / "controller"
CHAT_VIEW = CLIENT / "features/chat/view"
SHELL_PAGE = CLIENT / "qml/app/ChatShellPage.qml"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def normalized(source: str) -> str:
    return " ".join(source.split())


def chat_dialog_runtime_source() -> str:
    controller_dir = CLIENT / "features/chat/controller"
    files = (
        "ChatFeatureControllerDialogRuntimeInternal.h",
        "ChatFeatureControllerDialogRuntime.cpp",
        "ChatFeatureControllerDialogDecorations.cpp",
        "ChatFeatureControllerDialogDraftCommands.cpp",
        "ChatFeatureControllerDialogReadCommands.cpp",
        "ChatFeatureControllerDialogMeta.cpp",
        "ChatFeatureControllerPrivateConversation.cpp",
    )
    return "\n".join(read(controller_dir / name) for name in files)


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


class ContactFacadeContractTests(unittest.TestCase):
    def test_contact_controller_exposes_qml_state_and_actions(self):
        header = read(CONTACT / "controller/ContactController.h")
        compact_header = normalized(header)

        expected_tokens = (
            "class ContactController : public QObject",
            "Q_OBJECT",
            "Q_PROPERTY(int contactPane READ contactPane NOTIFY contactPaneChanged)",
            "Q_PROPERTY(QString currentContactName READ currentContactName NOTIFY currentContactChanged)",
            "Q_PROPERTY(QString currentContactNick READ currentContactNick NOTIFY currentContactChanged)",
            "Q_PROPERTY(QString currentContactIcon READ currentContactIcon NOTIFY currentContactChanged)",
            "Q_PROPERTY(QString currentContactBack READ currentContactBack NOTIFY currentContactChanged)",
            "Q_PROPERTY(int currentContactSex READ currentContactSex NOTIFY currentContactChanged)",
            "Q_PROPERTY(QString currentContactUserId READ currentContactUserId NOTIFY currentContactChanged)",
            "Q_PROPERTY(int currentContactUid READ currentContactUid NOTIFY currentContactChanged)",
            "Q_PROPERTY(bool hasCurrentContact READ hasCurrentContact NOTIFY currentContactChanged)",
            "Q_PROPERTY(FriendListModel* contactListModel READ contactListModel NOTIFY modelChanged)",
            "Q_PROPERTY(SearchResultModel* searchResultModel READ searchResultModel NOTIFY modelChanged)",
            "Q_PROPERTY(ApplyRequestModel* applyRequestModel READ applyRequestModel NOTIFY modelChanged)",
            "Q_PROPERTY(bool searchPending READ searchPending NOTIFY searchPendingChanged)",
            "Q_PROPERTY(QString searchStatusText READ searchStatusText NOTIFY searchStatusChanged)",
            "Q_PROPERTY(bool searchStatusError READ searchStatusError NOTIFY searchStatusChanged)",
            "Q_PROPERTY(QString authStatusText READ authStatusText NOTIFY authStatusChanged)",
            "Q_PROPERTY(bool authStatusError READ authStatusError NOTIFY authStatusChanged)",
            "Q_PROPERTY(bool hasPendingApply READ hasPendingApply NOTIFY pendingApplyChanged)",
            "Q_PROPERTY(bool contactLoadingMore READ contactLoadingMore NOTIFY contactLoadingMoreChanged)",
            "Q_PROPERTY(bool canLoadMoreContacts READ canLoadMoreContacts NOTIFY canLoadMoreContactsChanged)",
            "Q_PROPERTY(bool contactsReady READ contactsReady NOTIFY contactsReadyChanged)",
            "Q_INVOKABLE void ensureContactsInitialized();",
            "Q_INVOKABLE void ensureApplyInitialized();",
            "Q_INVOKABLE void selectContactIndex(int index);",
            "Q_INVOKABLE void searchUser(const QString& uidText);",
            "Q_INVOKABLE void clearSearchState();",
            "Q_INVOKABLE void requestAddFriend(int uid, const QString& bakName, const QVariantList& labels = QVariantList());",
            "Q_INVOKABLE void approveFriend(int uid, const QString& backName, const QVariantList& labels = QVariantList());",
            "Q_INVOKABLE QVariantMap contactProfileByUid(int uid) const;",
            "Q_INVOKABLE void refreshContactProfileByUid(int uid);",
            "Q_INVOKABLE void deleteFriend(int uid);",
            "Q_INVOKABLE void showApplyRequests();",
            "Q_INVOKABLE void jumpChatWithCurrentContact();",
            "Q_INVOKABLE void loadMoreContacts();",
            "Q_INVOKABLE void clearAuthStatus();",
            "void syncModels(FriendListModel* contactListModel,",
            "void syncCurrentContact(int uid,",
            "void setCommandPort(ContactCommandPort port);",
            "struct ContactCommandPort",
            "void contactProfilesChanged();",
        )
        for token in expected_tokens:
            with self.subTest(token=token):
                self.assertIn(token, compact_header if "\n" not in token else header)

    def test_contact_controller_keeps_transport_helpers(self):
        header = read(CONTACT / "controller/ContactController.h")

        helper_tokens = (
            "bool sendSearchUser(const QString& uidText, QString* errorText) const;",
            "void sendAddFriend(int selfUid,",
            "void sendApproveFriend(int selfUid,",
            "void sendDeleteFriend(int selfUid, int friendUid) const;",
        )
        for token in helper_tokens:
            with self.subTest(token=token):
                self.assertIn(token, header)

        payload_header = read(CONTACT / "controller/ContactRequestPayloads.h")
        payload_source = read(CONTACT / "controller/ContactRequestPayloads.cpp")
        self.assertIn("QJsonObject buildDeleteFriendPayload(int selfUid, int friendUid);", payload_header)
        self.assertIn("QJsonObject buildDeleteFriendPayload(int selfUid, int friendUid)", payload_source)

    def test_sparse_contact_upserts_preserve_existing_user_id(self):
        user_mgr_friends = read(CLIENT / "core/session/UserMgrFriends.cpp")
        friend_model = read(CONTACT / "model/FriendListModel.cpp")

        for source in (user_mgr_friends, friend_model):
            with self.subTest(source=source[:40]):
                self.assertIn("QString nonEmptyOrExisting(const QString& next, const QString& existing)", source)
                self.assertIn("return next.trimmed().isEmpty() ? existing : next;", source)

        self.assertIn("void mergeSparseFriendInfo(FriendInfo& stored, const FriendInfo& next)", user_mgr_friends)
        self.assertIn("stored._user_id = nonEmptyOrExisting(next._user_id, stored._user_id);", user_mgr_friends)
        self.assertIn("void upsertSparseFriendInfo(", user_mgr_friends)
        self.assertIn("const auto listInfo = findFriendInList(friends, friendInfo->_uid);", user_mgr_friends)
        self.assertIn("const auto target = listInfo ? listInfo : mapIter.value();", user_mgr_friends)
        self.assertIn("friendMap[friendInfo->_uid] = target;", user_mgr_friends)
        self.assertIn(
            "upsertSparseFriendInfo(_friend_list, _friend_map, std::make_shared<FriendInfo>(auth_rsp));",
            user_mgr_friends,
        )
        self.assertIn(
            "upsertSparseFriendInfo(_friend_list, _friend_map, std::make_shared<FriendInfo>(auth_info));",
            user_mgr_friends,
        )
        self.assertNotIn("_friend_map[friend_info->_uid] = friend_info;\n}", user_mgr_friends)

        upsert_body = extract_function(friend_model, "void FriendListModel::upsert(const FriendEntry& entry)")
        merge_body = extract_function(
            friend_model,
            "FriendListModel::FriendEntry FriendListModel::mergeSparseEntry(const FriendEntry& entry, const FriendEntry& existing)",
        )
        self.assertIn("stored = mergeSparseEntry(entry, stored);", upsert_body)
        self.assertIn("merged.userId = nonEmptyOrExisting(entry.userId, existing.userId);", merge_body)
        self.assertIn("merged.name = nonEmptyOrExisting(entry.name, existing.name);", merge_body)
        self.assertNotIn("stored = entry;", upsert_body)

        mutations = read(CONTACT / "model/FriendListModelMutations.cpp")
        self.assertIn("FriendListModel::FriendEntry FriendListModel::toEntry", mutations)
        self.assertIn("FriendListModel::FriendEntry FriendListModel::mergeWithCurrentEntry", mutations)
        set_friends_body = extract_function(
            mutations,
            "void FriendListModel::setFriends(const std::vector<std::shared_ptr<FriendInfo>>& friends)",
        )
        self.assertIn("std::vector<FriendEntry> nextItems;", set_friends_body)
        self.assertIn("nextItems.push_back(mergeWithCurrentEntry(toEntry(friendInfo)));", set_friends_body)
        self.assertIn("_items = std::move(nextItems);", set_friends_body)
        upsert_batch_body = extract_function(
            mutations,
            "void FriendListModel::upsertBatch(const std::vector<std::shared_ptr<FriendInfo>>& friends, bool resetFirst)",
        )
        reset_body = upsert_batch_body[: upsert_batch_body.index("bool changed = false;")]
        self.assertIn("nextItems.push_back(mergeWithCurrentEntry(toEntry(friendInfo)));", reset_body)
        self.assertIn("_items = std::move(nextItems);", reset_body)
        non_reset_body = upsert_batch_body[upsert_batch_body.index("bool changed = false;") :]
        self.assertIn("upsert(toEntry(friendInfo));", non_reset_body)
        self.assertNotIn("_items[static_cast<size_t>(existingIdx)] = entry;", non_reset_body)

    def test_appcontroller_exposes_additive_contact_composition_surface(self):
        header = read(APP / "controller/AppController.h")
        registry = read(APP_FEATURE_REGISTRY_H)
        controller = read(APP / "controller/AppController.cpp")
        port_binder = read(APP_CONTACT_FEATURE_PORT_BINDER)
        model_props = read(APP / "controller/AppControllerModelProperties.cpp")
        composition = read(APP / "composition/AppComposition.cpp")
        public = header[header.index("public:") : header.index("signals:")]
        private = header[header.index("private:") :]

        self.assertNotIn("Q_PROPERTY(ContactController* contact READ contact CONSTANT)", header)
        self.assertNotIn("ContactController* contact();", header)
        self.assertIn("ContactController* contactController();", public)
        self.assertIn("AppFeatureRegistry _features", header)
        self.assertIn("ContactController contactController;", registry)
        self.assertIn("void syncContactControllerState();", private)

        self.assertNotIn("ContactController* AppController::contact()", model_props)
        self.assertIn("ContactController* AppController::contactController()", model_props)
        self.assertIn("return &_features.contactController;", model_props)

        self.assertNotIn("ContactController::searchUserRequested", controller)
        self.assertNotIn("ContactController::requestAddFriendRequested", controller)
        self.assertNotIn("ContactController::approveFriendRequested", controller)
        self.assertNotIn("ContactController::deleteFriendRequested", controller)
        self.assertNotIn("ContactController::selectContactIndexRequested", controller)
        self.assertNotIn("ContactController::loadMoreContactsRequested", controller)
        normalized_port_binder = normalized(port_binder)
        self.assertIn("_features.contactController.setBootstrapPort( ContactBootstrapPort{", normalized_port_binder)
        self.assertIn("_features.contactController.setCommandPort(ContactCommandPort{", normalized_port_binder)
        self.assertNotIn("_features.contactController.setBootstrapPort(ContactBootstrapPort{", controller)
        self.assertNotIn("_features.contactController.setCommandPort(ContactCommandPort{", controller)
        self.assertIn("void AppController::syncContactControllerState()", controller)
        self.assertIn('context.setContextProperty("contact", contact())', composition)

    def test_contact_bootstrap_initial_page_is_feature_owned(self):
        app_models = read(APP / "controller/AppControllerModels.cpp")
        app_navigation = read(APP / "controller/AppControllerNavigation.cpp")
        app_controller = read(APP / "controller/AppController.cpp")
        port_binder = read(APP_CONTACT_FEATURE_PORT_BINDER)
        contact_header = read(CONTACT / "controller/ContactController.h")
        contact_source = read(CONTACT / "controller/ContactController.cpp")

        bootstrap_body = extract_function(app_models, "void AppController::bootstrapContacts")
        ensure_body = extract_function(app_navigation, "void AppController::ensureContactsInitialized")
        contact_ensure_body = extract_function(contact_source, "void ContactController::ensureContactsInitialized")
        contact_set_body = extract_function(
            contact_source,
            "void ContactController::setContacts(const std::vector<std::shared_ptr<FriendInfo>>& contacts)",
        )
        contact_upsert_body = extract_function(
            contact_source,
            "void ContactController::upsertContact(const std::shared_ptr<FriendInfo>& friendInfo)",
        )

        self.assertIn("_features.contactController.ensureContactsInitialized();", bootstrap_body)
        self.assertIn("_features.contactController.ensureContactsInitialized();", ensure_body)
        for forbidden in (
            "GetConListPerPage",
            "UpdateContactLoadedCount",
            "refreshContactLoadMoreState",
            "setContactsReady(true)",
            "_features.contactController.setContacts",
            "ensureChatListInitialized();",
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, bootstrap_body)
                self.assertNotIn(forbidden, ensure_body)

        self.assertIn("struct ContactBootstrapPort", contact_header)
        self.assertIn("void setBootstrapPort(ContactBootstrapPort port);", contact_header)
        self.assertIn("_bootstrap_port.ensureChatListInitialized", contact_ensure_body)
        self.assertIn("_bootstrap_port.nextPage", contact_ensure_body)
        self.assertIn("setContacts(contacts);", contact_ensure_body)
        self.assertIn("_bootstrap_port.markPageLoaded", contact_ensure_body)
        self.assertIn("setCanLoadMoreContacts(!_bootstrap_port.loadFinished());", contact_ensure_body)
        self.assertIn("setContactsReady(true);", contact_ensure_body)
        self.assertIn("refreshCurrentContactFromStore();", contact_set_body)
        self.assertIn("refreshCurrentContactFromStore();", contact_upsert_body)
        self.assertIn("void refreshCurrentContactFromStore();", contact_header)
        self.assertIn("return _gateway.userMgr()->GetConListPerPage();", port_binder)
        self.assertIn("_gateway.userMgr()->UpdateContactLoadedCount();", port_binder)
        self.assertNotIn("return _gateway.userMgr()->GetConListPerPage();", app_controller)

    def test_contact_apply_bootstrap_is_feature_owned(self):
        app_models = read(APP / "controller/AppControllerModels.cpp")
        app_navigation = read(APP / "controller/AppControllerNavigation.cpp")
        app_controller = read(APP / "controller/AppController.cpp")
        port_binder = read(APP_CONTACT_FEATURE_PORT_BINDER)
        contact_header = read(CONTACT / "controller/ContactController.h")
        contact_source = read(CONTACT / "controller/ContactController.cpp")

        ensure_body = extract_function(app_navigation, "void AppController::ensureApplyInitialized")
        contact_ensure_body = extract_function(contact_source, "void ContactController::ensureApplyInitialized")
        contact_refresh_body = extract_function(contact_source, "void ContactController::refreshApplySnapshot")

        self.assertIn("_features.contactController.ensureApplyInitialized();", ensure_body)
        self.assertNotIn("void AppController::bootstrapApplies", app_models)
        self.assertNotIn("void AppController::refreshApplyModel", app_models)
        self.assertNotIn("refreshApplyModel();", app_controller)
        self.assertNotIn("bootstrapApplies", app_controller)
        for forbidden in (
            "refreshApplyModel();",
            "setApplyReady(true);",
            "GetApplyListSnapshot",
            "_features.contactController.setApplies",
            "_features.contactController.applyReady()",
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, ensure_body)

        self.assertIn("struct ContactApplyBootstrapPort", contact_header)
        self.assertIn("void setApplyBootstrapPort(ContactApplyBootstrapPort port);", contact_header)
        self.assertIn("void refreshApplySnapshot();", contact_header)
        self.assertIn("refreshApplySnapshot();", contact_ensure_body)
        self.assertIn("_apply_bootstrap_port.applySnapshot", contact_refresh_body)
        self.assertIn("setApplies(applies);", contact_refresh_body)
        self.assertIn("setApplyReady(true);", contact_refresh_body)
        self.assertIn(
            "_features.contactController.setApplyBootstrapPort( ContactApplyBootstrapPort{",
            normalized(port_binder),
        )
        self.assertIn("GetApplyListSnapshot();", port_binder)
        self.assertNotIn("_features.contactController.setApplyBootstrapPort(ContactApplyBootstrapPort{", app_controller)
        self.assertNotIn("std::function<void()> bootstrapApplies;", contact_header)

    def test_relation_apply_refresh_is_contact_owned(self):
        app_controller = read(APP / "controller/AppController.cpp")
        port_binder = read(APP / "composition/AppRelationBootstrapPortBinder.cpp")
        coordinators = read(APP / "coordinators/AppCoordinators.h")
        relation = read(APP / "session/SessionRelationBootstrap.cpp")
        contact_header = read(CONTACT / "controller/ContactController.h")
        contact_source = read(CONTACT / "controller/ContactController.cpp")

        relation_port = coordinators[coordinators.index("struct RelationBootstrapPort") :]
        relation_port = relation_port[: relation_port.index("};") + 2]
        relation_updated = extract_function(relation, "void SessionRelationBootstrap::onRelationBootstrapUpdated")
        refresh_body = extract_function(contact_source, "void ContactController::refreshApplySnapshot")
        refresh_profiles_body = extract_function(contact_source, "void ContactController::refreshContactProfiles")

        self.assertIn("std::function<void()> refreshApplySnapshot;", relation_port)
        self.assertIn("std::function<void()> refreshContactProfiles;", relation_port)
        self.assertIn("std::function<std::vector<std::shared_ptr<FriendInfo>>()> friendListSnapshot;", relation_port)
        self.assertIn("std::function<std::vector<std::shared_ptr<FriendInfo>>()> nextContactPage;", relation_port)
        self.assertNotIn("refreshApplyModel", relation_port)
        self.assertNotIn("setApplyReady", relation_port)
        self.assertIn("_port.refreshContactProfiles();", relation_updated)
        self.assertIn("const auto contactList = _port.nextContactPage();", relation_updated)
        self.assertIn("_port.refreshApplySnapshot();", relation_updated)
        self.assertNotIn("_port.refreshApplyModel();", relation_updated)
        self.assertNotIn("_port.setApplyReady(true);", relation_updated)
        self.assertIn("void refreshApplySnapshot();", contact_header)
        self.assertIn("void refreshContactProfiles();", contact_header)
        self.assertIn("void handleContactHttpFinished(ReqId id, const QString& res, ErrorCodes err);", contact_header)
        self.assertIn("_features.contactController.refreshContactProfiles();", port_binder)
        self.assertIn("const auto friends = _gateway->userMgr()->GetFriendListSnapshot();", refresh_profiles_body)
        self.assertIn("_contact_list_model->indexOfUid(friendInfo->_uid) >= 0", refresh_profiles_body)
        self.assertIn("_contact_list_model->upsertFriend(friendInfo);", refresh_profiles_body)
        self.assertIn("_apply_bootstrap_port.applySnapshot", refresh_body)
        self.assertIn("setApplies(applies);", refresh_body)
        self.assertIn("setApplyReady(true);", refresh_body)
        self.assertIn("_features.contactController.refreshApplySnapshot();", port_binder)
        self.assertNotIn("_features.contactController.refreshApplySnapshot();", app_controller)
        self.assertNotIn("refreshApplyModel();\n                                },", app_controller + port_binder)

    def test_contact_commands_are_feature_owned_not_appcontroller_forwarded(self):
        header = read(APP / "controller/AppController.h")
        controller = read(APP / "controller/AppController.cpp")
        contact_header = read(CONTACT / "controller/ContactController.h")
        contact_source = read(CONTACT / "controller/ContactController.cpp")

        forbidden_app_tokens = (
            "class ContactCoordinatorShell",
            "friend class ContactCoordinatorShell",
            "std::unique_ptr<ContactCoordinatorShell>",
            "_contact_coordinator_shell",
            "void searchUser(const QString& uidText);",
            "void requestAddFriend(int uid, const QString& bakName, const QVariantList& labels = QVariantList());",
            "void approveFriend(int uid, const QString& backName, const QVariantList& labels = QVariantList());",
        )
        for token in forbidden_app_tokens:
            with self.subTest(token=token):
                self.assertNotIn(token, header)
                self.assertNotIn(token, controller)

        forbidden_signals = (
            "void searchUserRequested(const QString& uidText);",
            "void requestAddFriendRequested(int uid, const QString& bakName, const QVariantList& labels);",
            "void approveFriendRequested(int uid, const QString& backName, const QVariantList& labels);",
        )
        for token in forbidden_signals:
            with self.subTest(signal=token):
                self.assertNotIn(token, contact_header)
                self.assertNotIn(token, contact_source)

        expected_feature_owned_tokens = (
            "void ContactController::searchUser(const QString& uidText)",
            "sendSearchUser(uidText, &errorText)",
            "void ContactController::requestAddFriend(int uid, const QString& bakName, const QVariantList& labels)",
            "sendAddFriend(selfInfo->_uid, selfInfo->_name, uid, bakName, labels);",
            "void ContactController::approveFriend(int uid, const QString& backName, const QVariantList& labels)",
            "sendApproveFriend(selfInfo->_uid, uid, remark, labels);",
            "void ContactController::selectContactIndex(int index)",
            "emit currentContactSelected(uid);",
            "void ContactController::clearSearchState()",
            "void ContactController::deleteFriend(int uid)",
            "sendDeleteFriend(selfInfo->_uid, uid);",
            "void ContactController::showApplyRequests()",
            "void ContactController::loadMoreContacts()",
            "void ContactController::clearAuthStatus()",
        )
        for token in expected_feature_owned_tokens:
            with self.subTest(feature_token=token):
                self.assertIn(token, contact_source)

        forbidden_feature_signals = (
            "ensureContactsInitializedRequested",
            "ensureApplyInitializedRequested",
            "selectContactIndexRequested",
            "clearSearchStateRequested",
            "deleteFriendRequested",
            "showApplyRequestsRequested",
            "jumpChatWithCurrentContactRequested",
            "loadMoreContactsRequested",
            "clearAuthStatusRequested",
        )
        for token in forbidden_feature_signals:
            with self.subTest(forbidden_feature_signal=token):
                self.assertNotIn(token, contact_header)
                self.assertNotIn(token, contact_source)

    def test_contact_server_events_are_feature_owned_not_appcontroller_applied(self):
        app_events = read(APP_CONTROLLER / "AppControllerContactEvents.cpp")
        chat_dispatcher_router = read(APP / "events/AppChatDispatcherEventRouter.cpp")
        app_header = read(APP_CONTROLLER / "AppController.h")
        contact_service_header = read(CONTACT / "controller/ContactEventService.h")
        contact_service_source = read(CONTACT / "controller/ContactEventService.cpp")
        contact_manifest = read(CONTACT / "sources.cmake")
        contact_test_manifest = read(
            REPO_ROOT / "tests/cpp/apps/client/desktop/MemoChat-qml/features/contact/controller/CMakeLists.txt"
        )

        expected_service_tokens = (
            "struct ContactEventDependencies",
            "class ContactEventService",
            "handleAddAuthFriend",
            "handleAuthRsp",
            "handleDeleteFriendRsp",
            "handleUserSearch",
            "handleFriendApply",
        )
        combined_service = contact_service_header + "\n" + contact_service_source
        for token in expected_service_tokens:
            with self.subTest(service_token=token):
                self.assertIn(token, combined_service)

        for path_token in (
            "features/contact/controller/ContactEventService.cpp",
            "features/contact/controller/ContactEventService.h",
        ):
            with self.subTest(path_token=path_token):
                self.assertIn(path_token, contact_manifest)
                self.assertIn(path_token, contact_test_manifest)

        self.assertIn('#include "ContactEventDependenciesFactory.h"', app_events)
        self.assertIn(
            "memochat::app::ContactEventActions AppController::contactEventActions()",
            app_events,
        )
        self.assertIn("memochat::app::ContactEventActions contactEventActions();", app_header)
        self.assertNotIn("ContactEventDependencies contactEventDependencies();", app_header)

        forbidden_forwarding_tokens = (
            "ContactEventDependencies AppController::contactEventDependencies()",
            "ContactEventService::handleUserSearch(_features.contactController, std::move(searchInfo), contactEventDependencies());",
            "ContactEventService::handleAddAuthFriend(_features.contactController, std::move(authInfo), contactEventDependencies());",
            "ContactEventService::handleAuthRsp(_features.contactController, std::move(authRsp), contactEventDependencies());",
            "ContactEventService::handleDeleteFriendRsp(_features.contactController, error, friendUid, contactEventDependencies());",
            "ContactEventService::handleFriendApply(_features.contactController, std::move(applyInfo), contactEventDependencies());",
        )
        for token in forbidden_forwarding_tokens:
            with self.subTest(forbidden_forwarding_token=token):
                self.assertNotIn(token, app_events)

        expected_forwarding_tokens = (
            "ContactEventService::handleUserSearch(_contact_controller",
            "ContactEventService::handleAddAuthFriend(_contact_controller",
            "ContactEventService::handleAuthRsp(_contact_controller",
            "ContactEventService::handleDeleteFriendRsp(_contact_controller",
            "ContactEventService::handleFriendApply(_contact_controller",
        )
        for token in expected_forwarding_tokens:
            with self.subTest(forwarding_token=token):
                self.assertIn(token, chat_dispatcher_router)

        expected_factory_calls = (
            "ContactEventService::handleUserSearch(_contact_controller,\n"
            "                                          std::move(searchInfo),\n"
            "                                          _make_contact_event_dependencies());",
            "ContactEventService::handleAddAuthFriend(_contact_controller,\n"
            "                                             std::move(authInfo),\n"
            "                                             _make_contact_event_dependencies());",
            "ContactEventService::handleAuthRsp(_contact_controller,\n"
            "                                       std::move(authRsp),\n"
            "                                       _make_contact_event_dependencies());",
            "ContactEventService::handleDeleteFriendRsp(_contact_controller,\n"
            "                                               error,\n"
            "                                               friendUid,\n"
            "                                               _make_contact_event_dependencies());",
            "ContactEventService::handleFriendApply(_contact_controller,\n"
            "                                           std::move(applyInfo),\n"
            "                                           _make_contact_event_dependencies());",
        )
        normalized_router = normalized(chat_dispatcher_router)
        for token in expected_factory_calls:
            with self.subTest(factory_call=token.split("(", 1)[0]):
                self.assertIn(normalized(token), normalized_router)

        forbidden_event_logic = (
            'setSearchStatus("未找到该用户", true)',
            'setSearchStatus("不能搜索自己", true)',
            'setSearchStatus("已是好友，已切换到会话", false)',
            'setSearchStatus("已找到用户", false)',
            'setAuthStatus("好友添加成功", false)',
            'setAuthStatus("联系人已删除", false)',
            'setAuthStatus(QString("删除联系人失败',
            'setAuthStatus(QString("收到来自 %1 的好友申请"',
            "_features.contactController.upsertContact(authInfo)",
            "_features.contactController.upsertContact(authRsp)",
            "_features.contactController.upsertApply(apply)",
            "_features.contactController.markApplyApproved",
            "_features.contactController.setSearchResult",
            "_features.contactController.removeContactByUid",
            "auto apply = std::make_shared<ApplyInfo>",
            "if (!searchInfo)",
            "if (searchInfo->_uid ==",
            "if (_gateway.userMgr()->CheckFriendById",
            "if (_gateway.userMgr()->AlreadyApply",
        )
        for token in forbidden_event_logic:
            with self.subTest(forbidden_event_logic=token):
                self.assertNotIn(token, app_events)

        self.assertIn('contact.setSearchStatus(QStringLiteral("未找到该用户"), true);', contact_service_source)
        self.assertIn('contact.setAuthStatus(QStringLiteral("好友添加成功"), false);', contact_service_source)
        self.assertIn('contact.setAuthStatus(QStringLiteral("联系人已删除"), false);', contact_service_source)

    def test_delete_friend_chat_reset_is_delegated_to_chat_feature(self):
        app_events = read(APP / "controller/AppControllerContactEvents.cpp")
        chat_header = read(CLIENT / "features/chat/controller/ChatFeatureController.h")
        chat_runtime_header = read(CLIENT / "features/chat/controller/ChatDialogRuntimeState.h")
        chat_runtime = chat_dialog_runtime_source()

        clear_adapter = app_events.split("actions.removeChatContact = [this](int friendUid)", 1)[1].split(
            "actions.openPrivateChat", 1
        )[0]

        self.assertIn("ChatPrivateConversationClearPort port;", clear_adapter)
        self.assertIn("_features.chatFeatureController.removeContactConversation(friendUid, port);", clear_adapter)
        self.assertIn("struct ChatPrivateConversationClearPort", chat_runtime_header)
        self.assertIn("clearPrivateConversationIfSelected", chat_header)
        self.assertIn("removeContactConversation", chat_header)
        self.assertIn("ChatFeatureController::clearPrivateConversationIfSelected", chat_runtime)
        self.assertIn("ChatFeatureController::removeContactConversation", chat_runtime)
        self.assertIn("_chatListModel.removeByUid(friendUid);", chat_runtime)
        self.assertIn("_dialogListModel.removeByUid(friendUid);", chat_runtime)
        self.assertIn("removeDialogDecoration(friendUid);", chat_runtime)
        self.assertIn("port.clearMessageModel = [this]()", clear_adapter)
        self.assertIn("port.emitCurrentDialogUidChangedIfNeeded = [this]()", clear_adapter)
        for forbidden in (
            "ChatPrivateConversationClearRequest request;",
            "_features.chatFeatureController.chatListModel().removeByUid",
            "_features.chatFeatureController.dialogListModel().removeByUid",
            "_features.chatFeatureController.removeDialogDecoration",
            "_features.chatFeatureController.clearPrivateConversationIfSelected",
            "if (_chat_state.uid != friendUid)",
            "setCurrentChatPeerName(QString());",
            'setCurrentChatPeerIcon(QStringLiteral("qrc:/res/head_1.png"));',
            "setCurrentDraftText(QString());",
            "setCurrentDialogPinned(false);",
            "setCurrentDialogMuted(false);",
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, clear_adapter)

    def test_contact_qml_uses_contact_facade_for_left_panel(self):
        qml = read(CHAT_VIEW / "ChatNormalFace.qml")

        expected_contact_tokens = (
            "hasPendingApply: contact.hasPendingApply",
            "contactsReady: contact.contactsReady",
            "contactModel: contact.contactListModel",
            "searchModel: contact.searchResultModel",
            "searchPending: contact.searchPending",
            "searchStatusText: contact.searchStatusText",
            "searchStatusError: contact.searchStatusError",
            "canLoadMoreContacts: contact.canLoadMoreContacts",
            "contactLoadingMore: contact.contactLoadingMore",
            "onContactIndexSelected: function(index) { contact.selectContactIndex(index) }",
            "onOpenApplyRequested: contact.showApplyRequests()",
            "onRequestContactLoadMore: contact.loadMoreContacts()",
            "onSearchRequested: function(uidText) { contact.searchUser(uidText) }",
            "onSearchCleared: contact.clearSearchState()",
            "onAddFriendRequested: function(uid, bakName, tags) { contact.requestAddFriend(uid, bakName, tags) }",
        )
        for token in expected_contact_tokens:
            with self.subTest(token=token):
                self.assertIn(token, qml)

    def test_contact_qml_uses_contact_facade_for_detail_pane(self):
        qml = read(CHAT_VIEW / "ChatShellContent.qml")

        expected_contact_tokens = (
            "paneIndex: contact.contactPane",
            "contactName: contact.currentContactName",
            "contactNick: contact.currentContactNick",
            "contactIcon: contact.currentContactIcon",
            "contactBack: contact.currentContactBack",
            "contactSex: contact.currentContactSex",
            "contactUserId: contact.currentContactUserId",
            "hasCurrentContact: contact.hasCurrentContact",
            "applyModel: contact.applyRequestModel",
            "authStatusText: contact.authStatusText",
            "authStatusError: contact.authStatusError",
            "onApproveFriendRequested: function(uid, backName, tags) { contact.approveFriend(uid, backName, tags) }",
            "onAuthStatusCleared: contact.clearAuthStatus()",
            "onMessageChatRequested: contact.jumpChatWithCurrentContact()",
            "onDeleteContactRequested: contact.deleteFriend(contact.currentContactUid)",
        )
        for token in expected_contact_tokens:
            with self.subTest(token=token):
                self.assertIn(token, qml)

    def test_contact_bootstrap_and_shell_modal_use_contact_facade(self):
        left_panel = read(CHAT_VIEW / "ChatLeftPanel.qml")
        shell_page = read(SHELL_PAGE)

        self.assertIn("Component.onCompleted: contact.ensureContactsInitialized()", left_panel)
        self.assertIn("friendModel: contact.contactListModel", shell_page)

    def test_chat_avatar_profile_flow_carries_public_user_id_fallback(self):
        user_message_data = read(CLIENT / "core/session/UserMessageData.h")
        private_dispatcher = read(CLIENT / "core/network/ChatMessageDispatcherPrivate.cpp")
        group_dispatcher = read(CLIENT / "core/network/ChatMessageDispatcherGroup.cpp")
        group_history = read(CLIENT / "core/network/ChatMessageDispatcherGroupHistory.cpp")
        payload_service = read(CLIENT / "features/chat/services/MessagePayloadService.cpp")
        cache_header = read(CLIENT / "features/chat/cache/ChatCacheMessageCodec.h")
        private_cache = read(CLIENT / "features/chat/cache/PrivateChatCacheStore.cpp")
        group_cache = read(CLIENT / "features/chat/cache/GroupChatCacheStore.cpp")
        model_header = read(CLIENT / "features/chat/model/ChatMessageModel.h")
        model_cpp = read(CLIENT / "features/chat/model/ChatMessageModel.cpp")
        model_content = read(CLIENT / "features/chat/model/ChatMessageModelContent.cpp")
        message_avatar = read(CHAT_VIEW / "conversation/MessageAvatar.qml")
        message_delegate = read(CHAT_VIEW / "conversation/ChatMessageDelegate.qml")
        message_list = read(CHAT_VIEW / "conversation/ChatMessageListView.qml")
        conversation = read(CHAT_VIEW / "ChatConversationPane.qml")
        shell_content = read(CHAT_VIEW / "ChatShellContent.qml")
        normal_face = read(CHAT_VIEW / "ChatNormalFace.qml")
        modal_layer = read(CHAT_VIEW / "ChatModalLayer.qml")
        shell_page = read(SHELL_PAGE)
        profile_popup = read(CONTACT / "view/ContactProfilePopup.qml")

        for source, token in (
            (user_message_data, "QString _from_user_id;"),
            (private_dispatcher, 'jsonObj["from_user_id"].toString()'),
            (group_dispatcher, 'jsonObj.value("from_user_id").toString()'),
            (group_history, 'message.value("from_user_id").toString()'),
            (payload_service, 'pickString(payload, msgObj, "from_user_id")'),
            (cache_header, "QString fromUserId;"),
            (private_cache, "from_user_id TEXT"),
            (group_cache, "from_user_id TEXT"),
            (model_header, "FromUserIdRole"),
            (model_cpp, "case FromUserIdRole:\n            return entry.fromUserId;"),
            (model_cpp, '{FromUserIdRole, "fromUserId"}'),
            (model_content, "entry.fromUserId = message->_from_user_id;"),
        ):
            with self.subTest(token=token):
                self.assertIn(token, source)

        for source in (message_avatar, message_delegate, message_list, conversation, shell_content, normal_face):
            with self.subTest(qml=source[:60]):
                self.assertIn("string userId", source)

        self.assertIn("property string senderUserId", message_avatar)
        self.assertIn(
            "root.avatarClicked(root.senderUid, root.senderName, root.avatarSource, root.senderUserId)",
            message_avatar,
        )
        self.assertIn("property string senderUserId", message_delegate)
        self.assertIn("senderUserId: root.senderUserId", message_delegate)
        self.assertIn("property int selfUid: 0", message_list)
        self.assertIn('property string selfUserId: ""', message_list)
        self.assertIn("senderUid: model.outgoing ? root.selfUid : model.fromUid", message_list)
        self.assertIn("senderUserId: model.outgoing ? root.selfUserId : model.fromUserId", message_list)
        self.assertIn("property int selfUid: 0", conversation)
        self.assertIn('property string selfUserId: ""', conversation)
        self.assertIn("selfUid: root.selfUid", conversation)
        self.assertIn("selfUserId: root.selfUserId", conversation)
        self.assertIn("selfUid: shell.currentUserUid", shell_content)
        self.assertIn("selfUserId: shell.currentUserId", shell_content)
        self.assertIn(
            "root.avatarProfileRequested(uid, name, icon, userId)",
            message_list,
        )
        self.assertIn("function openProfile(uid, name, icon, userId)", modal_layer)
        self.assertIn('contactProfilePopup.openProfile(uid, name, icon, userId || "")', modal_layer)
        self.assertIn('modalLayer.openProfile(uid, name, icon, userId || "")', shell_page)
        self.assertIn("readonly property string profileIdentityText", profile_popup)
        self.assertIn("root.profileUserId && root.profileUserId.length > 0", profile_popup)
        self.assertIn(': "ID: 未分配"', profile_popup)
        self.assertNotIn('"UID: " + root.profileUid', profile_popup)
        self.assertIn("function reloadProfileFromController()", profile_popup)
        self.assertIn("contactController.refreshContactProfileByUid(profileUid)", profile_popup)
        self.assertIn("function onContactProfilesChanged()", profile_popup)
        self.assertIn("root.reloadProfileFromController()", profile_popup)
        self.assertIn("var hasProfile = profile && profile.uid !== undefined", profile_popup)
        self.assertIn("isFriend = hasProfile && profile.isFriend === true", profile_popup)

    def test_contact_profile_popup_can_fetch_public_user_id_by_uid_without_exposing_internal_uid(self):
        contact_header = read(CONTACT / "controller/ContactController.h")
        contact_source = read(CONTACT / "controller/ContactController.cpp")
        http_header = read(CLIENT / "core/network/httpmgr.h")
        http_source = read(CLIENT / "core/network/httpmgr.cpp")
        global_header = read(CLIENT / "core/common/global.h")
        app_http_binder = read(APP / "composition/AppHttpSignalBinder.cpp")
        app_http_router_h = read(APP / "events/AppHttpEventRouter.h")
        app_http_router_cpp = read(APP / "events/AppHttpEventRouter.cpp")
        profile_popup = read(CONTACT / "view/ContactProfilePopup.qml")

        self.assertIn("ID_GET_USER_INFO = 1026", global_header)
        self.assertIn("CONTACTMOD = 6", global_header)
        self.assertIn("void sig_contact_mod_finish(ReqId id, QString res, ErrorCodes err);", http_header)
        self.assertIn("emit sig_contact_mod_finish(id, res, err);", http_source)
        self.assertIn("&HttpMgr::sig_contact_mod_finish", app_http_binder)
        self.assertIn("&AppHttpEventRouter::onContactHttpFinished", app_http_binder)
        self.assertIn("ContactController& contactController", app_http_router_h)
        self.assertIn("void onContactHttpFinished(ReqId id, QString res, ErrorCodes err);", app_http_router_h)
        self.assertIn("_contact_controller.handleContactHttpFinished(id, res, err);", app_http_router_cpp)
        self.assertIn("QHash<int, QVariantMap> _profile_lookup_cache;", contact_header)
        self.assertIn("QSet<int> _profile_lookup_pending_uids;", contact_header)
        self.assertIn("void ContactController::requestPublicProfileByUid(int uid)", contact_source)
        self.assertIn("ReqId::ID_GET_USER_INFO", contact_source)
        self.assertIn("Modules::CONTACTMOD", contact_source)
        self.assertIn('QStringLiteral("/get_user_info")', contact_source)
        self.assertIn("QVariantMap publicProfileMapFromJson", contact_source)
        self.assertIn('{"isFriend", false}', contact_source)
        self.assertIn('{"isFriend", true}', contact_source)
        self.assertIn("if (alreadyHasPublicId)", contact_source)
        self.assertIn("requestPublicProfileByUid(uid);", contact_source)
        self.assertIn("profile.isFriend === true", profile_popup)
        self.assertNotIn('"UID: " + root.profileUid', profile_popup)

    def test_contact_identity_views_hide_internal_uid_when_public_id_is_missing(self):
        contact_list = read(CONTACT / "view/ContactListPane.qml")
        contact_pane = read(CONTACT / "view/ContactPane.qml")
        friend_info = read(CONTACT / "view/FriendInfoPane.qml")
        apply_delegate = read(CONTACT / "view/ApplyRequestDelegate.qml")
        dialog_entry_builder = read(CHAT_VIEW.parent / "services/DialogListEntryBuilder.cpp")
        moments_delegate = read(CLIENT / "features/moments/view/MomentsDelegate.qml")
        moments_feed = read(CLIENT / "features/moments/view/MomentsFeedPane.qml")
        moments_side = read(CLIENT / "features/moments/view/MomentsFriendSidePane.qml")
        create_group = read(CLIENT / "features/group/view/CreateGroupDialog.qml")
        find_friend = read(CHAT_VIEW / "sidebar/ChatFindFriendPopup.qml")
        shell_content = read(CHAT_VIEW / "ChatShellContent.qml")

        self.assertIn("readonly property string contactIdentityText", contact_list)
        self.assertIn('? ("ID: " + userId)', contact_list)
        self.assertIn(': "ID: 未分配"', contact_list)
        self.assertNotIn('"UID: " + uid', contact_list)
        self.assertIn("text: contactDelegate.contactIdentityText", contact_list)
        self.assertIn("property int contactUid: 0", contact_pane)
        self.assertIn("contactUid: root.contactUid", contact_pane)
        self.assertIn("property int contactUid: 0", friend_info)
        self.assertIn("readonly property string contactIdentityText", friend_info)
        self.assertIn("? root.contactUserId", friend_info)
        self.assertIn(': "未分配"', friend_info)
        self.assertNotIn('"UID: " + root.contactUid', friend_info)
        self.assertIn("text: root.contactIdentityText", friend_info)
        self.assertIn("contactUid: contact.currentContactUid", shell_content)
        self.assertIn("readonly property string identityText", apply_delegate)
        self.assertIn('? ("ID: " + root.userId)', apply_delegate)
        self.assertIn(': "ID: 未分配"', apply_delegate)
        self.assertNotIn('"UID: " + root.uid', apply_delegate)
        self.assertIn("root.desc.length > 0 ? root.desc : root.identityText", apply_delegate)
        self.assertIn('QStringLiteral("用户")', dialog_entry_builder)
        self.assertNotIn("QString::number(uid)", dialog_entry_builder)
        self.assertNotIn("QString::number(friendInfo->_uid)", dialog_entry_builder)
        self.assertIn('property string userId: momentData ? momentData.userId : ""', moments_delegate)
        self.assertIn("signal avatarClicked(int uid, string name, string icon, string userId)", moments_delegate)
        self.assertIn("root.avatarClicked(root.uid,", moments_delegate)
        self.assertIn("root.userId)", moments_delegate)
        self.assertIn("onAvatarClicked: function(uid, name, icon, userId)", moments_feed)
        self.assertIn("contactProfilePopup.openProfile(uid,", moments_feed)
        self.assertNotIn(
            "onAvatarClicked: {\n                                    contactProfilePopup.openProfile(model.uid",
            moments_feed,
        )
        self.assertIn("readonly property string identityText", moments_side)
        self.assertIn('? ("ID: " + userId)', moments_side)
        self.assertIn(': "ID: 未分配"', moments_side)
        self.assertNotIn('"UID: " + uid', moments_side)
        self.assertIn("text: momentFriendDelegate.identityText", moments_side)
        self.assertIn('text: userId && userId.length > 0 ? userId : "未分配"', create_group)
        self.assertNotIn('"UID: " + uid', create_group)
        self.assertIn('text: "ID: " + (userId && userId.length > 0 ? userId : "未分配")', find_friend)
        self.assertNotIn('"UID: " + uid', find_friend)

    def test_migrated_contact_qml_avoids_old_controller_contact_surface(self):
        qml_sources = {
            "ChatNormalFace.qml": read(CHAT_VIEW / "ChatNormalFace.qml"),
            "ChatShellContent.qml": read(CHAT_VIEW / "ChatShellContent.qml"),
            "ChatLeftPanel.qml": read(CHAT_VIEW / "ChatLeftPanel.qml"),
            "ChatShellPage.qml": read(SHELL_PAGE),
        }
        forbidden_tokens = (
            "controller.contactPane",
            "controller.currentContact",
            "controller.contactListModel",
            "controller.searchResultModel",
            "controller.applyRequestModel",
            "controller.searchPending",
            "controller.searchStatusText",
            "controller.searchStatusError",
            "controller.authStatusText",
            "controller.authStatusError",
            "controller.hasPendingApply",
            "controller.contactLoadingMore",
            "controller.canLoadMoreContacts",
            "controller.ensureContactsInitialized()",
            "controller.ensureApplyInitialized()",
            "controller.selectContactIndex(",
            "controller.searchUser(",
            "controller.clearSearchState()",
            "controller.requestAddFriend(",
            "controller.approveFriend(",
            "controller.deleteFriend(",
            "controller.contactProfileByUid(",
            "controller.showApplyRequests()",
            "controller.jumpChatWithCurrentContact()",
            "controller.loadMoreContacts()",
            "controller.clearAuthStatus()",
        )
        for file_name, source in qml_sources.items():
            compact_source = normalized(source)
            for token in forbidden_tokens:
                with self.subTest(file=file_name, token=token):
                    self.assertNotIn(token, compact_source)

    def test_appcontroller_prunes_legacy_contact_qml_surface_and_keeps_shell_targets(self):
        header = read(APP / "controller/AppController.h")

        legacy_qml_tokens = (
            "Q_INVOKABLE void ensureContactsInitialized();",
            "Q_INVOKABLE void ensureApplyInitialized();",
            "Q_INVOKABLE void selectContactIndex(int index);",
            "Q_INVOKABLE void deleteFriend(int uid);",
            "Q_INVOKABLE void showApplyRequests();",
            "Q_INVOKABLE void jumpChatWithCurrentContact();",
            "Q_INVOKABLE void loadMoreContacts();",
            "Q_INVOKABLE void searchUser(const QString& uidText);",
            "Q_INVOKABLE void clearSearchState();",
            "Q_INVOKABLE void requestAddFriend(int uid, const QString& bakName, const QVariantList& labels = QVariantList());",
            "Q_INVOKABLE void approveFriend(int uid, const QString& backName, const QVariantList& labels = QVariantList());",
            "Q_INVOKABLE void clearAuthStatus();",
            "Q_PROPERTY(FriendListModel* contactListModel READ contactListModel CONSTANT)",
            "Q_PROPERTY(SearchResultModel* searchResultModel READ searchResultModel CONSTANT)",
            "Q_PROPERTY(ApplyRequestModel* applyRequestModel READ applyRequestModel CONSTANT)",
            "FriendListModel* contactListModel();",
            "SearchResultModel* searchResultModel();",
            "ApplyRequestModel* applyRequestModel();",
        )
        for token in legacy_qml_tokens:
            with self.subTest(token=token):
                self.assertNotIn(token, header)

        shell_targets = (
            "void ensureContactsInitialized();",
            "void ensureApplyInitialized();",
            "void jumpChatWithCurrentContact();",
        )
        for token in shell_targets:
            with self.subTest(token=token):
                self.assertIn(token, header)
                self.assertIn(token, header[header.index("private:") :])
                self.assertNotIn(token, header[header.index("public:") : header.index("signals:")])

        removed_cpp_targets = (
            "void selectContactIndex(int index);",
            "void deleteFriend(int uid);",
            "void showApplyRequests();",
            "void loadMoreContacts();",
            "void clearSearchState();",
            "void clearAuthStatus();",
            "QVariantMap contactProfileByUid(int uid) const;",
        )
        for token in removed_cpp_targets:
            with self.subTest(removed_token=token):
                self.assertNotIn(token, header)


if __name__ == "__main__":
    unittest.main()
