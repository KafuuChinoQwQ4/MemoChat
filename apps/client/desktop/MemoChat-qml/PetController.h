#ifndef PETCONTROLLER_H
#define PETCONTROLLER_H

#include "PetModel.h"

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QPointer>
#include <QProcess>
#include <QSet>
#include <QStringList>
#include <QVariantMap>

#if HAVE_QT_MULTIMEDIA
class QVideoFrame;
#endif

class ClientGateway;

class PetController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(PetModel* model READ model CONSTANT)
    Q_PROPERTY(QString sessionId READ sessionId NOTIFY stateChanged)
    Q_PROPERTY(bool streaming READ streaming NOTIFY stateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(QString error READ error NOTIFY stateChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY stateChanged)
    Q_PROPERTY(int schemaVersion READ schemaVersion NOTIFY petStateChanged)
    Q_PROPERTY(QString eventId READ eventId NOTIFY petStateChanged)
    Q_PROPERTY(QString turnId READ turnId NOTIFY petStateChanged)
    Q_PROPERTY(QString phase READ phase NOTIFY petStateChanged)
    Q_PROPERTY(QString speechText READ speechText NOTIFY petStateChanged)
    Q_PROPERTY(QString speechTranslation READ speechTranslation NOTIFY petStateChanged)
    Q_PROPERTY(QString speechDisplayText READ speechDisplayText NOTIFY petStateChanged)
    Q_PROPERTY(QString speechLanguage READ speechLanguage NOTIFY petStateChanged)
    Q_PROPERTY(bool speechFinal READ speechFinal NOTIFY petStateChanged)
    Q_PROPERTY(QString audioUrl READ audioUrl NOTIFY petStateChanged)
    Q_PROPERTY(QString audioState READ audioState NOTIFY petStateChanged)
    Q_PROPERTY(QString emotion READ emotion NOTIFY petStateChanged)
    Q_PROPERTY(QString expression READ expression NOTIFY petStateChanged)
    Q_PROPERTY(QString motion READ motion NOTIFY petStateChanged)
    Q_PROPERTY(qreal intensity READ intensity NOTIFY petStateChanged)
    Q_PROPERTY(qreal gazeX READ gazeX NOTIFY petStateChanged)
    Q_PROPERTY(qreal gazeY READ gazeY NOTIFY petStateChanged)
    Q_PROPERTY(qreal lipSyncValue READ lipSyncValue NOTIFY petStateChanged)
    Q_PROPERTY(bool voiceTrainingBusy READ voiceTrainingBusy NOTIFY voiceTrainingChanged)
    Q_PROPERTY(QString voiceTrainingJobId READ voiceTrainingJobId NOTIFY voiceTrainingChanged)
    Q_PROPERTY(QString voiceTrainingStatus READ voiceTrainingStatus NOTIFY voiceTrainingChanged)
    Q_PROPERTY(QString voiceTrainingStage READ voiceTrainingStage NOTIFY voiceTrainingChanged)
    Q_PROPERTY(int voiceTrainingProgress READ voiceTrainingProgress NOTIFY voiceTrainingChanged)
    Q_PROPERTY(QString voiceTrainingArtifactPath READ voiceTrainingArtifactPath NOTIFY voiceTrainingChanged)
    Q_PROPERTY(QString voiceTrainingMessage READ voiceTrainingMessage NOTIFY voiceTrainingChanged)
    Q_PROPERTY(bool windowsImeBridgeAvailable READ windowsImeBridgeAvailable NOTIFY windowsImeBridgeChanged)
    Q_PROPERTY(bool windowsImeBridgeBusy READ windowsImeBridgeBusy NOTIFY windowsImeBridgeChanged)
    Q_PROPERTY(bool windowsCameraBridgeAvailable READ windowsCameraBridgeAvailable NOTIFY windowsCameraBridgeChanged)
    Q_PROPERTY(bool windowsCameraBridgeBusy READ windowsCameraBridgeBusy NOTIFY windowsCameraBridgeChanged)
    Q_PROPERTY(QString selectedModelType READ selectedModelType NOTIFY modelSelectionChanged)
    Q_PROPERTY(QString selectedModelName READ selectedModelName NOTIFY modelSelectionChanged)
    Q_PROPERTY(QString replyLanguage READ replyLanguage NOTIFY replyLanguageChanged)
    Q_PROPERTY(QString speechRules READ speechRules NOTIFY speechRulesChanged)

public:
    explicit PetController(ClientGateway *gateway, QObject *parent = nullptr);
    ~PetController() override;

    PetModel *model() { return &_model; }
    QString sessionId() const { return _session_id; }
    bool streaming() const { return _streaming; }
    bool busy() const { return _busy; }
    QString error() const { return _error; }
    QString statusText() const { return _status_text; }
    int schemaVersion() const { return _model.schemaVersion(); }
    QString eventId() const { return _model.eventId(); }
    QString turnId() const { return _model.turnId(); }
    QString phase() const { return _model.phase(); }
    QString speechText() const { return _model.speechText(); }
    QString speechTranslation() const { return _model.speechTranslation(); }
    QString speechDisplayText() const { return _model.speechDisplayText(); }
    QString speechLanguage() const { return _model.speechLanguage(); }
    bool speechFinal() const { return _model.speechFinal(); }
    QString audioUrl() const;
    QString audioState() const { return _model.audioState(); }
    QString emotion() const { return _model.emotion(); }
    QString expression() const { return _model.expression(); }
    QString motion() const { return _model.motion(); }
    qreal intensity() const { return _model.intensity(); }
    qreal gazeX() const { return _model.gazeX(); }
    qreal gazeY() const { return _model.gazeY(); }
    qreal lipSyncValue() const { return _model.lipSyncValue(); }
    bool voiceTrainingBusy() const { return _voice_training_busy; }
    QString voiceTrainingJobId() const { return _voice_training_job_id; }
    QString voiceTrainingStatus() const { return _voice_training_status; }
    QString voiceTrainingStage() const { return _voice_training_stage; }
    int voiceTrainingProgress() const { return _voice_training_progress; }
    QString voiceTrainingArtifactPath() const { return _voice_training_artifact_path; }
    QString voiceTrainingMessage() const { return _voice_training_message; }
    bool windowsImeBridgeAvailable() const;
    bool windowsImeBridgeBusy() const { return _windows_ime_bridge_busy; }
    bool windowsCameraBridgeAvailable() const;
    bool windowsCameraBridgeBusy() const { return _windows_camera_bridge_busy; }
    QString selectedModelType() const { return _selected_model_type; }
    QString selectedModelName() const { return _selected_model_name; }
    QString replyLanguage() const { return _reply_language; }
    QString speechRules() const { return _speech_rules; }

    Q_INVOKABLE void startSession();
    Q_INVOKABLE void sendText(const QString &text);
    Q_INVOKABLE void sendObservation(const QVariantMap &observation);
    Q_INVOKABLE void captureVisionFrame(const QString &frameBase64,
                                         const QString &frameMime = QStringLiteral("image/jpeg"),
                                         int frameWidth = 0,
                                         int frameHeight = 0);
#if HAVE_QT_MULTIMEDIA
    Q_INVOKABLE bool captureVisionVideoFrame(const QVideoFrame &frame,
                                             int frameWidth = 0,
                                             int frameHeight = 0);
#endif
    Q_INVOKABLE QString nextVisionCaptureFilePath() const;
    Q_INVOKABLE void captureVisionFrameFile(const QString &filePath, int frameWidth = 0, int frameHeight = 0);
    Q_INVOKABLE void interrupt();
    Q_INVOKABLE void stopStream();
    Q_INVOKABLE void clearSpeech();
    Q_INVOKABLE void startVoiceTraining(const QVariantMap &request);
    Q_INVOKABLE void refreshVoiceTrainingJob(const QString &jobId = QString());
    Q_INVOKABLE void openWindowsImeBridge(const QString &initialText = QString());
    Q_INVOKABLE bool captureVisionWindowsCameraFrame();
    Q_INVOKABLE void setModelSelection(const QString &modelType, const QString &modelName);
    Q_INVOKABLE void setReplyLanguage(const QString &language);
    Q_INVOKABLE void setSpeechRules(const QString &rules);

signals:
    void stateChanged();
    void petStateChanged();
    void voiceTrainingChanged();
    void controlEventReceived(const QVariantMap &event);
    void windowsImeBridgeChanged();
    void windowsCameraBridgeChanged();
    void windowsImeTextCommitted(const QString &text);
    void modelSelectionChanged();
    void replyLanguageChanged();
    void speechRulesChanged();

private:
    void postJson(const QUrl &url, const QJsonObject &payload, const QString &op);
    void getJson(const QUrl &url, const QString &op);
    void startStream();
    void handleJsonReply(QNetworkReply *reply);
    void applyControlEvent(const QJsonObject &event);
    bool rememberControlEvent(const QJsonObject &event);
    void resetControlEventDedupe();
    void consumeStreamBytes(const QByteArray &bytes);
    void consumeSseLine(const QByteArray &line);
    void setBusy(bool busy, const QString &statusText = QString());
    void setVoiceTrainingBusy(bool busy, const QString &message = QString());
    void postVisionCapture(const QString &frameBase64,
                           const QString &frameMime,
                           int frameWidth,
                           int frameHeight,
                           const QString &source,
                           const QString &transport);
    void captureVisionFrameFileWithMetadata(const QString &filePath,
                                            int frameWidth,
                                            int frameHeight,
                                            const QString &source,
                                            const QString &transport);
    void applyVoiceTrainingJob(const QJsonObject &job);
    void setWindowsImeBridgeBusy(bool busy);
    void setWindowsCameraBridgeBusy(bool busy);
    void setStatusText(const QString &statusText);
    void setError(const QString &error);
    QJsonObject authPayload() const;
    QJsonObject defaultObservationPayload() const;
    QUrl petUrl(const QString &path) const;
    QString encodedSessionId() const;

    ClientGateway *_gateway = nullptr;
    PetModel _model;
    QNetworkAccessManager _network;
    QNetworkAccessManager _stream_network;
    QPointer<QNetworkReply> _stream_reply;
    QByteArray _stream_buffer;
    QString _session_id;
    bool _streaming = false;
    bool _busy = false;
    bool _input_request_in_flight = false;
    bool _voice_training_busy = false;
    bool _windows_ime_bridge_busy = false;
    bool _windows_camera_bridge_busy = false;
    QPointer<QProcess> _windows_ime_bridge_process;
    QPointer<QProcess> _windows_camera_bridge_process;
    QString _voice_training_job_id;
    QString _voice_training_status = QStringLiteral("idle");
    QString _voice_training_stage;
    int _voice_training_progress = 0;
    QString _voice_training_artifact_path;
    QString _voice_training_message;
    QString _error;
    QString _status_text;
    QString _selected_model_type;
    QString _selected_model_name;
    QString _reply_language = QStringLiteral("zh-CN");
    QString _speech_rules;
    QSet<QString> _applied_control_event_keys;
    QStringList _applied_control_event_order;
};

#endif // PETCONTROLLER_H
