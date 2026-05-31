#include "R18Controller.h"

#include "ClientGateway.h"
#include "R18ControllerPrivate.h"
#include "global.h"
#include "usermgr.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

using namespace memochat::r18;

void R18Controller::postJson(const QString& path, const QJsonObject& payload, const QString& op)
{
    const bool quiet = op == QStringLiteral("history_update");
    if (!quiet)
    {
        setLoading(true);
        setError({});
    }
    QNetworkRequest request(QUrl(gate_url_prefix + path));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    applyRequestOptions(request);
    auto* reply = _network.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    armTimeout(reply);
    reply->setProperty("r18_op", op);
    connect(reply,
            &QNetworkReply::finished,
            this,
            [this, reply]()
            {
                const QString op = reply->property("r18_op").toString();
                const bool quiet = op == QStringLiteral("history_update");
                const auto networkError = reply->error();
                const QString networkErrorText = reply->errorString();
                const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                const auto bytes = reply->readAll();
                reply->deleteLater();
                if (!quiet)
                {
                    setLoading(false);
                }
                if (networkError != QNetworkReply::NoError)
                {
                    if (!quiet)
                    {
                        setError(QStringLiteral("R18 网络请求失败: %1").arg(networkErrorText));
                    }
                    return;
                }
                if (httpStatus >= 400)
                {
                    if (!quiet)
                    {
                        setError(QStringLiteral("R18 HTTP %1").arg(httpStatus));
                    }
                    return;
                }
                const auto doc = QJsonDocument::fromJson(bytes);
                if (!doc.isObject())
                {
                    if (!quiet)
                    {
                        setError(QStringLiteral("R18 响应格式错误"));
                    }
                    return;
                }
                handleResponse(op, doc.object());
            });
}

void R18Controller::getJson(const QUrl& url, const QString& op)
{
    setLoading(true);
    setError({});
    QNetworkRequest request(url);
    applyRequestOptions(request);
    auto* reply = _network.get(request);
    armTimeout(reply);
    reply->setProperty("r18_op", op);
    connect(reply,
            &QNetworkReply::finished,
            this,
            [this, reply, url]()
            {
                const QString op = reply->property("r18_op").toString();
                const auto networkError = reply->error();
                const QString networkErrorText = reply->errorString();
                const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                const auto bytes = reply->readAll();
                reply->deleteLater();
                setLoading(false);
                if (networkError != QNetworkReply::NoError)
                {
                    setError(QStringLiteral("R18 网络请求失败: %1").arg(networkErrorText));
                    return;
                }
                if (httpStatus >= 400)
                {
                    setError(QStringLiteral("R18 HTTP %1").arg(httpStatus));
                    return;
                }
                const auto doc = QJsonDocument::fromJson(bytes);
                if (op == QStringLiteral("official_sources") && doc.isArray())
                {
                    QUrl catalogUrl = url;
                    auto items = doc.array().toVariantList();
                    for (auto& entry : items)
                    {
                        auto map = entry.toMap();
                        map[QStringLiteral("catalog_url")] = catalogUrl.toString();
                        const QUrl scriptUrl = resolveOfficialSourceUrl(map);
                        map[QStringLiteral("source_url")] = scriptUrl.toString();
                        entry = map;
                    }
                    _official_sources.setItems(items);
                    setStatusText(QStringLiteral("自定义源目录已加载: %1 项").arg(items.size()));
                    return;
                }
                if (!doc.isObject())
                {
                    setError(QStringLiteral("R18 响应格式错误"));
                    return;
                }
                handleResponse(op, doc.object());
            });
}

QJsonObject R18Controller::authPayload() const
{
    QJsonObject payload;
    if (_gateway && _gateway->userMgr())
    {
        payload[QStringLiteral("uid")] = _gateway->userMgr()->GetUid();
        payload[QStringLiteral("token")] = _gateway->userMgr()->GetToken();
    }
    return payload;
}
