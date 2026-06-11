#include "AppCoordinators.h"

#include "CallController.h"
#include "usermgr.h"

bool CallCoordinator::ensureCallTargetFromCurrentChat()
{
    const CallShellSnapshot current = snapshot();
    if (current.hasCurrentContact)
    {
        return true;
    }
    if (current.currentGroupId > 0 || current.currentChatUid <= 0 || !_port.friendById ||
        !_port.setCurrentContactFromFriend)
    {
        return false;
    }
    auto friendInfo = _port.friendById(current.currentChatUid);
    if (!friendInfo)
    {
        return false;
    }
    _port.setCurrentContactFromFriend(friendInfo);
    return snapshot().hasCurrentContact;
}

void CallCoordinator::startCallFlow(const QString& callType)
{
    const CallShellSnapshot current = snapshot();
    auto* call = callController();
    if (!call)
    {
        return;
    }
    if (current.selfUid <= 0)
    {
        _port.setAuthStatus("用户状态异常，请重新登录", true);
        return;
    }
    if (current.currentContactUid <= 0)
    {
        _port.setAuthStatus("请选择联系人", true);
        return;
    }
    call->startCall(current.selfUid, current.authToken, current.currentContactUid, callType);
    _port.setAuthStatus("正在发起通话", false);
}

void CallCoordinator::startVoiceChat()
{
    if (ensureCallTargetFromCurrentChat())
    {
        startCallFlow("voice");
    }
}

void CallCoordinator::startVideoChat()
{
    if (ensureCallTargetFromCurrentChat())
    {
        startCallFlow("video");
    }
}

void CallCoordinator::acceptIncomingCall()
{
    const CallShellSnapshot current = snapshot();
    auto* call = callController();
    if (!call || current.selfUid <= 0 || call->callId().isEmpty())
    {
        return;
    }
    call->acceptCall(current.selfUid, current.authToken, call->callId());
    _port.setAuthStatus("正在接听通话", false);
}

void CallCoordinator::rejectIncomingCall()
{
    const CallShellSnapshot current = snapshot();
    auto* call = callController();
    if (!call || current.selfUid <= 0 || call->callId().isEmpty())
    {
        return;
    }
    call->rejectCall(current.selfUid, current.authToken, call->callId());
}

void CallCoordinator::endCurrentCall()
{
    const CallShellSnapshot current = snapshot();
    auto* call = callController();
    if (!call || current.selfUid <= 0 || call->callId().isEmpty())
    {
        return;
    }
    call->leaveRoom();
    if (!call->callActive() && !call->callIncoming())
    {
        call->cancelCall(current.selfUid, current.authToken, call->callId());
        return;
    }
    call->hangupCall(current.selfUid, current.authToken, call->callId());
}

void CallCoordinator::finalizeEndedCall(const QString& statusText)
{
    auto* call = callController();
    if (!call)
    {
        return;
    }
    call->leaveRoom();
    call->markEnded(statusText);
    _port.setAuthStatus(statusText, false);
    if (_port.runDelayed)
    {
        _port.runDelayed(1800,
                         [this]()
                         {
                             if (auto* currentCall = callController())
                             {
                                 currentCall->resetCallSurface();
                             }
                         });
    }
}

void CallCoordinator::toggleCallMuted()
{
    auto* call = callController();
    if (!call)
    {
        return;
    }
    call->toggleMic();
    call->setMuted(!call->muted());
}

void CallCoordinator::toggleCallCamera()
{
    auto* call = callController();
    if (!call || call->callType() != QStringLiteral("video"))
    {
        return;
    }
    call->toggleCamera();
    call->setCameraEnabled(!call->cameraEnabled());
}
