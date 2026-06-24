#ifndef APPHTTPEVENTROUTER_H
#define APPHTTPEVENTROUTER_H

#include <QObject>
#include <QString>

#include "global.h"

class AppSessionCoordinator;
class ContactController;
class ProfileController;

class AppHttpEventRouter : public QObject
{
    Q_OBJECT

public:
    AppHttpEventRouter(AppSessionCoordinator& sessionCoordinator,
                       ProfileController& profileController,
                       ContactController& contactController,
                       QObject* parent = nullptr);

public slots:
    void onLoginHttpFinished(ReqId id, QString res, ErrorCodes err);
    void onRegisterHttpFinished(ReqId id, QString res, ErrorCodes err);
    void onResetHttpFinished(ReqId id, QString res, ErrorCodes err);
    void onSettingsHttpFinished(ReqId id, QString res, ErrorCodes err);
    void onContactHttpFinished(ReqId id, QString res, ErrorCodes err);

private:
    AppSessionCoordinator& _session_coordinator;
    ProfileController& _profile_controller;
    ContactController& _contact_controller;
};

#endif // APPHTTPEVENTROUTER_H
