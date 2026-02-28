#include "ClientGateway.h"
#include "httpmgr.h"
#include "tcpmgr.h"
#include "usermgr.h"

std::shared_ptr<HttpMgr> ClientGateway::httpMgr() const
{
    return HttpMgr::GetInstance();
}

std::shared_ptr<TcpMgr> ClientGateway::tcpMgr() const
{
    return TcpMgr::GetInstance();
}

std::shared_ptr<UserMgr> ClientGateway::userMgr() const
{
    return UserMgr::GetInstance();
}
