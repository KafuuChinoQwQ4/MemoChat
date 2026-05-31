#include "AppController.h"
#include "usermgr.h"

void AppController::setChatLoadingMore(bool loading)
{
    if (_loading_state.chatLoadingMore == loading)
    {
        return;
    }

    _loading_state.chatLoadingMore = loading;
    emit chatLoadingMoreChanged();
}

void AppController::setPrivateHistoryLoading(bool loading)
{
    if (_loading_state.privateHistoryLoading == loading)
    {
        return;
    }
    _loading_state.privateHistoryLoading = loading;
    emit privateHistoryLoadingChanged();
}

void AppController::setCanLoadMorePrivateHistory(bool canLoad)
{
    if (_loading_state.canLoadMorePrivateHistory == canLoad)
    {
        return;
    }
    _loading_state.canLoadMorePrivateHistory = canLoad;
    emit canLoadMorePrivateHistoryChanged();
}

void AppController::setContactLoadingMore(bool loading)
{
    if (_loading_state.contactLoadingMore == loading)
    {
        return;
    }

    _loading_state.contactLoadingMore = loading;
    emit contactLoadingMoreChanged();
}

void AppController::refreshChatLoadMoreState()
{
    const bool canLoad = !_gateway.userMgr()->IsLoadChatFin();
    if (_loading_state.canLoadMoreChats == canLoad)
    {
        return;
    }

    _loading_state.canLoadMoreChats = canLoad;
    emit canLoadMoreChatsChanged();
}

void AppController::refreshContactLoadMoreState()
{
    const bool canLoad = !_gateway.userMgr()->IsLoadConFin();
    if (_loading_state.canLoadMoreContacts == canLoad)
    {
        return;
    }

    _loading_state.canLoadMoreContacts = canLoad;
    emit canLoadMoreContactsChanged();
}
