#include "AppController.h"

#include "usermgr.h"

bool AppController::ensureCallTargetFromCurrentChat()
{
    if (hasCurrentContact())
    {
        return true;
    }
    if (_group_state.currentId > 0)
    {
        return false;
    }
    if (_chat_state.uid <= 0)
    {
        return false;
    }

    auto friendInfo = _gateway.userMgr()->GetFriendById(_chat_state.uid);
    if (!friendInfo)
    {
        return false;
    }

    setCurrentContact(friendInfo->_uid,
                      friendInfo->_name,
                      friendInfo->_nick,
                      friendInfo->_icon,
                      friendInfo->_back,
                      friendInfo->_sex,
                      friendInfo->_user_id);
    return hasCurrentContact();
}

void AppController::sendCallInvite(const QString& callType)
{
    startCallFlow(callType);
}
