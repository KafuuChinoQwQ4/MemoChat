#ifndef LIVE2DASSETVOICEFILES_H
#define LIVE2DASSETVOICEFILES_H

#include <QString>
#include <QStringList>

namespace Live2DAssetVoiceFiles
{

QStringList suffixes();
QStringList nameFilters();
int count(const QString& voiceDirectory);
void appendPackageFiles(QStringList& packageFiles, const QString& voiceDirectory);

} // namespace Live2DAssetVoiceFiles

#endif // LIVE2DASSETVOICEFILES_H
