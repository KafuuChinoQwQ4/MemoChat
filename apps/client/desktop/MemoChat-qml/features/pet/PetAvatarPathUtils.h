#ifndef PETAVATARPATHUTILS_H
#define PETAVATARPATHUTILS_H

#include <QString>
#include <QStringList>

class QCryptographicHash;

namespace memochat::pet_asset_settings::avatar_path_utils
{

bool isQrcPath(const QString& path);
QString qrcFilePath(const QString& path);
QString resolveAvatarInputPath(const QString& value, const QString& baseDirectory = QString());
QString resolveAvatarModelReference(const QString& modelDirectory, const QString& reference);
QString avatarCacheDirectory();
void addPathFingerprint(QCryptographicHash& hash, const QString& path);
QStringList modelTexturePaths(const QString& modelJsonPath, QString* modelDirectory);

} // namespace memochat::pet_asset_settings::avatar_path_utils

#endif // PETAVATARPATHUTILS_H
