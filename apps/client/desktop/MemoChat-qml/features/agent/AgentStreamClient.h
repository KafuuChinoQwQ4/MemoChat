#ifndef AGENTSTREAMCLIENT_H
#define AGENTSTREAMCLIENT_H

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QString>
#include <QUrl>

class AgentStreamClient : public QObject
{
    Q_OBJECT

public:
    explicit AgentStreamClient(QObject* parent = nullptr);
    ~AgentStreamClient() override;

    void start(const QUrl& url, const QJsonObject& payload);
    void cancel();
    bool isRunning() const;

signals:
    void chunkReceived(const QJsonObject& chunk);
    void finalReceived(const QJsonObject& chunk);
    void errorOccurred(int networkError, const QString& errorString);
    void finished(int networkError, const QString& errorString);

private:
    void onReadyRead();
    void onFinished();
    void consumeLine(const QString& line);

    QNetworkAccessManager* _network = nullptr;
    QNetworkReply* _reply = nullptr;
    QString _buffer;
};

#endif // AGENTSTREAMCLIENT_H
