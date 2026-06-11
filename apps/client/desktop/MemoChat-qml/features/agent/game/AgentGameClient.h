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

    void get(const QUrl& url, const QString& op, const QString& statusText, int uid);
    void post(const QUrl& url, const QJsonObject& payload, const QString& op, const QString& statusText, int uid);
    void deleteResource(const QUrl& url, const QString& op, const QString& statusText, int uid);

signals:
    void responseReady(const QString& op, const QJsonObject& root, int uid);
    void networkError(const QString& op, const QString& errorText, int uid);
    void formatError(const QString& op, int uid);

private:
    enum class Verb
    {
        Get,
        Post,
        Delete
    };

    void send(Verb verb, const QUrl& url, const QJsonObject& payload, const QString& op, int uid);
    void handleReply(QNetworkReply* reply, const QString& op, int uid);

    QNetworkAccessManager* _network = nullptr;
};

#endif // AGENTGAMECLIENT_H
