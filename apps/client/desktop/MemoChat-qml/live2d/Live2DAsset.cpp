#include "Live2DAsset.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDirIterator>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QCryptographicHash>
#include <QSet>
#include <QUrl>
#include <QVariantMap>
#include <algorithm>

namespace
{
constexpr auto kStatusEmpty = "empty";
constexpr auto kStatusMissing = "missing";
constexpr auto kStatusInvalid = "invalid";
constexpr auto kStatusReady = "ready";

bool isQrcPath(const QString& path)
{
    return path.startsWith(QStringLiteral(":/")) || path.startsWith(QStringLiteral("qrc:/"));
}

QString normalizedKey(QString value)
{
    return value.trimmed().toLower();
}

bool fileExists(const QString& path)
{
    if (path.isEmpty())
    {
        return false;
    }
    if (isQrcPath(path))
    {
        QFile file(path.startsWith(QStringLiteral("qrc:/")) ? QStringLiteral(":") + QUrl(path).path() : path);
        return file.exists();
    }
    return QFileInfo::exists(path);
}

QString qrcFilePath(const QString& path)
{
    if (path.startsWith(QStringLiteral("qrc:/")))
    {
        return QStringLiteral(":") + QUrl(path).path();
    }
    return path;
}

QString normalizedChecksumPath(const QString& modelDirectory, const QString& path)
{
    if (path.isEmpty())
    {
        return {};
    }
    const QString cleanedPath = QDir::cleanPath(path);
    if (!isQrcPath(cleanedPath) && !modelDirectory.isEmpty() && !isQrcPath(modelDirectory))
    {
        const QDir dir(modelDirectory);
        const QString relative = dir.relativeFilePath(cleanedPath);
        if (!relative.startsWith(QStringLiteral("..")))
        {
            return QDir::fromNativeSeparators(relative);
        }
    }
    return QDir::fromNativeSeparators(cleanedPath);
}

void appendExistingPackageFile(QStringList& files, const QString& path)
{
    if (path.isEmpty() || !fileExists(path) || QFileInfo(path).isDir())
    {
        return;
    }
    const QString cleaned = QDir::cleanPath(path);
    if (!files.contains(cleaned))
    {
        files.append(cleaned);
    }
}

void appendJsonReferenceWarning(QStringList& warnings,
                                const QString& label,
                                const QString& reference,
                                const QString& resolvedPath)
{
    if (reference.trimmed().isEmpty())
    {
        return;
    }
    if (!fileExists(resolvedPath) || QFileInfo(resolvedPath).isDir())
    {
        warnings.append(QStringLiteral("%1不存在：%2").arg(label, reference));
    }
}

bool appendJsonParseWarning(QStringList& warnings, const QString& label, const QString& path, QStringList* packageFiles)
{
    if (path.trimmed().isEmpty())
    {
        return false;
    }
    if (!fileExists(path) || QFileInfo(path).isDir())
    {
        warnings.append(QStringLiteral("%1不存在：%2").arg(label, path));
        return false;
    }
    QFile file(qrcFilePath(path));
    if (!file.open(QIODevice::ReadOnly))
    {
        warnings.append(QStringLiteral("%1无法读取：%2").arg(label, file.errorString()));
        return false;
    }
    QJsonParseError error;
    QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError)
    {
        warnings.append(QStringLiteral("%1格式无效：%2").arg(label, error.errorString()));
        return false;
    }
    if (packageFiles)
    {
        appendExistingPackageFile(*packageFiles, path);
    }
    return true;
}

QString findSiblingVtubeMapping(const QString& modelDirectory)
{
    if (modelDirectory.isEmpty() || isQrcPath(modelDirectory))
    {
        return {};
    }
    const QDir dir(modelDirectory);
    const QFileInfoList files = dir.entryInfoList({QStringLiteral("*.vtube.json")}, QDir::Files | QDir::NoDotAndDotDot);
    if (files.isEmpty())
    {
        return {};
    }
    return files.first().absoluteFilePath();
}

QString computePackageChecksum(const QString& modelDirectory, const QStringList& packageFiles)
{
    QStringList files = packageFiles;
    files.removeDuplicates();
    std::sort(files.begin(),
              files.end(),
              [&modelDirectory](const QString& left, const QString& right)
              {
                  return normalizedChecksumPath(modelDirectory, left) < normalizedChecksumPath(modelDirectory, right);
              });

    QCryptographicHash hash(QCryptographicHash::Sha256);
    for (const QString& path : files)
    {
        QFile file(qrcFilePath(path));
        if (!file.open(QIODevice::ReadOnly))
        {
            continue;
        }
        const QByteArray relative = normalizedChecksumPath(modelDirectory, path).toUtf8();
        hash.addData(relative);
        hash.addData(QByteArrayView("\0", 1));
        hash.addData(file.readAll());
        hash.addData(QByteArrayView("\0", 1));
    }
    return QString::fromLatin1(hash.result().toHex());
}

void appendMissingDirectoryWarning(QStringList& warnings,
                                   const QString& label,
                                   const QString& input,
                                   const QString& resolvedPath)
{
    if (input.trimmed().isEmpty() || isQrcPath(resolvedPath))
    {
        return;
    }
    const QFileInfo info(resolvedPath);
    if (!info.exists() || !info.isDir())
    {
        warnings.append(QStringLiteral("%1不存在：%2").arg(label, input));
    }
}

QString actionDisplayName(const QString& name, const QString& file, const QString& fallback)
{
    const QString trimmedName = name.trimmed();
    if (!trimmedName.isEmpty())
    {
        return trimmedName;
    }
    const QString fileName = QFileInfo(file).fileName();
    const QString lowerName = fileName.toLower();
    QString baseName;
    if (lowerName.endsWith(QStringLiteral(".motion3.json")))
    {
        baseName = fileName.left(fileName.size() - QStringLiteral(".motion3.json").size());
    }
    else if (lowerName.endsWith(QStringLiteral(".exp3.json")))
    {
        baseName = fileName.left(fileName.size() - QStringLiteral(".exp3.json").size());
    }
    else
    {
        baseName = QFileInfo(file).completeBaseName();
    }
    baseName = baseName.trimmed();
    if (!baseName.isEmpty())
    {
        return baseName;
    }
    return fallback.trimmed();
}

QString actionFileKey(const QString& kind, const QString& file)
{
    const QString cleaned = file.trimmed();
    if (cleaned.isEmpty())
    {
        return {};
    }
    QString normalizedFile;
    if (cleaned.startsWith(QStringLiteral("qrc:/")))
    {
        normalizedFile = QStringLiteral("qrc:") + QUrl(cleaned).path();
    }
    else if (cleaned.startsWith(QStringLiteral(":/")))
    {
        normalizedFile = QDir::cleanPath(cleaned);
    }
    else if (QFileInfo(cleaned).isAbsolute())
    {
        normalizedFile = QDir::cleanPath(QFileInfo(cleaned).absoluteFilePath());
    }
    else
    {
        normalizedFile = QDir::cleanPath(cleaned);
    }
    return kind + QStringLiteral("|file|") + normalizedKey(normalizedFile);
}

void appendActionItem(QVariantList& items,
                      QSet<QString>& actionKeys,
                      QSet<QString>& actionFiles,
                      const QString& kind,
                      const QString& name,
                      const QString& trigger,
                      const QString& group,
                      int index,
                      const QString& file,
                      const QString& source)
{
    const QString cleanKind = kind.trimmed();
    QString cleanName = name.trimmed();
    QString cleanTrigger = trigger.trimmed();
    if (cleanTrigger.isEmpty())
    {
        cleanTrigger = cleanName;
    }
    if (cleanName.isEmpty())
    {
        cleanName = cleanTrigger;
    }
    if (cleanKind.isEmpty() || cleanName.isEmpty() || cleanTrigger.isEmpty())
    {
        return;
    }

    const QString fileKey = actionFileKey(cleanKind, file);
    if (!fileKey.isEmpty())
    {
        if (actionFiles.contains(fileKey))
        {
            return;
        }
    }
    else
    {
        const QString itemKey = cleanKind + QLatin1Char('|') + normalizedKey(cleanTrigger);
        if (actionKeys.contains(itemKey))
        {
            return;
        }
    }

    QVariantMap item;
    item[QStringLiteral("kind")] = cleanKind;
    item[QStringLiteral("name")] = cleanName;
    item[QStringLiteral("trigger")] = cleanTrigger;
    item[QStringLiteral("group")] = group.trimmed();
    item[QStringLiteral("index")] = index;
    item[QStringLiteral("file")] = file.trimmed();
    item[QStringLiteral("source")] = source.trimmed();
    items.append(item);
    if (fileKey.isEmpty())
    {
        actionKeys.insert(cleanKind + QLatin1Char('|') + normalizedKey(cleanTrigger));
    }
    else
    {
        actionFiles.insert(fileKey);
    }
}

void appendDirectoryActionItems(QVariantList& items,
                                QSet<QString>& actionKeys,
                                QSet<QString>& actionFiles,
                                const QString& kind,
                                const QString& directoryPath,
                                const QStringList& nameFilters)
{
    if (directoryPath.isEmpty() || isQrcPath(directoryPath))
    {
        return;
    }
    const QDir dir(directoryPath);
    if (!dir.exists())
    {
        return;
    }

    QDirIterator it(dir.absolutePath(), nameFilters, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        const QString path = it.next();
        const QString name = actionDisplayName({}, path, {});
        appendActionItem(items, actionKeys, actionFiles, kind, name, name, {}, -1, path, QStringLiteral("directory"));
    }
}
} // namespace

Live2DAsset::Live2DAsset(QObject* parent)
    : QObject(parent)
{
}

void Live2DAsset::setModelRoot(const QString& value)
{
    if (!assignIfChanged(_model_root, value))
    {
        return;
    }
    emit assetInputsChanged();
}

void Live2DAsset::setModelJson(const QString& value)
{
    if (!assignIfChanged(_model_json, value))
    {
        return;
    }
    emit assetInputsChanged();
}

void Live2DAsset::setMotionDirectory(const QString& value)
{
    if (!assignIfChanged(_motion_directory, value))
    {
        return;
    }
    emit assetInputsChanged();
}

void Live2DAsset::setExpressionDirectory(const QString& value)
{
    if (!assignIfChanged(_expression_directory, value))
    {
        return;
    }
    emit assetInputsChanged();
}

void Live2DAsset::setVoiceDirectory(const QString& value)
{
    if (!assignIfChanged(_voice_directory, value))
    {
        return;
    }
    emit assetInputsChanged();
}

void Live2DAsset::setVtubeMappingFile(const QString& value)
{
    if (!assignIfChanged(_vtube_mapping_file, value))
    {
        return;
    }
    _resolved_vtube_mapping_file.clear();
    emit assetInputsChanged();
    emit validationChanged();
}

void Live2DAsset::validate()
{
    QStringList errors;
    QStringList warnings;
    QStringList packageFiles;
    int motionCount = 0;
    int expressionCount = 0;
    int textureCount = 0;
    QVariantList actionItems;
    QSet<QString> actionKeys;
    QSet<QString> actionFiles;
    QString physicsFile;
    QString poseFile;
    QString userDataFile;
    QString resolvedVtubeMapping;

    const QString modelRootPath = resolveInputPath(_model_root);
    if (cleanedInput(_model_root).isEmpty())
    {
        warnings.append(QStringLiteral("模型根目录待填写"));
    }
    else if (!QFileInfo::exists(modelRootPath) || !QFileInfo(modelRootPath).isDir())
    {
        warnings.append(QStringLiteral("模型根目录不存在：%1").arg(_model_root));
    }

    if (cleanedInput(_model_json).isEmpty())
    {
        publishResult(
            QString::fromLatin1(kStatusEmpty),
            QStringLiteral("等待选择 model3.json"),
                errors,
                warnings,
                motionCount,
                expressionCount,
                textureCount,
                countFilesWithSuffixes(
                    resolveInputPath(_voice_directory),
                    {QStringLiteral(".wav"),
                                    QStringLiteral(".mp3"),
                                                   QStringLiteral(".ogg"),
                                                                  QStringLiteral(".flac"), QStringLiteral(".m4a")}),
                                    {},
                                    {},
                                    {},
                                    {},
                                    {},
                                    0);
        return;
    }

    const QString modelJsonPath = resolveInputPath(_model_json, modelRootPath);
    if (!modelJsonPath.endsWith(QStringLiteral(".model3.json"), Qt::CaseInsensitive))
    {
        warnings.append(QStringLiteral("建议选择 .model3.json 文件"));
    }
    if (!fileExists(modelJsonPath) || QFileInfo(modelJsonPath).isDir())
    {
        errors.append(QStringLiteral("model3.json 不存在：%1").arg(_model_json));
        publishResult(
            QString::fromLatin1(kStatusMissing),
            errors.first(),
            errors,
            warnings,
            motionCount,
            expressionCount,
            textureCount,
            countFilesWithSuffixes(
                resolveInputPath(_voice_directory),
                {QStringLiteral(".wav"),
                    QStringLiteral(".mp3"), QStringLiteral(".ogg"), QStringLiteral(".flac"), QStringLiteral(".m4a")}),
                    {},
                    {},
                    {},
                    {},
                    {},
                    0);
        return;
    }

    QFile modelFile(qrcFilePath(modelJsonPath));
    if (!modelFile.open(QIODevice::ReadOnly))
    {
        errors.append(QStringLiteral("无法读取 model3.json：%1").arg(modelFile.errorString()));
        publishResult(
            QString::fromLatin1(kStatusInvalid),
            errors.first(),
            errors,
            warnings,
            motionCount,
            expressionCount,
            textureCount,
            countFilesWithSuffixes(
                resolveInputPath(_voice_directory),
                {QStringLiteral(".wav"),
                    QStringLiteral(".mp3"), QStringLiteral(".ogg"), QStringLiteral(".flac"), QStringLiteral(".m4a")}),
                    {},
                    {},
                    {},
                    {},
                    {},
                    0);
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(modelFile.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
    {
        errors.append(QStringLiteral("model3.json 格式无效：%1").arg(parseError.errorString()));
        publishResult(
            QString::fromLatin1(kStatusInvalid),
            errors.first(),
            errors,
            warnings,
            motionCount,
            expressionCount,
            textureCount,
            countFilesWithSuffixes(
                resolveInputPath(_voice_directory),
                {QStringLiteral(".wav"),
                    QStringLiteral(".mp3"), QStringLiteral(".ogg"), QStringLiteral(".flac"), QStringLiteral(".m4a")}),
                    {},
                    {},
                    {},
                    {},
                    {},
                    0);
        return;
    }

    const QFileInfo modelInfo(modelJsonPath);
    const QString modelDirectory = isQrcPath(modelJsonPath)
                                       ? QString(modelJsonPath).left(modelJsonPath.lastIndexOf(QLatin1Char('/')))
                                       : modelInfo.absolutePath();
    const QJsonObject model = document.object();
    const QJsonObject refs = model.value(QStringLiteral("FileReferences")).toObject();
    appendExistingPackageFile(packageFiles, modelJsonPath);
    if (refs.isEmpty())
    {
        errors.append(QStringLiteral("model3.json 缺少 FileReferences"));
    }

    const QString moc = refs.value(QStringLiteral("Moc")).toString().trimmed();
    if (moc.isEmpty())
    {
        errors.append(QStringLiteral("model3.json 缺少 FileReferences.Moc"));
    }
    else
    {
        const QString resolvedMoc = resolveModelReference(modelDirectory, moc);
        if (!fileExists(resolvedMoc))
        {
            errors.append(QStringLiteral("Moc 文件不存在：%1").arg(moc));
        }
        else
        {
            appendExistingPackageFile(packageFiles, resolvedMoc);
        }
    }

    const QStringList textures = jsonStringArray(refs.value(QStringLiteral("Textures")));
    textureCount = textures.size();
    if (textures.isEmpty())
    {
        warnings.append(QStringLiteral("model3.json 未声明贴图"));
    }
    for (const QString& texture : textures)
    {
        const QString resolvedTexture = resolveModelReference(modelDirectory, texture);
        if (!fileExists(resolvedTexture))
        {
            errors.append(QStringLiteral("贴图文件不存在：%1").arg(texture));
        }
        else
        {
            appendExistingPackageFile(packageFiles, resolvedTexture);
        }
    }

    const QString physicsReference = refs.value(QStringLiteral("Physics")).toString().trimmed();
    if (!physicsReference.isEmpty())
    {
        physicsFile = physicsReference;
        const QString resolvedPhysics = resolveModelReference(modelDirectory, physicsReference);
        appendJsonReferenceWarning(warnings, QStringLiteral("物理文件"), physicsReference, resolvedPhysics);
        if (fileExists(resolvedPhysics) && !QFileInfo(resolvedPhysics).isDir())
        {
            physicsFile = resolvedPhysics;
            appendJsonParseWarning(warnings, QStringLiteral("物理文件"), resolvedPhysics, &packageFiles);
        }
    }

    const QString poseReference = refs.value(QStringLiteral("Pose")).toString().trimmed();
    if (!poseReference.isEmpty())
    {
        poseFile = poseReference;
        const QString resolvedPose = resolveModelReference(modelDirectory, poseReference);
        appendJsonReferenceWarning(warnings, QStringLiteral("姿势文件"), poseReference, resolvedPose);
        if (fileExists(resolvedPose) && !QFileInfo(resolvedPose).isDir())
        {
            poseFile = resolvedPose;
            appendJsonParseWarning(warnings, QStringLiteral("姿势文件"), resolvedPose, &packageFiles);
        }
    }

    const QString userDataReference = refs.value(QStringLiteral("UserData")).toString().trimmed();
    if (!userDataReference.isEmpty())
    {
        userDataFile = userDataReference;
        const QString resolvedUserData = resolveModelReference(modelDirectory, userDataReference);
        appendJsonReferenceWarning(warnings, QStringLiteral("用户数据文件"), userDataReference, resolvedUserData);
        if (fileExists(resolvedUserData) && !QFileInfo(resolvedUserData).isDir())
        {
            userDataFile = resolvedUserData;
            appendJsonParseWarning(warnings, QStringLiteral("用户数据文件"), resolvedUserData, &packageFiles);
        }
    }

    resolvedVtubeMapping = cleanedInput(_vtube_mapping_file).isEmpty()
                               ? findSiblingVtubeMapping(modelDirectory)
                               : resolveInputPath(_vtube_mapping_file, modelDirectory);
    if (!resolvedVtubeMapping.isEmpty())
    {
        appendJsonParseWarning(warnings, QStringLiteral("VTube 映射"), resolvedVtubeMapping, &packageFiles);
    }

    const QJsonObject motions = refs.value(QStringLiteral("Motions")).toObject();
    for (auto category = motions.constBegin(); category != motions.constEnd(); ++category)
    {
        const QJsonArray items = category.value().toArray();
        int motionIndex = 0;
        for (const QJsonValue& itemValue : items)
        {
            const QJsonObject item = itemValue.toObject();
            const QString file = item.value(QStringLiteral("File")).toString().trimmed();
            if (file.isEmpty())
            {
                ++motionIndex;
                continue;
            }
            ++motionCount;
            const QString group = category.key();
            const QString resolvedMotion = resolveModelReference(modelDirectory, file);
            const QString displayName =
                actionDisplayName(item.value(QStringLiteral("Name")).toString(),
                                             resolvedMotion,
                                             QStringLiteral("%1 %2").arg(group).arg(motionIndex + 1));
            if (!fileExists(resolvedMotion))
            {
                warnings.append(QStringLiteral("动作文件不存在：%1").arg(file));
                ++motionIndex;
                continue;
            }
            appendActionItem(actionItems,
                             actionKeys,
                             actionFiles,
                             QStringLiteral("motion"),
                                            displayName,
                                            QStringLiteral("%1#%2").arg(group).arg(motionIndex),
                                                           group,
                                                           motionIndex,
                                                           resolvedMotion,
                                                           QStringLiteral("model3"));
            appendExistingPackageFile(packageFiles, resolvedMotion);
            ++motionIndex;
        }
    }

    const QJsonArray expressions = refs.value(QStringLiteral("Expressions")).toArray();
    for (const QJsonValue& itemValue : expressions)
    {
        const QJsonObject item = itemValue.toObject();
        const QString file = item.value(QStringLiteral("File")).toString().trimmed();
        if (file.isEmpty())
        {
            continue;
        }
        ++expressionCount;
        const QString resolvedExpression = resolveModelReference(modelDirectory, file);
        const QString displayName = actionDisplayName(
            item.value(QStringLiteral("Name")).toString(), resolvedExpression, QFileInfo(file).completeBaseName());
        if (!fileExists(resolvedExpression))
        {
            warnings.append(QStringLiteral("表情文件不存在：%1").arg(file));
            continue;
        }
        appendActionItem(actionItems,
                         actionKeys,
                         actionFiles,
                         QStringLiteral("expression"),
                                        displayName,
                                        displayName,
                                        {},
                                        -1,
                                        resolvedExpression,
                                        QStringLiteral("model3"));
        appendExistingPackageFile(packageFiles, resolvedExpression);
    }

    const QString resolvedMotionDirectory = cleanedInput(_motion_directory).isEmpty()
                                                ? modelDirectory
                                                : resolveInputPath(_motion_directory, modelDirectory);
    const QString resolvedExpressionDirectory = cleanedInput(_expression_directory).isEmpty()
                                                    ? modelDirectory
                                                    : resolveInputPath(_expression_directory, modelDirectory);
    const QString resolvedVoiceDirectory = resolveInputPath(_voice_directory);
    appendMissingDirectoryWarning(warnings, QStringLiteral("动作目录"), _motion_directory, resolvedMotionDirectory);
    appendMissingDirectoryWarning(warnings,
                                  QStringLiteral("表情目录"), _expression_directory, resolvedExpressionDirectory);
    appendMissingDirectoryWarning(warnings, QStringLiteral("语音目录"), _voice_directory, resolvedVoiceDirectory);

    appendDirectoryActionItems(actionItems,
                               actionKeys,
                               actionFiles,
                               QStringLiteral("motion"), resolvedMotionDirectory, {QStringLiteral("*.motion3.json")});
    appendDirectoryActionItems(
        actionItems,
        actionKeys,
        actionFiles,
        QStringLiteral("expression"), resolvedExpressionDirectory, {QStringLiteral("*.exp3.json")});

    motionCount = qMax(motionCount, countFilesWithSuffixes(resolvedMotionDirectory, {QStringLiteral(".motion3.json")}));
    expressionCount =
        qMax(expressionCount, countFilesWithSuffixes(resolvedExpressionDirectory, {QStringLiteral(".exp3.json")}));
    const int voiceCount = countFilesWithSuffixes(
        resolvedVoiceDirectory,
        {QStringLiteral(".wav"),
                        QStringLiteral(".mp3"),
                                       QStringLiteral(".ogg"), QStringLiteral(".flac"), QStringLiteral(".m4a")});
    if (!resolvedVoiceDirectory.isEmpty() && !isQrcPath(resolvedVoiceDirectory))
    {
        const QDir voiceDir(resolvedVoiceDirectory);
        const QFileInfoList voiceFiles = voiceDir.entryInfoList(
            {QStringLiteral("*.wav"),
                            QStringLiteral("*.mp3"),
                                           QStringLiteral("*.ogg"),
                                                          QStringLiteral("*.flac"), QStringLiteral("*.m4a")},
                                                          QDir::Files | QDir::NoDotAndDotDot);
        for (const QFileInfo& voiceFile : voiceFiles)
        {
            appendExistingPackageFile(packageFiles, voiceFile.absoluteFilePath());
        }
    }

    const QString status = errors.isEmpty() ? QString::fromLatin1(kStatusReady) : QString::fromLatin1(kStatusInvalid);
    const QString statusText = errors.isEmpty() ? QStringLiteral("资源校验通过：%1 动作 / %2 表情 / %3 贴图 / %4 语音")
                                                                     .arg(motionCount)
                                                                     .arg(expressionCount)
                                                                     .arg(textureCount)
                                                                     .arg(voiceCount)
                                                : errors.first();
    const QString checksum = errors.isEmpty() ? computePackageChecksum(modelDirectory, packageFiles) : QString();
    publishResult(status,
                  statusText,
                  errors,
                  warnings,
                  motionCount,
                  expressionCount,
                  textureCount,
                  voiceCount,
                  physicsFile,
                  poseFile,
                  userDataFile,
                  resolvedVtubeMapping,
                  checksum,
                  checksum.isEmpty() ? 0 : packageFiles.size(),
                  actionItems);
}

void Live2DAsset::clear()
{
    _model_root.clear();
    _model_json.clear();
    _motion_directory.clear();
    _expression_directory.clear();
    _voice_directory.clear();
    _vtube_mapping_file.clear();
    emit assetInputsChanged();
    publishResult(QString::fromLatin1(kStatusEmpty),
                  QStringLiteral("等待选择 model3.json"), {}, {}, 0, 0, 0, 0, {}, {}, {}, {}, {}, 0);
}

bool Live2DAsset::assignIfChanged(QString& target, const QString& value)
{
    if (target == value)
    {
        return false;
    }
    target = value;
    return true;
}

QString Live2DAsset::cleanedInput(const QString& value)
{
    return value.trimmed();
}

QString Live2DAsset::resolveInputPath(const QString& value, const QString& baseDirectory)
{
    const QString cleaned = cleanedInput(value);
    if (cleaned.isEmpty())
    {
        return {};
    }

    const QUrl url(cleaned);
    if (url.isLocalFile())
    {
        return QDir::cleanPath(url.toLocalFile());
    }
    if (cleaned.startsWith(QStringLiteral("qrc:/")) || cleaned.startsWith(QStringLiteral(":/")))
    {
        return cleaned;
    }
    if (QDir::isAbsolutePath(cleaned))
    {
        return QDir::cleanPath(cleaned);
    }

    QStringList candidates;
    if (!baseDirectory.isEmpty() && !cleaned.contains(QLatin1Char('/')) && !cleaned.contains(QLatin1Char('\\')))
    {
        candidates.append(QDir(baseDirectory).absoluteFilePath(cleaned));
    }
    candidates.append(QDir::current().absoluteFilePath(cleaned));
    if (!baseDirectory.isEmpty())
    {
        candidates.append(QDir(baseDirectory).absoluteFilePath(cleaned));
    }

    QSet<QString> seen;
    for (const QString& candidate : candidates)
    {
        const QString path = QDir::cleanPath(candidate);
        if (seen.contains(path))
        {
            continue;
        }
        seen.insert(path);
        if (fileExists(path))
        {
            return path;
        }
    }
    return QDir::cleanPath(candidates.value(0, QDir::current().absoluteFilePath(cleaned)));
}

QString Live2DAsset::resolveModelReference(const QString& modelDirectory, const QString& reference)
{
    const QString cleaned = cleanedInput(reference);
    if (cleaned.isEmpty())
    {
        return {};
    }
    if (cleaned.startsWith(QStringLiteral("qrc:/")) || cleaned.startsWith(QStringLiteral(":/")))
    {
        return cleaned;
    }
    const QUrl url(cleaned);
    if (url.isLocalFile())
    {
        return QDir::cleanPath(url.toLocalFile());
    }
    if (QDir::isAbsolutePath(cleaned))
    {
        return QDir::cleanPath(cleaned);
    }
    if (isQrcPath(modelDirectory))
    {
        return QDir::cleanPath(modelDirectory + QLatin1Char('/') + cleaned);
    }
    return QDir(modelDirectory).absoluteFilePath(cleaned);
}

int Live2DAsset::countFilesWithSuffixes(const QString& directoryPath, const QStringList& suffixes)
{
    if (directoryPath.isEmpty() || isQrcPath(directoryPath))
    {
        return 0;
    }
    const QDir dir(directoryPath);
    if (!dir.exists())
    {
        return 0;
    }

    int count = 0;
    const QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    for (const QFileInfo& file : files)
    {
        const QString name = file.fileName().toLower();
        for (const QString& suffix : suffixes)
        {
            if (name.endsWith(suffix))
            {
                ++count;
                break;
            }
        }
    }
    return count;
}

QStringList Live2DAsset::jsonStringArray(const QJsonValue& value)
{
    QStringList items;
    const QJsonArray array = value.toArray();
    for (const QJsonValue& item : array)
    {
        const QString text = item.toString().trimmed();
        if (!text.isEmpty())
        {
            items.append(text);
        }
    }
    return items;
}

void Live2DAsset::publishResult(const QString& status,
                                const QString& statusText,
                                const QStringList& errors,
                                const QStringList& warnings,
                                int motionCount,
                                int expressionCount,
                                int textureCount,
                                int voiceCount,
                                const QString& physicsFile,
                                const QString& poseFile,
                                const QString& userDataFile,
                                const QString& vtubeMappingFile,
                                const QString& packageChecksum,
                                int referencedFileCount,
                                const QVariantList& actionItems)
{
    _status = status;
    _valid = status == QString::fromLatin1(kStatusReady);
    _status_text = statusText;
    _errors = errors;
    _warnings = warnings;
    _motion_count = motionCount;
    _expression_count = expressionCount;
    _texture_count = textureCount;
    _voice_count = voiceCount;
    _physics_file = physicsFile;
    _pose_file = poseFile;
    _user_data_file = userDataFile;
    _resolved_vtube_mapping_file = vtubeMappingFile;
    _package_checksum = packageChecksum;
    _referenced_file_count = referencedFileCount;
    _action_items = actionItems;
    emit validationChanged();
}
