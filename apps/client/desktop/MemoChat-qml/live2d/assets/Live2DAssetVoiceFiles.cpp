#include "Live2DAssetVoiceFiles.h"

#include "Live2DAssetPaths.h"
#include "Live2DModelAssetParser.h"

#include <QDir>
#include <QFileInfo>

namespace Live2DAssetVoiceFiles
{

QStringList suffixes()
{
    return {QStringLiteral(".wav"),
                           QStringLiteral(".mp3"),
                                          QStringLiteral(".ogg"), QStringLiteral(".flac"), QStringLiteral(".m4a")};
}

QStringList nameFilters()
{
    return {QStringLiteral("*.wav"),
                           QStringLiteral("*.mp3"),
                                          QStringLiteral("*.ogg"), QStringLiteral("*.flac"), QStringLiteral("*.m4a")};
}

int count(const QString& voiceDirectory)
{
    return Live2DAssetPaths::countFilesWithSuffixes(Live2DAssetPaths::resolveInputPath(voiceDirectory), suffixes());
}

void appendPackageFiles(QStringList& packageFiles, const QString& voiceDirectory)
{
    const QString resolvedVoiceDirectory = Live2DAssetPaths::resolveInputPath(voiceDirectory);
    if (resolvedVoiceDirectory.isEmpty() || Live2DModelAssetParser::isQrcPath(resolvedVoiceDirectory))
    {
        return;
    }

    const QDir voiceDir(resolvedVoiceDirectory);
    const QFileInfoList voiceFiles = voiceDir.entryInfoList(nameFilters(), QDir::Files | QDir::NoDotAndDotDot);
    for (const QFileInfo& voiceFile : voiceFiles)
    {
        Live2DModelAssetParser::appendExistingPackageFile(packageFiles, voiceFile.absoluteFilePath());
    }
}

} // namespace Live2DAssetVoiceFiles
