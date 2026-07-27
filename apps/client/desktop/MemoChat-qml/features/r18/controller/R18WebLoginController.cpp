#include "R18WebLoginController.h"

#include "R18ControllerPrivate.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QWebEngineCookieStore>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QQuickWebEngineProfile>
#else
#include <QtWebEngineWidgets/QWebEngineProfile>
using QQuickWebEngineProfile = QWebEngineProfile;
#endif

namespace
{
const QStringList kAllowedNames = {"ipb_member_id", "ipb_pass_hash", "igneous", "sk"};
const QStringList kAllowedDomains = {".e-hentai.org", "e-hentai.org", ".exhentai.org", "exhentai.org"};
constexpr int kCollectDelayMs = 2500; // wait briefly for igneous after ipb_* appear
} // namespace

R18WebLoginController::R18WebLoginController(QObject* parent)
    : QObject(parent)
{
    m_status = QStringLiteral("idle");
    m_message = {};
    m_collectTimer.setSingleShot(true);
    m_collectTimer.setInterval(kCollectDelayMs);
    connect(&m_collectTimer, &QTimer::timeout, this, &R18WebLoginController::checkAndImport);
}

R18WebLoginController::~R18WebLoginController()
{
    cleanup();
}

QQuickWebEngineProfile* R18WebLoginController::profile() const
{
    return m_profile.data();
}

void R18WebLoginController::startLogin(const QString& sourceId, const QString& gatewayUrl, const QString& authToken)
{
    if (m_busy)
        return;

    cleanup();

    m_sourceId = sourceId;
    m_gatewayUrl = gatewayUrl;
    m_authToken = authToken;
    m_importing = false;

    setStatus(QStringLiteral("waiting"));
    setMessage(tr("请在打开的 E-Hentai 论坛页面完成登录…"));
    setBusy(true);

    // Off-the-record profile — no persistent cookies or cache.
    auto* p = new QQuickWebEngineProfile(this);
    p->setOffTheRecord(true);

    auto* cookieStore = p->cookieStore();
    connect(cookieStore, &QWebEngineCookieStore::cookieAdded, this, &R18WebLoginController::onCookieAdded);

    m_profile = p;
    emit profileChanged();
}

void R18WebLoginController::cancel()
{
    m_collectTimer.stop();
    setStatus(QStringLiteral("canceled"));
    setMessage(tr("已取消"));
    setBusy(false);
    emit completed(false);
    cleanup();
}

void R18WebLoginController::onCookieAdded(const QNetworkCookie& cookie)
{
    if (!isAllowedEhentaiCookie(cookie))
        return;

    const QString name = QString::fromUtf8(cookie.name());
    const QString value = QString::fromUtf8(cookie.value());

    if (name == QLatin1String("ipb_member_id"))
        m_ipb_member_id = value;
    else if (name == QLatin1String("ipb_pass_hash"))
        m_ipb_pass_hash = value;
    else if (name == QLatin1String("igneous"))
        m_igneous = value;
    else if (name == QLatin1String("sk"))
        m_sk = value;

    // Arm short collection window once we have the mandatory pair.
    if (!m_ipb_member_id.isEmpty() && !m_ipb_pass_hash.isEmpty() && !m_importing)
    {
        if (!m_collectTimer.isActive())
        {
            setStatus(QStringLiteral("collecting"));
            setMessage(tr("Cookie 已收集，等待 igneous…"));
            m_collectTimer.start();
        }
    }
}

void R18WebLoginController::checkAndImport()
{
    if (m_importing || m_ipb_member_id.isEmpty() || m_ipb_pass_hash.isEmpty())
        return;

    m_importing = true;
    setStatus(QStringLiteral("validating"));
    setMessage(tr("正在向 MemoChat 提交 Cookie…"));
    importSession();
}

void R18WebLoginController::importSession()
{
    const QString url = m_gatewayUrl.trimmed().endsWith(QLatin1Char('/'))
        ? m_gatewayUrl + QStringLiteral("api/r18/account/session/import")
        : m_gatewayUrl + QStringLiteral("/api/r18/account/session/import");

    QJsonObject cookies;
    cookies[QLatin1String("ipb_member_id")] = m_ipb_member_id;
    cookies[QLatin1String("ipb_pass_hash")] = m_ipb_pass_hash;
    cookies[QLatin1String("igneous")] = m_igneous;
    cookies[QLatin1String("sk")] = m_sk;

    QJsonObject payload;
    payload[QLatin1String("source_id")] = m_sourceId;
    payload[QLatin1String("cookies")] = cookies;

    QUrl qurl(url);
    QNetworkRequest request(qurl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    if (!m_authToken.isEmpty())
        request.setRawHeader("Authorization", ("Bearer " + m_authToken).toUtf8());
    memochat::r18::applyRequestOptions(request);

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
                    cleanup();
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
                    cleanup();
                    return;
                }

                const auto data = root[QLatin1String("data")].toObject();
                const bool success = data[QLatin1String("success")].toBool(false);

                if (success)
                {
                    setStatus(QStringLiteral("authenticated"));
                    setMessage(tr("E-Hentai 登录成功"));
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
                cleanup();
            });
}

bool R18WebLoginController::isAllowedEhentaiCookie(const QNetworkCookie& c)
{
    const QString name = QString::fromUtf8(c.name());
    const QString domain = c.domain();
    if (!kAllowedNames.contains(name))
        return false;
    for (const QString& d : kAllowedDomains)
        if (domain.contains(d, Qt::CaseInsensitive))
            return true;
    return false;
}

void R18WebLoginController::setStatus(const QString& s)
{
    if (m_status == s)
        return;
    m_status = s;
    emit statusChanged();
}

void R18WebLoginController::setMessage(const QString& m)
{
    if (m_message == m)
        return;
    m_message = m;
    emit messageChanged();
}

void R18WebLoginController::setBusy(bool b)
{
    if (m_busy == b)
        return;
    m_busy = b;
    emit busyChanged();
}

void R18WebLoginController::cleanup()
{
    m_collectTimer.stop();
    // Erase cookie values from memory.
    m_ipb_member_id.clear();
    m_ipb_pass_hash.clear();
    m_igneous.clear();
    m_sk.clear();
    m_authToken.clear();

    if (!m_profile.isNull())
    {
        m_profile->deleteLater();
        m_profile = nullptr;
        emit profileChanged();
    }
}
