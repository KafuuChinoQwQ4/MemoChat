#include "AppController.h"
#include "usermgr.h"

QString AppController::currentUserName() const
{
    return _shell_state.currentUser().name;
}

QString AppController::currentUserNick() const
{
    return _shell_state.currentUser().nick;
}

QString AppController::currentUserIcon() const
{
    return _shell_state.currentUser().icon;
}

QString AppController::currentUserDesc() const
{
    return _shell_state.currentUser().desc;
}

QString AppController::currentUserId() const
{
    return _shell_state.currentUser().userId;
}

int AppController::currentUserUid() const
{
    const auto userMgr = _gateway.userMgr();
    return userMgr ? userMgr->GetUid() : 0;
}

QString AppController::currentContactName() const
{
    return _features.contactController.currentContactName();
}

QString AppController::currentContactNick() const
{
    return _features.contactController.currentContactNick();
}

QString AppController::currentContactIcon() const
{
    return _features.contactController.currentContactIcon();
}

QString AppController::currentContactBack() const
{
    return _features.contactController.currentContactBack();
}

int AppController::currentContactSex() const
{
    return _features.contactController.currentContactSex();
}

QString AppController::currentContactUserId() const
{
    return _features.contactController.currentContactUserId();
}

int AppController::currentContactUid() const
{
    return _features.contactController.currentContactUid();
}

bool AppController::hasCurrentContact() const
{
    return _features.contactController.hasCurrentContact();
}

QString AppController::currentChatPeerName() const
{
    return _chat_state.peerName;
}

QString AppController::currentChatPeerIcon() const
{
    return _chat_state.peerIcon;
}

bool AppController::hasCurrentChat() const
{
    return _chat_state.uid > 0 || currentGroupId() > 0;
}
