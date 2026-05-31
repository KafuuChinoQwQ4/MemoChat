#pragma once

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QUrlQuery>
#include <functional>

class OpsApiTransport : public QObject
{
    Q_OBJECT

public:
    using JsonHandler = std::function<void(const QJsonObject&)>;

    explicit OpsApiTransport(QString baseUrl, QObject* parent = nullptr);

    void getJson(const QString& path, const JsonHandler& onObject);
    void postJson(const QString& path, const JsonHandler& onObject);
    void postJsonWithQuery(const QString& path, const QUrlQuery& query, const JsonHandler& onObject);

signals:
    void requestStarted();
    void requestFinished();
    void requestSucceeded();
    void requestFailed(const QString& message);

private:
    void postJsonBody(const QString& path, const QUrlQuery& query, const JsonHandler& onObject);
    QString urlForPath(const QString& path, const QUrlQuery& query = QUrlQuery()) const;
    void handleReply(QNetworkReply* reply, const JsonHandler& onObject);

    QString m_baseUrl;
    QNetworkAccessManager m_network;
};
