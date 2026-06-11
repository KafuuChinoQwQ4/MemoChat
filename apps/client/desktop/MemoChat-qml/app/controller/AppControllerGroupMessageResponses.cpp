#include "AppController.h"
#include "ChatEventDependenciesFactory.h"
#include "IChatTransport.h"
#include "usermgr.h"

#include <QJsonObject>

void AppController::handleGroupMessageMutationRsp(ReqId reqId, const QJsonObject& payload)
{
    GroupMutationRequest request;
    request.context = groupConversationContext(_gateway.userMgr()->GetUid());
    request.reqId = reqId;
    request.payload = payload;
    const GroupMutationResult result = _features.chatFeatureController.handleGroupMessageMutation(
        request,
        memochat::app::makeGroupConversationDependencies(
            _features.chatFeatureController,
            _gateway.userMgr(),
            _features.groupController.groupListModel(),
            [this](int dispatchReqId, const QByteArray& dispatchPayload)
            {
                if (const auto transport = _gateway.chatTransport())
                {
                    transport->slot_send_data(static_cast<ReqId>(dispatchReqId), dispatchPayload);
                }
            }));
    if (!result.statusText.isEmpty())
    {
        setGroupStatus(result.statusText, result.statusIsError);
    }
}

void AppController::handleGroupMessageAckRsp(ReqId reqId, const QJsonObject& payload)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    GroupAckRequest request;
    request.context = groupConversationContext(selfInfo ? selfInfo->_uid : _gateway.userMgr()->GetUid());
    request.reqId = reqId;
    request.payload = payload;
    const GroupAckResult result = _features.chatFeatureController.handleGroupMessageAck(
        request,
        memochat::app::makeGroupConversationDependencies(
            _features.chatFeatureController,
            _gateway.userMgr(),
            _features.groupController.groupListModel(),
            [this](int dispatchReqId, const QByteArray& dispatchPayload)
            {
                if (const auto transport = _gateway.chatTransport())
                {
                    transport->slot_send_data(static_cast<ReqId>(dispatchReqId), dispatchPayload);
                }
            }));
    if (result.requestedReadAckTs > 0)
    {
        _features.chatFeatureController.sendGroupReadAckForGroup(result.groupId, result.requestedReadAckTs);
    }
    if (!result.statusText.isEmpty())
    {
        setGroupStatus(result.statusText, result.statusIsError);
    }
}
