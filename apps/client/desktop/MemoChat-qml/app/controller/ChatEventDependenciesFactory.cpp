#include "ChatEventDependenciesFactory.h"

#include "ChatFeatureController.h"

#include <utility>

namespace memochat::app
{

PrivateChatEventDependencies
makePrivateChatEventDependencies(ChatFeatureController& controller,
                                 std::shared_ptr<UserMgr> userMgr,
                                 std::function<void(std::shared_ptr<AuthInfo>)> upsertContact,
                                 std::function<void(int, qint64)> requestReadAck)
{
    return controller.privateEventDependencies(std::move(userMgr), std::move(upsertContact), std::move(requestReadAck));
}

GroupConversationDependencies
makeGroupConversationDependencies(ChatFeatureController& controller,
                                  std::shared_ptr<UserMgr> userMgr,
                                  FriendListModel* groupListModel,
                                  std::function<void(int, const QByteArray&)> dispatchPayload)
{
    return controller.groupConversationDependencies(std::move(userMgr), groupListModel, std::move(dispatchPayload));
}

} // namespace memochat::app
