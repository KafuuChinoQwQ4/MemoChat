import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
GROUP_MESSAGE_SERVICE = REPO_ROOT / "apps/server/core/ChatServer/GroupMessageService.cpp"
PRIVATE_MESSAGE_SERVICE = REPO_ROOT / "apps/server/core/ChatServer/PrivateMessageService.cpp"
CHAT_MESSAGE_DISPATCHER = REPO_ROOT / "apps/client/desktop/MemoChatShared/ChatMessageDispatcher.cpp"
CHAT_MESSAGE_MODEL = REPO_ROOT / "apps/client/desktop/MemoChat-qml/ChatMessageModel.cpp"
APP_CONTROLLER = REPO_ROOT / "apps/client/desktop/MemoChat-qml/AppController.cpp"
APP_CONTROLLER_PRIVATE_EVENTS = REPO_ROOT / "apps/client/desktop/MemoChat-qml/AppControllerPrivateEvents.cpp"
APP_CONTROLLER_STATE = REPO_ROOT / "apps/client/desktop/MemoChat-qml/AppControllerState.cpp"
CONVERSATION_SYNC_SERVICE = REPO_ROOT / "apps/client/desktop/MemoChat-qml/ConversationSyncService.cpp"


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
                return source[start:index + 1]
    raise AssertionError(f"Function body not found for {signature}")


class GroupResponseSerializationTests(unittest.TestCase):
    def test_group_responses_use_object_json_not_double_serialized_strings(self):
        source = GROUP_MESSAGE_SERVICE.read_text(encoding="utf-8")

        self.assertNotIn("session->Send(rtvalue.and_then", source)
        self.assertNotIn(
            "session->Send(rtvalue.and_then([](auto&& v){ return glz::write_json(v); }).value_or(\"{}\")",
            source,
        )
        self.assertIn("JsonToWireString(rtvalue)", source)

    def test_group_list_response_initializes_arrays_before_appending(self):
        source = GROUP_MESSAGE_SERVICE.read_text(encoding="utf-8")

        self.assertIn('out["group_list"] = memochat::json::arrayValue;', source)
        self.assertIn('out["pending_group_apply_list"] = memochat::json::arrayValue;', source)
        self.assertLess(
            source.index('out["group_list"] = memochat::json::arrayValue;'),
            source.index('out["group_list"].append(one);'),
        )
        self.assertLess(
            source.index('out["pending_group_apply_list"] = memochat::json::arrayValue;'),
            source.index('out["pending_group_apply_list"].append(one);'),
        )
        self.assertNotIn('append(out["group_list"], one);', source)
        self.assertNotIn('append(out["pending_group_apply_list"], one);', source)

    def test_group_history_response_initializes_messages_array(self):
        source = GROUP_MESSAGE_SERVICE.read_text(encoding="utf-8")

        handler_start = source.index("void GroupMessageService::HandleGroupHistory")
        handler_end = source.index("void GroupMessageService::HandleEditGroupMessage", handler_start)
        handler = source[handler_start:handler_end]

        self.assertIn('rtvalue["messages"] = memochat::json::arrayValue;', handler)
        self.assertLess(
            handler.index('rtvalue["messages"] = memochat::json::arrayValue;'),
            handler.index('append(rtvalue["messages"], item);'),
        )

    def test_private_history_response_initializes_messages_array_and_uses_wire_json(self):
        source = PRIVATE_MESSAGE_SERVICE.read_text(encoding="utf-8")

        handler = extract_function(source, "void PrivateMessageService::HandlePrivateHistory")

        self.assertIn('rtvalue["messages"] = memochat::json::arrayValue;', handler)
        self.assertLess(
            handler.index('rtvalue["messages"] = memochat::json::arrayValue;'),
            handler.index('append(rtvalue["messages"], item);'),
        )
        self.assertIn("JsonToWireString(rtvalue)", handler)
        self.assertNotIn("session->Send(rtvalue.and_then", handler)

    def test_group_list_is_stored_before_group_response_signal(self):
        source = CHAT_MESSAGE_DISPATCHER.read_text(encoding="utf-8")

        create_start = source.index("_handlers.insert(ID_CREATE_GROUP_RSP")
        create_end = source.index("_handlers.insert(ID_GET_GROUP_LIST_RSP", create_start)
        create_handler = source[create_start:create_end]
        self.assertLess(
            create_handler.index("UserMgr::GetInstance()->SetGroupList"),
            create_handler.index("emit sig_group_rsp"),
        )
        self.assertLess(
            create_handler.index("UserMgr::GetInstance()->SetGroupList"),
            create_handler.index("emit sig_group_list_updated"),
        )

    def test_group_dialog_uid_mapping_uses_shared_resolver(self):
        controller = APP_CONTROLLER.read_text(encoding="utf-8")
        private_events = APP_CONTROLLER_PRIVATE_EVENTS.read_text(encoding="utf-8")
        state = APP_CONTROLLER_STATE.read_text(encoding="utf-8")
        sync = CONVERSATION_SYNC_SERVICE.read_text(encoding="utf-8")

        self.assertIn("makeGroupDialogUid", sync)
        self.assertIn("groupIdForDialogUid", sync)
        self.assertNotIn("-static_cast<int>(group->_group_id)", controller)
        self.assertNotIn("-static_cast<int>(groupId)", private_events)
        self.assertNotIn("-static_cast<int>(_current_group_id)", state)
        self.assertIn("ConversationSyncService::makeGroupDialogUid(_current_group_id)", state)

    def test_group_message_sender_icons_are_normalized_before_qml(self):
        source = CHAT_MESSAGE_MODEL.read_text(encoding="utf-8")

        self.assertIn('#include "IconPathUtils.h"', source)
        self.assertIn("QString ChatMessageModel::normalizeSenderIcon(const QString &icon) const", source)

        normalizer = extract_function(
            source,
            "QString ChatMessageModel::normalizeSenderIcon(const QString &icon) const",
        )
        self.assertIn("const QString trimmed = icon.trimmed();", normalizer)
        self.assertIn("if (trimmed.isEmpty())", normalizer)
        self.assertIn("return QString();", normalizer)
        self.assertIn("return withDownloadAuth(normalizeIconForQml(trimmed));", normalizer)

        to_entry = extract_function(
            source,
            "ChatMessageModel::MessageEntry ChatMessageModel::toEntry",
        )
        self.assertIn("entry.senderIcon = normalizeSenderIcon(message->_from_icon);", to_entry)
        self.assertNotIn("entry.senderIcon = message->_from_icon;", to_entry)

        set_auth_context = extract_function(
            source,
            "void ChatMessageModel::setDownloadAuthContext",
        )
        self.assertIn("entry.senderIcon = normalizeSenderIcon(entry.senderIcon);", set_auth_context)


if __name__ == "__main__":
    unittest.main()
