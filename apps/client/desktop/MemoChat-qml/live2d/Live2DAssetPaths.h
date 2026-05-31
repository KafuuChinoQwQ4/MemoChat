#ifndef LIVE2DASSETPATHS_H
#define LIVE2DASSETPATHS_H

#include <QJsonValue>
#include <QString>
#include <QStringList>

namespace Live2DAssetPaths
{

QString cleanedInput(const QString& value);
QString resolveInputPath(const QString& value, const QString& baseDirectory = QString());
QString resolveModelReference(const QString& modelDirectory, const QString& reference);
int countFilesWithSuffixes(const QString& directoryPath, const QStringList& suffixes);
QStringList jsonStringArray(const QJsonValue& value);

} // namespace Live2DAssetPaths

#endif // LIVE2DASSETPATHS_H
