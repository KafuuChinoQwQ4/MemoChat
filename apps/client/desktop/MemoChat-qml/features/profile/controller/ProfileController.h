#ifndef PROFILECONTROLLER_H
#define PROFILECONTROLLER_H

#include <QObject>
#include <QString>

class ClientGateway;

class ProfileController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
    Q_PROPERTY(bool statusError READ statusError NOTIFY statusChanged)

public:
    explicit ProfileController(ClientGateway* gateway, QObject* parent = nullptr);

    QString statusText() const;
    bool statusError() const;

    Q_INVOKABLE void chooseAvatar(int source = 0);
    Q_INVOKABLE void saveProfile(const QString& nick, const QString& desc);
    Q_INVOKABLE void clearStatus();

    bool validateProfile(const QString& nick, const QString& desc, QString* errorText) const;
    void
    sendSaveProfile(int uid, const QString& name, const QString& nick, const QString& desc, const QString& icon) const;
    void syncStatus(const QString& text, bool isError);

signals:
    void statusChanged();
    void chooseAvatarRequested(int source);
    void saveProfileRequested(const QString& nick, const QString& desc);
    void clearStatusRequested();

private:
    ClientGateway* _gateway;
    QString _status_text;
    bool _status_error = false;
};

#endif // PROFILECONTROLLER_H
