#ifndef AUTHVIEWMODEL_H
#define AUTHVIEWMODEL_H

#include "AuthState.h"

#include <QObject>
#include <QString>

class AuthService;

class AuthViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString tipText READ tipText NOTIFY stateChanged)
    Q_PROPERTY(bool tipError READ tipError NOTIFY stateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(bool registerSuccessPage READ registerSuccessPage NOTIFY stateChanged)
    Q_PROPERTY(int registerCountdown READ registerCountdown NOTIFY stateChanged)
    Q_PROPERTY(int registerCodeCooldownSeconds READ registerCodeCooldownSeconds NOTIFY stateChanged)
    Q_PROPERTY(bool registerCodeRequestPending READ registerCodeRequestPending NOTIFY stateChanged)
    Q_PROPERTY(QString loginCredentialCacheJson READ loginCredentialCacheJson NOTIFY loginCredentialCacheChanged)

public:
    explicit AuthViewModel(AuthService* service, QObject* parent = nullptr);

    QString tipText() const;
    bool tipError() const;
    bool busy() const;
    bool registerSuccessPage() const;
    int registerCountdown() const;
    int registerCodeCooldownSeconds() const;
    bool registerCodeRequestPending() const;
    QString loginCredentialCacheJson() const;

    void syncTip(const QString& text, bool error);
    void syncBusy(bool busy);
    void syncRegisterSuccessPage(bool success);
    void syncRegisterCountdown(int seconds);
    void syncRegisterCodeCooldownSeconds(int seconds);
    void syncRegisterCodeRequestPending(bool pending);
    void syncLoginCredentialCacheJson(const QString& json);

    Q_INVOKABLE void clearTip();
    Q_INVOKABLE void saveLoginCredential(const QString& email, const QString& password);
    Q_INVOKABLE void login(const QString& email, const QString& password);
    Q_INVOKABLE void requestRegisterCode(const QString& email);
    Q_INVOKABLE void registerUser(const QString& user,
                                  const QString& email,
                                  const QString& password,
                                  const QString& confirm,
                                  const QString& verifyCode);
    Q_INVOKABLE void requestResetCode(const QString& email);
    Q_INVOKABLE void
    resetPassword(const QString& user, const QString& email, const QString& password, const QString& verifyCode);

signals:
    void stateChanged();
    void loginCredentialCacheChanged();
    void clearTipRequested();
    void saveLoginCredentialRequested(QString email, QString password);
    void loginRequested(QString email, QString password);
    void registerCodeRequested(QString email);
    void registerUserRequested(QString user, QString email, QString password, QString confirm, QString verifyCode);
    void resetCodeRequested(QString email);
    void resetPasswordRequested(QString user, QString email, QString password, QString verifyCode);

private:
    AuthService* _service = nullptr;
    AuthState _state;
    QString _loginCredentialCacheJson = QStringLiteral("[]");
};

#endif // AUTHVIEWMODEL_H
