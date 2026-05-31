#ifndef AGENTGAMECLIENT_H
#define AGENTGAMECLIENT_H

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QUrl>

class AgentGameClient : public QObject
{
    Q_OBJECT

public:
    explicit AgentGameClient(QObject* parent = nullptr);

    void get(const QUrl& url, const QString& op, const QString& statusText);
    void post(const QUrl& url, const QJsonObject& payload, const QString& op, const QString& statusText);
    void deleteResource(const QUrl& url, const QString& op, const QString& statusText);

signals:
    void responseReady(const QString& op, const QJsonObject& root);
    void networkError(const QString& op, const QString& errorText);
    void formatError(const QString& op);

private:
    enum class Verb
    {
        Get,
        Post,
        Delete
    };

    void send(Verb verb, const QUrl& url, const QJsonObject& payload, const QString& op);
    void handleReply(QNetworkReply* reply, const QString& op);

    QNetworkAccessManager* _network = nullptr;
};

#endif // AGENTGAMECLIENT_H
