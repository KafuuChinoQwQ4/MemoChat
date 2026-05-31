#include "AppController.h"
#include "ConversationSyncService.h"
#include "usermgr.h"

#include <QJsonArray>
#include <QJsonObject>

void AppController::handleGroupHistoryRsp(const QJsonObject& payload)
{
    const qint64 groupId = payload.value("groupid").toVariant().toLongLong();
    auto groupInfo = _gateway.userMgr()->GetGroupById(groupId);
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (groupInfo && selfInfo && _group_cache_store.isReady())
    {
        _group_cache_store.upsertMessages(selfInfo->_uid, groupId, groupInfo->_chat_msgs);
    }
    const bool isCurrentGroupHistory = groupId == _group_state.currentId;
    if (isCurrentGroupHistory)
    {
        _loading_state.groupHistoryLoading = false;
        setPrivateHistoryLoading(false);
    }
    qInfo() << "Group history response, group id:" << groupId << "current group id:" << _group_state.currentId
            << "message count:" << payload.value("messages").toArray().size()
            << "cached group msg count:" << (groupInfo ? static_cast<qlonglong>(groupInfo->_chat_msgs.size()) : 0LL)
            << "has more:" << payload.value("has_more").toBool(false);
    if (isCurrentGroupHistory && groupInfo)
    {
        _group_state.historyHasMore = payload.value("has_more").toBool(false);
        setCanLoadMorePrivateHistory(_group_state.historyHasMore);
        _group_state.historyBeforeSeq = payload.value("next_before_seq").toVariant().toLongLong();
        _message_model.setMessages(groupInfo->_chat_msgs, _gateway.userMgr()->GetUid());
        if (_group_state.historyBeforeSeq <= 0)
        {
            for (const auto& one : groupInfo->_chat_msgs)
            {
                if (!one || one->_group_seq <= 0)
                {
                    continue;
                }
                if (_group_state.historyBeforeSeq <= 0 || one->_group_seq < _group_state.historyBeforeSeq)
                {
                    _group_state.historyBeforeSeq = one->_group_seq;
                }
            }
        }
        qint64 readTs = 0;
        if (!groupInfo->_chat_msgs.empty() && groupInfo->_chat_msgs.back())
        {
            readTs = groupInfo->_chat_msgs.back()->_created_at;
        }
        sendGroupReadAck(groupId, readTs);
        const int dialogUid = ConversationSyncService::resolveGroupDialogUid(_group_state.dialogUidMap, groupId);
        ConversationSyncService::clearGroupUnreadAndMention(_dialog_list_model, _dialog_state.mentionMap, dialogUid);
        setGroupStatus("历史消息已加载", false);
    }
    else
    {
        qInfo() << "Group history response cached for non-current group, group id:" << groupId;
    }
}
