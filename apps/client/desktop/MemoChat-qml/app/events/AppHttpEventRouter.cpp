#include "AppHttpEventRouter.h"

#include "AppCoordinators.h"
#include "ProfileController.h"

AppHttpEventRouter::AppHttpEventRouter(AppSessionCoordinator& sessionCoordinator,
                                       ProfileController& profileController,
                                       QObject* parent)
    : QObject(parent)
    , _session_coordinator(sessionCoordinator)
    , _profile_controller(profileController)
{
}

void AppHttpEventRouter::onLoginHttpFinished(ReqId id, QString res, ErrorCodes err)
{
    _session_coordinator.onLoginHttpFinished(id, res, err);
}

void AppHttpEventRouter::onRegisterHttpFinished(ReqId id, QString res, ErrorCodes err)
{
    _session_coordinator.onRegisterHttpFinished(id, res, err);
}

void AppHttpEventRouter::onResetHttpFinished(ReqId id, QString res, ErrorCodes err)
{
    _session_coordinator.onResetHttpFinished(id, res, err);
}

void AppHttpEventRouter::onSettingsHttpFinished(ReqId id, QString res, ErrorCodes err)
{
    _profile_controller.handleSettingsHttpFinished(id, res, err);
}
