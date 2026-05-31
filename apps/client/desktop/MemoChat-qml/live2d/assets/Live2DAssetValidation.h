#ifndef LIVE2DASSETVALIDATION_H
#define LIVE2DASSETVALIDATION_H

#include <QString>
#include <QStringList>
#include <QVariantList>

namespace Live2DAssetValidation
{

struct Inputs
{
    QString modelRoot;
    QString modelJson;
    QString motionDirectory;
    QString expressionDirectory;
    QString voiceDirectory;
    QString vtubeMappingFile;
};

struct Result
{
    QString status = QStringLiteral("empty");
    QString statusText = QStringLiteral("等待选择 model3.json");
    QStringList errors;
    QStringList warnings;
    int motionCount = 0;
    int expressionCount = 0;
    int textureCount = 0;
    int voiceCount = 0;
    QString physicsFile;
    QString poseFile;
    QString userDataFile;
    QString vtubeMappingFile;
    QString packageChecksum;
    int referencedFileCount = 0;
    QVariantList actionItems;
};

Result validate(const Inputs& inputs);

} // namespace Live2DAssetValidation

#endif // LIVE2DASSETVALIDATION_H
