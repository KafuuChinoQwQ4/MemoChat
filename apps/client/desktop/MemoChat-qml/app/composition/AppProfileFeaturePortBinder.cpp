#include "AppCoordinators.h"
#include "AppPortRegistry.h"
#include "MediaUploadService.h"
#include "usermgr.h"

void AppPortRegistry::bindProfileFeaturePorts()
{
    _features.profileController.setCommandPort(ProfileCommandPort{
        [this]()
        {
            ProfileCommandSnapshot snapshot;
            if (const auto selfInfo = _gateway.userMgr() ? _gateway.userMgr()->GetUserInfo() : nullptr)
            {
                snapshot.selfUid = selfInfo->_uid;
                snapshot.selfName = selfInfo->_name;
            }
            const AppCurrentUserState& currentUser = _shell_state.currentUser();
            snapshot.currentIcon = currentUser.icon;
            snapshot.uploadToken = _session_coordinator->authToken();
            snapshot.avatarUploadInProgress = _media_upload_state.settingsAvatarUploadInProgress;
            return snapshot;
        },
        [this](bool inProgress)
        {
            _media_upload_state.settingsAvatarUploadInProgress = inProgress;
        },
        [](const QString& path, int uid, const QString& token, UploadedMediaInfo* uploaded, QString* errorText)
        {
            return MediaUploadService::uploadLocalFile(path, QStringLiteral("avatar"), uid, token, uploaded, errorText);
        },
        [this](const QString& icon)
        {
            const auto userMgr = _gateway.userMgr();
            const auto selfInfo = userMgr ? userMgr->GetUserInfo() : nullptr;
            _features.profileController.applyCurrentUserProfile(selfInfo ? selfInfo->_uid : currentUserUid(),
                                                                selfInfo ? selfInfo->_name : currentUserName(),
                                                                selfInfo ? selfInfo->_nick : currentUserNick(),
                                                                icon,
                                                                selfInfo ? selfInfo->_desc : currentUserDesc(),
                                                                selfInfo ? selfInfo->_user_id : currentUserId(),
                                                                selfInfo ? selfInfo->_sex : 0,
                                                                false);
        }});
    _features.profileController.setStatePort(
        ProfileStatePort{[this]()
                         {
                             ProfileCurrentUserState snapshot;
                             snapshot.pendingUid = _session_coordinator->pendingUid();
                             snapshot.uid = currentUserUid();
                             const AppCurrentUserState& currentUser = _shell_state.currentUser();
                             snapshot.name = currentUser.name;
                             snapshot.nick = currentUser.nick;
                             snapshot.icon = currentUser.icon;
                             snapshot.desc = currentUser.desc;
                             snapshot.userId = currentUser.userId;
                             if (const auto selfInfo = _gateway.userMgr() ? _gateway.userMgr()->GetUserInfo() : nullptr)
                             {
                                 snapshot.sex = selfInfo->_sex;
                             }
                             return snapshot;
                         },
                         [this](const ProfileAppliedUserState& user)
                         {
                             _actions.syncCurrentUserProfileState(user);
                         }});
}
