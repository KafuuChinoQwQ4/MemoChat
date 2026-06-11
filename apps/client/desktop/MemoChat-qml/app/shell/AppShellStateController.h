#ifndef APPSHELLSTATECONTROLLER_H
#define APPSHELLSTATECONTROLLER_H

#include "AppControllerRuntimeState.h"
#include "AppControllerUserState.h"

struct ProfileAppliedUserState;

class AppShellStateController
{
public:
    AppShellStateController(int initialPage, int initialChatTab);

    int page() const;
    bool setPage(int page);

    int chatTab() const;
    bool setChatTab(int chatTab);

    const AppCurrentUserState& currentUser() const;
    bool syncCurrentUser(const ProfileAppliedUserState& user);
    bool resetCurrentUser(int previousUserUid);

    AppLoadingRuntimeState& loadingState();
    const AppLoadingRuntimeState& loadingState() const;
    AppLazyBootstrapState& bootstrapState();
    const AppLazyBootstrapState& bootstrapState() const;

private:
    int _page;
    int _chat_tab;
    AppCurrentUserState _current_user;
    AppLoadingRuntimeState _loading_state;
    AppLazyBootstrapState _bootstrap_state;
};

#endif // APPSHELLSTATECONTROLLER_H
