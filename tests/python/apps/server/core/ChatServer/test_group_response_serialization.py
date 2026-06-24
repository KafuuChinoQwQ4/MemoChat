import unittest

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
GROUP_MESSAGE_SERVICE = REPO_ROOT / "apps/server/core/ChatServer/domain/message/GroupMessageService.cpp"
PRIVATE_MESSAGE_SERVICE = REPO_ROOT / "apps/server/core/ChatServer/domain/message/PrivateMessageService.cpp"
CHAT_MESSAGE_DISPATCHER_GROUP = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/core/network/ChatMessageDispatcherGroup.cpp"
)
CHAT_MESSAGE_MODEL = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModel.cpp"
CHAT_MESSAGE_MODEL_CONTENT = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/chat/model/ChatMessageModelContent.cpp"
)
ICON_PATH_UTILS = REPO_ROOT / "apps/client/desktop/MemoChat-qml/shared/utils/IconPathUtils.h"
APP_CONTROLLER_GROUP_SELECTION = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/controller/AppControllerGroupSelection.cpp"
)
APP_CONTROLLER_PRIVATE_EVENTS = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/controller/AppControllerPrivateEvents.cpp"
)
APP_CONTROLLER_DIALOG_STATE = REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/controller/AppControllerDialogState.cpp"
CONVERSATION_SYNC_SERVICE = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/chat/services/ConversationSyncService.cpp"
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


def normalized(source: str) -> str:
    return " ".join(source.split())


class GroupResponseSerializationTests(unittest.TestCase):
    def test_group_responses_use_object_json_not_double_serialized_strings(self):
        source = GROUP_MESSAGE_SERVICE.read_text(encoding="utf-8")

        self.assertNotIn("session->Send(rtvalue.and_then", source)
        self.assertNotIn(
            'session->Send(rtvalue.and_then([](auto&& v){ return glz::write_json(v); }).value_or("{}")',
            source,
        )
        self.assertIn("JsonToWireString(rtvalue)", source)

    def test_group_list_response_initializes_arrays_before_appending(self):
        source = GROUP_MESSAGE_SERVICE.read_text(encoding="utf-8")

        self.assertIn('out["group_list"] = memochat::json::arrayValue;', source)
        self.assertIn('out["pending_group_apply_list"] = memochat::json::arrayValue;', source)
        self.assertLess(
            source.index('out["group_list"] = memochat::json::arrayValue;'),
            source.index('out["group_list"].append(ChatOutput::ToJsonValue(row));'),
        )
        self.assertLess(
            source.index('out["pending_group_apply_list"] = memochat::json::arrayValue;'),
            source.index('out["pending_group_apply_list"].append(ChatOutput::ToJsonValue(row));'),
        )

    def test_group_history_response_initializes_messages_array(self):
        source = GROUP_MESSAGE_SERVICE.read_text(encoding="utf-8")

        # The transport HandleGroupHistory now delegates to the GroupHistory command method,
        # where the response is actually built. Verify the invariant where it lives.
        handler = extract_function(source, "MessageCommandResult GroupMessageService::GroupHistory")

        self.assertIn('rtvalue["messages"] = memochat::json::arrayValue;', handler)
        self.assertLess(
            handler.index('rtvalue["messages"] = memochat::json::arrayValue;'),
            handler.index('append(rtvalue["messages"], item);'),
        )

    def test_private_history_response_initializes_messages_array_and_uses_wire_json(self):
        source = PRIVATE_MESSAGE_SERVICE.read_text(encoding="utf-8")

        # HandlePrivateHistory delegates to the PrivateHistory command method; the response
        # (array init + wire-json serialization) is built there.
        handler = extract_function(source, "MessageCommandResult PrivateMessageService::PrivateHistory")

        self.assertIn('rtvalue["messages"] = memochat::json::arrayValue;', handler)
        self.assertLess(
            handler.index('rtvalue["messages"] = memochat::json::arrayValue;'),
            handler.index('append(rtvalue["messages"], item);'),
        )
        self.assertIn("JsonToWireString(rtvalue)", handler)
        self.assertNotIn("session->Send(rtvalue.and_then", handler)

    def test_private_message_payloads_carry_public_user_id(self):
        private_source = PRIVATE_MESSAGE_SERVICE.read_text(encoding="utf-8")
        command_dtos = (REPO_ROOT / "apps/server/core/ChatServer/domain/ChatMessageCommandDtos.h").read_text(
            encoding="utf-8"
        )
        history_dtos = (REPO_ROOT / "apps/server/core/ChatServer/domain/ChatHistoryOutputDtos.h").read_text(
            encoding="utf-8"
        )
        async_dispatcher = (REPO_ROOT / "apps/server/core/ChatServer/messaging/AsyncEventDispatcher.cpp").read_text(
            encoding="utf-8"
        )
        grpc_notify = (REPO_ROOT / "apps/server/core/ChatServer/transport/ChatServiceImpl.cpp").read_text(
            encoding="utf-8"
        )
        session_source = (REPO_ROOT / "apps/server/core/ChatServer/domain/session/ChatSessionService.cpp").read_text(
            encoding="utf-8"
        )

        self.assertIn("std::optional<std::string> from_user_id;", command_dtos)
        self.assertIn('value["from_user_id"] = *dto.from_user_id;', command_dtos)
        self.assertIn("std::string from_user_id;", history_dtos)
        self.assertIn('item["from_user_id"] = dto.from_user_id;', history_dtos)
        self.assertIn('root["from_user_id"] = dto.from_user_id;', history_dtos)

        send_handler = extract_function(private_source, "MessageCommandResult PrivateMessageService::TextChatMessage")
        self.assertIn("ResolveSenderPublicUserId(uid, _relation_repository)", send_handler)
        self.assertIn("msg.from_user_id = sender_public_user_id;", send_handler)
        self.assertIn("rtdto.from_user_id = sender_public_user_id;", send_handler)
        self.assertIn('event_payload["from_user_id"] = sender_public_user_id;', send_handler)

        history_handler = extract_function(private_source, "MessageCommandResult PrivateMessageService::PrivateHistory")
        self.assertIn("ResolveSenderPublicUserId(one->from_uid, _relation_repository)", history_handler)
        self.assertIn("one->from_user_id = cached_user_id->second;", history_handler)

        self.assertIn('notify_payload["from_user_id"] = sender_public_user_id;', async_dispatcher)
        self.assertIn('one["from_user_id"] = sender_public_user_id;', async_dispatcher)
        self.assertIn('rtvalue["from_user_id"] = sender_info->user_id;', grpc_notify)
        self.assertIn('element["from_user_id"] = sender_info->user_id;', grpc_notify)
        self.assertIn("msg->from_user_id = sender_public_user_id;", session_source)
        self.assertIn(".from_user_id = sender_public_user_id", normalized(session_source))

    def test_group_list_is_stored_before_group_response_signal(self):
        source = CHAT_MESSAGE_DISPATCHER_GROUP.read_text(encoding="utf-8")

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
        controller = APP_CONTROLLER_GROUP_SELECTION.read_text(encoding="utf-8")
        private_events = APP_CONTROLLER_PRIVATE_EVENTS.read_text(encoding="utf-8")
        state = APP_CONTROLLER_DIALOG_STATE.read_text(encoding="utf-8")
        sync = CONVERSATION_SYNC_SERVICE.read_text(encoding="utf-8")

        self.assertIn("makeGroupDialogUid", sync)
        self.assertIn("groupIdForDialogUid", sync)
        self.assertNotIn("-static_cast<int>(group->_group_id)", controller)
        self.assertNotIn("-static_cast<int>(groupId)", private_events)
        self.assertNotIn("-static_cast<int>(_current_group_id)", state)
        self.assertIn("ConversationSyncService::makeGroupDialogUid(currentGroupId())", state)

    def test_group_message_sender_icons_are_normalized_before_qml(self):
        source = CHAT_MESSAGE_MODEL_CONTENT.read_text(encoding="utf-8")

        self.assertIn('#include "IconPathUtils.h"', source)
        self.assertRegex(
            source,
            r"QString\s+ChatMessageModel::normalizeSenderIcon\s*\(\s*const\s+QString\s*&\s*icon\s*\)\s*const",
        )

        normalizer = extract_function(
            source,
            "QString ChatMessageModel::normalizeSenderIcon",
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

    def test_local_media_download_urls_use_stable_gate_http_port(self):
        source = ICON_PATH_UTILS.read_text(encoding="utf-8")

        media_base = extract_function(source, "inline QString mediaDownloadBaseUrl")
        self.assertIn("gate_media_url_prefix.trimmed()", media_base)
        self.assertIn("gate_url_prefix.trimmed()", media_base)
        self.assertNotIn("parsed.setPort(8080)", media_base)
        self.assertNotIn("port == 80 || port == 8443", media_base)

        normalizer = extract_function(source, "inline QString normalizeLocalMediaDownloadUrl")
        self.assertIn("normalized.setScheme(parsedBase.scheme())", normalizer)
        self.assertIn("normalized.setHost(parsedBase.host())", normalizer)
        self.assertIn("normalized.setPort(parsedBase.port(-1))", normalizer)


if __name__ == "__main__":
    unittest.main()
