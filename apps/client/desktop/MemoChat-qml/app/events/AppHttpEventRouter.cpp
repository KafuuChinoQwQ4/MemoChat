#include "AppHttpEventRouter.h"

#include "AppCoordinators.h"
#include "ContactController.h"
#include "ProfileController.h"

AppHttpEventRouter::AppHttpEventRouter(AppSessionCoordinator& sessionCoordinator,
                                       ProfileController& profileController,
                                       ContactController& contactController,
                                       QObject* parent)
    : QObject(parent)
    , _session_coordinator(sessionCoordinator)
    , _profile_controller(profileController)
    , _contact_controller(contactController)
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

void AppHttpEventRouter::onContactHttpFinished(ReqId id, QString res, ErrorCodes err)
{
    _contact_controller.handleContactHttpFinished(id, res, err);
}
