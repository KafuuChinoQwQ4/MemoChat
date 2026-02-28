#ifndef CLIENTGATEWAY_H
#define CLIENTGATEWAY_H

#include <memory>

class HttpMgr;
class TcpMgr;
class UserMgr;

class ClientGateway
{
public:
    std::shared_ptr<HttpMgr> httpMgr() const;
    std::shared_ptr<TcpMgr> tcpMgr() const;
    std::shared_ptr<UserMgr> userMgr() const;
};

#endif // CLIENTGATEWAY_H
