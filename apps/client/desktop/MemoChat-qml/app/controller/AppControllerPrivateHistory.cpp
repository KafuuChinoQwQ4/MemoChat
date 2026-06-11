#include "AppController.h"
#include "DialogListService.h"
#include "IChatTransport.h"
#include "PrivateHistoryDependenciesFactory.h"
#include "PrivateChatHistoryRequestService.h"
#include "usermgr.h"

#include <QDebug>
#include <utility>

void AppController::loadCurrentChatMessages()
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();

    auto friendInfo = _gateway.userMgr()->GetFriendById(_chat_state.uid);
    qInfo() << "Loading current private chat, peer uid:" << _chat_state.uid << "existing friend msg count:"
            << (friendInfo ? static_cast<qlonglong>(friendInfo->_chat_msgs.size()) : 0LL);

    PrivateHistoryLoadCurrentRequest request;
    request.currentPeerUid = _chat_state.uid;
    request.selfUid = selfInfo ? selfInfo->_uid : 0;
    const PrivateHistoryLoadCurrentResult result = _features.chatFeatureController.loadCurrentPrivateHistory(
        request,
        memochat::app::makePrivateHistoryLoadCurrentDependencies(
            _features.chatFeatureController,
            _gateway.userMgr(),
            [this](const QString& name)
            {
                setCurrentChatPeerName(name);
            },
            [this](const QString& icon)
            {
                setCurrentChatPeerIcon(icon);
            },
            [this](bool loading)
            {
                setPrivateHistoryLoading(loading);
            },
            [this](bool canLoad)
            {
                setCanLoadMorePrivateHistory(canLoad);
            },
            [this](int peerUid, qint64 readTs)
            {
                _features.chatFeatureController.sendPrivateReadAckForPeer(peerUid, readTs);
            }));
    if (!result.success)
    {
        resetGroupConversationFeatureState();
        _shell_state.loadingState().groupHistoryLoading = false;
        return;
    }

    if (result.localCacheCount > 0)
    {
        qInfo() << "Merged local private cache, peer uid:" << result.peerUid
                << "cache count:" << result.localCacheCount;
    }
    qInfo() << "Private chat loaded, peer uid:" << _chat_state.uid << "friend msg count:" << result.friendMessageCount
            << "message model count:" << _features.chatFeatureController.messageCount()
            << "earliest created at:" << result.beforeTs << "earliest msg id:" << result.beforeMsgId;
    if (result.shouldRequestInitialHistory)
    {
        requestPrivateHistory(_chat_state.uid, 0, QString());
    }
}

void AppController::requestPrivateHistory(int peerUid, qint64 beforeTs, const QString& beforeMsgId)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    PrivateHistoryRequestBuildRequest request;
    request.peerUid = peerUid;
    request.selfUid = selfInfo ? selfInfo->_uid : 0;
    request.beforeTs = beforeTs;
    request.beforeMsgId = beforeMsgId;
    request.privateHistoryLoading = _shell_state.loadingState().privateHistoryLoading;
    _features.chatFeatureController.buildPrivateHistoryRequest(
        request,
        memochat::app::makePrivateHistoryRequestBuildDependencies(
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
            }));
}

void AppController::requestPrivateHistoryForBootstrap(int peerUid)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    PrivateHistoryBootstrapRequest request;
    request.peerUid = peerUid;
    request.selfUid = selfInfo ? selfInfo->_uid : 0;
    PrivateHistoryBootstrapDependencies dependencies;
    dependencies.dispatchPayload = [this](int reqId, const QByteArray& payload)
    {
        if (const auto transport = _gateway.chatTransport())
        {
            transport->slot_send_data(static_cast<ReqId>(reqId), payload);
        }
    };
    _features.chatFeatureController.buildPrivateHistoryBootstrapRequest(request, dependencies);
}
