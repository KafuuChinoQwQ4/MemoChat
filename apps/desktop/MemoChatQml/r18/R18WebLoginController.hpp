#pragma once

#include <QObject>
#include <QPointer>
#include <QString>
#include <QTimer>
#include <QWebEngineCookieStore>
#include <QQuickWebEngineProfile>
#include <QNetworkCookie>

class QQmlApplicationEngine;

namespace memochat
{

/**
 * R18WebLoginController — Qt WebEngine-based E-Hentai login controller.
 *
 * Creates an off-the-record profile, observes E-Hentai session cookies in C++
 * (never exposed to QML), and calls /api/r18/account/session/import once collected.
 *
 * Security:
 *  - Off-the-record profile (no persistent browser data)
 *  - Cookie filtering in C++ slot (QML never sees cookie values)
 *  - Automatic profile cleanup on completion/failure/cancellation
 *  - Navigation restricted to E-Hentai/ExHentai/Cloudflare domains
 */
class R18WebLoginController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickWebEngineProfile* profile READ profile NOTIFY profileChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString message READ message NOTIFY messageChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    explicit R18WebLoginController(QObject* parent = nullptr);
    ~R18WebLoginController() override;

    QQuickWebEngineProfile* profile() const
    {
        return m_profile;
    }
    QString status() const
    {
        return m_status;
    }
    QString message() const
    {
        return m_message;
    }
    bool busy() const
    {
        return m_busy;
    }

    Q_INVOKABLE void startLogin(const QString& sourceId);
    Q_INVOKABLE void cancel();

signals:
    void profileChanged();
    void statusChanged();
    void messageChanged();
    void busyChanged();
    void completed(bool success);

private slots:
    void onCookieAdded(const QNetworkCookie& cookie);
    void onImportSuccess();
    void onImportFailure(const QString& error);

private:
    void setStatus(const QString& status);
    void setMessage(const QString& message);
    void setBusy(bool busy);
    void checkCompleteness();
    void importSession();
    void cleanup();
    bool isEhentaiCookie(const QNetworkCookie& cookie) const;

    QPointer<QQuickWebEngineProfile> m_profile;
    QString m_sourceId;
    QString m_status; // "idle", "waiting", "validating", "authenticated", "failed"
    QString m_message;
    bool m_busy = false;

    // Collected cookies (never exposed to QML)
    QString m_ipb_member_id;
    QString m_ipb_pass_hash;
    QString m_igneous;
    QString m_sk;

    QTimer* m_completenessCheckTimer = nullptr;
};

} // namespace memochat
