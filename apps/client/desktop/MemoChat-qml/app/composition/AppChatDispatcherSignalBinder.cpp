#include "AppController.h"

#include "AppChatDispatcherEventRouter.h"
#include "AppChatDispatcherRouterFactory.h"
#include "AppChatDispatcherSignalRoutes.h"
#include "ChatMessageDispatcher.h"
#include "IChatTransport.h"

void AppController::bindAppChatDispatcherSignals()
{
    const auto chatDispatcher = _gateway.chatMessageDispatcher();
    _chat_dispatcher_event_router = memochat::app::makeAppChatDispatcherEventRouter(
        *_session_coordinator,
        *_chat_connection_coordinator,
        *_call_coordinator,
        _features.chatFeatureController,
        _gateway.userMgr(),
        _features.contactController,
        _features.groupController,
        [this]()
        {
            return contactEventActions();
        },
        [this]()
        {
            return incomingMessageReadinessInputs();
        },
        [this]()
        {
            return incomingMessageDispatchActions();
        },
        [this](int selfUid)
        {
            return groupConversationContext(selfUid);
        },
        _features.groupController.groupListModel(),
        [this](int reqId, const QByteArray& payload)
        {
            if (const auto transport = _gateway.chatTransport())
            {
                transport->slot_send_data(static_cast<ReqId>(reqId), payload);
            }
        },
        [this]()
        {
            return groupManagementEventContext();
        },
        [this]()
        {
            return groupManagementEventEffectActions();
        },
        groupResponseHandlers(),
        [this]()
        {
            return dialogListResponseContext();
        },
        [this]()
        {
            return dialogListAppPort();
        },
        [this](int selfUid)
        {
            return privateChatEventContext(selfUid);
        },
        [this]()
        {
            return _chat_state.uid;
        },
        [this](bool loading)
        {
            setPrivateHistoryLoading(loading);
        },
        [this](bool canLoad)
        {
            setCanLoadMorePrivateHistory(canLoad);
        },
        this);

    connect(chatDispatcher.get(),
            &ChatMessageDispatcher::sig_login_failed,
            _chat_dispatcher_event_router.get(),
            &AppChatDispatcherEventRouter::onChatLoginFailed);
    connect(chatDispatcher.get(),
            &ChatMessageDispatcher::sig_notify_offline,
            _chat_dispatcher_event_router.get(),
            &AppChatDispatcherEventRouter::onNotifyOffline);
    connect(chatDispatcher.get(),
            &ChatMessageDispatcher::sig_heartbeat_ack,
            _chat_dispatcher_event_router.get(),
            &AppChatDispatcherEventRouter::onHeartbeatAck);
    memochat::app::bindAppChatDispatcherSignalRoutes(chatDispatcher, *_chat_dispatcher_event_router);
}
