#include "AppHttpEventRouter.h"

#include "AppCoordinators.h"
#include "ProfileController.h"

#include <utility>

AppHttpEventRouter::AppHttpEventRouter(AppSessionCoordinator& sessionCoordinator,
                                       ProfileController& profileController,
                                       std::function<void()> emitSettingsStatusChanged,
                                       QObject* parent)
    : QObject(parent)
    , _session_coordinator(sessionCoordinator)
    , _profile_controller(profileController)
    , _emit_settings_status_changed(std::move(emitSettingsStatusChanged))
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
    const QString previousText = _profile_controller.statusText();
    const bool previousError = _profile_controller.statusError();
    _profile_controller.handleSettingsHttpFinished(id, res, err);
    if (_profile_controller.statusText() != previousText || _profile_controller.statusError() != previousError)
    {
        _emit_settings_status_changed();
    }
}
