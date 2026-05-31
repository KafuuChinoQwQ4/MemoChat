#ifndef CHATMESSAGEDISPATCHERGROUPHISTORY_H
#define CHATMESSAGEDISPATCHERGROUPHISTORY_H

#include <QJsonObject>

#include <memory>

#include "UserMessageData.h"

namespace ChatMessageDispatcherGroupHistory
{
std::shared_ptr<TextChatData> buildHistoryTextMessage(const QJsonObject& message);
} // namespace ChatMessageDispatcherGroupHistory

#endif // CHATMESSAGEDISPATCHERGROUPHISTORY_H
