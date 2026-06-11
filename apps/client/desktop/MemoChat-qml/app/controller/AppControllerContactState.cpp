#include "AppController.h"

void AppController::setContactPane(ContactPane pane)
{
    _features.contactController.setContactPane(static_cast<int>(pane));
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
    _features.contactController.setCurrentContact(uid, name, nick, normalizedIcon, back, sex, userId);
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
