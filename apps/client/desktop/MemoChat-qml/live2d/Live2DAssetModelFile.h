#ifndef LIVE2DASSETMODELFILE_H
#define LIVE2DASSETMODELFILE_H

#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace Live2DAssetModelFile
{

struct Result
{
    bool loaded = false;
    QString terminalStatus;
    QString terminalStatusText;
    QString modelJsonPath;
    QString modelDirectory;
    QJsonObject refs;
};

Result load(const QString& modelRoot,
            const QString& modelJson,
            QStringList& errors,
            QStringList& warnings,
            QStringList& packageFiles);

} // namespace Live2DAssetModelFile

#endif // LIVE2DASSETMODELFILE_H
