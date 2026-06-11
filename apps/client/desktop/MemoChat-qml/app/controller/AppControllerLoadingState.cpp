#include "AppController.h"
#include "usermgr.h"

void AppController::setChatLoadingMore(bool loading)
{
    if (_shell_state.loadingState().chatLoadingMore == loading)
    {
        return;
    }

    _shell_state.loadingState().chatLoadingMore = loading;
    emit chatLoadingMoreChanged();
}

void AppController::setPrivateHistoryLoading(bool loading)
{
    if (_shell_state.loadingState().privateHistoryLoading == loading)
    {
        return;
    }
    _shell_state.loadingState().privateHistoryLoading = loading;
    emit privateHistoryLoadingChanged();
}

void AppController::setCanLoadMorePrivateHistory(bool canLoad)
{
    if (_shell_state.loadingState().canLoadMorePrivateHistory == canLoad)
    {
        return;
    }
    _shell_state.loadingState().canLoadMorePrivateHistory = canLoad;
    emit canLoadMorePrivateHistoryChanged();
}

void AppController::setContactLoadingMore(bool loading)
{
    _features.contactController.setContactLoadingMore(loading);
}

void AppController::refreshChatLoadMoreState()
{
    const bool canLoad = !_gateway.userMgr()->IsLoadChatFin();
    if (_shell_state.loadingState().canLoadMoreChats == canLoad)
    {
        return;
    }

    _shell_state.loadingState().canLoadMoreChats = canLoad;
    emit canLoadMoreChatsChanged();
}

void AppController::refreshContactLoadMoreState()
{
    const bool canLoad = !_gateway.userMgr()->IsLoadConFin();
    _features.contactController.setCanLoadMoreContacts(canLoad);
}
