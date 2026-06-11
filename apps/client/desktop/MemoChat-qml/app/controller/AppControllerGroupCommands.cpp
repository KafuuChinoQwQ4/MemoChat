#include "AppController.h"
#include "ChatEventDependenciesFactory.h"
#include "IChatTransport.h"
#include "usermgr.h"

void AppController::refreshGroupList()
{
    _features.groupController.refreshGroupList();
}

void AppController::requestDialogList()
{
    _features.groupController.requestDialogList();
}

void AppController::requestGroupHistoryForBootstrap(qint64 groupId)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    GroupHistoryRequestCommand request;
    request.selfUid = selfInfo ? selfInfo->_uid : 0;
    request.currentGroupId = currentGroupId();
    request.targetGroupId = groupId;
    request.currentGroupLoading = _shell_state.loadingState().groupHistoryLoading;
    request.limit = 50;

    GroupHistoryRequestPort port;
    port.setGroupHistoryLoading = [this](bool loading)
    {
        _shell_state.loadingState().groupHistoryLoading = loading;
    };
    port.setPrivateHistoryLoading = [this](bool loading)
    {
        setPrivateHistoryLoading(loading);
    };

    const GroupHistoryRequestResult result = _features.chatFeatureController.requestGroupHistoryForBootstrap(
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
    if (!result.success || result.skipped)
    {
        if (result.skippedByLoading)
        {
            qInfo() << "Skip bootstrap group history because current group is already loading, group id:"
                    << result.groupId;
        }
        return;
    }
    qInfo() << "Requesting bootstrap group history, group id:" << result.groupId
            << "current group id:" << result.currentGroupId
            << "private history loading:" << _shell_state.loadingState().privateHistoryLoading;
}
