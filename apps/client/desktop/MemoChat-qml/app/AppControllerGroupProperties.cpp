#include "AppController.h"
#include "usermgr.h"

namespace
{
constexpr qint64 kPermChangeGroupInfo = 1LL << 0;
constexpr qint64 kPermDeleteMessages = 1LL << 1;
constexpr qint64 kPermInviteUsers = 1LL << 2;
constexpr qint64 kPermManageAdmins = 1LL << 3;
constexpr qint64 kPermPinMessages = 1LL << 4;
constexpr qint64 kPermBanUsers = 1LL << 5;
constexpr qint64 kPermManageTopics = 1LL << 6;
} // namespace

bool AppController::hasCurrentGroup() const
{
    return _group_state.currentId > 0;
}

int AppController::currentGroupRole() const
{
    if (_group_state.currentId <= 0)
    {
        return 0;
    }
    auto groupInfo = _gateway.userMgr()->GetGroupById(_group_state.currentId);
    if (!groupInfo)
    {
        return 0;
    }
    return groupInfo->_role;
}

QString AppController::currentGroupName() const
{
    return _group_state.currentName;
}

QString AppController::currentGroupCode() const
{
    return _group_state.currentCode;
}

bool AppController::currentGroupCanChangeInfo() const
{
    return hasCurrentGroupPermission(kPermChangeGroupInfo);
}

bool AppController::currentGroupCanDeleteMessages() const
{
    return hasCurrentGroupPermission(kPermDeleteMessages);
}

bool AppController::currentGroupCanInviteUsers() const
{
    return hasCurrentGroupPermission(kPermInviteUsers);
}

bool AppController::currentGroupCanManageAdmins() const
{
    return hasCurrentGroupPermission(kPermManageAdmins);
}

bool AppController::currentGroupCanPinMessages() const
{
    return hasCurrentGroupPermission(kPermPinMessages);
}

bool AppController::currentGroupCanBanUsers() const
{
    return hasCurrentGroupPermission(kPermBanUsers);
}

bool AppController::currentGroupCanManageTopics() const
{
    return hasCurrentGroupPermission(kPermManageTopics);
}
