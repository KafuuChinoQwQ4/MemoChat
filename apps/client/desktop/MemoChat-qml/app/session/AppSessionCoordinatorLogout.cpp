#include "AppCoordinators.h"

#include <utility>

namespace
{
template <typename Handler, typename... Args> void invokeIfSet(const Handler& handler, Args&&... args)
{
    if (handler)
    {
        handler(std::forward<Args>(args)...);
    }
}
} // namespace

void AppSessionCoordinator::resetForLogout()
{
    const int previousUserUid = _logout_port.currentUserUid ? _logout_port.currentUserUid() : 0;

    stopRegisterCountdownTimer();
    invokeIfSet(_logout_port.stopSessionTimers);
    setIgnoreNextLoginDisconnect(true);

    invokeIfSet(_logout_port.resetPresenceSurfaces);
    clearSessionForLogout();
    invokeIfSet(_logout_port.resetConnectionRuntime);
    invokeIfSet(_logout_port.closeNetworkResources);

    invokeIfSet(_logout_port.resetAuthShellState);
    invokeIfSet(_logout_port.resetFeatureModelsForLogout);
    invokeIfSet(_logout_port.resetFeatureRuntimeForLogout);
    invokeIfSet(_logout_port.clearCurrentUserState, previousUserUid);

    clearSessionForLogout();
    invokeIfSet(_logout_port.resetMediaRuntimeForLogout);
    invokeIfSet(_logout_port.clearDownloadAuthContext);
}
