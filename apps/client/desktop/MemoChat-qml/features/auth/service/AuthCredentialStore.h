#ifndef AUTHCREDENTIALSTORE_H
#define AUTHCREDENTIALSTORE_H

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSettings>
#include <QString>
#include <QStringList>

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

inline void writeCredentialCacheJson(const QString& json)
{
    QSettings settings = makeAuthSettings();
    settings.beginGroup(QString::fromLatin1(kLoginCredentialSettingsGroup));
    settings.setValue(QString::fromLatin1(kLoginCredentialCacheKey), json);
    settings.endGroup();
    settings.sync();
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
