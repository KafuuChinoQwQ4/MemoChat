import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CLIENT = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
CHAT = CLIENT / "features/chat"
APP = CLIENT / "app"
APP_FEATURE_REGISTRY_H = APP / "composition/AppFeatureRegistry.h"
QML = CLIENT / "features/chat/view"
LEGACY_QML = CLIENT / "qml"
CHAT_BINDING = APP / "controller/AppControllerChatFeatureBinding.cpp"
AGENT_CONTROLLER = CLIENT / "features/agent/controller"
CHAT_BINDING_FILES = (
    APP / "controller/AppControllerChatFeatureBinding.cpp",
    APP / "controller/AppChatProjectionBinding.cpp",
    APP / "controller/AppChatDialogBinding.cpp",
    APP / "controller/AppChatHistoryBinding.cpp",
    APP / "controller/AppChatGroupBinding.cpp",
    APP / "controller/AppChatMediaBinding.cpp",
    APP / "controller/AppChatReadMutationBinding.cpp",
    APP / "controller/AppChatSendBinding.cpp",
)


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def normalized(source: str) -> str:
    return " ".join(source.split())


def chat_binding_text() -> str:
    return "\n".join(read(path) for path in CHAT_BINDING_FILES)


class ChatQmlSurfaceContractTests(unittest.TestCase):
    def test_chat_viewmodel_exposes_chat_qml_surface(self):
        header = read(CHAT / "viewmodel/ChatViewModel.h")
        expected_tokens = (
            "Q_PROPERTY(int chatTab READ chatTab NOTIFY stateChanged)",
            "Q_PROPERTY(int currentDialogUid READ currentDialogUid NOTIFY stateChanged)",
            "Q_PROPERTY(QString currentChatPeerName READ currentChatPeerName NOTIFY stateChanged)",
            "Q_PROPERTY(QString currentChatPeerIcon READ currentChatPeerIcon NOTIFY stateChanged)",
            "Q_PROPERTY(bool hasCurrentChat READ hasCurrentChat NOTIFY stateChanged)",
            "Q_PROPERTY(bool hasCurrentGroup READ hasCurrentGroup NOTIFY stateChanged)",
            "Q_PROPERTY(int currentGroupRole READ currentGroupRole NOTIFY stateChanged)",
            "Q_PROPERTY(FriendListModel* dialogListModel READ dialogListModel NOTIFY modelChanged)",
            "Q_PROPERTY(ChatMessageModel* messageModel READ messageModel NOTIFY modelChanged)",
            "Q_PROPERTY(QString currentDraftText READ currentDraftText NOTIFY stateChanged)",
            "Q_PROPERTY(QVariantList currentPendingAttachments READ currentPendingAttachments NOTIFY stateChanged)",
            "Q_PROPERTY(bool hasPendingAttachments READ hasPendingAttachments NOTIFY stateChanged)",
            "Q_PROPERTY(bool currentDialogPinned READ currentDialogPinned NOTIFY stateChanged)",
            "Q_PROPERTY(bool currentDialogMuted READ currentDialogMuted NOTIFY stateChanged)",
            "Q_PROPERTY(bool hasPendingReply READ hasPendingReply NOTIFY stateChanged)",
            "Q_PROPERTY(QString replyTargetName READ replyTargetName NOTIFY stateChanged)",
            "Q_PROPERTY(QString replyPreviewText READ replyPreviewText NOTIFY stateChanged)",
            "Q_PROPERTY(bool dialogsReady READ dialogsReady NOTIFY stateChanged)",
            "Q_PROPERTY(bool chatLoadingMore READ chatLoadingMore NOTIFY stateChanged)",
            "Q_PROPERTY(bool canLoadMoreChats READ canLoadMoreChats NOTIFY stateChanged)",
            "Q_PROPERTY(bool privateHistoryLoading READ privateHistoryLoading NOTIFY stateChanged)",
            "Q_PROPERTY(bool canLoadMorePrivateHistory READ canLoadMorePrivateHistory NOTIFY stateChanged)",
            "Q_PROPERTY(bool mediaUploadInProgress READ mediaUploadInProgress NOTIFY stateChanged)",
            "Q_PROPERTY(QString mediaUploadProgressText READ mediaUploadProgressText NOTIFY stateChanged)",
            "Q_INVOKABLE void ensureChatListInitialized()",
            "Q_INVOKABLE void ensureGroupsInitialized()",
            "Q_INVOKABLE void selectDialogByUid(int uid)",
            "Q_INVOKABLE void selectChatIndex(int index)",
            "Q_INVOKABLE void selectGroupIndex(int index)",
            "Q_INVOKABLE void loadMoreChats()",
            "Q_INVOKABLE void loadMorePrivateHistory()",
            "Q_INVOKABLE void sendCurrentComposerPayload(const QString& text)",
            "Q_INVOKABLE void sendImageMessage()",
            "Q_INVOKABLE void sendFileMessage()",
            "Q_INVOKABLE void removePendingAttachment(const QString& attachmentId)",
            "Q_INVOKABLE void clearPendingAttachments()",
            "Q_INVOKABLE void updateCurrentDraft(const QString& text)",
            "Q_INVOKABLE void toggleCurrentDialogPinned()",
            "Q_INVOKABLE void toggleCurrentDialogMuted()",
            "Q_INVOKABLE void toggleDialogPinnedByUid(int dialogUid)",
            "Q_INVOKABLE void toggleDialogMutedByUid(int dialogUid)",
            "Q_INVOKABLE void markDialogReadByUid(int dialogUid)",
            "Q_INVOKABLE void clearDialogDraftByUid(int dialogUid)",
            "Q_INVOKABLE void beginReplyMessage(const QString& msgId,",
            "Q_INVOKABLE void cancelReplyMessage()",
            "Q_INVOKABLE void startVoiceChat()",
            "Q_INVOKABLE void startVideoChat()",
        )
        for token in expected_tokens:
            with self.subTest(token=token):
                self.assertIn(token, header)

    def test_appcontroller_keeps_chat_cpp_accessors_without_qml_metadata(self):
        header = read(APP / "controller/AppController.h")
        registry = read(APP_FEATURE_REGISTRY_H)
        composition = read(APP / "composition/AppComposition.cpp")
        controller = read(APP / "controller/AppController.cpp")
        public = header[header.index("public:") : header.index("signals:")]

        self.assertNotIn("Q_PROPERTY(ChatViewModel* chat READ chat CONSTANT)", header)
        self.assertNotIn("ChatViewModel* chat();", header)
        self.assertIn("ChatViewModel* chatViewModel();", public)
        self.assertIn("AppFeatureRegistry _features", header)
        self.assertIn("ChatViewModel chatViewModel;", registry)
        self.assertIn('context.setContextProperty("chat", chat())', composition)
        self.assertNotIn("ChatViewModel* AppController::chat()", controller)
        self.assertIn("ChatViewModel* AppController::chatViewModel()", controller)

    def test_chat_feature_controller_uses_purpose_specific_ports(self):
        header = read(CHAT / "controller/ChatFeatureController.h")
        source = read(CHAT / "controller/ChatFeatureController.cpp")
        app_controller = read(APP / "controller/AppController.cpp") + "\n" + chat_binding_text()
        combined = header + "\n" + source

        self.assertNotIn("ChatFeatureRequestHandlers", combined)
        self.assertNotIn("setHandlers(", combined)
        self.assertNotIn("_handlers.", combined)

        expected_ports = (
            "ChatViewProjectionPort",
            "ChatDialogNavigationPort",
            "ChatGroupConversationPort",
            "ChatPrivateCommandPort",
            "ChatCurrentSendPort",
            "ChatContentDispatchPort",
            "ChatCurrentHistoryPort",
            "ChatUploadedAttachmentDispatchPort",
            "ChatPendingAttachmentPort",
            "ChatDialogRuntimePort",
            "ChatShellIntentPort",
        )
        for port in expected_ports:
            with self.subTest(port=port):
                self.assertIn(f"struct {port}", header)
                self.assertIn(port, app_controller)

        expected_setters = (
            "setViewProjectionPort",
            "setDialogNavigationPort",
            "setGroupConversationPort",
            "setPrivateCommandPort",
            "setCurrentSendPort",
            "setContentDispatchPort",
            "setCurrentHistoryPort",
            "setUploadedAttachmentDispatchPort",
            "setPendingAttachmentPort",
            "setDialogRuntimePort",
            "setShellIntentPort",
        )
        for setter in expected_setters:
            with self.subTest(setter=setter):
                self.assertIn(setter, header)
                self.assertIn(setter, source)
                self.assertIn(setter, app_controller)

        stale_setters = (
            "setEnsureChatListInitializedHandler",
            "setSendCurrentComposerPayloadHandler",
            "setRemovePendingAttachmentHandler",
            "setUpdateCurrentDraftHandler",
            "setStartVoiceChatHandler",
        )
        for setter in stale_setters:
            with self.subTest(setter=setter):
                self.assertNotIn(setter, combined)
                self.assertNotIn(setter, app_controller)

    def test_chat_feature_owns_current_send_request_surface(self):
        header = read(CHAT / "controller/ChatFeatureController.h")
        private_send = read(CHAT / "controller/ChatFeatureControllerPrivateSend.cpp")
        group_send = read(CHAT / "controller/ChatFeatureControllerGroupConversation.cpp")

        expected_header_tokens = (
            "struct ChatCurrentSendSnapshot",
            "int selfUid = 0;",
            "int currentPrivatePeerUid = 0;",
            "GroupConversationContext context;",
            "qint64 currentGroupId = 0;",
            "struct ChatCurrentSendPort",
            "std::function<ChatCurrentSendSnapshot()> snapshot;",
            "std::function<PrivateChatSendDependencies()> privateDependencies;",
            "std::function<GroupConversationDependencies()> groupDependencies;",
            "void setCurrentSendPort(ChatCurrentSendPort port);",
            "struct ChatContentDispatchPort",
            "void setContentDispatchPort(ChatContentDispatchPort port);",
            "PrivateChatSendResult sendCurrentPrivateText(const QString& content, const QString& previewText);",
            "GroupSendResult sendCurrentGroupText(const QString& content, const QString& previewText);",
            "bool dispatchCurrentPrivateContent(const QString& content, const QString& previewText);",
            "bool dispatchCurrentGroupContent(const QString& content, const QString& previewText);",
        )
        for token in expected_header_tokens:
            with self.subTest(token=token):
                self.assertIn(token, header)

        self.assertIn("ChatFeatureController::sendCurrentPrivateText", private_send)
        self.assertIn("ChatFeatureController::dispatchCurrentPrivateContent", private_send)
        self.assertIn("sendCurrentPrivateText(content, previewText)", private_send)
        self.assertIn("PrivateChatSendRequest request;", private_send)
        self.assertIn("request.peerUid = snapshot.currentPrivatePeerUid;", private_send)
        self.assertIn("dependencies.selfUid = snapshot.selfUid;", private_send)
        self.assertIn("return sendPrivateText(request, dependencies);", private_send)
        self.assertIn("ChatFeatureController::sendCurrentGroupText", group_send)
        self.assertIn("ChatFeatureController::dispatchCurrentGroupContent", group_send)
        self.assertIn("sendCurrentGroupText(content, previewText)", group_send)
        self.assertIn("GroupSendRequest request;", group_send)
        self.assertIn("request.context = snapshot.context;", group_send)
        self.assertIn("request.groupId = snapshot.currentGroupId;", group_send)
        self.assertIn("request.replyToMsgId = replyToMsgId();", group_send)
        self.assertIn("return sendGroupText(request, dependencies);", group_send)

        compact_header = normalized(header)
        self.assertNotIn("ChatCurrentPrivateSendCommand", compact_header)
        self.assertNotIn("ChatCurrentGroupSendCommand", compact_header)

    def test_chat_feature_owns_uploaded_attachment_dispatch_surface(self):
        header = read(CHAT / "controller/ChatFeatureController.h")
        source = read(CHAT / "controller/ChatFeatureController.cpp")
        pending_send = read(CHAT / "controller/ChatFeatureControllerPendingSend.cpp")
        app_controller = read(APP / "controller/AppController.cpp") + "\n" + chat_binding_text()
        upload_queue = read(APP / "controller/AppControllerMediaUploadQueue.cpp")

        expected_header_tokens = (
            "struct ChatUploadedAttachmentDispatchPort",
            "std::function<bool(const QString&, const QString&)> dispatchCurrentPrivateContent;",
            "std::function<bool(const QString&, const QString&)> dispatchCurrentGroupContent;",
            "std::function<bool(const OutgoingChatPacket&, QString*)> dispatchPrivatePacket;",
            "std::function<void(int, const QByteArray&)> dispatchGroupPayload;",
            "void setUploadedAttachmentDispatchPort(ChatUploadedAttachmentDispatchPort port);",
            "UploadedAttachmentDispatchResult dispatchUploadedAttachment(const QVariantMap& attachment,",
        )
        for token in expected_header_tokens:
            with self.subTest(token=token):
                self.assertIn(token, header)

        self.assertIn("ChatFeatureController::setUploadedAttachmentDispatchPort", source)
        self.assertIn("ChatFeatureController::dispatchUploadedAttachment", pending_send)
        self.assertIn("UploadedAttachmentDispatchPort port;", pending_send)
        self.assertIn(
            "UploadedAttachmentDispatchService::dispatch(attachment, uploaded, destination, port)", pending_send
        )
        self.assertIn("void AppController::bindChatFeatureSendPorts()", app_controller)
        self.assertIn("ChatUploadedAttachmentDispatchPort attachmentDispatchPort;", app_controller)
        self.assertIn(
            "_features.chatFeatureController.setUploadedAttachmentDispatchPort(std::move(attachmentDispatchPort));",
            app_controller,
        )
        self.assertIn(
            "_features.chatFeatureController.dispatchUploadedAttachment(attachment, uploaded, destination)",
            upload_queue,
        )
        self.assertNotIn("UploadedAttachmentDispatchPort", upload_queue)
        self.assertNotIn("dispatchPrivatePacket", upload_queue)
        self.assertNotIn("dispatchGroupPayload", upload_queue)

    def test_chat_shell_content_uses_chat_surface_for_conversation(self):
        shell = read(QML / "ChatShellContent.qml")
        expected_chat_tokens = (
            "peerName: chat.currentChatPeerName",
            "peerAvatar: chat.currentChatPeerIcon",
            "hasCurrentChat: chat.hasCurrentChat",
            "isGroupChat: chat.hasCurrentGroup",
            "messageModel: chat.messageModel",
            "currentDraftText: chat.currentDraftText",
            "currentPendingAttachments: chat.currentPendingAttachments",
            "mediaUploadInProgress: chat.mediaUploadInProgress",
            "mediaUploadProgressText: chat.mediaUploadProgressText",
            "onSendComposer: function(text) { chat.sendCurrentComposerPayload(text) }",
            "onSendImage: chat.sendImageMessage()",
            "onSendFile: chat.sendFileMessage()",
            "onRemovePendingAttachment: function(attachmentId) { chat.removePendingAttachment(attachmentId) }",
            "onDraftEdited: function(text) { chat.updateCurrentDraft(text) }",
            "onRequestLoadMoreHistory: chat.loadMorePrivateHistory()",
        )
        for token in expected_chat_tokens:
            with self.subTest(token=token):
                self.assertIn(token, shell)

        for cross_feature_token in (
            "AgentPane {",
            "agentController: agent",
            "messageModel: agentMessages",
            "MomentsFeedPane {",
            "momentsController: root.momentsControllerContext",
            "petController: pet",
            "friendModel: contact.contactListModel",
        ):
            with self.subTest(cross_feature_token=cross_feature_token):
                self.assertIn(cross_feature_token, shell)

    def test_chat_left_panel_uses_passed_chat_surface_for_session_bootstrap(self):
        normal_face = read(QML / "ChatNormalFace.qml")
        left_panel = read(QML / "ChatLeftPanel.qml")

        self.assertIn("property var chatViewModel: chat", normal_face)
        self.assertIn("chatViewModel: root.chatViewModel", normal_face)
        self.assertIn("property var chatViewModel: null", left_panel)
        self.assertIn("root.chatViewModel.ensureChatListInitialized()", left_panel)
        self.assertIn("root.chatViewModel.ensureGroupsInitialized()", left_panel)

    def test_live2d_entry_card_is_account_bound_and_has_empty_state(self):
        normal_face = read(QML / "ChatNormalFace.qml")
        left_panel = read(QML / "ChatLeftPanel.qml")
        entry = read(QML / "ChatLive2DEntryPane.qml")

        for token in (
            "property int currentUserUid: 0",
            'property string currentUserId: ""',
            "live2dPetSettings.bindAccount(root.currentUserUid, root.currentUserId)",
            "live2dPetSettings.load()",
            "root.live2dCharacterName",
            "root.live2dModelPath",
            "root.live2dModelImported",
            "characterName: root.live2dCharacterName",
            "modelPath: root.live2dModelPath",
            "hasImportedModel: root.live2dModelImported",
        ):
            with self.subTest(token=token):
                self.assertIn(token, left_panel)

        self.assertIn("currentUserUid: shell.currentUserUid", normal_face)
        self.assertIn("currentUserId: shell.currentUserId", normal_face)

        for token in (
            "property string characterName",
            "property string modelPath",
            "property bool hasImportedModel",
            "未导入角色",
            "等待导入 Live2D model3.json",
        ):
            with self.subTest(entry_token=token):
                self.assertIn(token, entry)

        for stale_token in ("Kafuu Chino", "resources/live2d/KafuuChino", "香风智乃"):
            with self.subTest(stale_token=stale_token):
                self.assertNotIn(stale_token, left_panel + "\n" + entry)

    def test_agent_side_pane_can_rename_sessions_without_model_subtitle(self):
        side_pane = read(QML / "ChatAgentSidePane.qml")
        left_panel = read(QML / "ChatLeftPanel.qml")
        normal_face = read(QML / "ChatNormalFace.qml")
        controller_header = read(AGENT_CONTROLLER / "AgentController.h")
        controller_sessions = read(AGENT_CONTROLLER / "AgentControllerSessions.cpp")

        for token in (
            "signal sessionRenamed(string sessionId, string title)",
            "function openRenameDialog(entry)",
            "MenuItem {",
            'text: "重命名"',
            "agentRenameDialog.open()",
            "root.sessionRenamed(sessionId, title)",
        ):
            with self.subTest(side_token=token):
                self.assertIn(token, side_pane)

        self.assertNotIn('"subtitle": session.model_name || currentModelName || "AI 会话"', side_pane)
        self.assertNotIn("session.model_name || currentModelName", side_pane)

        self.assertIn("signal agentSessionRenamed(string sessionId, string title)", left_panel)
        self.assertIn(
            "onSessionRenamed: function(sessionId, title) { root.agentSessionRenamed(sessionId, title) }", left_panel
        )
        self.assertIn("onAgentSessionRenamed: function(sessionId, title) {", normal_face)
        self.assertIn("agent.renameSession(sessionId, title)", normal_face)
        self.assertIn(
            "Q_INVOKABLE void renameSession(const QString& sessionId, const QString& title);", controller_header
        )
        self.assertIn(
            "void AgentController::renameSession(const QString& sessionId, const QString& title)", controller_sessions
        )


if __name__ == "__main__":
    unittest.main()
