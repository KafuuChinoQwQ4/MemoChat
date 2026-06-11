#pragma once

#include "GroupConversationService.h"
#include "PrivateChatEventService.h"

#include <functional>
#include <memory>
#include <vector>

class ChatFeatureController;
class UserMgr;

namespace memochat::app
{

PrivateChatEventDependencies
makePrivateChatEventDependencies(ChatFeatureController& controller,
                                 std::shared_ptr<UserMgr> userMgr,
                                 std::function<void(std::shared_ptr<AuthInfo>)> upsertContact,
                                 std::function<void(int, qint64)> requestReadAck);

GroupConversationDependencies
makeGroupConversationDependencies(ChatFeatureController& controller,
                                  std::shared_ptr<UserMgr> userMgr,
                                  FriendListModel* groupListModel,
                                  std::function<void(int, const QByteArray&)> dispatchPayload);

} // namespace memochat::app
