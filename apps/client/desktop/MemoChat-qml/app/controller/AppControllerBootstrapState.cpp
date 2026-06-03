#include "AppController.h"

void AppController::setDialogsReady(bool ready)
{
    if (_bootstrap_state.dialogsReady == ready)
    {
        return;
    }
    _bootstrap_state.dialogsReady = ready;
    emit lazyBootstrapStateChanged();
}

void AppController::setContactsReady(bool ready)
{
    if (_bootstrap_state.contactsReady == ready)
    {
        return;
    }
    _bootstrap_state.contactsReady = ready;
    syncContactControllerState();
    emit lazyBootstrapStateChanged();
}

void AppController::setGroupsReady(bool ready)
{
    if (_bootstrap_state.groupsReady == ready)
    {
        return;
    }
    _bootstrap_state.groupsReady = ready;
    syncGroupControllerState();
    emit lazyBootstrapStateChanged();
}

void AppController::setApplyReady(bool ready)
{
    if (_bootstrap_state.applyReady == ready)
    {
        return;
    }
    _bootstrap_state.applyReady = ready;
    emit lazyBootstrapStateChanged();
}
