#include "PetController.h"

#include "ClientGateway.h"
#include "global.h"
#include "usermgr.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkRequest>
#include <QProcess>
#include <QStandardPaths>
#include <QUrl>
#include <QUrlQuery>
#include <QDateTime>
#include <QtGlobal>

namespace {
constexpr int kMaxRememberedPetControlEvents = 256;

bool isWslRuntime()
{
#ifdef Q_OS_LINUX
    QFile file(QStringLiteral("/proc/sys/kernel/osrelease"));
    if (file.open(QIODevice::ReadOnly)) {
        const QString release = QString::fromLatin1(file.readAll()).toLower();
        return release.contains(QStringLiteral("microsoft")) || release.contains(QStringLiteral("wsl"));
    }
#endif
    return false;
}

QString windowsPowerShellPath()
{
    const QString fromPath = QStandardPaths::findExecutable(QStringLiteral("powershell.exe"));
    if (!fromPath.isEmpty()) {
        return fromPath;
    }
    const QString fallback = QStringLiteral("/mnt/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe");
    return QFileInfo::exists(fallback) ? fallback : QString();
}

QString powershellEncodedCommand(const QString &script)
{
    const auto *utf16 = reinterpret_cast<const char *>(script.utf16());
    const QByteArray bytes(utf16, script.size() * int(sizeof(ushort)));
    return QString::fromLatin1(bytes.toBase64());
}

QString windowsImeBridgeScript(const QString &initialText)
{
    const QString initialBase64 = QString::fromLatin1(initialText.toUtf8().toBase64());
    return QString::fromUtf8(R"PS(
$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

$initialText = [System.Text.Encoding]::UTF8.GetString([System.Convert]::FromBase64String('%1'))
$form = New-Object System.Windows.Forms.Form
$form.Text = 'MemoChat Chinese Input'
$form.StartPosition = 'CenterScreen'
$form.Width = 560
$form.Height = 180
$form.TopMost = $true
$form.FormBorderStyle = [System.Windows.Forms.FormBorderStyle]::FixedDialog
$form.MaximizeBox = $false
$form.MinimizeBox = $false

$textBox = New-Object System.Windows.Forms.TextBox
$textBox.Multiline = $true
$textBox.AcceptsReturn = $true
$textBox.AcceptsTab = $false
$textBox.ImeMode = [System.Windows.Forms.ImeMode]::On
$textBox.Font = New-Object System.Drawing.Font('Microsoft YaHei UI', 12)
$textBox.Left = 12
$textBox.Top = 12
$textBox.Width = 518
$textBox.Height = 76
$textBox.Text = $initialText

$okButton = New-Object System.Windows.Forms.Button
$okButton.Text = 'OK'
$okButton.Left = 354
$okButton.Top = 100
$okButton.Width = 82
$okButton.Height = 30
$okButton.DialogResult = [System.Windows.Forms.DialogResult]::OK

$cancelButton = New-Object System.Windows.Forms.Button
$cancelButton.Text = 'Cancel'
$cancelButton.Left = 448
$cancelButton.Top = 100
$cancelButton.Width = 82
$cancelButton.Height = 30
$cancelButton.DialogResult = [System.Windows.Forms.DialogResult]::Cancel

$form.Controls.AddRange(@($textBox, $okButton, $cancelButton))
$form.AcceptButton = $okButton
$form.CancelButton = $cancelButton
$form.Add_Shown({ $textBox.Focus(); $textBox.Select($textBox.TextLength, 0) })

if ($form.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK) {
    [Console]::Out.Write($textBox.Text)
}
)PS").arg(initialBase64);
}

}

PetController::PetController(ClientGateway *gateway, QObject *parent)
    : QObject(parent)
    , _gateway(gateway)
{
    connect(&_model, &PetModel::changed, this, &PetController::petStateChanged);
}

PetController::~PetController()
{
    stopStream();
}

QString PetController::audioUrl() const
{
    const QString raw = _model.audioUrl().trimmed();
    if (raw.isEmpty()) {
        return {};
    }
    const QUrl parsed(raw);
    if (parsed.isValid() && !parsed.isRelative()) {
        return raw;
    }
    const QString path = raw.startsWith(QLatin1Char('/')) ? raw : QStringLiteral("/%1").arg(raw);
    QUrl url = petUrl(path);
    QUrlQuery query(url);
    if (!_model.turnId().isEmpty()) {
        query.addQueryItem(QStringLiteral("turn"), _model.turnId());
    }
    if (!_model.eventId().isEmpty()) {
        query.addQueryItem(QStringLiteral("event"), _model.eventId());
    }
    url.setQuery(query);
    return url.toString();
}

void PetController::startSession()
{
    if (_busy) {
        return;
    }
    resetControlEventDedupe();
    setError({});
    setBusy(true, QStringLiteral("正在连接桌宠"));
    QJsonObject payload = authPayload();
    payload[QStringLiteral("profile_id")] = QStringLiteral("default");
    payload[QStringLiteral("persona")] = QStringLiteral("memo-pet");
    payload[QStringLiteral("provider")] = QStringLiteral("scripted");
    postJson(petUrl(QStringLiteral("/sessions")), payload, QStringLiteral("create_session"));
}

void PetController::sendText(const QString &text)
{
    const QString content = text.trimmed();
    if (content.isEmpty()) {
        return;
    }
    if (_input_request_in_flight) {
        setStatusText(QStringLiteral("上一条消息仍在发送"));
        return;
    }
    if (_session_id.isEmpty()) {
        setError(QStringLiteral("桌宠会话未连接"));
        return;
    }

    _model.clearSpeech();
    _input_request_in_flight = true;
    setBusy(true, QStringLiteral("正在发送"));
    QJsonObject payload = authPayload();
    payload[QStringLiteral("content")] = content;
    if (!_selected_model_type.trimmed().isEmpty()) {
        payload[QStringLiteral("model_type")] = _selected_model_type.trimmed();
    }
    if (!_selected_model_name.trimmed().isEmpty()) {
        payload[QStringLiteral("model_name")] = _selected_model_name.trimmed();
    }
    QJsonObject metadata;
    metadata[QStringLiteral("reply_language")] = _reply_language.trimmed().isEmpty()
                                                    ? QStringLiteral("zh-CN")
                                                    : _reply_language.trimmed();
    payload[QStringLiteral("metadata")] = metadata;
    postJson(petUrl(QStringLiteral("/sessions/%1/input").arg(encodedSessionId())),
             payload,
             QStringLiteral("input"));
}

void PetController::sendObservation(const QVariantMap &observation)
{
    if (_session_id.isEmpty()) {
        return;
    }
    QJsonObject payload = QJsonObject::fromVariantMap(observation);
    if (payload.isEmpty()) {
        payload = defaultObservationPayload();
    }
    postJson(petUrl(QStringLiteral("/sessions/%1/observation").arg(encodedSessionId())),
             payload,
             QStringLiteral("observation"));
}

void PetController::captureVisionFrame(const QString &frameBase64,
                                       const QString &frameMime,
                                       int frameWidth,
                                       int frameHeight)
{
    const QString encodedFrame = frameBase64.trimmed();
    if (_session_id.isEmpty() || encodedFrame.isEmpty()) {
        return;
    }

    QJsonObject metadata;
    metadata[QStringLiteral("source")] = QStringLiteral("qt_camera");
    metadata[QStringLiteral("transport")] = QStringLiteral("local_frame_upload");

    QJsonObject payload;
    payload[QStringLiteral("analyzer")] = QStringLiteral("opencv");
    payload[QStringLiteral("include_frame")] = false;
    payload[QStringLiteral("frame_base64")] = encodedFrame;
    payload[QStringLiteral("frame_mime")] = frameMime.trimmed().isEmpty()
                                                ? QStringLiteral("image/jpeg")
                                                : frameMime.trimmed();
    payload[QStringLiteral("frame_width")] = qMax(0, frameWidth);
    payload[QStringLiteral("frame_height")] = qMax(0, frameHeight);
    payload[QStringLiteral("metadata")] = metadata;

    postJson(petUrl(QStringLiteral("/sessions/%1/capture").arg(encodedSessionId())),
             payload,
             QStringLiteral("vision_capture"));
}

QString PetController::nextVisionCaptureFilePath() const
{
    QString root = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (root.trimmed().isEmpty()) {
        root = QDir::tempPath();
    }
    const QString dir = QDir(root).filePath(QStringLiteral("pet-vision"));
    QDir().mkpath(dir);
    return QDir(dir).filePath(QStringLiteral("capture-%1.jpg").arg(QDateTime::currentMSecsSinceEpoch()));
}

void PetController::captureVisionFrameFile(const QString &filePath, int frameWidth, int frameHeight)
{
    QString localPath = filePath.trimmed();
    if (localPath.startsWith(QStringLiteral("file://"))) {
        localPath = QUrl(localPath).toLocalFile();
    }
    if (_session_id.isEmpty() || localPath.isEmpty()) {
        return;
    }

    const QFileInfo info(localPath);
    if (info.exists() && info.size() > 8 * 1024 * 1024) {
        QFile::remove(localPath);
        setError(QStringLiteral("摄像头帧过大，已跳过"));
        return;
    }

    QFile file(localPath);
    if (!file.open(QIODevice::ReadOnly)) {
        setError(QStringLiteral("摄像头帧读取失败"));
        return;
    }
    const QByteArray bytes = file.readAll();
    file.close();
    QFile::remove(localPath);
    if (bytes.isEmpty()) {
        setError(QStringLiteral("摄像头帧为空"));
        return;
    }

    QString mime = QStringLiteral("image/jpeg");
    const QString suffix = info.suffix().toLower();
    if (suffix == QStringLiteral("png")) {
        mime = QStringLiteral("image/png");
    } else if (suffix == QStringLiteral("webp")) {
        mime = QStringLiteral("image/webp");
    }
    captureVisionFrame(QString::fromLatin1(bytes.toBase64()), mime, frameWidth, frameHeight);
}

void PetController::interrupt()
{
    if (_session_id.isEmpty()) {
        return;
    }
    postJson(petUrl(QStringLiteral("/sessions/%1/interrupt").arg(encodedSessionId())),
             QJsonObject{},
             QStringLiteral("interrupt"));
}

void PetController::stopStream()
{
    if (_stream_reply) {
        auto *reply = _stream_reply.data();
        _stream_reply.clear();
        reply->disconnect(this);
        reply->abort();
        reply->deleteLater();
    }
    _stream_buffer.clear();
    if (_streaming) {
        _streaming = false;
        emit stateChanged();
    }
}

void PetController::clearSpeech()
{
    _model.clearSpeech();
}

void PetController::startVoiceTraining(const QVariantMap &request)
{
    if (_voice_training_busy) {
        return;
    }
    QJsonObject payload = authPayload();
    const QJsonObject requestObject = QJsonObject::fromVariantMap(request);
    for (auto it = requestObject.constBegin(); it != requestObject.constEnd(); ++it) {
        payload.insert(it.key(), it.value());
    }
    if (!payload.value(QStringLiteral("consent_confirmed")).toBool(false)) {
        setError(QStringLiteral("声音训练需要先确认参考音频授权"));
        _voice_training_status = QStringLiteral("blocked");
        _voice_training_message = QStringLiteral("请先勾选参考音频授权确认");
        emit voiceTrainingChanged();
        return;
    }
    QString referenceAudioPath = payload.value(QStringLiteral("reference_audio_path")).toString().trimmed();
    if (referenceAudioPath.startsWith(QStringLiteral("file://"))) {
        referenceAudioPath = QUrl(referenceAudioPath).toLocalFile();
        payload[QStringLiteral("reference_audio_path")] = referenceAudioPath;
    }
    if (referenceAudioPath.isEmpty()) {
        setError(QStringLiteral("声音训练缺少参考音频路径"));
        _voice_training_status = QStringLiteral("blocked");
        _voice_training_message = QStringLiteral("请先设置默认音频文件");
        emit voiceTrainingChanged();
        return;
    }
    QFileInfo referenceAudio(referenceAudioPath);
    if (referenceAudio.exists() && referenceAudio.isFile() && referenceAudio.size() > 0
        && referenceAudio.size() <= 32 * 1024 * 1024) {
        QFile file(referenceAudio.absoluteFilePath());
        if (file.open(QIODevice::ReadOnly)) {
            QJsonObject metadata = payload.value(QStringLiteral("metadata")).toObject();
            metadata[QStringLiteral("reference_audio_base64")] =
                QString::fromLatin1(file.readAll().toBase64());
            metadata[QStringLiteral("reference_audio_file_name")] = referenceAudio.fileName();
            metadata[QStringLiteral("reference_audio_size_bytes")] = qint64(referenceAudio.size());
            metadata[QStringLiteral("reference_audio_transfer")] = QStringLiteral("client_base64");
            payload[QStringLiteral("metadata")] = metadata;
        }
    }
    setError({});
    setVoiceTrainingBusy(true, QStringLiteral("正在提交声音训练准备任务"));
    postJson(petUrl(QStringLiteral("/voice-training/jobs")),
             payload,
             QStringLiteral("voice_training_create"));
}

bool PetController::windowsImeBridgeAvailable() const
{
    return isWslRuntime() && !windowsPowerShellPath().isEmpty();
}

void PetController::openWindowsImeBridge(const QString &initialText)
{
    if (_windows_ime_bridge_busy) {
        return;
    }
    const QString program = windowsPowerShellPath();
    if (!isWslRuntime() || program.isEmpty()) {
        setError(QStringLiteral("Windows 中文输入桥只在 WSL 中可用"));
        return;
    }

    auto *process = new QProcess(this);
    _windows_ime_bridge_process = process;
    setWindowsImeBridgeBusy(true);

    connect(process,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
                if (_windows_ime_bridge_process == process) {
                    _windows_ime_bridge_process.clear();
                }
                const QString output = QString::fromUtf8(process->readAllStandardOutput()).trimmed();
                const QString error = QString::fromUtf8(process->readAllStandardError()).trimmed();
                process->deleteLater();
                setWindowsImeBridgeBusy(false);
                if (exitStatus != QProcess::NormalExit || exitCode != 0) {
                    setError(error.isEmpty()
                                 ? QStringLiteral("Windows 中文输入桥已退出")
                                 : QStringLiteral("Windows 中文输入桥失败: %1").arg(error));
                    return;
                }
                if (!output.isEmpty()) {
                    emit windowsImeTextCommitted(output);
                }
            });

    const QString script = windowsImeBridgeScript(initialText);
    process->setProgram(program);
    process->setArguments({
        QStringLiteral("-NoProfile"),
        QStringLiteral("-Sta"),
        QStringLiteral("-ExecutionPolicy"),
        QStringLiteral("Bypass"),
        QStringLiteral("-EncodedCommand"),
        powershellEncodedCommand(script),
    });
    process->start();
    if (!process->waitForStarted(3000)) {
        if (_windows_ime_bridge_process == process) {
            _windows_ime_bridge_process.clear();
        }
        const QString error = process->errorString();
        process->deleteLater();
        setWindowsImeBridgeBusy(false);
        setError(QStringLiteral("Windows 中文输入桥启动失败: %1").arg(error));
    }
}

void PetController::setModelSelection(const QString &modelType, const QString &modelName)
{
    const QString nextType = modelType.trimmed();
    const QString nextName = modelName.trimmed();
    if (_selected_model_type == nextType && _selected_model_name == nextName) {
        return;
    }
    _selected_model_type = nextType;
    _selected_model_name = nextName;
    emit modelSelectionChanged();
}

void PetController::setReplyLanguage(const QString &language)
{
    QString nextLanguage = language.trimmed();
    if (nextLanguage.isEmpty()) {
        nextLanguage = QStringLiteral("zh-CN");
    }
    if (_reply_language == nextLanguage) {
        return;
    }
    _reply_language = nextLanguage;
    emit replyLanguageChanged();
}

void PetController::refreshVoiceTrainingJob(const QString &jobId)
{
    const QString selectedJobId = jobId.trimmed().isEmpty() ? _voice_training_job_id : jobId.trimmed();
    if (selectedJobId.isEmpty() || _voice_training_busy) {
        return;
    }
    setError({});
    setVoiceTrainingBusy(true, QStringLiteral("正在刷新声音训练任务"));
    getJson(petUrl(QStringLiteral("/voice-training/jobs/%1").arg(QString::fromLatin1(QUrl::toPercentEncoding(selectedJobId)))),
            QStringLiteral("voice_training_get"));
}

void PetController::postJson(const QUrl &url, const QJsonObject &payload, const QString &op)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    auto *reply = _network.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    reply->setProperty("pet_op", op);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleJsonReply(reply);
    });
}

void PetController::getJson(const QUrl &url, const QString &op)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    auto *reply = _network.get(request);
    reply->setProperty("pet_op", op);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleJsonReply(reply);
    });
}

void PetController::startStream()
{
    if (_session_id.isEmpty() || _streaming) {
        return;
    }

    QNetworkRequest request(petUrl(QStringLiteral("/sessions/%1/stream").arg(encodedSessionId())));
    request.setRawHeader("Accept", "text/event-stream");
    request.setRawHeader("Cache-Control", "no-cache");
    _stream_reply = _stream_network.get(request);
    _streaming = true;
    _stream_buffer.clear();
    setStatusText(QStringLiteral("桌宠事件流已连接"));
    emit stateChanged();

    connect(_stream_reply.data(), &QNetworkReply::readyRead, this, [this]() {
        if (_stream_reply) {
            consumeStreamBytes(_stream_reply->readAll());
        }
    });
    connect(_stream_reply.data(), &QNetworkReply::finished, this, [this]() {
        if (!_stream_reply) {
            return;
        }
        const auto err = _stream_reply->error();
        const QString errText = _stream_reply->errorString();
        _stream_reply->deleteLater();
        _stream_reply.clear();
        _streaming = false;
        if (err != QNetworkReply::NoError && err != QNetworkReply::OperationCanceledError) {
            setError(QStringLiteral("桌宠事件流断开: %1").arg(errText));
        } else {
            setStatusText(QStringLiteral("桌宠事件流已关闭"));
        }
        emit stateChanged();
    });
}

void PetController::handleJsonReply(QNetworkReply *reply)
{
    const QString op = reply->property("pet_op").toString();
    const auto networkError = reply->error();
    const QString networkErrorText = reply->errorString();
    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray bytes = reply->readAll();
    reply->deleteLater();

    if (op == QStringLiteral("create_session")) {
        setBusy(false);
    } else if (op == QStringLiteral("input")) {
        _input_request_in_flight = false;
        setBusy(false);
    } else if (op == QStringLiteral("voice_training_create")
               || op == QStringLiteral("voice_training_get")) {
        setVoiceTrainingBusy(false);
    }

    if (networkError != QNetworkReply::NoError) {
        setError(QStringLiteral("桌宠请求失败: %1").arg(networkErrorText));
        return;
    }
    if (httpStatus >= 400) {
        setError(QStringLiteral("桌宠 HTTP %1").arg(httpStatus));
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(bytes);
    if (!doc.isObject()) {
        setError(QStringLiteral("桌宠响应格式错误"));
        return;
    }
    const QJsonObject root = doc.object();
    if (root.value(QStringLiteral("code")).toInt(0) != 0) {
        setError(root.value(QStringLiteral("message")).toString(QStringLiteral("桌宠请求失败")));
        return;
    }

    if (op == QStringLiteral("create_session")) {
        const QJsonObject session = root.value(QStringLiteral("session")).toObject();
        const QString nextSessionId = session.value(QStringLiteral("session_id")).toString();
        if (nextSessionId.isEmpty()) {
            setError(QStringLiteral("桌宠会话缺少 session_id"));
            return;
        }
        if (_session_id != nextSessionId) {
            resetControlEventDedupe();
        }
        _session_id = nextSessionId;
        setStatusText(QStringLiteral("桌宠会话已创建"));
        emit stateChanged();
        startStream();
        return;
    }

    if (op == QStringLiteral("voice_training_create")
        || op == QStringLiteral("voice_training_get")) {
        applyVoiceTrainingJob(root.value(QStringLiteral("job")).toObject());
        return;
    }

    if (!_streaming) {
        const QJsonArray events = root.value(QStringLiteral("events")).toArray();
        for (const QJsonValue &value : events) {
            applyControlEvent(value.toObject());
        }
        const QJsonObject event = root.value(QStringLiteral("event")).toObject();
        if (!event.isEmpty()) {
            applyControlEvent(event);
        }
    }
}

void PetController::applyControlEvent(const QJsonObject &event)
{
    if (event.value(QStringLiteral("type")).toString() != QStringLiteral("pet.control")) {
        return;
    }
    if (!rememberControlEvent(event)) {
        return;
    }
    _model.applyControlEvent(event);
    emit controlEventReceived(event.toVariantMap());
}

bool PetController::rememberControlEvent(const QJsonObject &event)
{
    const QString sessionId = event.value(QStringLiteral("session_id")).toString(_session_id).trimmed();
    const QString eventId = event.value(QStringLiteral("event_id")).toString().trimmed();
    const QString turnId = event.value(QStringLiteral("turn_id")).toString().trimmed();
    const QString phase = event.value(QStringLiteral("phase")).toString().trimmed();
    const QString actionName = event.value(QStringLiteral("action")).toObject().value(QStringLiteral("name")).toString().trimmed();
    const QString textDelta = event.value(QStringLiteral("text")).toObject().value(QStringLiteral("delta")).toString();
    const QString audioUrl = event.value(QStringLiteral("audio")).toObject().value(QStringLiteral("url")).toString();
    QString key;
    if (!eventId.isEmpty()) {
        key = sessionId + QStringLiteral(":event:") + eventId;
    } else if (!turnId.isEmpty()) {
        key = sessionId + QStringLiteral(":turn:")
            + turnId + QStringLiteral(":") + phase + QStringLiteral(":")
            + actionName + QStringLiteral(":") + textDelta + QStringLiteral(":") + audioUrl;
    } else {
        const int sequence = event.value(QStringLiteral("seq")).toInt(-1);
        if (sequence < 0) {
            return true;
        }
        key = sessionId + QStringLiteral(":seq:") + QString::number(sequence);
    }

    if (_applied_control_event_keys.contains(key)) {
        return false;
    }
    _applied_control_event_keys.insert(key);
    _applied_control_event_order.append(key);
    while (_applied_control_event_order.size() > kMaxRememberedPetControlEvents) {
        _applied_control_event_keys.remove(_applied_control_event_order.takeFirst());
    }
    return true;
}

void PetController::resetControlEventDedupe()
{
    _applied_control_event_keys.clear();
    _applied_control_event_order.clear();
}

void PetController::consumeStreamBytes(const QByteArray &bytes)
{
    _stream_buffer += bytes;
    for (;;) {
        const int newline = _stream_buffer.indexOf('\n');
        if (newline < 0) {
            return;
        }
        QByteArray line = _stream_buffer.left(newline);
        _stream_buffer.remove(0, newline + 1);
        if (line.endsWith('\r')) {
            line.chop(1);
        }
        consumeSseLine(line);
    }
}

void PetController::consumeSseLine(const QByteArray &line)
{
    if (!line.startsWith("data: ")) {
        return;
    }
    const QByteArray payload = line.mid(6);
    if (payload == "[DONE]") {
        stopStream();
        return;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (!doc.isObject()) {
        return;
    }
    applyControlEvent(doc.object());
}

void PetController::setBusy(bool busy, const QString &statusText)
{
    const bool changed = _busy != busy || (!statusText.isNull() && _status_text != statusText);
    _busy = busy;
    if (!statusText.isNull()) {
        _status_text = statusText;
    }
    if (changed) {
        emit stateChanged();
    }
}

void PetController::setVoiceTrainingBusy(bool busy, const QString &message)
{
    const bool changed = _voice_training_busy != busy || (!message.isNull() && _voice_training_message != message);
    _voice_training_busy = busy;
    if (!message.isNull()) {
        _voice_training_message = message;
    }
    if (changed) {
        emit voiceTrainingChanged();
    }
}

void PetController::applyVoiceTrainingJob(const QJsonObject &job)
{
    if (job.isEmpty()) {
        setError(QStringLiteral("声音训练响应缺少任务信息"));
        return;
    }
    _voice_training_job_id = job.value(QStringLiteral("job_id")).toString();
    _voice_training_status = job.value(QStringLiteral("status")).toString(QStringLiteral("prepared"));
    _voice_training_stage = job.value(QStringLiteral("stage")).toString();
    _voice_training_progress = qBound(0, job.value(QStringLiteral("progress")).toInt(0), 100);
    _voice_training_artifact_path = job.value(QStringLiteral("artifact_path")).toString();
    _voice_training_message = job.value(QStringLiteral("message")).toString(QStringLiteral("声音训练任务已准备"));
    if (_voice_training_stage == QStringLiteral("ready_for_worker")) {
        _voice_training_stage = QStringLiteral("ready_for_gpt_sovits");
        if (_voice_training_status == QStringLiteral("prepared")
            || _voice_training_status == QStringLiteral("idle")
            || _voice_training_status == QStringLiteral("blocked")) {
            _voice_training_status = QStringLiteral("ready");
        }
        if (_voice_training_progress < 70) {
            _voice_training_progress = 70;
        }
        if (_voice_training_message.isEmpty()
            || _voice_training_message.contains(QStringLiteral("worker"), Qt::CaseInsensitive)) {
            _voice_training_message = QStringLiteral("声音参考已就绪，可直接用于 GPT-SoVITS 零样本合成。");
        }
    }
    setStatusText(_voice_training_message);
    emit voiceTrainingChanged();
}

void PetController::setWindowsImeBridgeBusy(bool busy)
{
    if (_windows_ime_bridge_busy == busy) {
        return;
    }
    _windows_ime_bridge_busy = busy;
    emit windowsImeBridgeChanged();
}

void PetController::setStatusText(const QString &statusText)
{
    if (_status_text == statusText) {
        return;
    }
    _status_text = statusText;
    emit stateChanged();
}

void PetController::setError(const QString &error)
{
    if (_error == error) {
        return;
    }
    _error = error;
    emit stateChanged();
}

QJsonObject PetController::authPayload() const
{
    QJsonObject payload;
    if (_gateway && _gateway->userMgr()) {
        payload[QStringLiteral("uid")] = _gateway->userMgr()->GetUid();
    }
    return payload;
}

QJsonObject PetController::defaultObservationPayload() const
{
    QJsonObject audio;
    audio[QStringLiteral("vad")] = QStringLiteral("idle");
    audio[QStringLiteral("rms")] = 0.0;
    audio[QStringLiteral("interrupt")] = false;

    QJsonObject vision;
    vision[QStringLiteral("enabled")] = false;
    vision[QStringLiteral("mode")] = QStringLiteral("landmarks_only");
    vision[QStringLiteral("face_present")] = false;

    QJsonObject privacy;
    privacy[QStringLiteral("raw_frame_sent")] = false;
    privacy[QStringLiteral("raw_audio_recorded")] = false;

    QJsonObject payload;
    payload[QStringLiteral("type")] = QStringLiteral("pet.observation");
    payload[QStringLiteral("audio")] = audio;
    payload[QStringLiteral("vision")] = vision;
    payload[QStringLiteral("privacy")] = privacy;
    return payload;
}

QUrl PetController::petUrl(const QString &path) const
{
    return QUrl(gate_url_prefix + QStringLiteral("/ai/pet") + path);
}

QString PetController::encodedSessionId() const
{
    return QString::fromLatin1(QUrl::toPercentEncoding(_session_id));
}
