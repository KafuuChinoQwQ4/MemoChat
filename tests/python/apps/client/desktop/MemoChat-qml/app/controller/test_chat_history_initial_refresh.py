import re
import unittest

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
APP_CONTROLLER_PRIVATE_HISTORY = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/controller/AppControllerPrivateHistory.cpp"
)
APP_CONTROLLER_GROUP_SELECTION = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/controller/AppControllerGroupSelection.cpp"
)
CHAT_DIALOG_SELECTION_SERVICE = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/chat/services/ChatDialogSelectionService.cpp"
)
CHAT_FEATURE_GROUP = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/chat/controller/ChatFeatureControllerGroupConversation.cpp"
)
GROUP_CONVERSATION_SERVICE = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/chat/services/GroupConversationService.cpp"
)
CHAT_SERVICE_DIR = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/chat/services"


def group_conversation_service_source() -> str:
    files = (
        "GroupConversationServiceInternal.h",
        "GroupConversationService.cpp",
        "GroupConversationAckService.cpp",
        "GroupConversationHistoryService.cpp",
        "GroupConversationIncomingService.cpp",
        "GroupConversationMutationService.cpp",
    )
    return "\n".join((CHAT_SERVICE_DIR / name).read_text(encoding="utf-8") for name in files)


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


class ChatHistoryInitialRefreshTests(unittest.TestCase):
    def test_private_chat_selection_requests_latest_history_page(self):
        source = APP_CONTROLLER_PRIVATE_HISTORY.read_text(encoding="utf-8")
        body = extract_function(source, "void AppController::loadCurrentChatMessages()")

        self.assertIn("requestPrivateHistory(_chat_state.uid, 0, QString());", body)
        self.assertNotIn(
            "requestPrivateHistory(_chat_state.uid, _private_history_before_ts, _private_history_before_msg_id);",
            body,
        )

    def test_group_chat_selection_requests_latest_history_page_before_pagination_cursor(self):
        adapter = extract_function(
            APP_CONTROLLER_GROUP_SELECTION.read_text(encoding="utf-8"), "void AppController::selectGroupIndex"
        )
        source = CHAT_DIALOG_SELECTION_SERVICE.read_text(encoding="utf-8")
        body = extract_function(source, "void ChatDialogSelectionService::selectGroupByIndex")
        feature = CHAT_FEATURE_GROUP.read_text(encoding="utf-8")
        service = group_conversation_service_source()

        initial_load = body[: body.rindex("openCurrentGroupConversation(groupId, port);")]
        self.assertIn("port.resetGroupConversationState();", initial_load)
        self.assertIn("openCurrentGroupConversation(groupId, port);", body)
        self.assertIn("_features.chatFeatureController.selectGroupByIndex(index);", adapter)
        self.assertIn("dependencies.state = &controller.groupConversationState();", feature)
        self.assertIn("fillFeatureOwnedGroupConversationDependencies(*this, dependencies);", feature)
        cursor_writes = re.findall(r"dependencies\.state->historyBeforeSeq\s*=", service)
        self.assertGreaterEqual(len(cursor_writes), 2)
        self.assertIn("dependencies.state->historyBeforeSeq = 0;", service)
        self.assertNotIn("one->_group_seq < _group_history_before_seq", initial_load)
        self.assertIn("port.loadGroupHistory();", source)


if __name__ == "__main__":
    unittest.main()
