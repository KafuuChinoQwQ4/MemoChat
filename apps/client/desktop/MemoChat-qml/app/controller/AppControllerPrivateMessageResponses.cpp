#include "AppController.h"
#include "ChatEventDependenciesFactory.h"
#include "PrivateChatEventService.h"
#include "usermgr.h"

#include <QJsonObject>

void AppController::handlePrivateMessageMutationRsp(ReqId reqId, const QJsonObject& payload)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo)
    {
        return;
    }

    PrivateMessageMutationResponseRequest request;
    request.context = privateChatEventContext(selfInfo->_uid);
    request.reqId = reqId;
    request.payload = payload;
    const PrivateMessageChangedResult result = _features.chatFeatureController.handlePrivateMessageMutationResponse(
        request,
        memochat::app::makePrivateChatEventDependencies(
            _features.chatFeatureController,
            _gateway.userMgr(),
            [this](std::shared_ptr<AuthInfo> authInfo)
            {
                _features.contactController.upsertContact(std::move(authInfo));
            },
            [this](int peerUid, qint64 readTs)
            {
                _features.chatFeatureController.sendPrivateReadAckForPeer(peerUid, readTs);
            }));
    if (!result.statusText.isEmpty())
    {
        setTip(result.statusText, result.statusIsError);
    }
}

void AppController::handlePrivateForwardRsp(const QJsonObject& payload)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo)
    {
        return;
    }

    PrivateForwardResponseRequest request;
    request.context = privateChatEventContext(selfInfo->_uid);
    request.payload = payload;
    const PrivateForwardResponseResult result = _features.chatFeatureController.handlePrivateForwardResponse(
        request,
        memochat::app::makePrivateChatEventDependencies(
            _features.chatFeatureController,
            _gateway.userMgr(),
            [this](std::shared_ptr<AuthInfo> authInfo)
            {
                _features.contactController.upsertContact(std::move(authInfo));
            },
            [this](int peerUid, qint64 readTs)
            {
                _features.chatFeatureController.sendPrivateReadAckForPeer(peerUid, readTs);
            }));

    setTip(result.statusText, result.isError);
}
