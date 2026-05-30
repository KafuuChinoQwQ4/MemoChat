#include "usermgr.h"

#include <QDebug>

void UserMgr::AppendFriendChatMsg(int friend_id, std::vector<std::shared_ptr<TextChatData>> msgs)
{
    auto find_iter = _friend_map.find(friend_id);
    if (find_iter == _friend_map.end())
    {
        qDebug() << "append friend uid  " << friend_id << " not found";
        return;
    }

    find_iter.value()->AppendChatMsgs(msgs);
}

bool UserMgr::UpdatePrivateChatMsgState(int peer_uid, const QString& msgId, const QString& state)
{
    auto iter = _friend_map.find(peer_uid);
    if (iter == _friend_map.end() || !iter.value() || msgId.isEmpty())
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

bool UserMgr::UpdatePrivateChatMsgContent(int peer_uid,
                                          const QString& msgId,
                                          const QString& content,
                                          const QString& state,
                                          qint64 editedAtMs,
                                          qint64 deletedAtMs)
{
    auto iter = _friend_map.find(peer_uid);
    if (iter == _friend_map.end() || !iter.value() || msgId.isEmpty())
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
        if (!chatMsgs.empty() && chatMsgs.back() && chatMsgs.back()->_msg_id == msgId)
        {
            iter.value()->_last_msg = content;
        }
        return true;
    }
    return false;
}

int UserMgr::MarkPrivateOutgoingReadUntil(int peer_uid, int self_uid, qint64 read_ts)
{
    auto iter = _friend_map.find(peer_uid);
    if (iter == _friend_map.end() || !iter.value() || self_uid <= 0 || read_ts <= 0)
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
