#include "AppCoordinators.h"
#include "AppPortRegistry.h"
#include "IChatTransport.h"
#include "LocalFilePickerService.h"
#include "MediaUploadService.h"
#include "usermgr.h"

#include <QDebug>
#include <vector>

void AppPortRegistry::bindGroupFeaturePorts()
{
    _features.groupController.setCommandPort(GroupCommandPort{
        [this]()
        {
            GroupCommandSnapshot snapshot;
            if (const auto selfInfo = _gateway.userMgr() ? _gateway.userMgr()->GetUserInfo() : nullptr)
            {
                snapshot.selfUid = selfInfo->_uid;
                snapshot.selfUserId = selfInfo->_user_id;
            }
            snapshot.uploadToken = _session_coordinator->authToken();
            snapshot.currentGroupId = currentGroupId();
            snapshot.currentGroupRole = currentGroupRole();
            snapshot.groupsReady = _shell_state.bootstrapState().groupsReady;
            snapshot.canChangeInfo = currentGroupCanChangeInfo();
            snapshot.canManageAdmins = currentGroupCanManageAdmins();
            return snapshot;
        },
        [this]()
        {
            if (!_gateway.userMgr())
            {
                return std::vector<std::shared_ptr<FriendInfo>>{};
            }
            return _gateway.userMgr()->GetFriendListSnapshot();
        },
        [this](ReqId reqId, const QByteArray& payload)
        {
            _gateway.chatTransport()->slot_send_data(reqId, payload);
        },
        [this]()
        {
            bootstrapGroups();
        },
        [this](const QString& text, bool isError)
        {
            setGroupStatus(text, isError);
        },
        [this](const QString& text)
        {
            _media_coordinator->sendTextMessage(text);
        },
        [this]()
        {
            _media_coordinator->sendImageMessage();
        },
        [this]()
        {
            _media_coordinator->sendFileMessage();
        },
        [](int source, QString* avatarUrl, QString* errorText)
        {
            if (source == 0)
            {
                return LocalFilePickerService::pickAvatarUrl(avatarUrl, errorText);
            }
            if (source == 1)
            {
                return LocalFilePickerService::pickAvatarFromScreen(avatarUrl, errorText);
            }
            return LocalFilePickerService::pickAvatarFromWebcam(avatarUrl, errorText);
        },
        [](const QString& path, int uid, const QString& token, UploadedMediaInfo* uploaded, QString* errorText)
        {
            return MediaUploadService::uploadLocalFile(path, QStringLiteral("avatar"), uid, token, uploaded, errorText);
        },
        [this](int index)
        {
            selectGroupIndex(index);
        },
        [this](const QString& msgId, const QString& text)
        {
            _features.chatFeatureController.editCurrentMessage(msgId, text);
        },
        [this](const QString& msgId)
        {
            _features.chatFeatureController.revokeCurrentMessage(msgId);
        },
        [this](const QString& msgId)
        {
            _features.chatFeatureController.forwardCurrentMessage(msgId);
        },
        [this]()
        {
            const GroupHistoryRequestResult result = _features.chatFeatureController.requestCurrentGroupHistory();
            if (!result.success || result.skipped)
            {
                if (result.skippedByLoading)
                {
                    qInfo() << "Skip group history request because one is already loading, group id:" << result.groupId;
                }
                return;
            }
            qInfo() << "Requesting group history, group id:" << result.groupId << "before seq:" << result.beforeSeq
                    << "private history loading:" << _shell_state.loadingState().privateHistoryLoading;
        }});
}
