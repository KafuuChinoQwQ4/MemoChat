#include "ChatFeatureController.h"

#include <utility>

IncomingMessageRouteResult
ChatFeatureController::routePrivateIncomingMessage(const IncomingMessageRouterReadiness& readiness,
                                                   std::shared_ptr<TextChatMsg> message,
                                                   const IncomingMessageRouterDispatch& dispatch)
{
    return _incomingMessageRouter.routePrivateMessage(readiness, std::move(message), dispatch);
}

IncomingMessageRouteResult
ChatFeatureController::routeGroupIncomingMessage(const IncomingMessageRouterReadiness& readiness,
                                                 std::shared_ptr<GroupChatMsg> message,
                                                 const IncomingMessageRouterDispatch& dispatch)
{
    return _incomingMessageRouter.routeGroupMessage(readiness, std::move(message), dispatch);
}

int ChatFeatureController::flushIncomingMessages(const IncomingMessageRouterReadiness& readiness,
                                                 const IncomingMessageRouterDispatch& dispatch)
{
    return _incomingMessageRouter.flush(readiness, dispatch);
}

void ChatFeatureController::clearIncomingMessages()
{
    _incomingMessageRouter.clear();
}

int ChatFeatureController::pendingIncomingMessageCount() const
{
    return _incomingMessageRouter.pendingCount();
}
