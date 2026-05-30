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
    emit loginCredentialCacheChanged();
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

bool AppController::parseJson(const QString& res, QJsonObject& obj)
{
    return _auth_controller.parseJson(res, obj);
}

bool AppController::checkEmailValid(const QString& email)
{
    QString errorText;
    if (!_auth_controller.checkEmail(email, &errorText))
    {
        setTip(errorText, true);
        return false;
    }
    return true;
}

bool AppController::checkPwdValid(const QString& password)
{
    QString errorText;
    if (!_auth_controller.checkPassword(password, &errorText))
    {
        setTip(errorText, true);
        return false;
    }
    return true;
}

bool AppController::checkUserValid(const QString& user)
{
    QString errorText;
    if (!_auth_controller.checkUser(user, &errorText))
    {
        setTip(errorText, true);
        return false;
    }
    return true;
}

bool AppController::checkVerifyCodeValid(const QString& code)
{
    QString errorText;
    if (!_auth_controller.checkVerifyCode(code, &errorText))
    {
        setTip(errorText, true);
        return false;
    }
    return true;
}
