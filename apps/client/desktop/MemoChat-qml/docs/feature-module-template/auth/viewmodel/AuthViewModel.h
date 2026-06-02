#ifndef MEMOCHAT_TEMPLATE_AUTHVIEWMODEL_H
#define MEMOCHAT_TEMPLATE_AUTHVIEWMODEL_H

#include "AuthService.h"
#include "AuthState.h"

#include <QObject>

class AuthViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString tipText READ tipText NOTIFY stateChanged)
    Q_PROPERTY(bool tipError READ tipError NOTIFY stateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)

public:
    explicit AuthViewModel(AuthService* service, QObject* parent = nullptr);

    QString tipText() const;
    bool tipError() const;
    bool busy() const;

    Q_INVOKABLE void clearTip();
    Q_INVOKABLE void login(const QString& email, const QString& password);

signals:
    void stateChanged();
    void loginAccepted();

private:
    void setTip(const QString& text, bool error);
    void setBusy(bool busy);

    AuthService* _service = nullptr;
    AuthState _state;
};

#endif // MEMOCHAT_TEMPLATE_AUTHVIEWMODEL_H
