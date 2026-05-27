#include "PetAssetSettings.h"

#include "Live2DAvatarOpenGLRenderer.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QRect>
#include <QStandardPaths>
#include <QUrl>
#include <QVector>
#include <QtGlobal>
#include <algorithm>

namespace {
constexpr int kSchemaVersion = 1;
constexpr int kLanguageOptionCount = 6;

QString clientSourcePath(const QString &relativePath)
{
#ifdef MEMOCHAT_QML_SOURCE_DIR
    const QString root = QString::fromUtf8(MEMOCHAT_QML_SOURCE_DIR);
#else
    const QString root = QDir::currentPath();
#endif
    return QDir(root).absoluteFilePath(relativePath);
}

QString stringValue(const QVariantMap &values, const QString &key, const QString &fallback)
{
    const QVariant value = values.value(key);
    return value.canConvert<QString>() ? value.toString() : fallback;
}

int intValue(const QVariantMap &values, const QString &key, int fallback, int minimum, int maximum)
{
    bool ok = false;
    const int parsed = values.value(key).toInt(&ok);
    return qBound(minimum, ok ? parsed : fallback, maximum);
}

qreal realValue(const QVariantMap &values, const QString &key, qreal fallback, qreal minimum, qreal maximum)
{
    bool ok = false;
    const qreal parsed = values.value(key).toDouble(&ok);
    return qBound(minimum, ok ? parsed : fallback, maximum);
}

bool boolValue(const QVariantMap &values, const QString &key, bool fallback)
{
    const QVariant value = values.value(key);
    if (value.userType() == QMetaType::Bool) {
        return value.toBool();
    }
    if (value.canConvert<QString>()) {
        const QString text = value.toString().trimmed().toLower();
        if (text == QStringLiteral("true") || text == QStringLiteral("1") || text == QStringLiteral("yes")) {
            return true;
        }
        if (text == QStringLiteral("false") || text == QStringLiteral("0") || text == QStringLiteral("no")) {
            return false;
        }
    }
    return fallback;
}

bool isQrcPath(const QString &path)
{
    return path.startsWith(QStringLiteral(":/")) || path.startsWith(QStringLiteral("qrc:/"));
}

QString qrcFilePath(const QString &path)
{
    if (path.startsWith(QStringLiteral("qrc:/"))) {
        return QStringLiteral(":") + QUrl(path).path();
    }
    return path;
}

QString resolveAvatarInputPath(const QString &value, const QString &baseDirectory = QString())
{
    const QString cleaned = value.trimmed();
    if (cleaned.isEmpty()) {
        return {};
    }

    const QUrl url(cleaned);
    if (url.isLocalFile()) {
        return QDir::cleanPath(url.toLocalFile());
    }
    if (isQrcPath(cleaned)) {
        return cleaned;
    }
    if (QDir::isAbsolutePath(cleaned)) {
        return QDir::cleanPath(cleaned);
    }

    QStringList candidates;
    if (!baseDirectory.isEmpty()) {
        candidates.append(QDir(baseDirectory).absoluteFilePath(cleaned));
    }
    candidates.append(clientSourcePath(cleaned));
    candidates.append(QDir::current().absoluteFilePath(cleaned));

    for (const QString &candidate : candidates) {
        const QString path = QDir::cleanPath(candidate);
        if (isQrcPath(path) || QFileInfo::exists(path)) {
            return path;
        }
    }
    return QDir::cleanPath(candidates.value(0, cleaned));
}

QString resolveAvatarModelReference(const QString &modelDirectory, const QString &reference)
{
    const QString cleaned = reference.trimmed();
    if (cleaned.isEmpty()) {
        return {};
    }

    const QUrl url(cleaned);
    if (url.isLocalFile()) {
        return QDir::cleanPath(url.toLocalFile());
    }
    if (isQrcPath(cleaned) || QDir::isAbsolutePath(cleaned)) {
        return QDir::cleanPath(cleaned);
    }
    if (isQrcPath(modelDirectory)) {
        return QDir::cleanPath(modelDirectory + QLatin1Char('/') + cleaned);
    }
    return QDir(modelDirectory).absoluteFilePath(cleaned);
}

QString avatarCacheDirectory()
{
    QString root = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (root.isEmpty()) {
        root = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    }
    if (root.isEmpty()) {
        root = QDir::home().absoluteFilePath(QStringLiteral(".memochat/cache"));
    }
    return QDir(root).absoluteFilePath(QStringLiteral("pet/live2d-avatars"));
}

void addPathFingerprint(QCryptographicHash &hash, const QString &path)
{
    const QByteArray separator(1, '\0');
    hash.addData(path.toUtf8());
    hash.addData(separator);
    if (path.isEmpty() || isQrcPath(path)) {
        return;
    }
    const QFileInfo info(path);
    if (!info.exists()) {
        return;
    }
    hash.addData(QByteArray::number(info.size()));
    hash.addData(separator);
    hash.addData(QByteArray::number(info.lastModified().toMSecsSinceEpoch()));
    hash.addData(separator);
}

QStringList modelTexturePaths(const QString &modelJsonPath, QString *modelDirectory)
{
    QFile modelFile(qrcFilePath(modelJsonPath));
    if (!modelFile.open(QIODevice::ReadOnly)) {
        return {};
    }

    const QJsonDocument document = QJsonDocument::fromJson(modelFile.readAll());
    if (!document.isObject()) {
        return {};
    }

    const QFileInfo modelInfo(modelJsonPath);
    const QString directory = isQrcPath(modelJsonPath)
                                  ? QString(modelJsonPath).left(modelJsonPath.lastIndexOf(QLatin1Char('/')))
                                  : modelInfo.absolutePath();
    if (modelDirectory) {
        *modelDirectory = directory;
    }

    QStringList paths;
    const QJsonArray textures = document.object()
                                    .value(QStringLiteral("FileReferences"))
                                    .toObject()
                                    .value(QStringLiteral("Textures"))
                                    .toArray();
    for (const QJsonValue &texture : textures) {
        const QString texturePath = texture.toString().trimmed();
        if (!texturePath.isEmpty()) {
            paths.append(resolveAvatarModelReference(directory, texturePath));
        }
    }
    return paths;
}

int avatarCandidateScore(const QFileInfo &info)
{
    const QString name = info.completeBaseName().toLower();
    int score = 0;
    const QStringList strongTokens = {
        QStringLiteral("avatar"),
        QStringLiteral("icon"),
        QStringLiteral("preview"),
        QStringLiteral("profile"),
        QStringLiteral("head"),
        QStringLiteral("face"),
        QStringLiteral("头像"),
        QStringLiteral("头"),
        QStringLiteral("脸"),
        QStringLiteral("预览"),
        QStringLiteral("立绘"),
    };
    for (const QString &token : strongTokens) {
        if (name.contains(token)) {
            score += 80;
        }
    }
    if (name.contains(QStringLiteral("texture"))) {
        score -= 30;
    }
    if (name.contains(QStringLiteral("normal")) || name.contains(QStringLiteral("mask"))) {
        score -= 20;
    }
    return score;
}

QStringList packageImageCandidates(const QString &modelDirectory, const QStringList &texturePaths)
{
    QStringList candidates;
    if (!modelDirectory.isEmpty() && !isQrcPath(modelDirectory)) {
        struct ScoredPath {
            int score = 0;
            QString path;
        };
        QVector<ScoredPath> scored;
        QDirIterator it(modelDirectory,
                        {QStringLiteral("*.png"),
                         QStringLiteral("*.jpg"),
                         QStringLiteral("*.jpeg"),
                         QStringLiteral("*.webp")},
                        QDir::Files,
                        QDirIterator::Subdirectories);
        while (it.hasNext() && scored.size() < 256) {
            it.next();
            const QFileInfo info = it.fileInfo();
            scored.push_back({avatarCandidateScore(info), info.absoluteFilePath()});
        }
        std::stable_sort(scored.begin(), scored.end(), [](const ScoredPath &left, const ScoredPath &right) {
            return left.score > right.score;
        });
        for (const ScoredPath &item : scored) {
            if (item.score > 0 && !candidates.contains(item.path)) {
                candidates.append(item.path);
            }
        }
    }

    for (const QString &texture : texturePaths) {
        if (!candidates.contains(texture)) {
            candidates.append(texture);
        }
    }
    return candidates;
}

QImage readAvatarImage(const QString &path)
{
    QImageReader reader(qrcFilePath(path));
    reader.setAutoTransform(true);
    return reader.read();
}

QRect alphaBounds(const QImage &image, int alphaThreshold = 12)
{
    if (image.isNull()) {
        return {};
    }
    const QImage argb = image.convertToFormat(QImage::Format_ARGB32);
    int left = argb.width();
    int top = argb.height();
    int right = -1;
    int bottom = -1;
    for (int y = 0; y < argb.height(); ++y) {
        const QRgb *line = reinterpret_cast<const QRgb *>(argb.constScanLine(y));
        for (int x = 0; x < argb.width(); ++x) {
            if (qAlpha(line[x]) <= alphaThreshold) {
                continue;
            }
            left = qMin(left, x);
            top = qMin(top, y);
            right = qMax(right, x);
            bottom = qMax(bottom, y);
        }
    }
    if (right < left || bottom < top) {
        return {};
    }
    return QRect(QPoint(left, top), QPoint(right, bottom));
}

QRect clampedSquare(const QRect &source, const QRect &imageRect)
{
    if (source.isEmpty() || imageRect.isEmpty()) {
        return {};
    }

    const int side = qMin(qMax(source.width(), source.height()), qMin(imageRect.width(), imageRect.height()));
    int x = source.center().x() - side / 2;
    int y = source.center().y() - side / 2;
    x = qBound(imageRect.left(), x, imageRect.right() - side + 1);
    y = qBound(imageRect.top(), y, imageRect.bottom() - side + 1);
    return QRect(x, y, side, side).intersected(imageRect);
}

QImage squareAvatarImage(const QImage &source, const QRect &crop)
{
    if (source.isNull() || crop.isEmpty()) {
        return {};
    }

    QImage avatar(256, 256, QImage::Format_ARGB32_Premultiplied);
    avatar.fill(Qt::transparent);
    QPainter painter(&avatar);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.drawImage(QRectF(0, 0, avatar.width(), avatar.height()), source, crop);
    return avatar;
}

QImage cropHeadAvatar(const QImage &source)
{
    if (source.isNull()) {
        return {};
    }

    const QRect bounds = alphaBounds(source);
    if (bounds.isEmpty()) {
        return {};
    }

    const int maxSide = qMax(bounds.width(), bounds.height());
    const int side = qMin(maxSide,
                          qMax(24, qRound(qMax(bounds.width() * 0.62, bounds.height() * 0.30))));
    const QPoint center(bounds.center().x(), bounds.top() + qRound(bounds.height() * 0.20));
    QRect crop(center.x() - side / 2, center.y() - side / 2, side, side);
    const int pad = qMax(4, qRound(side * 0.08));
    crop = crop.adjusted(-pad, -pad, pad, pad);
    return squareAvatarImage(source, clampedSquare(crop, source.rect()));
}

QString saveAvatarToCache(const QImage &avatar, const QString &cachePath)
{
    if (avatar.isNull()) {
        return {};
    }
    QDir dir(QFileInfo(cachePath).absolutePath());
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        return {};
    }
    if (!avatar.save(cachePath, "PNG")) {
        return {};
    }
    return QUrl::fromLocalFile(cachePath).toString();
}

QImage renderedLive2DAvatar(const QString &modelJsonPath)
{
    QString error;
    const QImage avatar = Live2DAvatarOpenGLRenderer().renderAvatar(modelJsonPath, &error);
    if (avatar.isNull() && !error.isEmpty()) {
        qWarning().noquote() << "Live2D avatar OpenGL render unavailable:" << error;
    }
    return avatar;
}
} // namespace

PetAssetSettings::PetAssetSettings(QObject *parent)
    : QObject(parent)
    , _storage_path(defaultStoragePath())
{
    applyDefaults(false);
}

bool PetAssetSettings::load()
{
    QFile file(_storage_path);
    if (!file.exists()) {
        applyDefaults(false);
        _status_text = QStringLiteral("暂无本地草稿");
        emit settingsChanged();
        return true;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        _status_text = QStringLiteral("无法读取本地草稿：%1").arg(file.errorString());
        emit settingsChanged();
        return false;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        applyDefaults(false);
        _status_text = QStringLiteral("本地草稿格式错误，已使用默认值");
        emit settingsChanged();
        return false;
    }

    const QJsonObject object = document.object();
    applyObject(object.toVariantMap(), false);
    normalizeVoiceTrainingState();
    _status_text = QStringLiteral("已载入本地草稿");
    emit settingsChanged();
    return true;
}

bool PetAssetSettings::save()
{
    const QFileInfo info(_storage_path);
    QDir dir(info.absolutePath());
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        _status_text = QStringLiteral("无法创建草稿目录：%1").arg(info.absolutePath());
        emit settingsChanged();
        return false;
    }

    QFile file(_storage_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        _status_text = QStringLiteral("无法写入本地草稿：%1").arg(file.errorString());
        emit settingsChanged();
        return false;
    }

    const QJsonDocument document(QJsonObject::fromVariantMap(toVariantMap()));
    file.write(document.toJson(QJsonDocument::Indented));
    _dirty = false;
    _status_text = QStringLiteral("本地草稿已保存");
    emit settingsChanged();
    return true;
}

QString PetAssetSettings::pickLocalFilePath(const QString &title, const QString &startPath, const QString &nameFilter)
{
    QString directory;
    const QString trimmedStartPath = startPath.trimmed();
    if (!trimmedStartPath.isEmpty()) {
        const QFileInfo info(trimmedStartPath);
        directory = info.isDir() ? info.absoluteFilePath() : info.absolutePath();
    }

    const QString picked = QFileDialog::getOpenFileName(
        nullptr,
        title.trimmed().isEmpty() ? QStringLiteral("选择文件") : title,
        directory,
        nameFilter.trimmed().isEmpty() ? QStringLiteral("所有文件 (*.*)") : nameFilter);
    return picked;
}

QString PetAssetSettings::pickLocalDirectoryPath(const QString &title, const QString &startPath)
{
    QString directory = startPath.trimmed();
    if (!directory.isEmpty()) {
        const QFileInfo info(directory);
        directory = info.isDir() ? info.absoluteFilePath() : info.absolutePath();
    }

    const QString picked = QFileDialog::getExistingDirectory(
        nullptr,
        title.trimmed().isEmpty() ? QStringLiteral("选择目录") : title,
        directory);
    return picked;
}

void PetAssetSettings::resetToDefaults()
{
    applyDefaults(true);
    _status_text = QStringLiteral("已恢复默认草稿，资源路径留空");
    emit settingsChanged();
}

QVariantMap PetAssetSettings::toVariantMap() const
{
    QVariantMap values;
    values[QStringLiteral("schema_version")] = kSchemaVersion;
    values[QStringLiteral("characterName")] = _character_name;
    values[QStringLiteral("roleIdentity")] = _role_identity;
    values[QStringLiteral("modelRoot")] = _model_root;
    values[QStringLiteral("modelJson")] = _model_json;
    values[QStringLiteral("motionDirectory")] = _motion_directory;
    values[QStringLiteral("expressionDirectory")] = _expression_directory;
    values[QStringLiteral("voiceDirectory")] = _voice_directory;
    values[QStringLiteral("defaultVoice")] = _default_voice;
    values[QStringLiteral("idleMotion")] = _idle_motion;
    values[QStringLiteral("speakingMotion")] = _speaking_motion;
    values[QStringLiteral("fallbackExpression")] = _fallback_expression;
    values[QStringLiteral("personalityTags")] = _personality_tags;
    values[QStringLiteral("relationshipStyle")] = _relationship_style;
    values[QStringLiteral("worldSetting")] = _world_setting;
    values[QStringLiteral("speechRules")] = _speech_rules;
    values[QStringLiteral("catchphrases")] = _catchphrases;
    values[QStringLiteral("forbiddenRules")] = _forbidden_rules;
    values[QStringLiteral("toneIndex")] = _tone_index;
    values[QStringLiteral("responseLengthIndex")] = _response_length_index;
    values[QStringLiteral("languageIndex")] = _language_index;
    values[QStringLiteral("emotionLevel")] = _emotion_level;
    values[QStringLiteral("creativityLevel")] = _creativity_level;
    values[QStringLiteral("voiceSpeed")] = _voice_speed;
    values[QStringLiteral("lipSyncSensitivity")] = _lip_sync_sensitivity;
    values[QStringLiteral("voiceLipSyncEnabled")] = _voice_lip_sync_enabled;
    values[QStringLiteral("emotionSoundEnabled")] = _emotion_sound_enabled;
    values[QStringLiteral("idleMotionEnabled")] = _idle_motion_enabled;
    values[QStringLiteral("gazeFollowEnabled")] = _gaze_follow_enabled;
    values[QStringLiteral("memoryEnabled")] = _memory_enabled;
    values[QStringLiteral("interruptEnabled")] = _interrupt_enabled;
    values[QStringLiteral("cameraEnabled")] = _camera_enabled;
    values[QStringLiteral("cloudVisionEnabled")] = _cloud_vision_enabled;
    values[QStringLiteral("autoStartPetOnClientStart")] = _auto_start_pet_on_client_start;
    values[QStringLiteral("voiceTrainingConsent")] = _voice_training_consent;
    values[QStringLiteral("voiceTrainingConsentScope")] = _voice_training_consent_scope;
    values[QStringLiteral("voiceTrainingJobId")] = _voice_training_job_id;
    values[QStringLiteral("voiceTrainingStatus")] = _voice_training_status;
    values[QStringLiteral("voiceTrainingStage")] = _voice_training_stage;
    values[QStringLiteral("voiceTrainingProgress")] = _voice_training_progress;
    values[QStringLiteral("voiceTrainingArtifactPath")] = _voice_training_artifact_path;
    values[QStringLiteral("voiceTrainingMessage")] = _voice_training_message;
    return values;
}

QString PetAssetSettings::live2dAvatarUrl() const
{
    return resolveLive2DAvatarUrl(_model_json, _model_root);
}

QString PetAssetSettings::resolveLive2DAvatarUrl(const QString &modelJson, const QString &modelRoot) const
{
    const QString resolvedRoot = resolveAvatarInputPath(modelRoot);
    const QString resolvedModelJson = resolveAvatarInputPath(modelJson, resolvedRoot);
    if (resolvedModelJson.isEmpty()) {
        return {};
    }

    QString modelDirectory;
    const QStringList textures = modelTexturePaths(resolvedModelJson, &modelDirectory);
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(QByteArrayLiteral("memochat-live2d-avatar-opengl-v3"));
    addPathFingerprint(hash, resolvedModelJson);
    for (const QString &texture : textures) {
        addPathFingerprint(hash, texture);
    }

    const QString cacheKey = QString::fromLatin1(hash.result().toHex());
    const QDir cacheDir(avatarCacheDirectory());
    const QString renderCachePath = cacheDir.absoluteFilePath(QStringLiteral("avatar_gl_%1.png").arg(cacheKey));
    if (QFileInfo::exists(renderCachePath)) {
        return QUrl::fromLocalFile(renderCachePath).toString();
    }

    QImage avatar = renderedLive2DAvatar(resolvedModelJson);
    if (!avatar.isNull()) {
        return saveAvatarToCache(avatar, renderCachePath);
    }

    const QString fallbackCachePath = cacheDir.absoluteFilePath(QStringLiteral("avatar_%1.png").arg(cacheKey));
    if (QFileInfo::exists(fallbackCachePath)) {
        return QUrl::fromLocalFile(fallbackCachePath).toString();
    }

    if (avatar.isNull()) {
        const QStringList candidates = packageImageCandidates(modelDirectory, textures);
        for (const QString &candidate : candidates) {
            avatar = cropHeadAvatar(readAvatarImage(candidate));
            if (!avatar.isNull()) {
                break;
            }
        }
    }

    return saveAvatarToCache(avatar, fallbackCachePath);
}

void PetAssetSettings::setCharacterName(const QString &value) { if (assignString(_character_name, value)) markDirty(); }
void PetAssetSettings::setRoleIdentity(const QString &value) { if (assignString(_role_identity, value)) markDirty(); }
void PetAssetSettings::setModelRoot(const QString &value) { if (assignString(_model_root, value)) markDirty(); }
void PetAssetSettings::setModelJson(const QString &value) { if (assignString(_model_json, value)) markDirty(); }
void PetAssetSettings::setMotionDirectory(const QString &value) { if (assignString(_motion_directory, value)) markDirty(); }
void PetAssetSettings::setExpressionDirectory(const QString &value) { if (assignString(_expression_directory, value)) markDirty(); }
void PetAssetSettings::setVoiceDirectory(const QString &value) { if (assignString(_voice_directory, value)) markDirty(); }
void PetAssetSettings::setDefaultVoice(const QString &value) { if (assignString(_default_voice, value)) markDirty(); }
void PetAssetSettings::setIdleMotion(const QString &value) { if (assignString(_idle_motion, value)) markDirty(); }
void PetAssetSettings::setSpeakingMotion(const QString &value) { if (assignString(_speaking_motion, value)) markDirty(); }
void PetAssetSettings::setFallbackExpression(const QString &value) { if (assignString(_fallback_expression, value)) markDirty(); }
void PetAssetSettings::setPersonalityTags(const QString &value) { if (assignString(_personality_tags, value)) markDirty(); }
void PetAssetSettings::setRelationshipStyle(const QString &value) { if (assignString(_relationship_style, value)) markDirty(); }
void PetAssetSettings::setWorldSetting(const QString &value) { if (assignString(_world_setting, value)) markDirty(); }
void PetAssetSettings::setSpeechRules(const QString &value) { if (assignString(_speech_rules, value)) markDirty(); }
void PetAssetSettings::setCatchphrases(const QString &value) { if (assignString(_catchphrases, value)) markDirty(); }
void PetAssetSettings::setForbiddenRules(const QString &value) { if (assignString(_forbidden_rules, value)) markDirty(); }
void PetAssetSettings::setToneIndex(int value) { if (assignInt(_tone_index, value, 0, 3)) markDirty(); }
void PetAssetSettings::setResponseLengthIndex(int value) { if (assignInt(_response_length_index, value, 0, 2)) markDirty(); }
void PetAssetSettings::setLanguageIndex(int value) { if (assignInt(_language_index, value, 0, kLanguageOptionCount - 1)) markDirty(); }
void PetAssetSettings::setEmotionLevel(qreal value) { if (assignReal(_emotion_level, value, 0.0, 1.0)) markDirty(); }
void PetAssetSettings::setCreativityLevel(qreal value) { if (assignReal(_creativity_level, value, 0.0, 1.0)) markDirty(); }
void PetAssetSettings::setVoiceSpeed(qreal value) { if (assignReal(_voice_speed, value, 0.70, 1.35)) markDirty(); }
void PetAssetSettings::setLipSyncSensitivity(qreal value) { if (assignReal(_lip_sync_sensitivity, value, 0.0, 1.0)) markDirty(); }
void PetAssetSettings::setVoiceLipSyncEnabled(bool value) { if (assignBool(_voice_lip_sync_enabled, value)) markDirty(); }
void PetAssetSettings::setEmotionSoundEnabled(bool value) { if (assignBool(_emotion_sound_enabled, value)) markDirty(); }
void PetAssetSettings::setIdleMotionEnabled(bool value) { if (assignBool(_idle_motion_enabled, value)) markDirty(); }
void PetAssetSettings::setGazeFollowEnabled(bool value) { if (assignBool(_gaze_follow_enabled, value)) markDirty(); }
void PetAssetSettings::setMemoryEnabled(bool value) { if (assignBool(_memory_enabled, value)) markDirty(); }
void PetAssetSettings::setInterruptEnabled(bool value) { if (assignBool(_interrupt_enabled, value)) markDirty(); }
void PetAssetSettings::setCameraEnabled(bool value) { if (assignBool(_camera_enabled, value)) markDirty(); }
void PetAssetSettings::setCloudVisionEnabled(bool value) { if (assignBool(_cloud_vision_enabled, value)) markDirty(); }
void PetAssetSettings::setAutoStartPetOnClientStart(bool value) { if (assignBool(_auto_start_pet_on_client_start, value)) markDirty(); }
void PetAssetSettings::setVoiceTrainingConsent(bool value) { if (assignBool(_voice_training_consent, value)) markDirty(); }
void PetAssetSettings::setVoiceTrainingConsentScope(const QString &value) { if (assignString(_voice_training_consent_scope, value)) markDirty(); }
void PetAssetSettings::setVoiceTrainingJobId(const QString &value) { if (assignString(_voice_training_job_id, value)) markDirty(); }
void PetAssetSettings::setVoiceTrainingStatus(const QString &value) { if (assignString(_voice_training_status, value)) markDirty(); }
void PetAssetSettings::setVoiceTrainingStage(const QString &value) { if (assignString(_voice_training_stage, value)) markDirty(); }
void PetAssetSettings::setVoiceTrainingProgress(int value) { if (assignInt(_voice_training_progress, value, 0, 100)) markDirty(); }
void PetAssetSettings::setVoiceTrainingArtifactPath(const QString &value) { if (assignString(_voice_training_artifact_path, value)) markDirty(); }
void PetAssetSettings::setVoiceTrainingMessage(const QString &value) { if (assignString(_voice_training_message, value)) markDirty(); }

void PetAssetSettings::normalizeVoiceTrainingState()
{
    const QString normalizedStage = _voice_training_stage.trimmed().toLower();
    const QString normalizedStatus = _voice_training_status.trimmed().toLower();
    if (normalizedStage != QStringLiteral("ready_for_worker")) {
        if (normalizedStatus == QStringLiteral("blocked")
            && (normalizedStage == QStringLiteral("reference_not_visible")
                || normalizedStage == QStringLiteral("reference_unreadable")
                || normalizedStage == QStringLiteral("reference_too_short"))) {
            _voice_training_job_id.clear();
            _voice_training_status = QStringLiteral("idle");
            _voice_training_stage.clear();
            _voice_training_progress = 0;
            _voice_training_artifact_path.clear();
            _voice_training_message = QStringLiteral("等待确认参考音频权限");
        }
        return;
    }

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

bool PetAssetSettings::assignString(QString &target, const QString &value)
{
    if (target == value) {
        return false;
    }
    target = value;
    return true;
}

bool PetAssetSettings::assignInt(int &target, int value, int minimum, int maximum)
{
    const int bounded = qBound(minimum, value, maximum);
    if (target == bounded) {
        return false;
    }
    target = bounded;
    return true;
}

bool PetAssetSettings::assignReal(qreal &target, qreal value, qreal minimum, qreal maximum)
{
    const qreal bounded = qBound(minimum, value, maximum);
    if (qFuzzyCompare(target + 1.0, bounded + 1.0)) {
        return false;
    }
    target = bounded;
    return true;
}

bool PetAssetSettings::assignBool(bool &target, bool value)
{
    if (target == value) {
        return false;
    }
    target = value;
    return true;
}

void PetAssetSettings::applyDefaults(bool dirty)
{
    _character_name = QStringLiteral("香风智乃");
    _role_identity = QStringLiteral("Live2D 桌宠助手");
    _model_root = clientSourcePath(QStringLiteral("src/KafuuChino/香风智乃live2D"));
    _model_json = clientSourcePath(QStringLiteral("src/KafuuChino/香风智乃live2D/香风智乃.model3.json"));
    _motion_directory = _model_root;
    _expression_directory = _model_root;
    _voice_directory = clientSourcePath(QStringLiteral("src/KafuuChino/香风智乃voice"));
    _default_voice = QStringLiteral("Kafuuchino-voice.mp3");
    _idle_motion = QStringLiteral("Idle");
    _speaking_motion = QStringLiteral("Talk");
    _fallback_expression = QStringLiteral("脸红");
    _personality_tags = QStringLiteral("认真, 安静, 可靠, 轻声提醒");
    _relationship_style = QStringLiteral("熟悉但不过界的桌面同伴");
    _world_setting = QStringLiteral("以香风智乃素材作为本地默认角色，在 MemoChat 旁边陪用户聊天、学习和整理资料。");
    _speech_rules = QStringLiteral("使用当前选择的单一语言回复，不混用中文、日语或英语。少说套话，先回应情绪，再给明确建议。");
    _catchphrases = QStringLiteral("收到，我会记住。\n先别急，我们一步一步来。");
    _forbidden_rules = QStringLiteral("不要伪装成真人；不要主动索要隐私；不替用户做高风险决定。");
    _tone_index = 0;
    _response_length_index = 1;
    _language_index = 0;
    _emotion_level = 0.62;
    _creativity_level = 0.48;
    _voice_speed = 1.0;
    _lip_sync_sensitivity = 0.55;
    _voice_lip_sync_enabled = true;
    _emotion_sound_enabled = true;
    _idle_motion_enabled = true;
    _gaze_follow_enabled = true;
    _memory_enabled = true;
    _interrupt_enabled = true;
    _camera_enabled = false;
    _cloud_vision_enabled = false;
    _auto_start_pet_on_client_start = false;
    _voice_training_consent = false;
    _voice_training_consent_scope = QStringLiteral("local_default_reference");
    _voice_training_job_id.clear();
    _voice_training_status = QStringLiteral("idle");
    _voice_training_stage.clear();
    _voice_training_progress = 0;
    _voice_training_artifact_path.clear();
    _voice_training_message = QStringLiteral("等待确认参考音频权限");
    _dirty = dirty;
    _status_text = dirty ? QStringLiteral("默认草稿待保存") : QStringLiteral("默认草稿已载入");
}

void PetAssetSettings::applyObject(const QVariantMap &values, bool dirty)
{
    applyDefaults(dirty);
    _character_name = stringValue(values, QStringLiteral("characterName"), _character_name);
    _role_identity = stringValue(values, QStringLiteral("roleIdentity"), _role_identity);
    _model_root = stringValue(values, QStringLiteral("modelRoot"), _model_root);
    _model_json = stringValue(values, QStringLiteral("modelJson"), _model_json);
    _motion_directory = stringValue(values, QStringLiteral("motionDirectory"), _motion_directory);
    _expression_directory = stringValue(values, QStringLiteral("expressionDirectory"), _expression_directory);
    _voice_directory = stringValue(values, QStringLiteral("voiceDirectory"), _voice_directory);
    _default_voice = stringValue(values, QStringLiteral("defaultVoice"), _default_voice);
    _idle_motion = stringValue(values, QStringLiteral("idleMotion"), _idle_motion);
    _speaking_motion = stringValue(values, QStringLiteral("speakingMotion"), _speaking_motion);
    _fallback_expression = stringValue(values, QStringLiteral("fallbackExpression"), _fallback_expression);
    _personality_tags = stringValue(values, QStringLiteral("personalityTags"), _personality_tags);
    _relationship_style = stringValue(values, QStringLiteral("relationshipStyle"), _relationship_style);
    _world_setting = stringValue(values, QStringLiteral("worldSetting"), _world_setting);
    _speech_rules = stringValue(values, QStringLiteral("speechRules"), _speech_rules);
    _catchphrases = stringValue(values, QStringLiteral("catchphrases"), _catchphrases);
    _forbidden_rules = stringValue(values, QStringLiteral("forbiddenRules"), _forbidden_rules);
    _tone_index = intValue(values, QStringLiteral("toneIndex"), _tone_index, 0, 3);
    _response_length_index = intValue(values, QStringLiteral("responseLengthIndex"), _response_length_index, 0, 2);
    _language_index = intValue(values, QStringLiteral("languageIndex"), _language_index, 0, kLanguageOptionCount - 1);
    _emotion_level = realValue(values, QStringLiteral("emotionLevel"), _emotion_level, 0.0, 1.0);
    _creativity_level = realValue(values, QStringLiteral("creativityLevel"), _creativity_level, 0.0, 1.0);
    _voice_speed = realValue(values, QStringLiteral("voiceSpeed"), _voice_speed, 0.70, 1.35);
    _lip_sync_sensitivity = realValue(values, QStringLiteral("lipSyncSensitivity"), _lip_sync_sensitivity, 0.0, 1.0);
    _voice_lip_sync_enabled = boolValue(values, QStringLiteral("voiceLipSyncEnabled"), _voice_lip_sync_enabled);
    _emotion_sound_enabled = boolValue(values, QStringLiteral("emotionSoundEnabled"), _emotion_sound_enabled);
    _idle_motion_enabled = boolValue(values, QStringLiteral("idleMotionEnabled"), _idle_motion_enabled);
    _gaze_follow_enabled = boolValue(values, QStringLiteral("gazeFollowEnabled"), _gaze_follow_enabled);
    _memory_enabled = boolValue(values, QStringLiteral("memoryEnabled"), _memory_enabled);
    _interrupt_enabled = boolValue(values, QStringLiteral("interruptEnabled"), _interrupt_enabled);
    _camera_enabled = boolValue(values, QStringLiteral("cameraEnabled"), _camera_enabled);
    _cloud_vision_enabled = boolValue(values, QStringLiteral("cloudVisionEnabled"), _cloud_vision_enabled);
    _auto_start_pet_on_client_start = boolValue(values, QStringLiteral("autoStartPetOnClientStart"), _auto_start_pet_on_client_start);
    _voice_training_consent = boolValue(values, QStringLiteral("voiceTrainingConsent"), _voice_training_consent);
    _voice_training_consent_scope = stringValue(values, QStringLiteral("voiceTrainingConsentScope"), _voice_training_consent_scope);
    _voice_training_job_id = stringValue(values, QStringLiteral("voiceTrainingJobId"), _voice_training_job_id);
    _voice_training_status = stringValue(values, QStringLiteral("voiceTrainingStatus"), _voice_training_status);
    _voice_training_stage = stringValue(values, QStringLiteral("voiceTrainingStage"), _voice_training_stage);
    _voice_training_progress = intValue(values, QStringLiteral("voiceTrainingProgress"), _voice_training_progress, 0, 100);
    _voice_training_artifact_path = stringValue(values, QStringLiteral("voiceTrainingArtifactPath"), _voice_training_artifact_path);
    _voice_training_message = stringValue(values, QStringLiteral("voiceTrainingMessage"), _voice_training_message);
    _dirty = dirty;
}

void PetAssetSettings::markDirty()
{
    _dirty = true;
    _status_text = QStringLiteral("本地草稿待保存");
    emit settingsChanged();
}

QString PetAssetSettings::defaultStoragePath() const
{
    QString root = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (root.isEmpty()) {
        root = QDir::home().absoluteFilePath(QStringLiteral(".memochat"));
    }
    return QDir(root).absoluteFilePath(QStringLiteral("pet/live2d-character-draft.json"));
}
