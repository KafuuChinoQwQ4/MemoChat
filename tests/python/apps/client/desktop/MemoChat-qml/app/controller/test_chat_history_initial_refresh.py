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
        source = APP_CONTROLLER_GROUP_SELECTION.read_text(encoding="utf-8")
        body = extract_function(source, "void AppController::selectGroupIndex(int index)")

        initial_load = body[: body.rindex("loadGroupHistory();")]
        cursor_writes = re.findall(r"_group_state\.historyBeforeSeq\s*=", initial_load)
        self.assertEqual(
            4,
            len(cursor_writes),
            "Group selection should only reset the history cursor before initial latest-page load",
        )
        self.assertNotIn("one->_group_seq < _group_history_before_seq", initial_load)
        self.assertIn("loadGroupHistory();", body)


if __name__ == "__main__":
    unittest.main()
