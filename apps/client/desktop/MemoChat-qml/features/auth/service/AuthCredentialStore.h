#ifndef AUTHCREDENTIALSTORE_H
#define AUTHCREDENTIALSTORE_H

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QString>
#include <QStringList>

#ifdef Q_OS_UNIX
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

class AuthCredentialStore
{
public:
    QString credentialCacheJson() const;
    void saveLoginCredential(const QString& email, const QString& password) const;
};

namespace AuthCredentialStoreDetail
{
inline constexpr int kMaxLoginCredentialCache = 8;
inline constexpr const char kLoginCredentialSettingsGroup[] = "LoginCredentialCache";
inline constexpr const char kLoginCredentialCacheKey[] = "credentialCacheJson";

inline QSettings makeAuthSettings()
{
    return QSettings(QStringLiteral("MemoChat"), QStringLiteral("MemoChatQml"));
}

inline QString compactCredentialJson(const QJsonArray& credentials)
{
    return QString::fromUtf8(QJsonDocument(credentials).toJson(QJsonDocument::Compact));
}

inline QJsonArray sanitizedCredentialArray(const QString& json)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isArray())
    {
        return {};
    }

    QJsonArray sanitized;
    QStringList seenEmails;
    const QJsonArray current = doc.array();
    for (const QJsonValue& value : current)
    {
        if (!value.isObject())
        {
            continue;
        }
        const QJsonObject obj = value.toObject();
        const QString itemEmail = obj.value(QStringLiteral("email")).toString().trimmed();
        if (itemEmail.isEmpty())
        {
            continue;
        }
        const QString itemEmailKey = itemEmail.toLower();
        if (seenEmails.contains(itemEmailKey))
        {
            continue;
        }
        seenEmails.append(itemEmailKey);
        sanitized.append(QJsonObject{{QStringLiteral("email"), itemEmail}});
        if (sanitized.size() >= kMaxLoginCredentialCache)
        {
            break;
        }
    }
    return sanitized;
}

inline QString sanitizeCredentialCacheJson(const QString& json)
{
    return compactCredentialJson(sanitizedCredentialArray(json));
}

inline bool preparePrivateSettingsStorage(QSettings& settings)
{
#ifdef Q_OS_UNIX
    const QFileInfo settingsInfo(settings.fileName());
    const QString parentPath = settingsInfo.absolutePath();
    constexpr QFileDevice::Permissions ownerDirectory =
        QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner;
    if (!QDir().mkpath(parentPath) || !QFile::setPermissions(parentPath, ownerDirectory))
    {
        return false;
    }

    int flags = O_WRONLY | O_CREAT;
#ifdef O_CLOEXEC
    flags |= O_CLOEXEC;
#endif
#ifdef O_NOFOLLOW
    flags |= O_NOFOLLOW;
#endif
    const QByteArray encodedPath = QFile::encodeName(settingsInfo.absoluteFilePath());
    const int fd = ::open(encodedPath.constData(), flags, S_IRUSR | S_IWUSR);
    if (fd < 0)
    {
        return false;
    }

    struct stat fileStatus{};
    const bool isPrivateRegularFile = ::fstat(fd, &fileStatus) == 0 && S_ISREG(fileStatus.st_mode) &&
                                      fileStatus.st_uid == ::geteuid() && fileStatus.st_nlink == 1 &&
                                      ::fchmod(fd, S_IRUSR | S_IWUSR) == 0;
    const bool closed = ::close(fd) == 0;
    return isPrivateRegularFile && closed;
#else
    Q_UNUSED(settings);
    return true;
#endif
}

inline bool syncWithPrivatePermissions(QSettings& settings)
{
    settings.sync();
    if (settings.status() != QSettings::NoError)
    {
        return false;
    }
#ifdef Q_OS_UNIX
    constexpr QFileDevice::Permissions ownerOnly = QFileDevice::ReadOwner | QFileDevice::WriteOwner;
    return QFile::setPermissions(settings.fileName(), ownerOnly);
#else
    return true;
#endif
}

inline void writeCredentialCacheJson(const QString& json)
{
    QSettings settings = makeAuthSettings();
    if (!preparePrivateSettingsStorage(settings))
    {
        return;
    }
    settings.beginGroup(QString::fromLatin1(kLoginCredentialSettingsGroup));
    settings.setValue(QString::fromLatin1(kLoginCredentialCacheKey), json);
    settings.endGroup();
    if (syncWithPrivatePermissions(settings))
    {
        return;
    }

    settings.beginGroup(QString::fromLatin1(kLoginCredentialSettingsGroup));
    settings.remove(QString::fromLatin1(kLoginCredentialCacheKey));
    settings.endGroup();
    settings.sync();
}
} // namespace AuthCredentialStoreDetail

inline QString AuthCredentialStore::credentialCacheJson() const
{
    QSettings settings = AuthCredentialStoreDetail::makeAuthSettings();
    if (!AuthCredentialStoreDetail::preparePrivateSettingsStorage(settings))
    {
        return QStringLiteral("[]");
    }
    settings.beginGroup(QString::fromLatin1(AuthCredentialStoreDetail::kLoginCredentialSettingsGroup));
    const QString json = settings.value(QString::fromLatin1(AuthCredentialStoreDetail::kLoginCredentialCacheKey),
                                        QStringLiteral("[]"))
                                            .toString();
    settings.endGroup();
    const QString sanitizedJson = AuthCredentialStoreDetail::sanitizeCredentialCacheJson(json);
    if (sanitizedJson != json.trimmed())
    {
        AuthCredentialStoreDetail::writeCredentialCacheJson(sanitizedJson);
    }
    return sanitizedJson;
}

inline void AuthCredentialStore::saveLoginCredential(const QString& email, const QString&) const
{
    const QString normalizedEmail = email.trimmed();
    if (normalizedEmail.isEmpty())
    {
        return;
    }

    QJsonArray next;
    next.append(QJsonObject{{QStringLiteral("email"), normalizedEmail}});

    const QString normalizedLower = normalizedEmail.toLower();
    const QJsonArray current = AuthCredentialStoreDetail::sanitizedCredentialArray(credentialCacheJson());
    for (const QJsonValue& value : current)
    {
        if (!value.isObject())
        {
            continue;
        }
        const QJsonObject obj = value.toObject();
        const QString itemEmail = obj.value(QStringLiteral("email")).toString().trimmed();
        if (itemEmail.isEmpty() || itemEmail.toLower() == normalizedLower)
        {
            continue;
        }
        next.append(QJsonObject{{QStringLiteral("email"), itemEmail}});
        if (next.size() >= AuthCredentialStoreDetail::kMaxLoginCredentialCache)
        {
            break;
        }
    }

    AuthCredentialStoreDetail::writeCredentialCacheJson(AuthCredentialStoreDetail::compactCredentialJson(next));
}

#endif // AUTHCREDENTIALSTORE_H
