#include "AppController.h"

#include "ChatEventDependenciesFactory.h"
#include "IChatTransport.h"
#include "usermgr.h"

#include <utility>

void AppController::bindChatFeatureGroupPorts()
{
    ChatCurrentGroupHistoryPort currentGroupHistoryPort;
    currentGroupHistoryPort.snapshot = [this]()
    {
        ChatCurrentGroupHistorySnapshot snapshot;
        if (const auto selfInfo = _gateway.userMgr() ? _gateway.userMgr()->GetUserInfo() : nullptr)
        {
            snapshot.selfUid = selfInfo->_uid;
        }
        snapshot.currentGroupId = currentGroupId();
        snapshot.groupHistoryLoading = _shell_state.loadingState().groupHistoryLoading;
        return snapshot;
    };
    currentGroupHistoryPort.dependencies = [this]()
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
    currentGroupHistoryPort.requestPort.setGroupHistoryLoading = [this](bool loading)
    {
        _shell_state.loadingState().groupHistoryLoading = loading;
    };
    currentGroupHistoryPort.requestPort.setPrivateHistoryLoading = [this](bool loading)
    {
        setPrivateHistoryLoading(loading);
    };

    ChatGroupConversationPort conversationPort;
    conversationPort.ensureGroupsInitialized = [this]()
    {
        ensureGroupsInitialized();
    };

    _features.chatFeatureController.setGroupConversationPort(std::move(conversationPort));
    _features.chatFeatureController.setCurrentGroupHistoryPort(std::move(currentGroupHistoryPort));
}
