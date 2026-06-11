#ifndef APPHTTPEVENTROUTER_H
#define APPHTTPEVENTROUTER_H

#include <QObject>
#include <QString>
#include <functional>

#include "global.h"

class AppSessionCoordinator;
class ProfileController;

class AppHttpEventRouter : public QObject
{
    Q_OBJECT

public:
    AppHttpEventRouter(AppSessionCoordinator& sessionCoordinator,
                       ProfileController& profileController,
                       std::function<void()> emitSettingsStatusChanged,
                       QObject* parent = nullptr);

public slots:
    void onLoginHttpFinished(ReqId id, QString res, ErrorCodes err);
    void onRegisterHttpFinished(ReqId id, QString res, ErrorCodes err);
    void onResetHttpFinished(ReqId id, QString res, ErrorCodes err);
    void onSettingsHttpFinished(ReqId id, QString res, ErrorCodes err);

private:
    AppSessionCoordinator& _session_coordinator;
    ProfileController& _profile_controller;
    std::function<void()> _emit_settings_status_changed;
};

#endif // APPHTTPEVENTROUTER_H
