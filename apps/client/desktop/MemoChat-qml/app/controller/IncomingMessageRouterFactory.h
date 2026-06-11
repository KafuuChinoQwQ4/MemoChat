#pragma once

#include "IncomingMessageRouter.h"

#include <functional>
#include <memory>

namespace memochat::app
{

struct IncomingMessageRouterReadinessInputs
{
    bool onChatPage = false;
    bool dialogsReady = false;
    bool chatListInitialized = false;
};

struct IncomingMessageRouterDispatchActions
{
    std::function<void(std::shared_ptr<TextChatMsg>)> applyPrivateMessage;
    std::function<void(std::shared_ptr<GroupChatMsg>)> applyGroupMessage;
};

IncomingMessageRouterReadiness makeIncomingMessageRouterReadiness(IncomingMessageRouterReadinessInputs inputs);

IncomingMessageRouterDispatch makeIncomingMessageRouterDispatch(IncomingMessageRouterDispatchActions actions);

} // namespace memochat::app
