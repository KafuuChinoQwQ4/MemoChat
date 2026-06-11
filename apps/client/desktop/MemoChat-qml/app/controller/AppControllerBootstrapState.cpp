#include "AppController.h"

void AppController::setDialogsReady(bool ready)
{
    if (_shell_state.bootstrapState().dialogsReady == ready)
    {
        return;
    }
    _shell_state.bootstrapState().dialogsReady = ready;
    emit lazyBootstrapStateChanged();
}

void AppController::setContactsReady(bool ready)
{
    _features.contactController.setContactsReady(ready);
}

void AppController::setGroupsReady(bool ready)
{
    if (_shell_state.bootstrapState().groupsReady == ready)
    {
        return;
    }
    _shell_state.bootstrapState().groupsReady = ready;
    syncGroupControllerState();
    emit lazyBootstrapStateChanged();
}

void AppController::setApplyReady(bool ready)
{
    _features.contactController.setApplyReady(ready);
}
