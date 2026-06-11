#ifndef APPCHATDISPATCHERSIGNALROUTES_H
#define APPCHATDISPATCHERSIGNALROUTES_H

#include <memory>

class AppChatDispatcherEventRouter;
class ChatMessageDispatcher;

namespace memochat::app
{
void bindAppChatDispatcherSignalRoutes(const std::shared_ptr<ChatMessageDispatcher>& chatDispatcher,
                                       AppChatDispatcherEventRouter& router);
} // namespace memochat::app

#endif // APPCHATDISPATCHERSIGNALROUTES_H
