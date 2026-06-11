#include "AppController.h"
#include "AppChatConnectionCoordinator.h"
#include "AppCoordinators.h"
#include "IconPathUtils.h"
#include "IChatTransport.h"
#include "httpmgr.h"
#include "usermgr.h"

#include <QDebug>
#include <QVariantList>

void AppController::switchToLogin()
{
    qInfo() << "Switching to login page, current page:" << _shell_state.page()
            << "pending uid:" << _session_coordinator->pendingUid()
            << "chat connected:" << _gateway.chatTransport()->isConnected();
    setPage(LoginPage);
    _session_coordinator->resetForLogout();
}

void AppController::switchToRegister()
{
    _session_coordinator->stopRegisterCountdownTimer();
    _heartbeat_timer.stop();
    _chat_connection_coordinator->resetHeartbeatTracking();
    _features.authViewModel.syncRegisterSuccessPage(false);
    setPage(RegisterPage);
    setTip(QString(), false);
}

void AppController::switchToReset()
{
    _session_coordinator->stopRegisterCountdownTimer();
    _heartbeat_timer.stop();
    _chat_connection_coordinator->resetHeartbeatTracking();
    _features.authViewModel.syncRegisterSuccessPage(false);
    setPage(ResetPage);
    setTip(QString(), false);
}

void AppController::switchChatTab(int tab)
{
    const int normalized = qBound(0, tab, static_cast<int>(Live2DTabPage));
    const ChatTab target = static_cast<ChatTab>(normalized);
    if (!_shell_state.setChatTab(static_cast<int>(target)))
    {
        return;
    }
    _features.shellViewModel.syncChatTab(_shell_state.chatTab());
    if (target == ContactTabPage)
    {
        ensureContactsInitialized();
        ensureApplyInitialized();
    }
    else if (target == MomentsTabPage)
    {
        if (_features.momentsController.model()->count() == 0)
        {
            _features.momentsController.loadFeed();
        }
    }
    else if (target == AgentTabPage)
    {
        _features.agentController.loadSessions();
        _features.agentController.refreshModelList();
    }
    syncChatViewModelState();
}

void AppController::ensureContactsInitialized()
{
    _features.contactController.ensureContactsInitialized();
}

void AppController::ensureGroupsInitialized()
{
    if (_shell_state.bootstrapState().groupsReady)
    {
        return;
    }
    bootstrapGroups();
}

void AppController::ensureApplyInitialized()
{
    _features.contactController.ensureApplyInitialized();
}

void AppController::ensureChatListInitialized()
{
    _features.chatFeatureController.ensureChatListInitialized();
}
