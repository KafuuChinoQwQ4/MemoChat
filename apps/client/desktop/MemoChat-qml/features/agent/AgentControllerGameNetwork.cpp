#include "AgentController.h"

#include "ClientGateway.h"
#include "usermgr.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QUrl>
#include <QtGlobal>

namespace
{
QString gameBaseUrl()
{
    return gate_url_prefix + QStringLiteral("/ai/games");
}

void applyLocalGateRequestOptions(QNetworkRequest& request)
{
    const QUrl url = request.url();
    const QString scheme = url.scheme().toLower();
    if (scheme == QLatin1String("https"))
    {
        QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        request.setSslConfiguration(sslConfig);
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
        request.setAttribute(QNetworkRequest::Http2AllowedAttribute, true);
#endif
    }
}
} // namespace

int AgentController::currentUid() const
{
    return _gateway && _gateway->userMgr() ? _gateway->userMgr()->GetUid() : 0;
}

void AgentController::setGameBusy(bool busy, const QString& statusText)
{
    if (_game_busy == busy && _game_status_text == statusText)
    {
        return;
    }
    _game_busy = busy;
    _game_status_text = statusText;
    emit gameStateChanged();
}

void AgentController::setGameError(const QString& error)
{
    if (_game_error == error)
    {
        return;
    }
    _game_error = error;
    emit gameStateChanged();
}

void AgentController::clearGameError()
{
    if (_game_error.isEmpty())
    {
        return;
    }
    _game_error.clear();
    emit gameStateChanged();
}

void AgentController::sendGameGet(const QUrl& url, const QString& op, const QString& statusText)
{
    clearGameError();
    setGameBusy(true, statusText);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    applyLocalGateRequestOptions(request);
    QNetworkReply* reply = _gameNetwork->get(request);
    connect(reply,
            &QNetworkReply::finished,
            this,
            [this, reply, op]()
            {
                const QByteArray body = reply->readAll();
                const QString networkError =
                    reply->error() == QNetworkReply::NoError ? QString() : reply->errorString();
                reply->deleteLater();

                if (!networkError.isEmpty())
                {
                    setGameBusy(false, QStringLiteral("请求失败"));
                    setGameError(QStringLiteral("Game 服务请求失败: %1").arg(networkError));
                    return;
                }

                const QJsonDocument doc = QJsonDocument::fromJson(body);
                if (!doc.isObject())
                {
                    setGameBusy(false, QStringLiteral("响应格式错误"));
                    setGameError(QStringLiteral("Game 服务响应格式错误"));
                    return;
                }
                handleGameResponse(op, doc.object());
            });
}

void AgentController::sendGamePost(const QUrl& url,
                                   const QJsonObject& payload,
                                   const QString& op,
                                   const QString& statusText)
{
    clearGameError();
    setGameBusy(true, statusText);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    applyLocalGateRequestOptions(request);
    QNetworkReply* reply = _gameNetwork->post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    connect(reply,
            &QNetworkReply::finished,
            this,
            [this, reply, op]()
            {
                const QByteArray body = reply->readAll();
                const QString networkError =
                    reply->error() == QNetworkReply::NoError ? QString() : reply->errorString();
                reply->deleteLater();

                if (!networkError.isEmpty())
                {
                    if (op == QStringLiteral("delete_room"))
                    {
                        _pendingDeleteGameRoomId.clear();
                    }
                    setGameBusy(false, QStringLiteral("请求失败"));
                    setGameError(QStringLiteral("Game 服务请求失败: %1").arg(networkError));
                    return;
                }

                const QJsonDocument doc = QJsonDocument::fromJson(body);
                if (!doc.isObject())
                {
                    if (op == QStringLiteral("delete_room"))
                    {
                        _pendingDeleteGameRoomId.clear();
                    }
                    setGameBusy(false, QStringLiteral("响应格式错误"));
                    setGameError(QStringLiteral("Game 服务响应格式错误"));
                    return;
                }
                handleGameResponse(op, doc.object());
            });
}

void AgentController::sendGameDelete(const QUrl& url, const QString& op, const QString& statusText)
{
    clearGameError();
    setGameBusy(true, statusText);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    applyLocalGateRequestOptions(request);
    QNetworkReply* reply = _gameNetwork->deleteResource(request);
    connect(reply,
            &QNetworkReply::finished,
            this,
            [this, reply, op]()
            {
                const QByteArray body = reply->readAll();
                const QString networkError =
                    reply->error() == QNetworkReply::NoError ? QString() : reply->errorString();
                reply->deleteLater();

                if (!networkError.isEmpty())
                {
                    setGameBusy(false, QStringLiteral("请求失败"));
                    setGameError(QStringLiteral("Game 服务请求失败: %1").arg(networkError));
                    return;
                }

                const QJsonDocument doc = QJsonDocument::fromJson(body);
                if (!doc.isObject())
                {
                    setGameBusy(false, QStringLiteral("响应格式错误"));
                    setGameError(QStringLiteral("Game 服务响应格式错误"));
                    return;
                }
                handleGameResponse(op, doc.object());
            });
}
