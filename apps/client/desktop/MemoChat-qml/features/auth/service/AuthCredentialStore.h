#ifndef AUTHCREDENTIALSTORE_H
#define AUTHCREDENTIALSTORE_H

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSettings>
#include <QString>

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
} // namespace AuthCredentialStoreDetail

inline QString AuthCredentialStore::credentialCacheJson() const
{
    QSettings settings = AuthCredentialStoreDetail::makeAuthSettings();
    settings.beginGroup(QString::fromLatin1(AuthCredentialStoreDetail::kLoginCredentialSettingsGroup));
    const QString json = settings.value(QString::fromLatin1(AuthCredentialStoreDetail::kLoginCredentialCacheKey),
                                        QStringLiteral("[]"))
                                            .toString();
    settings.endGroup();
    return json.trimmed().isEmpty() ? QStringLiteral("[]") : json;
}

inline void AuthCredentialStore::saveLoginCredential(const QString& email, const QString& password) const
{
    const QString normalizedEmail = email.trimmed();
    if (normalizedEmail.isEmpty() || password.isEmpty())
    {
        return;
    }

    QJsonArray next;
    next.append(QJsonObject{{QStringLiteral("email"), normalizedEmail}, {QStringLiteral("password"), password}});

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(credentialCacheJson().toUtf8(), &parseError);
    if (parseError.error == QJsonParseError::NoError && doc.isArray())
    {
        const QString normalizedLower = normalizedEmail.toLower();
        const QJsonArray current = doc.array();
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
            next.append(QJsonObject{{QStringLiteral("email"), itemEmail},
                                     {QStringLiteral("password"), obj.value(QStringLiteral("password")).toString()}});
            if (next.size() >= AuthCredentialStoreDetail::kMaxLoginCredentialCache)
            {
                break;
            }
        }
    }

    QSettings settings = AuthCredentialStoreDetail::makeAuthSettings();
    settings.beginGroup(QString::fromLatin1(AuthCredentialStoreDetail::kLoginCredentialSettingsGroup));
    settings.setValue(QString::fromLatin1(AuthCredentialStoreDetail::kLoginCredentialCacheKey),
                      QString::fromUtf8(QJsonDocument(next).toJson(QJsonDocument::Compact)));
    settings.endGroup();
    settings.sync();
}

#endif // AUTHCREDENTIALSTORE_H
