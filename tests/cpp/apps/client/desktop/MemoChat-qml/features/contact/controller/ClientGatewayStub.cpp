#include "ClientGateway.h"
#include "httpmgr.h"
#include "usermgr.h"

#include <QJsonObject>
#include <QUrl>
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
    return UserMgr::GetInstance();
}

void HttpMgr::PostHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod, const QString& module)
{
    Q_UNUSED(url)
    Q_UNUSED(json)
    Q_UNUSED(req_id)
    Q_UNUSED(mod)
    Q_UNUSED(module)
}
