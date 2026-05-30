#include "AppCoordinators.h"

#include "AppController.h"
#include "usermgr.h"

ContactCoordinatorShell::ContactCoordinatorShell(AppController& controller)
    : _app(controller)
{
}

void ContactCoordinatorShell::searchUser(const QString& uidText)
{
    QString errorText;
    if (!_app._contact_controller.sendSearchUser(uidText, &errorText))
    {
        _app.clearSearchResultOnly();
        _app.setSearchStatus(errorText, true);
        return;
    }
    _app.clearSearchResultOnly();
    _app.setSearchPending(true);
    _app.setSearchStatus("搜索中...", false);
}

void ContactCoordinatorShell::requestAddFriend(int uid, const QString& bakName, const QVariantList& labels)
{
    auto selfInfo = _app._gateway.userMgr()->GetUserInfo();
    if (!selfInfo)
    {
        return;
    }
    if (uid == selfInfo->_uid)
    {
        _app.setSearchStatus("不能添加自己", true);
        return;
    }
    if (_app._gateway.userMgr()->CheckFriendById(uid))
    {
        _app.setSearchStatus("已是好友，无需重复申请", false);
        return;
    }

    _app._contact_controller.sendAddFriend(selfInfo->_uid, selfInfo->_name, uid, bakName, labels);
    _app.setSearchStatus("好友申请已发送", false);
}

void ContactCoordinatorShell::approveFriend(int uid, const QString& backName, const QVariantList& labels)
{
    if (uid <= 0)
    {
        _app.setAuthStatus("非法好友申请", true);
        return;
    }
    auto selfInfo = _app._gateway.userMgr()->GetUserInfo();
    if (!selfInfo)
    {
        _app.setAuthStatus("用户状态异常，请重新登录", true);
        return;
    }
    if (_app._gateway.userMgr()->CheckFriendById(uid))
    {
        _app._apply_request_model.markApproved(uid);
        _app.setAuthStatus("已是好友", false);
        return;
    }

    QString remark = backName.trimmed();
    if (remark.isEmpty())
    {
        remark = _app._apply_request_model.nameByUid(uid);
    }
    if (remark.isEmpty())
    {
        remark = "MemoChat好友";
    }

    _app._contact_controller.sendApproveFriend(selfInfo->_uid, uid, remark, labels);
    _app._apply_request_model.setPending(uid, true);
    _app.setAuthStatus("好友认证请求已发送", false);
}
