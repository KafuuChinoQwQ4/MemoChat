#ifndef AGENTCONTROLLER_H
#define AGENTCONTROLLER_H

#include <QObject>
#include <QString>
#include <QJsonArray>
#include <QJsonObject>
#include <QVariantList>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include "global.h"
#include "AgentMessageModel.h"

class ClientGateway;

class AgentController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList sessions READ sessions NOTIFY sessionsChanged)
    Q_PROPERTY(AgentMessageModel* model READ model CONSTANT)
    Q_PROPERTY(QString currentSessionId READ currentSessionId NOTIFY sessionChanged)
    Q_PROPERTY(QString currentModel READ currentModel NOTIFY modelChanged)
    Q_PROPERTY(QVariantList availableModels READ availableModels NOTIFY modelsChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorOccurred)
    Q_PROPERTY(bool streaming READ streaming NOTIFY streamingChanged)

public:
    explicit AgentController(ClientGateway* gateway, QObject* parent = nullptr);
    ~AgentController();

    QVariantList sessions() const;
    AgentMessageModel* model() const;
    QString currentSessionId() const;
    QString currentModel() const;
    QVariantList availableModels() const;
    bool loading() const;
    QString error() const;
    bool streaming() const;

    Q_INVOKABLE void sendMessage(const QString& content);
    Q_INVOKABLE void sendStreamMessage(const QString& content);
    Q_INVOKABLE void createSession();
    Q_INVOKABLE void switchSession(const QString& sessionId);
    Q_INVOKABLE void deleteSession(const QString& sessionId);
    Q_INVOKABLE void loadHistory(const QString& sessionId);
    Q_INVOKABLE void loadSessions();
    Q_INVOKABLE void switchModel(const QString& backend, const QString& model);
    Q_INVOKABLE void refreshModelList();

    Q_INVOKABLE void summarizeChat(const QString& dialogUid, const QString& chatHistoryJson);
    Q_INVOKABLE void suggestReply(const QString& dialogUid, const QString& chatHistoryJson);
    Q_INVOKABLE void translateMessage(const QString& msgContent, const QString& targetLang);

    Q_INVOKABLE void uploadDocument(const QString& filePath);
    Q_INVOKABLE void searchKnowledgeBase(const QString& query);
    Q_INVOKABLE void listKnowledgeBases();
    Q_INVOKABLE void deleteKnowledgeBase(const QString& kbId);

    // 中止正在进行的流式请求
    Q_INVOKABLE void cancelStream();

signals:
    void sessionsChanged();
    void sessionChanged();
    void modelChanged();
    void modelsChanged();
    void loadingChanged();
    void streamingChanged();
    void errorOccurred(const QString& error);
    void aiResponseReceived(const QString& content);
    void streamingChunkReceived(const QString& msgId, const QString& chunk);
    void streamingFinished(const QString& msgId);
    void kbUploadProgress(int percent);
    void smartResultReady(const QString& featureType, const QString& result);
    void knowledgeBasesChanged();

private slots:
    void onHttpFinish(ReqId id, const QString& res, ErrorCodes err, Modules mod);
    void onStreamReadyRead();
    void onStreamFinished();

private:
    void handleChatRsp(ReqId id, const QString& res, ErrorCodes err);
    void handleSmartRsp(ReqId id, const QString& res, ErrorCodes err);
    void handleSessionRsp(ReqId id, const QString& res, ErrorCodes err);
    void handleHistoryRsp(ReqId id, const QString& res, ErrorCodes err);
    void handleModelListRsp(ReqId id, const QString& res, ErrorCodes err);
    void handleKbRsp(ReqId id, const QString& res, ErrorCodes err);

    // SSE 流式处理辅助
    void parseSSEChunk(const QString& line);
    void finishStream(const QString& msgId, const QString& finalContent);

    ClientGateway* _gateway;
    AgentMessageModel* _model;
    QString _current_session_id;
    QString _current_model_backend;
    QString _current_model_name;
    bool _loading = false;
    bool _streaming = false;
    QString _error;
    QVariantList _sessions;
    QVariantList _available_models;
    QMap<ReqId, QString> _pending_requests;

    // SSE 流式相关
    QNetworkAccessManager* _streamManager = nullptr;
    QNetworkReply* _currentStreamReply = nullptr;
    QString _streamBuffer;
    QString _currentStreamMsgId;
    QString _accumulatedContent;
};

#endif  // AGENTCONTROLLER_H
