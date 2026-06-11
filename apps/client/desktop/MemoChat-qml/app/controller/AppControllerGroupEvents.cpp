#include "AppController.h"
#include "ChatEventDependenciesFactory.h"
#include "GroupManagementEventService.h"
#include "IChatTransport.h"
#include "usermgr.h"

memochat::group::GroupManagementEventContext AppController::groupManagementEventContext() const
{
    memochat::group::GroupManagementEventContext context;
    context.selfUid = _gateway.userMgr()->GetUid();
    context.currentGroup.groupId = currentGroupId();
    context.currentGroup.name = currentGroupName();
    context.currentGroup.code = currentGroupCode();
    context.currentChat.currentGroupId = currentGroupId();
    context.currentChat.privatePeerUid = _chat_state.uid;
    context.currentChat.hasCurrentGroupChat = currentGroupId() > 0;
    if (currentGroupId() > 0)
    {
        const auto groupInfo = _gateway.userMgr()->GetGroupById(currentGroupId());
        if (groupInfo)
        {
            context.refreshedCurrentGroup.exists = true;
            context.refreshedCurrentGroup.groupId = groupInfo->_group_id;
            context.refreshedCurrentGroup.name = groupInfo->_name;
            context.refreshedCurrentGroup.code = groupInfo->_group_code;
            context.refreshedCurrentGroup.icon = groupInfo->_icon;
        }
    }
    return context;
}

void AppController::applyGroupChatMsg(std::shared_ptr<GroupChatMsg> msg)
{
    if (!msg || !msg->_msg)
    {
        return;
    }

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    GroupIncomingRequest request;
    request.context = groupConversationContext(selfInfo ? selfInfo->_uid : _gateway.userMgr()->GetUid());
    request.message = std::move(msg);
    const GroupIncomingResult result = _features.chatFeatureController.handleGroupIncomingMessage(
        request,
        memochat::app::makeGroupConversationDependencies(_features.chatFeatureController,
                                                         _gateway.userMgr(),
                                                         _features.groupController.groupListModel(),
                                                         [this](int reqId, const QByteArray& dispatchPayload)
                                                         {
                                                             if (const auto transport = _gateway.chatTransport())
                                                             {
                                                                 transport->slot_send_data(static_cast<ReqId>(reqId),
                                                                                           dispatchPayload);
                                                             }
                                                         }));
    if (result.requestedReadAckTs > 0)
    {
        _features.chatFeatureController.sendGroupReadAckForGroup(result.groupId, result.requestedReadAckTs);
    }
}
