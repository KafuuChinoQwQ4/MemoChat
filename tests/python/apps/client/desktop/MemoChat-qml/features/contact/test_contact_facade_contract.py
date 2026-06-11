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
            "Q_INVOKABLE void deleteFriend(int uid);",
            "Q_INVOKABLE void showApplyRequests();",
            "Q_INVOKABLE void jumpChatWithCurrentContact();",
            "Q_INVOKABLE void loadMoreContacts();",
            "Q_INVOKABLE void clearAuthStatus();",
            "void syncModels(FriendListModel* contactListModel,",
            "void syncCurrentContact(int uid,",
            "void setCommandPort(ContactCommandPort port);",
            "struct ContactCommandPort",
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

        self.assertIn("std::function<void()> refreshApplySnapshot;", relation_port)
        self.assertNotIn("refreshApplyModel", relation_port)
        self.assertNotIn("setApplyReady", relation_port)
        self.assertIn("_port.refreshApplySnapshot();", relation_updated)
        self.assertNotIn("_port.refreshApplyModel();", relation_updated)
        self.assertNotIn("_port.setApplyReady(true);", relation_updated)
        self.assertIn("void refreshApplySnapshot();", contact_header)
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
