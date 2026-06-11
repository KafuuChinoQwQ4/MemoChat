#include "AppController.h"
#include "ChatEventDependenciesFactory.h"
#include "IChatTransport.h"
#include "usermgr.h"

#include <QJsonObject>

void AppController::handleGroupHistoryRsp(const QJsonObject& payload)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    GroupHistoryResponseRequest request;
    request.context = groupConversationContext(selfInfo ? selfInfo->_uid : 0);
    request.payload = payload;

    GroupHistoryResponsePort port;
    port.setGroupHistoryLoading = [this](bool loading)
    {
        _shell_state.loadingState().groupHistoryLoading = loading;
    };
    port.setPrivateHistoryLoading = [this](bool loading)
    {
        setPrivateHistoryLoading(loading);
    };
    port.setCanLoadMorePrivateHistory = [this](bool canLoad)
    {
        setCanLoadMorePrivateHistory(canLoad);
    };
    port.sendGroupReadAck = [this](qint64 groupId, qint64 readTs)
    {
        _features.chatFeatureController.sendGroupReadAckForGroup(groupId, readTs);
    };
    port.setGroupStatus = [this](const QString& status, bool isError)
    {
        setGroupStatus(status, isError);
    };

    const GroupHistoryResponseResult result = _features.chatFeatureController.handleGroupHistoryResponse(
        request,
        memochat::app::makeGroupConversationDependencies(_features.chatFeatureController,
                                                         _gateway.userMgr(),
                                                         _features.groupController.groupListModel(),
                                                         [this](int reqId, const QByteArray& payload)
                                                         {
                                                             if (const auto transport = _gateway.chatTransport())
                                                             {
                                                                 transport->slot_send_data(static_cast<ReqId>(reqId),
                                                                                           payload);
                                                             }
                                                         }),
        port);
    qInfo() << "Group history response, group id:" << result.groupId << "current group id:" << result.currentGroupId
            << "message count:" << result.messageCount << "cached group msg count:" << result.cachedMessageCount
            << "has more:" << result.canLoadMore;
    if (!result.currentDialog)
    {
        qInfo() << "Group history response cached for non-current group, group id:" << result.groupId;
    }
}
