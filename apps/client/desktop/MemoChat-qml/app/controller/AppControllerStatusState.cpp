#include "AppController.h"

void AppController::setSearchPending(bool pending)
{
    _features.contactController.setSearchPending(pending);
}

void AppController::setSearchStatus(const QString& text, bool isError)
{
    _features.contactController.setSearchStatus(text, isError);
}

void AppController::clearSearchResultOnly()
{
    _features.contactController.clearSearchResultOnly();
}

void AppController::setAuthStatus(const QString& text, bool isError)
{
    _features.contactController.setAuthStatus(text, isError);
}

void AppController::setSettingsStatus(const QString& text, bool isError)
{
    if (_features.profileController.statusText() == text && _features.profileController.statusError() == isError)
    {
        return;
    }

    _features.profileController.syncStatus(text, isError);
    emit settingsStatusChanged();
}

void AppController::setGroupStatus(const QString& text, bool isError)
{
    if (_features.groupController.groupStatusText() == text && _features.groupController.groupStatusError() == isError)
    {
        return;
    }

    _features.groupController.syncGroupStatus(text, isError);
    emit groupStatusChanged();
}

void AppController::setTip(const QString& tip, bool isError)
{
    if (_features.authViewModel.tipText() == tip && _features.authViewModel.tipError() == isError)
    {
        return;
    }
    _features.authViewModel.syncTip(tip, isError);
    emit tipChanged();
}

void AppController::setBusy(bool value)
{
    if (_features.authViewModel.busy() == value)
    {
        return;
    }
    _features.authViewModel.syncBusy(value);
    emit busyChanged();
}

void AppController::setPage(Page newPage)
{
    if (!_shell_state.setPage(static_cast<int>(newPage)))
    {
        return;
    }
    _features.shellViewModel.syncPage(_shell_state.page());
    emit pageChanged();
}
