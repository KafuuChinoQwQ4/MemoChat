#include "AppCoordinators.h"

#include "AppController.h"
#include "usermgr.h"

CallCoordinator::CallCoordinator(AppController& controller)
    : _app(controller)
{
}

bool CallCoordinator::ensureCallTargetFromCurrentChat()
{
    if (_app.hasCurrentContact())
    {
        return true;
    }
    if (_app._group_state.currentId > 0 || _app._chat_state.uid <= 0)
    {
        return false;
    }
    auto friendInfo = _app._gateway.userMgr()->GetFriendById(_app._chat_state.uid);
    if (!friendInfo)
    {
        return false;
    }
    _app.setCurrentContact(friendInfo->_uid,
                           friendInfo->_name,
                           friendInfo->_nick,
                           friendInfo->_icon,
                           friendInfo->_back,
                           friendInfo->_sex,
                           friendInfo->_user_id);
    return _app.hasCurrentContact();
}

void CallCoordinator::startCallFlow(const QString& callType)
{
    _app.startCallFlow(callType);
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
    const auto selfInfo = _app._gateway.userMgr()->GetUserInfo();
    if (!selfInfo || _app._call_session_model.callId().isEmpty())
    {
        return;
    }
    _app._call_controller.acceptCall(selfInfo->_uid,
                                     _app._gateway.userMgr()->GetToken(),
                                     _app._call_session_model.callId());
    _app.setAuthStatus("正在接听通话", false);
}

void CallCoordinator::rejectIncomingCall()
{
    const auto selfInfo = _app._gateway.userMgr()->GetUserInfo();
    if (!selfInfo || _app._call_session_model.callId().isEmpty())
    {
        return;
    }
    _app._call_controller.rejectCall(selfInfo->_uid,
                                     _app._gateway.userMgr()->GetToken(),
                                     _app._call_session_model.callId());
}

void CallCoordinator::endCurrentCall()
{
    const auto selfInfo = _app._gateway.userMgr()->GetUserInfo();
    if (!selfInfo || _app._call_session_model.callId().isEmpty())
    {
        return;
    }
    _app._livekit_bridge.leaveRoom();
    if (!_app._call_session_model.active() && !_app._call_session_model.incoming())
    {
        _app._call_controller.cancelCall(selfInfo->_uid,
                                         _app._gateway.userMgr()->GetToken(),
                                         _app._call_session_model.callId());
        return;
    }
    _app._call_controller.hangupCall(selfInfo->_uid,
                                     _app._gateway.userMgr()->GetToken(),
                                     _app._call_session_model.callId());
}

void CallCoordinator::toggleCallMuted()
{
    _app._livekit_bridge.toggleMic();
    _app._call_session_model.setMuted(!_app._call_session_model.muted());
}

void CallCoordinator::toggleCallCamera()
{
    if (_app._call_session_model.callType() != QStringLiteral("video"))
    {
        return;
    }
    _app._livekit_bridge.toggleCamera();
    _app._call_session_model.setCameraEnabled(!_app._call_session_model.cameraEnabled());
}
