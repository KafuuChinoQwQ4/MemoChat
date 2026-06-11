#include "AppCoordinators.h"
#include "AppPortRegistry.h"

void AppPortRegistry::bindSessionPorts()
{
    _session_coordinator = std::make_unique<AppSessionCoordinator>(makeSessionAuthPort(),
                                                                   makePostLoginBootstrapPort(),
                                                                   makeRelationBootstrapPort(),
                                                                   makeRegisterCountdownPort(),
                                                                   makeSessionLogoutPort());
}
