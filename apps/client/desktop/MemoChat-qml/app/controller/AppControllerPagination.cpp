#include "AppController.h"
#include "usermgr.h"

void AppController::loadMoreChats()
{
    ensureChatListInitialized();
    if (_loading_state.chatLoadingMore)
    {
        return;
    }

    if (_gateway.userMgr()->IsLoadChatFin())
    {
        refreshChatLoadMoreState();
        return;
    }

    setChatLoadingMore(true);
    const auto chatList = _gateway.userMgr()->GetChatListPerPage();
    _chat_list_model.appendFriends(chatList);
    _gateway.userMgr()->UpdateChatLoadedCount();
    setChatLoadingMore(false);
    refreshChatLoadMoreState();
}

void AppController::loadMorePrivateHistory()
{
    if (_loading_state.privateHistoryLoading || !_loading_state.canLoadMorePrivateHistory)
    {
        return;
    }
    if (_group_state.currentId > 0)
    {
        loadGroupHistory();
        return;
    }
    if (_chat_state.uid > 0)
    {
        qint64 beforeTs = _message_model.earliestCreatedAt();
        QString beforeMsgId = _message_model.earliestMsgId();
        if (beforeTs <= 0)
        {
            beforeTs = _private_history_state.beforeTs;
            beforeMsgId = _private_history_state.beforeMsgId;
        }
        requestPrivateHistory(_chat_state.uid, beforeTs, beforeMsgId);
    }
}

void AppController::loadMoreContacts()
{
    ensureContactsInitialized();
    if (_loading_state.contactLoadingMore)
    {
        return;
    }

    if (_gateway.userMgr()->IsLoadConFin())
    {
        refreshContactLoadMoreState();
        return;
    }

    setContactLoadingMore(true);
    const auto contactList = _gateway.userMgr()->GetConListPerPage();
    _contact_list_model.appendFriends(contactList);
    _gateway.userMgr()->UpdateContactLoadedCount();
    setContactLoadingMore(false);
    refreshContactLoadMoreState();
}
