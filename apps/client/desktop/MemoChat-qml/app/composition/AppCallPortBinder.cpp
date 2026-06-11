#include "AppCoordinators.h"
#include "AppPortRegistry.h"
#include "usermgr.h"

#include <QTimer>
#include <utility>

void AppPortRegistry::bindCallPorts()
{
    _call_coordinator = std::make_unique<CallCoordinator>(
        CallShellPort{[this]()
                      {
                          CallShellSnapshot snapshot;
                          snapshot.hasCurrentContact = hasCurrentContact();
                          snapshot.currentGroupId = currentGroupId();
                          snapshot.currentChatUid = _chat_state.uid;
                          snapshot.currentContactUid = currentContactUid();
                          snapshot.currentContactName = currentContactName();
                          snapshot.currentContactIcon = currentContactIcon();
                          if (_gateway.userMgr())
                          {
                              snapshot.selfUid = _gateway.userMgr()->GetUid();
                              snapshot.authToken = _gateway.userMgr()->GetToken();
                          }
                          return snapshot;
                      },
                      [this](int uid)
                      {
                          return _gateway.userMgr() ? _gateway.userMgr()->GetFriendById(uid) : nullptr;
                      },
                      [this](const std::shared_ptr<FriendInfo>& friendInfo)
                      {
                          if (!friendInfo)
                          {
                              return;
                          }
                          setCurrentContact(friendInfo->_uid,
                                            friendInfo->_name,
                                            friendInfo->_nick,
                                            friendInfo->_icon,
                                            friendInfo->_back,
                                            friendInfo->_sex,
                                            friendInfo->_user_id);
                      },
                      [this](const QString& res, QJsonObject& obj)
                      {
                          return _features.authController.parseJson(res, obj);
                      },
                      [this](QString icon)
                      {
                          return normalizeIconPath(std::move(icon));
                      },
                      [this](const QString& text, bool isError)
                      {
                          setAuthStatus(text, isError);
                      },
                      [this](int delayMs, std::function<void()> callback)
                      {
                          QTimer::singleShot(delayMs, &_async_receiver, std::move(callback));
                      },
                      &_features.callController});

    _features.callController.setCommandPort(CallCommandPort{[this]()
                                                            {
                                                                _call_coordinator->startVoiceChat();
                                                            },
                                                            [this]()
                                                            {
                                                                _call_coordinator->startVideoChat();
                                                            },
                                                            [this]()
                                                            {
                                                                _call_coordinator->acceptIncomingCall();
                                                            },
                                                            [this]()
                                                            {
                                                                _call_coordinator->rejectIncomingCall();
                                                            },
                                                            [this]()
                                                            {
                                                                _call_coordinator->endCurrentCall();
                                                            },
                                                            [this]()
                                                            {
                                                                _call_coordinator->toggleCallMuted();
                                                            },
                                                            [this]()
                                                            {
                                                                _call_coordinator->toggleCallCamera();
                                                            }});
}
