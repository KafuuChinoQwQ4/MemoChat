#include "AppController.h"
#include "usermgr.h"

void AppController::onAddAuthFriend(std::shared_ptr<AuthInfo> authInfo)
{
    if (authInfo) {
        _gateway.userMgr()->AddFriend(authInfo);
    }
    _chat_list_model.upsertFriend(authInfo);
    _contact_list_model.upsertFriend(authInfo);
    refreshDialogModel();
    refreshChatLoadMoreState();
    refreshContactLoadMoreState();
    if (authInfo) {
        _gateway.userMgr()->MarkApplyStatus(authInfo->_uid, 1);
        _apply_request_model.markApproved(authInfo->_uid);
        if (_current_contact_uid == authInfo->_uid) {
            setCurrentContact(authInfo->_uid, authInfo->_name, authInfo->_nick, authInfo->_icon,
                              authInfo->_nick, authInfo->_sex, authInfo->_user_id);
        }
    }
}

void AppController::onAuthRsp(std::shared_ptr<AuthRsp> authRsp)
{
    if (authRsp) {
        _gateway.userMgr()->AddFriend(authRsp);
    }
    _chat_list_model.upsertFriend(authRsp);
    _contact_list_model.upsertFriend(authRsp);
    refreshDialogModel();
    refreshChatLoadMoreState();
    refreshContactLoadMoreState();
    if (authRsp) {
        _gateway.userMgr()->MarkApplyStatus(authRsp->_uid, 1);
        _apply_request_model.markApproved(authRsp->_uid);
        setAuthStatus("好友添加成功", false);
        if (_current_contact_uid == authRsp->_uid) {
            setCurrentContact(authRsp->_uid, authRsp->_name, authRsp->_nick, authRsp->_icon,
                              authRsp->_nick, authRsp->_sex, authRsp->_user_id);
        }
    }
}

void AppController::onDeleteFriendRsp(int error, int friendUid)
{
    if (error != ErrorCodes::SUCCESS || friendUid <= 0) {
        setAuthStatus(QString("删除联系人失败（错误码:%1）").arg(error), true);
        return;
    }

    _gateway.userMgr()->RemoveFriend(friendUid);
    _chat_list_model.removeByUid(friendUid);
    _contact_list_model.removeByUid(friendUid);
    _dialog_list_model.removeByUid(friendUid);
    _dialog_draft_map.remove(friendUid);
    _dialog_mention_map.remove(friendUid);
    _dialog_local_pinned_set.remove(friendUid);
    _dialog_server_pinned_map.remove(friendUid);
    _dialog_server_mute_map.remove(friendUid);

    if (_current_chat_uid == friendUid) {
        _current_chat_uid = 0;
        _message_model.clear();
        setCurrentChatPeerName(QString());
        setCurrentChatPeerIcon(QStringLiteral("qrc:/res/head_1.png"));
        setCurrentDraftText(QString());
        setCurrentDialogPinned(false);
        setCurrentDialogMuted(false);
        emitCurrentDialogUidChangedIfNeeded();
    }

    if (_current_contact_uid == friendUid) {
        setCurrentContact(0, QString(), QString(), QStringLiteral("qrc:/res/head_1.png"), QString(), 0);
        setContactPane(ApplyRequestPane);
    }

    refreshChatLoadMoreState();
    refreshContactLoadMoreState();
    setAuthStatus("联系人已删除", false);
}

void AppController::onUserSearch(std::shared_ptr<SearchInfo> searchInfo)
{
    setSearchPending(false);

    if (!searchInfo) {
        clearSearchResultOnly();
        setSearchStatus("未找到该用户", true);
        return;
    }

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        clearSearchResultOnly();
        setSearchStatus("用户状态异常，请重新登录", true);
        return;
    }

    if (searchInfo->_uid == selfInfo->_uid) {
        clearSearchResultOnly();
        setSearchStatus("不能搜索自己", true);
        return;
    }

    if (_gateway.userMgr()->CheckFriendById(searchInfo->_uid)) {
        clearSearchResultOnly();
        setSearchStatus("已是好友，已切换到会话", false);
        selectChatByUid(searchInfo->_uid);
        switchChatTab(ChatTabPage);
        return;
    }

    _search_result_model.setResult(searchInfo);
    setSearchStatus("已找到用户", false);
}

void AppController::onFriendApply(std::shared_ptr<AddFriendApply> applyInfo)
{
    if (!applyInfo) {
        return;
    }

    if (_gateway.userMgr()->AlreadyApply(applyInfo->_from_uid)) {
        return;
    }

    auto apply = std::make_shared<ApplyInfo>(applyInfo);
    _gateway.userMgr()->AddApplyList(apply);
    _apply_request_model.upsertApply(apply);
    setAuthStatus(QString("收到来自 %1 的好友申请").arg(applyInfo->_name), false);
}
