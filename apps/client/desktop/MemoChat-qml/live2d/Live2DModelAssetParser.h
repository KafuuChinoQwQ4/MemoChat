#ifndef LIVE2DMODELASSETPARSER_H
#define LIVE2DMODELASSETPARSER_H

#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace Live2DModelAssetParser
{

bool isQrcPath(const QString& path);
bool fileExists(const QString& path);
QString qrcFilePath(const QString& path);
void appendExistingPackageFile(QStringList& files, const QString& path);
void appendJsonReferenceWarning(QStringList& warnings,
                                const QString& label,
                                const QString& reference,
                                const QString& resolvedPath);
bool appendJsonParseWarning(QStringList& warnings,
                            const QString& label,
                            const QString& path,
                            QStringList* packageFiles);
QString findSiblingVtubeMapping(const QString& modelDirectory);
QString computePackageChecksum(const QString& modelDirectory, const QStringList& packageFiles);
void appendMissingDirectoryWarning(QStringList& warnings,
                                   const QString& label,
                                   const QString& input,
                                   const QString& resolvedPath);

} // namespace Live2DModelAssetParser

#endif // LIVE2DMODELASSETPARSER_H
