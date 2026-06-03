import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CLIENT = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
CONTACT = CLIENT / "features/contact"
APP = CLIENT / "app"
CHAT_VIEW = CLIENT / "features/chat/view"
SHELL_PAGE = CLIENT / "qml/app/ChatShellPage.qml"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def normalized(source: str) -> str:
    return " ".join(source.split())


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
            "void ensureContactsInitializedRequested();",
            "void searchUserRequested(const QString& uidText);",
            "void requestAddFriendRequested(int uid, const QString& bakName, const QVariantList& labels);",
            "void approveFriendRequested(int uid, const QString& backName, const QVariantList& labels);",
            "void deleteFriendRequested(int uid);",
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
        )
        for token in helper_tokens:
            with self.subTest(token=token):
                self.assertIn(token, header)

    def test_appcontroller_exposes_additive_contact_surface(self):
        header = read(APP / "controller/AppController.h")
        controller = read(APP / "controller/AppController.cpp")
        model_props = read(APP / "controller/AppControllerModelProperties.cpp")
        engine = read(APP / "bootstrap/MainQmlEngineSetup.cpp")

        self.assertNotIn("Q_PROPERTY(ContactController* contact READ contact CONSTANT)", header)
        self.assertIn("ContactController* contact();", header)
        self.assertIn("ContactController* contactController();", header)
        self.assertIn("ContactController _contact_controller;", header)
        self.assertIn("void syncContactControllerState();", header)

        self.assertIn("ContactController* AppController::contact()", model_props)
        self.assertIn("ContactController* AppController::contactController()", model_props)
        self.assertIn("return &_contact_controller;", model_props)

        self.assertIn(
            "connect(&_contact_controller, &ContactController::searchUserRequested, this, &AppController::searchUser);",
            controller,
        )
        self.assertIn(
            "connect(&_contact_controller, &ContactController::approveFriendRequested, this, &AppController::approveFriend);",
            controller,
        )
        self.assertIn("void AppController::syncContactControllerState()", controller)
        self.assertIn('setContextProperty("contact", controller.contactController())', engine)

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

    def test_appcontroller_prunes_legacy_contact_qml_surface_but_keeps_cpp_targets(self):
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
        )
        for token in legacy_qml_tokens:
            with self.subTest(token=token):
                self.assertNotIn(token, header)

        cpp_targets = (
            "void ensureContactsInitialized();",
            "void ensureApplyInitialized();",
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
            "FriendListModel* contactListModel();",
            "SearchResultModel* searchResultModel();",
            "ApplyRequestModel* applyRequestModel();",
        )
        for token in cpp_targets:
            with self.subTest(token=token):
                self.assertIn(token, header)


if __name__ == "__main__":
    unittest.main()
