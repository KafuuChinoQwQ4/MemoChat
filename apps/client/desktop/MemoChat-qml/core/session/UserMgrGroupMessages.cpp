#include "usermgr.h"

#include <algorithm>

namespace
{
void sortGroupMessages(std::vector<std::shared_ptr<TextChatData>>& chatMsgs)
{
    std::sort(chatMsgs.begin(),
              chatMsgs.end(),
              [](const std::shared_ptr<TextChatData>& lhs, const std::shared_ptr<TextChatData>& rhs)
              {
                  if (!lhs || !rhs)
                  {
                      return static_cast<bool>(lhs);
                  }
                  if (lhs->_group_seq > 0 && rhs->_group_seq > 0 && lhs->_group_seq != rhs->_group_seq)
                  {
                      return lhs->_group_seq < rhs->_group_seq;
                  }
                  if (lhs->_created_at != rhs->_created_at)
                  {
                      return lhs->_created_at < rhs->_created_at;
                  }
                  if (lhs->_server_msg_id > 0 && rhs->_server_msg_id > 0 && lhs->_server_msg_id != rhs->_server_msg_id)
                  {
                      return lhs->_server_msg_id < rhs->_server_msg_id;
                  }
                  return lhs->_msg_id < rhs->_msg_id;
              });
}
} // namespace

void UserMgr::AppendGroupChatMsg(qint64 group_id, const std::shared_ptr<TextChatData>& msg)
{
    auto iter = _group_map.find(group_id);
    if (iter == _group_map.end() || !iter.value() || !msg)
    {
        return;
    }
    auto& chatMsgs = iter.value()->_chat_msgs;
    for (const auto& one : chatMsgs)
    {
        if (one && one->_msg_id == msg->_msg_id)
        {
            return;
        }
    }
    chatMsgs.push_back(msg);
    sortGroupMessages(chatMsgs);
    iter.value()->_last_msg = msg->_msg_content;
}

void UserMgr::UpsertGroupChatMsg(qint64 group_id, const std::shared_ptr<TextChatData>& msg)
{
    auto iter = _group_map.find(group_id);
    if (iter == _group_map.end() || !iter.value() || !msg)
    {
        return;
    }

    auto& chatMsgs = iter.value()->_chat_msgs;
    bool found = false;
    for (auto& one : chatMsgs)
    {
        if (!one || one->_msg_id != msg->_msg_id)
        {
            continue;
        }
        one->_msg_content = msg->_msg_content;
        one->_from_uid = msg->_from_uid;
        one->_to_uid = msg->_to_uid;
        one->_from_name = msg->_from_name;
        one->_from_icon = msg->_from_icon;
        one->_created_at = msg->_created_at;
        one->_msg_state = msg->_msg_state;
        one->_server_msg_id = msg->_server_msg_id;
        one->_group_seq = msg->_group_seq;
        one->_reply_to_server_msg_id = msg->_reply_to_server_msg_id;
        one->_forward_meta_json = msg->_forward_meta_json;
        one->_edited_at_ms = msg->_edited_at_ms;
        one->_deleted_at_ms = msg->_deleted_at_ms;
        found = true;
        break;
    }

    if (!found)
    {
        chatMsgs.push_back(msg);
    }

    sortGroupMessages(chatMsgs);
    iter.value()->_last_msg = msg->_msg_content;
}

bool UserMgr::UpdateGroupChatMsgState(qint64 group_id, const QString& msgId, const QString& state)
{
    auto iter = _group_map.find(group_id);
    if (iter == _group_map.end() || !iter.value() || msgId.isEmpty())
    {
        return false;
    }

    auto& chatMsgs = iter.value()->_chat_msgs;
    for (auto& one : chatMsgs)
    {
        if (!one || one->_msg_id != msgId)
        {
            continue;
        }
        one->_msg_state = state;
        return true;
    }
    return false;
}

bool UserMgr::UpdateGroupChatMsgContent(qint64 group_id,
                                        const QString& msgId,
                                        const QString& content,
                                        const QString& state,
                                        qint64 editedAtMs,
                                        qint64 deletedAtMs)
{
    auto iter = _group_map.find(group_id);
    if (iter == _group_map.end() || !iter.value() || msgId.isEmpty())
    {
        return false;
    }

    auto& chatMsgs = iter.value()->_chat_msgs;
    for (auto& one : chatMsgs)
    {
        if (!one || one->_msg_id != msgId)
        {
            continue;
        }
        one->_msg_content = content;
        if (!state.isEmpty())
        {
            one->_msg_state = state;
        }
        if (editedAtMs > 0)
        {
            one->_edited_at_ms = editedAtMs;
        }
        if (deletedAtMs > 0)
        {
            one->_deleted_at_ms = deletedAtMs;
        }
        iter.value()->_last_msg = content;
        return true;
    }
    return false;
}

int UserMgr::MarkGroupOutgoingReadUntil(qint64 group_id, int self_uid, qint64 read_ts)
{
    auto iter = _group_map.find(group_id);
    if (iter == _group_map.end() || !iter.value() || self_uid <= 0 || read_ts <= 0)
    {
        return 0;
    }

    int updated = 0;
    auto& chatMsgs = iter.value()->_chat_msgs;
    for (auto& one : chatMsgs)
    {
        if (!one)
        {
            continue;
        }
        if (one->_from_uid != self_uid || one->_created_at > read_ts)
        {
            continue;
        }
        if (one->_msg_state == "read")
        {
            continue;
        }
        if (one->_msg_state == "sending" || one->_msg_state == "failed")
        {
            continue;
        }
        one->_msg_state = "read";
        ++updated;
    }
    return updated;
}
