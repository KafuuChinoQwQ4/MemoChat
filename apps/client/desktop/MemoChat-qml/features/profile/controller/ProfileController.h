#ifndef PROFILECONTROLLER_H
#define PROFILECONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include <QString>
#include <functional>
#include "global.h"

class ClientGateway;
struct UploadedMediaInfo;

struct ProfileCurrentUserState
{
    int pendingUid = 0;
    int uid = 0;
    QString name;
    QString nick;
    QString icon = QStringLiteral("qrc:/res/head_1.png");
    QString desc;
    QString userId;
    int sex = 0;
};

struct ProfileAppliedUserState
{
    int uid = 0;
    QString name;
    QString nick;
    QString icon = QStringLiteral("qrc:/res/head_1.png");
    QString desc;
    QString userId;
    int sex = 0;
};

struct ProfileStatePort
{
    std::function<ProfileCurrentUserState()> snapshot;
    std::function<void(const ProfileAppliedUserState&)> syncCurrentUser;
};

struct ProfileCommandSnapshot
{
    int selfUid = 0;
    QString selfName;
    QString currentIcon;
    QString uploadToken;
    bool avatarUploadInProgress = false;
};

struct ProfileCommandPort
{
    std::function<ProfileCommandSnapshot()> snapshot;
    std::function<void(bool)> setAvatarUploadInProgress;
    std::function<bool(const QString&, int, const QString&, UploadedMediaInfo*, QString*)> uploadAvatar;
    std::function<void(const QString&)> updateLocalIcon;
};

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
    Q_INVOKABLE void clearStatusState();

    bool validateProfile(const QString& nick, const QString& desc, QString* errorText) const;
    void
    sendSaveProfile(int uid, const QString& name, const QString& nick, const QString& desc, const QString& icon) const;
    void setStatus(const QString& text, bool isError);
    void syncStatus(const QString& text, bool isError);
    void setCommandPort(ProfileCommandPort port);
    void setStatePort(ProfileStatePort port);
    void handleSettingsHttpFinished(ReqId id, const QString& res, ErrorCodes err);
    void applyCurrentUserProfile(const QJsonObject& profile, bool preserveExistingIcon);
    void applyCurrentUserProfile(int uid,
                                 const QString& name,
                                 const QString& nick,
                                 const QString& icon,
                                 const QString& desc,
                                 const QString& userId,
                                 int sex,
                                 bool preserveExistingIcon);

signals:
    void statusChanged();

private:
    ClientGateway* _gateway;
    ProfileCommandPort _command_port;
    ProfileStatePort _state_port;
    QString _status_text;
    bool _status_error = false;
};

#endif // PROFILECONTROLLER_H
