#include "R18CookieLoginController.h"
#include "R18ControllerPrivate.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>

R18CookieLoginController::R18CookieLoginController(QObject* parent)
    : QObject(parent)
{
    m_status = QStringLiteral("idle");
    m_message = {};
}

void R18CookieLoginController::init(const QString& sourceId, const QString& gatewayUrl, const QString& authToken)
{
    m_sourceId = sourceId;
    m_gatewayUrl = gatewayUrl;
    m_authToken = authToken;
    setStatus(QStringLiteral("idle"));
    setMessage({});
    setBusy(false);
}

void R18CookieLoginController::submitCookies(const QVariantMap& cookieValues)
{
    if (m_busy)
        return;

    // Assemble "name=value; name2=value2" cookie header from the provided map.
    QStringList parts;
    for (auto it = cookieValues.cbegin(); it != cookieValues.cend(); ++it)
    {
        const QString val = it.value().toString().trimmed();
        if (!val.isEmpty())
            parts << (it.key() + QLatin1Char('=') + val);
    }
    if (parts.isEmpty())
    {
        setStatus(QStringLiteral("failed"));
        setMessage(tr("请至少填写一个 Cookie 值"));
        return;
    }

    importSession(parts.join(QStringLiteral("; ")));
}

void R18CookieLoginController::cancel()
{
    setBusy(false);
    setStatus(QStringLiteral("canceled"));
    setMessage(tr("已取消"));
    emit completed(false);
}

void R18CookieLoginController::reset()
{
    setBusy(false);
    setStatus(QStringLiteral("idle"));
    setMessage({});
}

void R18CookieLoginController::importSession(const QString& cookieHeader)
{
    const QString url = m_gatewayUrl.trimmed().endsWith(QLatin1Char('/'))
        ? m_gatewayUrl + QStringLiteral("api/r18/account/session/import")
        : m_gatewayUrl + QStringLiteral("/api/r18/account/session/import");

    QJsonObject payload;
    payload[QLatin1String("source_id")] = m_sourceId;
    payload[QLatin1String("cookie_header")] = cookieHeader;

    QUrl qurl(url);
    QNetworkRequest request(qurl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    if (!m_authToken.isEmpty())
        request.setRawHeader("Authorization", ("Bearer " + m_authToken).toUtf8());
    memochat::r18::applyRequestOptions(request);

    setBusy(true);
    setStatus(QStringLiteral("importing"));
    setMessage(tr("正在导入 Cookie…"));

    auto* reply = m_network.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    memochat::r18::armTimeout(reply);

    connect(reply,
            &QNetworkReply::finished,
            this,
            [this, reply]()
            {
                reply->deleteLater();

                if (reply->error() != QNetworkReply::NoError)
                {
                    setStatus(QStringLiteral("failed"));
                    setMessage(tr("网络错误：%1").arg(reply->errorString()));
                    setBusy(false);
                    emit completed(false);
                    return;
                }

                const auto doc = QJsonDocument::fromJson(reply->readAll());
                const auto root = doc.object();

                if (root[QLatin1String("error")].toInt() != 0)
                {
                    setStatus(QStringLiteral("failed"));
                    setMessage(root[QLatin1String("message")].toString(tr("导入失败")));
                    setBusy(false);
                    emit completed(false);
                    return;
                }

                const auto data = root[QLatin1String("data")].toObject();
                const bool success = data[QLatin1String("success")].toBool(false);

                if (success)
                {
                    setStatus(QStringLiteral("authenticated"));
                    setMessage(tr("Cookie 导入成功"));
                    setBusy(false);
                    emit completed(true);
                }
                else
                {
                    setStatus(QStringLiteral("failed"));
                    setMessage(data[QLatin1String("message")].toString(tr("导入失败")));
                    setBusy(false);
                    emit completed(false);
                }
            });
}

void R18CookieLoginController::setStatus(const QString& s)
{
    if (m_status == s)
        return;
    m_status = s;
    emit statusChanged();
}

void R18CookieLoginController::setMessage(const QString& m)
{
    if (m_message == m)
        return;
    m_message = m;
    emit messageChanged();
}

void R18CookieLoginController::setBusy(bool b)
{
    if (m_busy == b)
        return;
    m_busy = b;
    emit busyChanged();
}
