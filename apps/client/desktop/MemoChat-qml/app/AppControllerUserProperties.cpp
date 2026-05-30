#include "AppController.h"
#include "usermgr.h"

QString AppController::currentUserName() const
{
    return _user_state.name;
}

QString AppController::currentUserNick() const
{
    return _user_state.nick;
}

QString AppController::currentUserIcon() const
{
    return _user_state.icon;
}

QString AppController::currentUserDesc() const
{
    return _user_state.desc;
}

QString AppController::currentUserId() const
{
    return _user_state.userId;
}

int AppController::currentUserUid() const
{
    const auto userMgr = _gateway.userMgr();
    return userMgr ? userMgr->GetUid() : 0;
}

QString AppController::currentContactName() const
{
    return _contact_state.name;
}

QString AppController::currentContactNick() const
{
    return _contact_state.nick;
}

QString AppController::currentContactIcon() const
{
    return _contact_state.icon;
}

QString AppController::currentContactBack() const
{
    return _contact_state.back;
}

int AppController::currentContactSex() const
{
    return _contact_state.sex;
}

QString AppController::currentContactUserId() const
{
    return _contact_state.userId;
}

int AppController::currentContactUid() const
{
    return _contact_state.uid;
}

bool AppController::hasCurrentContact() const
{
    return _contact_state.uid > 0;
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
    return _chat_state.uid > 0 || _group_state.currentId > 0;
}
