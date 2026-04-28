#include "ChatMessageDispatcher.h"
#include "ClientGateway.h"
#include "IChatTransport.h"
#include "httpmgr.h"
#include "TransportSelector.h"
#include "usermgr.h"

std::shared_ptr<HttpMgr> ClientGateway::httpMgr() const
{
    return HttpMgr::GetInstance();
}

std::shared_ptr<IChatTransport> ClientGateway::chatTransport() const
{
    return transportSelector();
}

std::shared_ptr<ChatMessageDispatcher> ClientGateway::chatMessageDispatcher() const
{
    return transportSelector()->dispatcher();
}

std::shared_ptr<UserMgr> ClientGateway::userMgr() const
{
    return UserMgr::GetInstance();
}

std::shared_ptr<TransportSelector> ClientGateway::transportSelector() const
{
    static const std::shared_ptr<TransportSelector> selector = std::make_shared<TransportSelector>();
    return selector;
}
