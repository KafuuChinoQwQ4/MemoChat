#include "AppController.h"

void AppController::setContactPane(ContactPane pane)
{
    if (_contact_pane == pane)
    {
        return;
    }

    _contact_pane = pane;
    syncContactControllerState();
    emit contactPaneChanged();
}

void AppController::setCurrentContact(int uid,
                                      const QString& name,
                                      const QString& nick,
                                      const QString& icon,
                                      const QString& back,
                                      int sex,
                                      const QString& userId)
{
    const QString normalizedIcon = normalizeIconPath(icon);
    if (_contact_state.uid == uid && _contact_state.name == name && _contact_state.nick == nick &&
        _contact_state.icon == normalizedIcon && _contact_state.back == back && _contact_state.sex == sex &&
        _contact_state.userId == userId)
    {
        return;
    }

    _contact_state.uid = uid;
    _contact_state.name = name;
    _contact_state.nick = nick;
    _contact_state.icon = normalizedIcon;
    _contact_state.back = back;
    _contact_state.sex = sex;
    _contact_state.userId = userId;
    syncContactControllerState();
    emit currentContactChanged();
}

void AppController::setCurrentChatPeerName(const QString& name)
{
    if (_chat_state.peerName == name)
    {
        return;
    }

    _chat_state.peerName = name;
    emit currentChatPeerChanged();
}

void AppController::setCurrentChatPeerIcon(const QString& icon)
{
    const QString normalizedIcon = normalizeIconPath(icon);
    if (_chat_state.peerIcon == normalizedIcon)
    {
        return;
    }

    _chat_state.peerIcon = normalizedIcon;
    emit currentChatPeerChanged();
}
