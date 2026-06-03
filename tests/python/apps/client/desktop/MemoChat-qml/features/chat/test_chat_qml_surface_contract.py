import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CLIENT = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
CHAT = CLIENT / "features/chat"
APP = CLIENT / "app"
QML = CLIENT / "features/chat/view"
LEGACY_QML = CLIENT / "qml"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


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
        engine = read(APP / "bootstrap/MainQmlEngineSetup.cpp")
        controller = read(APP / "controller/AppController.cpp")

        self.assertNotIn("Q_PROPERTY(ChatViewModel* chat READ chat CONSTANT)", header)
        self.assertIn("ChatViewModel* chat();", header)
        self.assertIn("ChatViewModel* chatViewModel();", header)
        self.assertIn("ChatViewModel _chat_view_model;", header)
        self.assertIn('setContextProperty("chat", controller.chatViewModel())', engine)
        self.assertIn("ChatViewModel* AppController::chat()", controller)
        self.assertIn("ChatViewModel* AppController::chatViewModel()", controller)

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
            "onSendComposer: function(text) { chat.sendCurrentComposerPayload(text) }",
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


if __name__ == "__main__":
    unittest.main()
