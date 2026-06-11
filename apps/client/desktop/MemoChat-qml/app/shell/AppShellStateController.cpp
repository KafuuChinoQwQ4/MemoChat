#include "AppShellStateController.h"

#include "ProfileController.h"

AppShellStateController::AppShellStateController(int initialPage, int initialChatTab)
    : _page(initialPage)
    , _chat_tab(initialChatTab)
{
}

int AppShellStateController::page() const
{
    return _page;
}

bool AppShellStateController::setPage(int page)
{
    if (_page == page)
    {
        return false;
    }

    _page = page;
    return true;
}

int AppShellStateController::chatTab() const
{
    return _chat_tab;
}

bool AppShellStateController::setChatTab(int chatTab)
{
    if (_chat_tab == chatTab)
    {
        return false;
    }

    _chat_tab = chatTab;
    return true;
}

const AppCurrentUserState& AppShellStateController::currentUser() const
{
    return _current_user;
}

bool AppShellStateController::syncCurrentUser(const ProfileAppliedUserState& user)
{
    bool changed = false;
    if (_current_user.name != user.name)
    {
        _current_user.name = user.name;
        changed = true;
    }
    if (_current_user.nick != user.nick)
    {
        _current_user.nick = user.nick;
        changed = true;
    }
    if (_current_user.icon != user.icon)
    {
        _current_user.icon = user.icon;
        changed = true;
    }
    if (_current_user.desc != user.desc)
    {
        _current_user.desc = user.desc;
        changed = true;
    }
    if (_current_user.userId != user.userId)
    {
        _current_user.userId = user.userId;
        changed = true;
    }

    return changed;
}

bool AppShellStateController::resetCurrentUser(int previousUserUid)
{
    const bool changed = previousUserUid != 0 || !_current_user.name.isEmpty() || !_current_user.nick.isEmpty() ||
                         _current_user.icon != QStringLiteral("qrc:/res/head_1.png") ||
                                                              !_current_user.userId.isEmpty() ||
                                                              !_current_user.desc.isEmpty();
    _current_user.name.clear();
    _current_user.nick.clear();
    _current_user.icon = QStringLiteral("qrc:/res/head_1.png");
    _current_user.userId.clear();
    _current_user.desc.clear();
    return changed;
}

AppLoadingRuntimeState& AppShellStateController::loadingState()
{
    return _loading_state;
}

const AppLoadingRuntimeState& AppShellStateController::loadingState() const
{
    return _loading_state;
}

AppLazyBootstrapState& AppShellStateController::bootstrapState()
{
    return _bootstrap_state;
}

const AppLazyBootstrapState& AppShellStateController::bootstrapState() const
{
    return _bootstrap_state;
}
