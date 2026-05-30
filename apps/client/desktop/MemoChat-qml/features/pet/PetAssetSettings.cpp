#include "PetAssetSettings.h"

#include <QFileDialog>
#include <QFileInfo>

PetAssetSettings::PetAssetSettings(QObject* parent)
    : QObject(parent)
    , _storage_path(defaultStoragePath())
{
    applyDefaults(false);
}

QString PetAssetSettings::pickLocalFilePath(const QString& title, const QString& startPath, const QString& nameFilter)
{
    QString directory;
    const QString trimmedStartPath = startPath.trimmed();
    if (!trimmedStartPath.isEmpty())
    {
        const QFileInfo info(trimmedStartPath);
        directory = info.isDir() ? info.absoluteFilePath() : info.absolutePath();
    }

    const QString picked = QFileDialog::getOpenFileName(nullptr,
                                                        title.trimmed().isEmpty()
                                                        ? QStringLiteral("选择文件") : title,
                                                                         directory,
                                                                         nameFilter.trimmed().isEmpty()
                                                                         ? QStringLiteral("所有文件 (*.*)")
                                                                         : nameFilter);
    return picked;
}

QString PetAssetSettings::pickLocalDirectoryPath(const QString& title, const QString& startPath)
{
    QString directory = startPath.trimmed();
    if (!directory.isEmpty())
    {
        const QFileInfo info(directory);
        directory = info.isDir() ? info.absoluteFilePath() : info.absolutePath();
    }

    const QString picked = QFileDialog::getExistingDirectory(nullptr,
                                                             title.trimmed().isEmpty()
                                                             ? QStringLiteral("选择目录") : title, directory);
    return picked;
}

void PetAssetSettings::resetToDefaults()
{
    applyDefaults(true);
    _status_text = QStringLiteral("已恢复默认草稿，资源路径留空");
    emit settingsChanged();
}
