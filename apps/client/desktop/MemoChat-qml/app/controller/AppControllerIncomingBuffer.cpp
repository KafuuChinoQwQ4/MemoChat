#include "AppController.h"

#include "IncomingMessageRouterFactory.h"

memochat::app::IncomingMessageRouterReadinessInputs AppController::incomingMessageReadinessInputs() const
{
    memochat::app::IncomingMessageRouterReadinessInputs inputs;
    inputs.onChatPage = page() == ChatPage;
    inputs.dialogsReady = _shell_state.bootstrapState().dialogsReady;
    inputs.chatListInitialized = _shell_state.bootstrapState().chatListInitialized;
    return inputs;
}

memochat::app::IncomingMessageRouterDispatchActions AppController::incomingMessageDispatchActions()
{
    memochat::app::IncomingMessageRouterDispatchActions actions;
    actions.applyPrivateMessage = [this](std::shared_ptr<TextChatMsg> msg)
    {
        applyTextChatMsg(std::move(msg));
    };
    actions.applyGroupMessage = [this](std::shared_ptr<GroupChatMsg> msg)
    {
        applyGroupChatMsg(std::move(msg));
    };
    return actions;
}

void AppController::flushIncomingMessageRouter()
{
    _features.chatFeatureController.flushIncomingMessages(
        memochat::app::makeIncomingMessageRouterReadiness(incomingMessageReadinessInputs()),
        memochat::app::makeIncomingMessageRouterDispatch(incomingMessageDispatchActions()));
}

void AppController::clearIncomingMessageRouter()
{
    _features.chatFeatureController.clearIncomingMessages();
}
