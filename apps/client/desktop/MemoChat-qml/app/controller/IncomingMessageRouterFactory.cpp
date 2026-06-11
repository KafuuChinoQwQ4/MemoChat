#include "IncomingMessageRouterFactory.h"

#include <utility>

namespace memochat::app
{

IncomingMessageRouterReadiness makeIncomingMessageRouterReadiness(IncomingMessageRouterReadinessInputs inputs)
{
    IncomingMessageRouterReadiness readiness;
    readiness.onChatPage = inputs.onChatPage;
    readiness.dialogsReady = inputs.dialogsReady;
    readiness.chatListInitialized = inputs.chatListInitialized;
    return readiness;
}

IncomingMessageRouterDispatch makeIncomingMessageRouterDispatch(IncomingMessageRouterDispatchActions actions)
{
    IncomingMessageRouterDispatch dispatch;
    dispatch.applyPrivateMessage = std::move(actions.applyPrivateMessage);
    dispatch.applyGroupMessage = std::move(actions.applyGroupMessage);
    return dispatch;
}

} // namespace memochat::app
