#pragma once

#include "R18ControllerPrivate.h"

#include <QNetworkAccessManager>
#include <QNetworkCookie>
#include <QObject>
#include <QPointer>
#include <QQuickWebEngineProfile>
#include <QString>
#include <QTimer>

/**
 * R18WebLoginController — off-the-record WebEngine session login.
 *
 * Creates an off-the-record QQuickWebEngineProfile, observes E-Hentai cookies
 * in C++ via QWebEngineCookieStore::cookieAdded, and calls the backend
 * /api/r18/account/session/import once the required cookies are present.
 *
 * Security guarantees:
 *  - Cookie values never exposed to QML (only opaque status string).
 *  - Off-the-record profile: no persistent disk data.
 *  - Navigation policy enforced in QML (E-Hentai / Cloudflare domains only).
 *  - Profile destroyed on every terminal path (success/failure/cancel).
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

    QQuickWebEngineProfile* profile() const;
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

    Q_INVOKABLE void startLogin(const QString& sourceId, const QString& gatewayUrl, const QString& authToken);
    Q_INVOKABLE void cancel();

signals:
    void profileChanged();
    void statusChanged();
    void messageChanged();
    void busyChanged();
    void completed(bool success);

private slots:
    void onCookieAdded(const QNetworkCookie& cookie);

private:
    void setStatus(const QString& s);
    void setMessage(const QString& m);
    void setBusy(bool b);
    void checkAndImport();
    void importSession();
    void cleanup();

    static bool isAllowedEhentaiCookie(const QNetworkCookie& c);

    QPointer<QQuickWebEngineProfile> m_profile;
    QNetworkAccessManager m_network;
    QString m_sourceId;
    QString m_gatewayUrl;
    QString m_authToken;
    QString m_status;
    QString m_message;
    bool m_busy = false;
    bool m_importing = false;

    // Collected cookies — never exposed to QML.
    QString m_ipb_member_id;
    QString m_ipb_pass_hash;
    QString m_igneous;
    QString m_sk;

    QTimer m_collectTimer; // short delay after ipb_* appear so igneous can arrive
};
