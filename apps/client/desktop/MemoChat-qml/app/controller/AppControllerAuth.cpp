#include "AppController.h"
#include "AppCoordinators.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSettings>

namespace
{
constexpr int kMaxLoginCredentialCache = 8;
const char kLoginCredentialSettingsGroup[] = "LoginCredentialCache";
const char kLoginCredentialCacheKey[] = "credentialCacheJson";

QSettings makeAppLocalSettings()
{
    return QSettings(QStringLiteral("MemoChat"), QStringLiteral("MemoChatQml"));
}
} // namespace

QString AppController::loginCredentialCacheJson() const
{
    QSettings settings = makeAppLocalSettings();
    settings.beginGroup(QString::fromLatin1(kLoginCredentialSettingsGroup));
    const QString json = settings.value(QString::fromLatin1(kLoginCredentialCacheKey), QStringLiteral("[]")).toString();
    settings.endGroup();
    return json.trimmed().isEmpty() ? QStringLiteral("[]") : json;
}

void AppController::saveLoginCredential(const QString& email, const QString& password)
{
    const QString normalizedEmail = email.trimmed();
    if (normalizedEmail.isEmpty() || password.isEmpty())
    {
        return;
    }

    QJsonArray next;
    next.append(QJsonObject{{QStringLiteral("email"), normalizedEmail}, {QStringLiteral("password"), password}});

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(loginCredentialCacheJson().toUtf8(), &parseError);
    if (parseError.error == QJsonParseError::NoError && doc.isArray())
    {
        const QString normalizedLower = normalizedEmail.toLower();
        const QJsonArray current = doc.array();
        for (const QJsonValue& value : current)
        {
            if (!value.isObject())
                continue;
            const QJsonObject obj = value.toObject();
            const QString itemEmail = obj.value(QStringLiteral("email")).toString().trimmed();
            if (itemEmail.isEmpty() || itemEmail.toLower() == normalizedLower)
                continue;
            next.append(QJsonObject{{QStringLiteral("email"), itemEmail},
                                     {QStringLiteral("password"), obj.value(QStringLiteral("password")).toString()}});
            if (next.size() >= kMaxLoginCredentialCache)
                break;
        }
    }

    QSettings settings = makeAppLocalSettings();
    settings.beginGroup(QString::fromLatin1(kLoginCredentialSettingsGroup));
    settings.setValue(QString::fromLatin1(kLoginCredentialCacheKey),
                      QString::fromUtf8(QJsonDocument(next).toJson(QJsonDocument::Compact)));
    settings.endGroup();
    settings.sync();
    _auth_view_model.syncLoginCredentialCacheJson(loginCredentialCacheJson());
    emit loginCredentialCacheChanged();
}

void AppController::setRegisterSuccessPage(bool success)
{
    if (_shell_state.registerSuccessPage == success)
    {
        return;
    }
    _shell_state.registerSuccessPage = success;
    _auth_view_model.syncRegisterSuccessPage(success);
    emit registerSuccessPageChanged();
}

void AppController::setRegisterCountdown(int seconds)
{
    const int normalized = qMax(0, seconds);
    if (_shell_state.registerCountdown == normalized)
    {
        return;
    }
    _shell_state.registerCountdown = normalized;
    _auth_view_model.syncRegisterCountdown(normalized);
    emit registerCountdownChanged();
}

void AppController::login(const QString& email, const QString& password)
{
    saveLoginCredential(email, password);
    _session_coordinator->login(email, password);
}

void AppController::requestRegisterCode(const QString& email)
{
    _session_coordinator->requestRegisterCode(email);
}

void AppController::registerUser(const QString& user,
                                 const QString& email,
                                 const QString& password,
                                 const QString& confirm,
                                 const QString& verifyCode)
{
    _session_coordinator->registerUser(user, email, password, confirm, verifyCode);
}

void AppController::requestResetCode(const QString& email)
{
    _session_coordinator->requestResetCode(email);
}

void AppController::resetPassword(const QString& user,
                                  const QString& email,
                                  const QString& password,
                                  const QString& verifyCode)
{
    _session_coordinator->resetPassword(user, email, password, verifyCode);
}
