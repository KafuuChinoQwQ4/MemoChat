#ifndef CLIENTGATEWAY_H
#define CLIENTGATEWAY_H

#include <memory>

class HttpMgr;
class ChatMessageDispatcher;
class IChatTransport;
class TransportSelector;
class UserMgr;

class ClientGateway
{
public:
    std::shared_ptr<HttpMgr> httpMgr() const;
    std::shared_ptr<IChatTransport> chatTransport() const;
    std::shared_ptr<ChatMessageDispatcher> chatMessageDispatcher() const;
    std::shared_ptr<UserMgr> userMgr() const;

private:
    std::shared_ptr<TransportSelector> transportSelector() const;
};

#endif // CLIENTGATEWAY_H
