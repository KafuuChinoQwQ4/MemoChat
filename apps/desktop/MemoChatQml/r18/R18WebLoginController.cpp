#include "R18WebLoginController.hpp"

#include "core/network/GatewayClient.hpp"
#include "core/session/SessionManager.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QDebug>

namespace memochat
{

namespace
{
const QStringList kAllowedCookieNames = {"ipb_member_id", "ipb_pass_hash", "igneous", "sk"};
const QStringList kEhentaiDomains = {".e-hentai.org", "e-hentai.org", ".exhentai.org", "exhentai.org"};
} // namespace

R18WebLoginController::R18WebLoginController(QObject* parent)
    : QObject(parent)
{
    m_status = "idle";
}

R18WebLoginController::~R18WebLoginController()
{
    cleanup();
}

void R18WebLoginController::startLogin(const QString& sourceId)
{
    if (m_busy)
    {
        qWarning() << "R18WebLoginController: already in progress";
        return;
    }

    m_sourceId = sourceId;
    setStatus("waiting");
    setMessage("正在打开 E-Hentai 论坛登录页...");
    setBusy(true);

    // Create off-the-record profile
    cleanup();
    m_profile = new QQuickWebEngineProfile(this);
    m_profile->setOffTheRecord(true);
    m_profile->setHttpUserAgent("Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36");

    // Connect cookie store
    auto* cookieStore = m_profile->cookieStore();
    connect(cookieStore, &QWebEngineCookieStore::cookieAdded, this, &R18WebLoginController::onCookieAdded);

    emit profileChanged();

    // Start completeness check timer (check every 2 seconds)
    if (!m_completenessCheckTimer)
    {
        m_completenessCheckTimer = new QTimer(this);
        m_completenessCheckTimer->setInterval(2000);
        connect(m_completenessCheckTimer, &QTimer::timeout, this, &R18WebLoginController::checkCompleteness);
    }
    m_completenessCheckTimer->start();
}

void R18WebLoginController::cancel()
{
    setStatus("canceled");
    setMessage("已取消");
    setBusy(false);
    cleanup();
    emit completed(false);
}

void R18WebLoginController::onCookieAdded(const QNetworkCookie& cookie)
{
    if (!isEhentaiCookie(cookie))
        return;

    const QString name = cookie.name();
    const QString value = QString::fromUtf8(cookie.value());

    // Store cookie values (never log them)
    if (name == "ipb_member_id")
        m_ipb_member_id = value;
    else if (name == "ipb_pass_hash")
        m_ipb_pass_hash = value;
    else if (name == "igneous")
        m_igneous = value;
    else if (name == "sk")
        m_sk = value;
}

void R18WebLoginController::checkCompleteness()
{
    // Require at minimum ipb_member_id and ipb_pass_hash
    if (m_ipb_member_id.isEmpty() || m_ipb_pass_hash.isEmpty())
        return;

    if (m_status != "waiting")
        return;

    setStatus("validating");
    setMessage("Cookie 已收集，正在验证...");

    // Stop timer and import session
    if (m_completenessCheckTimer)
        m_completenessCheckTimer->stop();

    importSession();
}

void R18WebLoginController::importSession()
{
    auto* gateway = GatewayClient::instance();
    if (!gateway)
    {
        onImportFailure("GatewayClient not available");
        return;
    }

    QJsonObject cookies;
    cookies["ipb_member_id"] = m_ipb_member_id;
    cookies["ipb_pass_hash"] = m_ipb_pass_hash;
    cookies["igneous"] = m_igneous;
    cookies["sk"] = m_sk;

    QJsonObject payload;
    payload["source_id"] = m_sourceId;
    payload["cookies"] = cookies;

    auto* reply = gateway->post("/api/r18/account/session/import", payload);
    if (!reply)
    {
        onImportFailure("Failed to create network request");
        return;
    }

    connect(reply,
            &QNetworkReply::finished,
            this,
            [this, reply]()
            {
                reply->deleteLater();

                if (reply->error() != QNetworkReply::NoError)
                {
                    onImportFailure(reply->errorString());
                    return;
                }

                const QByteArray data = reply->readAll();
                const QJsonDocument doc = QJsonDocument::fromJson(data);
                const QJsonObject root = doc.object();

                if (root["error"].toInt() != 0)
                {
                    const QString message = root["message"].toString("导入失败");
                    onImportFailure(message);
                    return;
                }

                const QJsonObject dataObj = root["data"].toObject();
                const bool success = dataObj["success"].toBool(false);
                const QString message = dataObj["message"].toString();

                if (success)
                {
                    onImportSuccess();
                }
                else
                {
                    onImportFailure(message.isEmpty() ? "Session import failed" : message);
                }
            });
}

void R18WebLoginController::onImportSuccess()
{
    setStatus("authenticated");
    setMessage("E-Hentai 登录成功");
    setBusy(false);
    cleanup();
    emit completed(true);
}

void R18WebLoginController::onImportFailure(const QString& error)
{
    setStatus("failed");
    setMessage(error.isEmpty() ? "导入失败" : error);
    setBusy(false);
    cleanup();
    emit completed(false);
}

void R18WebLoginController::setStatus(const QString& status)
{
    if (m_status == status)
        return;
    m_status = status;
    emit statusChanged();
}

void R18WebLoginController::setMessage(const QString& message)
{
    if (m_message == message)
        return;
    m_message = message;
    emit messageChanged();
}

void R18WebLoginController::setBusy(bool busy)
{
    if (m_busy == busy)
        return;
    m_busy = busy;
    emit busyChanged();
}

void R18WebLoginController::cleanup()
{
    // Stop timer
    if (m_completenessCheckTimer)
    {
        m_completenessCheckTimer->stop();
    }

    // Clear cookie values (security: erase from memory)
    m_ipb_member_id.clear();
    m_ipb_pass_hash.clear();
    m_igneous.clear();
    m_sk.clear();

    // Delete profile (off-the-record, no persistent data)
    if (m_profile)
    {
        m_profile->deleteLater();
        m_profile = nullptr;
        emit profileChanged();
    }
}

bool R18WebLoginController::isEhentaiCookie(const QNetworkCookie& cookie) const
{
    const QString name = cookie.name();
    if (!kAllowedCookieNames.contains(name))
        return false;

    const QString domain = cookie.domain();
    for (const QString& ehDomain : kEhentaiDomains)
    {
        if (domain.contains(ehDomain, Qt::CaseInsensitive))
            return true;
    }

    return false;
}

} // namespace memochat
