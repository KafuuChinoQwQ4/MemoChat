#include "AppController.h"

#include "ChatEventDependenciesFactory.h"
#include "IChatTransport.h"
#include "PrivateHistoryDependenciesFactory.h"
#include "usermgr.h"

#include <utility>

void AppController::bindChatFeatureHistoryPorts()
{
    ChatCurrentHistoryPort currentHistoryPort;
    currentHistoryPort.snapshot = [this]()
    {
        auto selfInfo = _gateway.userMgr()->GetUserInfo();
        ChatHistoryLoadMoreRequest request;
        request.selfUid = selfInfo ? selfInfo->_uid : 0;
        request.currentPrivatePeerUid = _chat_state.uid;
        request.currentGroupId = currentGroupId();
        request.privateHistoryLoading = _shell_state.loadingState().privateHistoryLoading;
        request.canLoadMorePrivateHistory = _shell_state.loadingState().canLoadMorePrivateHistory;
        request.groupHistoryLoading = _shell_state.loadingState().groupHistoryLoading;
        request.groupLimit = 50;
        return request;
    };
    currentHistoryPort.privateDependencies = [this]()
    {
        return memochat::app::makePrivateHistoryLoadMoreDependencies(
            _features.chatFeatureController,
            [this](bool loading)
            {
                setPrivateHistoryLoading(loading);
            },
            [this](int reqId, const QByteArray& payload)
            {
                if (const auto transport = _gateway.chatTransport())
                {
                    transport->slot_send_data(static_cast<ReqId>(reqId), payload);
                }
            });
    };
    currentHistoryPort.groupDependencies = [this]()
    {
        return memochat::app::makeGroupConversationDependencies(
            _features.chatFeatureController,
            _gateway.userMgr(),
            _features.groupController.groupListModel(),
            [this](int reqId, const QByteArray& payload)
            {
                if (const auto transport = _gateway.chatTransport())
                {
                    transport->slot_send_data(static_cast<ReqId>(reqId), payload);
                }
            });
    };
    currentHistoryPort.groupRequestPort.setGroupHistoryLoading = [this](bool loading)
    {
        _shell_state.loadingState().groupHistoryLoading = loading;
    };
    currentHistoryPort.groupRequestPort.setPrivateHistoryLoading = [this](bool loading)
    {
        setPrivateHistoryLoading(loading);
    };

    _features.chatFeatureController.setCurrentHistoryPort(std::move(currentHistoryPort));
}
