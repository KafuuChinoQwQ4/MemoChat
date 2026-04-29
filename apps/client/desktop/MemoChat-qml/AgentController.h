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
    Q_PROPERTY(bool modelRefreshBusy READ modelRefreshBusy NOTIFY modelStateChanged)
    Q_PROPERTY(bool apiProviderBusy READ apiProviderBusy NOTIFY modelStateChanged)
    Q_PROPERTY(QString apiProviderStatus READ apiProviderStatus NOTIFY modelStateChanged)
    Q_PROPERTY(bool thinkingEnabled READ thinkingEnabled WRITE setThinkingEnabled NOTIFY thinkingChanged)
    Q_PROPERTY(bool currentModelSupportsThinking READ currentModelSupportsThinking NOTIFY thinkingChanged)
    Q_PROPERTY(QVariantList knowledgeBases READ knowledgeBases NOTIFY knowledgeBasesChanged)
    Q_PROPERTY(QString knowledgeSearchResult READ knowledgeSearchResult NOTIFY knowledgeSearchResultChanged)
    Q_PROPERTY(bool knowledgeBusy READ knowledgeBusy NOTIFY knowledgeStateChanged)
    Q_PROPERTY(QString knowledgeStatusText READ knowledgeStatusText NOTIFY knowledgeStateChanged)
    Q_PROPERTY(QString knowledgeError READ knowledgeError NOTIFY knowledgeStateChanged)
    Q_PROPERTY(QString currentTraceId READ currentTraceId NOTIFY traceChanged)
    Q_PROPERTY(QString currentSkill READ currentSkill NOTIFY traceChanged)
    Q_PROPERTY(QString currentFeedbackSummary READ currentFeedbackSummary NOTIFY traceChanged)
    Q_PROPERTY(QVariantList traceEvents READ traceEvents NOTIFY traceChanged)
    Q_PROPERTY(QVariantList traceObservations READ traceObservations NOTIFY traceChanged)
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
    bool modelRefreshBusy() const;
    bool apiProviderBusy() const;
    QString apiProviderStatus() const;
    bool thinkingEnabled() const;
    bool currentModelSupportsThinking() const;
    void setThinkingEnabled(bool enabled);
    QVariantList knowledgeBases() const;
    QString knowledgeSearchResult() const;
    bool knowledgeBusy() const;
    QString knowledgeStatusText() const;
    QString knowledgeError() const;
    QString currentTraceId() const;
    QString currentSkill() const;
    QString currentFeedbackSummary() const;
    QVariantList traceEvents() const;
    QVariantList traceObservations() const;
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
    Q_INVOKABLE void registerApiProvider(const QString& providerName, const QString& baseUrl, const QString& apiKey);

    Q_INVOKABLE void summarizeChat(const QString& dialogUid, const QString& chatHistoryJson);
    Q_INVOKABLE void suggestReply(const QString& dialogUid, const QString& chatHistoryJson);
    Q_INVOKABLE void translateMessage(const QString& msgContent, const QString& targetLang);

    Q_INVOKABLE void uploadDocument(const QString& filePath);
    Q_INVOKABLE void chooseAndUploadDocument();
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
    void knowledgeSearchResultChanged();
    void knowledgeStateChanged();
    void modelStateChanged();
    void traceChanged();
    void thinkingChanged();

private slots:
    void onHttpFinish(ReqId id, const QString& res, ErrorCodes err, Modules mod);
    void onStreamReadyRead();
    void onStreamFinished();

private:
    void handleChatRsp(ReqId id, const QString& res, ErrorCodes err, const QString& msgId);
    void handleSmartRsp(ReqId id, const QString& res, ErrorCodes err, const QString& reqType);
    void handleSessionRsp(ReqId id, const QString& res, ErrorCodes err, const QString& reqType);
    void handleHistoryRsp(ReqId id, const QString& res, ErrorCodes err);
    void handleModelListRsp(ReqId id, const QString& res, ErrorCodes err);
    void handleKbRsp(ReqId id, const QString& res, ErrorCodes err, const QString& reqType);
    void clearTrace();
    void updateTraceFromResponse(const QJsonObject& root);
    void setErrorState(const QString& error);
    void clearErrorState();
    void setKnowledgeBusy(bool busy, const QString& statusText = QString());
    void setKnowledgeError(const QString& error);
    void clearKnowledgeError();
    void setModelRefreshBusy(bool busy);
    void setApiProviderBusy(bool busy, const QString& statusText = QString());
    void clearCurrentSession();
    bool modelSupportsThinking(const QString& backend, const QString& modelName) const;
    QJsonObject buildChatMetadata() const;

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
    bool _model_refresh_busy = false;
    bool _api_provider_busy = false;
    QString _api_provider_status;
    bool _thinking_enabled = false;
    QVariantList _knowledge_bases;
    QString _knowledge_search_result;
    bool _knowledge_busy = false;
    QString _knowledge_status_text;
    QString _knowledge_error;
    QString _current_trace_id;
    QString _current_skill;
    QString _current_feedback_summary;
    QVariantList _trace_events;
    QVariantList _trace_observations;
    QMap<ReqId, QString> _pending_requests;

    // SSE 流式相关
    QNetworkAccessManager* _streamManager = nullptr;
    QNetworkReply* _currentStreamReply = nullptr;
    QString _streamBuffer;
    QString _currentStreamMsgId;
    QString _accumulatedContent;
    bool _streamFinalReceived = false;
    QString _pendingDeleteSessionId;
    bool _selectNewestSessionAfterList = false;
};

#endif  // AGENTCONTROLLER_H
