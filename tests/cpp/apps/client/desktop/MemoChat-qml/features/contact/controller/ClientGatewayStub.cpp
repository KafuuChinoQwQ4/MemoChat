#include "ClientGateway.h"
#include "usermgr.h"

#include <memory>

class ChatMessageDispatcher;
class HttpMgr;
class IChatTransport;
class TransportSelector;
class UserMgr;

std::shared_ptr<HttpMgr> ClientGateway::httpMgr() const
{
    return {};
}

std::shared_ptr<IChatTransport> ClientGateway::chatTransport() const
{
    return {};
}

std::shared_ptr<ChatMessageDispatcher> ClientGateway::chatMessageDispatcher() const
{
    return {};
}

std::shared_ptr<UserMgr> ClientGateway::userMgr() const
{
    return {};
}
