#pragma once

#include "R18ControllerPrivate.h"

#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantMap>

/**
 * R18CookieLoginController — generic cookie-based login for nhentai / hanime1.
 *
 * The user fills in labeled cookie value fields in QML; QML calls
 * submitCookies(values) with a QVariantMap { "cookie_name": "value", ... }.
 * The controller assembles a cookie header and POSTs to /api/r18/account/session/import.
 *
 * Security: cookie values are never held in QML state — they are passed directly
 * to C++ and cleared from the QVariantMap immediately after the network call.
 */
class R18CookieLoginController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString message READ message NOTIFY messageChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    explicit R18CookieLoginController(QObject* parent = nullptr);

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

    Q_INVOKABLE void init(const QString& sourceId, const QString& gatewayUrl, const QString& authToken);
    Q_INVOKABLE void submitCookies(const QVariantMap& cookieValues);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void reset();

signals:
    void statusChanged();
    void messageChanged();
    void busyChanged();
    void completed(bool success);

private:
    void setStatus(const QString& s);
    void setMessage(const QString& m);
    void setBusy(bool b);
    void importSession(const QString& cookieHeader);

    QNetworkAccessManager m_network;
    QString m_sourceId;
    QString m_gatewayUrl;
    QString m_authToken;
    QString m_status;
    QString m_message;
    bool m_busy = false;
};
