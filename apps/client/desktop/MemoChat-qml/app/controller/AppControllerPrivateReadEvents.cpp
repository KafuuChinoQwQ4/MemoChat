#include "AppController.h"

#include "usermgr.h"

#include <QDateTime>
#include <QJsonObject>

void AppController::onPrivateReadAck(QJsonObject payload)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo)
    {
        return;
    }
    if (payload.value("error").toInt(ErrorCodes::ERR_JSON) != ErrorCodes::SUCCESS)
    {
        return;
    }

    const int selfUid = selfInfo->_uid;
    const int readerUid = payload.value("fromuid").toInt();
    if (readerUid <= 0 || readerUid == selfUid)
    {
        return;
    }

    qint64 readTs = payload.value("read_ts").toVariant().toLongLong();
    if (readTs <= 0)
    {
        readTs = QDateTime::currentMSecsSinceEpoch();
    }
    else if (readTs < 100000000000LL)
    {
        readTs *= 1000;
    }
    if (_gateway.userMgr()->MarkPrivateOutgoingReadUntil(readerUid, selfUid, readTs) <= 0)
    {
        return;
    }

    auto friendInfo = _gateway.userMgr()->GetFriendById(readerUid);
    if (!friendInfo)
    {
        return;
    }
    if (_private_cache_store.isReady())
    {
        _private_cache_store.upsertMessages(selfUid, readerUid, friendInfo->_chat_msgs);
    }
    if (readerUid == _chat_state.uid)
    {
        for (const auto& one : friendInfo->_chat_msgs)
        {
            if (!one || one->_from_uid != selfUid || one->_created_at > readTs)
            {
                continue;
            }
            if (one->_msg_state != QStringLiteral("read"))
            {
                continue;
            }
            _message_model.updateMessageState(one->_msg_id, QStringLiteral("read"));
        }
    }
}
