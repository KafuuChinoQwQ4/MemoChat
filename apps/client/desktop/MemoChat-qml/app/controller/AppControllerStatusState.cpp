#include "AppController.h"

void AppController::setSearchPending(bool pending)
{
    if (_search_state.pending == pending)
    {
        return;
    }

    _search_state.pending = pending;
    syncContactControllerState();
    emit searchPendingChanged();
}

void AppController::setSearchStatus(const QString& text, bool isError)
{
    if (_search_state.statusText == text && _search_state.statusError == isError)
    {
        return;
    }

    _search_state.statusText = text;
    _search_state.statusError = isError;
    syncContactControllerState();
    emit searchStatusChanged();
}

void AppController::clearSearchResultOnly()
{
    _search_result_model.clear();
    syncContactControllerState();
}

void AppController::setAuthStatus(const QString& text, bool isError)
{
    if (_feature_status_state.authText == text && _feature_status_state.authError == isError)
    {
        return;
    }

    _feature_status_state.authText = text;
    _feature_status_state.authError = isError;
    syncContactControllerState();
    emit authStatusChanged();
}

void AppController::setSettingsStatus(const QString& text, bool isError)
{
    if (_feature_status_state.settingsText == text && _feature_status_state.settingsError == isError)
    {
        return;
    }

    _feature_status_state.settingsText = text;
    _feature_status_state.settingsError = isError;
    _profile_controller.syncStatus(text, isError);
    emit settingsStatusChanged();
}

void AppController::setGroupStatus(const QString& text, bool isError)
{
    if (_feature_status_state.groupText == text && _feature_status_state.groupError == isError)
    {
        return;
    }

    _feature_status_state.groupText = text;
    _feature_status_state.groupError = isError;
    syncGroupControllerState();
    emit groupStatusChanged();
}

void AppController::setTip(const QString& tip, bool isError)
{
    if (_shell_state.tipText == tip && _shell_state.tipError == isError)
    {
        return;
    }
    _shell_state.tipText = tip;
    _shell_state.tipError = isError;
    _auth_view_model.syncTip(tip, isError);
    emit tipChanged();
}

void AppController::setBusy(bool value)
{
    if (_shell_state.busy == value)
    {
        return;
    }
    _shell_state.busy = value;
    _auth_view_model.syncBusy(value);
    emit busyChanged();
}

void AppController::setRegisterCodeCooldownSeconds(int seconds)
{
    const int normalized = qMax(0, seconds);
    if (_shell_state.registerCodeCooldownSeconds == normalized)
    {
        return;
    }
    _shell_state.registerCodeCooldownSeconds = normalized;
    _auth_view_model.syncRegisterCodeCooldownSeconds(normalized);
    emit registerCodeCooldownChanged();
}

void AppController::setRegisterCodeRequestPending(bool pending)
{
    if (_shell_state.registerCodeRequestPending == pending)
    {
        return;
    }
    _shell_state.registerCodeRequestPending = pending;
    _auth_view_model.syncRegisterCodeRequestPending(pending);
    emit registerCodeCooldownChanged();
}

void AppController::setPage(Page newPage)
{
    if (_page == newPage)
    {
        return;
    }
    _page = newPage;
    _shell_view_model.syncPage(static_cast<int>(_page));
    emit pageChanged();
}
