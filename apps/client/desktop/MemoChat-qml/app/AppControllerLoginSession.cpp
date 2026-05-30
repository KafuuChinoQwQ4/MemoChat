#include "AppController.h"
#include "AppCoordinators.h"

void AppController::onLoginHttpFinished(ReqId id, QString res, ErrorCodes err)
{
    _session_coordinator->onLoginHttpFinished(id, res, err);
}
