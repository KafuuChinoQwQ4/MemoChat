#include "ShellViewModel.h"

ShellViewModel::ShellViewModel(QObject* parent)
    : QObject(parent)
{
}

int ShellViewModel::page() const
{
    return _page;
}

int ShellViewModel::chatTab() const
{
    return _chat_tab;
}

QString ShellViewModel::currentUserName() const
{
    return _current_user_name;
}

QString ShellViewModel::currentUserNick() const
{
    return _current_user_nick;
}

QString ShellViewModel::currentUserIcon() const
{
    return _current_user_icon;
}

QString ShellViewModel::currentUserDesc() const
{
    return _current_user_desc;
}

QString ShellViewModel::currentUserId() const
{
    return _current_user_id;
}

int ShellViewModel::currentUserUid() const
{
    return _current_user_uid;
}

void ShellViewModel::switchToLogin()
{
    emit switchToLoginRequested();
}

void ShellViewModel::switchToRegister()
{
    emit switchToRegisterRequested();
}

void ShellViewModel::switchToReset()
{
    emit switchToResetRequested();
}

void ShellViewModel::switchChatTab(int tab)
{
    emit switchChatTabRequested(tab);
}

void ShellViewModel::beginPostLoginBootstrap()
{
    emit beginPostLoginBootstrapRequested();
}

void ShellViewModel::openExternalResource(const QString& url)
{
    emit openExternalResourceRequested(url);
}

void ShellViewModel::syncPage(int page)
{
    if (_page == page)
    {
        return;
    }

    _page = page;
    emit pageChanged();
}

void ShellViewModel::syncChatTab(int chatTab)
{
    if (_chat_tab == chatTab)
    {
        return;
    }

    _chat_tab = chatTab;
    emit chatTabChanged();
}

void ShellViewModel::syncCurrentUser(const QString& name,
                                     const QString& nick,
                                     const QString& icon,
                                     const QString& desc,
                                     const QString& userId,
                                     int uid)
{
    if (_current_user_name == name && _current_user_nick == nick && _current_user_icon == icon &&
        _current_user_desc == desc && _current_user_id == userId && _current_user_uid == uid)
    {
        return;
    }

    _current_user_name = name;
    _current_user_nick = nick;
    _current_user_icon = icon;
    _current_user_desc = desc;
    _current_user_id = userId;
    _current_user_uid = uid;
    emit currentUserChanged();
}
