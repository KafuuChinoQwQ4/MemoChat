#include "GroupController.h"
#include "ConversationSyncService.h"
#include "GroupManagementEffectApplier.h"
#include "GroupRequestPayloads.h"
#include "FriendListModel.h"
#include "MediaUploadResult.h"
#include "UserGroupData.h"
#include "userdata.h"

#include <QFutureWatcher>
#include <QJsonDocument>
#include <QtConcurrent>
#include <QtGlobal>

GroupController::GroupController(QObject* parent)
    : QObject(parent)
    , _group_list_model(new FriendListModel(this))
{
}

FriendListModel* GroupController::groupListModel() const
{
    return _group_list_model;
}

qint64 GroupController::currentGroupId() const
{
    return _current_group_id;
}

bool GroupController::hasCurrentGroup() const
{
    return _has_current_group;
}

int GroupController::currentGroupRole() const
{
    return _current_group_role;
}

QString GroupController::currentGroupName() const
{
    return _current_group_name;
}

QString GroupController::currentGroupCode() const
{
    return _current_group_code;
}

qint64 GroupController::currentGroupPermissionBitsRaw() const
{
    return _current_group_permission_bits;
}

bool GroupController::currentGroupCanChangeInfo() const
{
    return _current_group_can_change_info;
}

bool GroupController::currentGroupCanDeleteMessages() const
{
    return _current_group_can_delete_messages;
}

bool GroupController::currentGroupCanInviteUsers() const
{
    return _current_group_can_invite_users;
}

bool GroupController::currentGroupCanManageAdmins() const
{
    return _current_group_can_manage_admins;
}

bool GroupController::currentGroupCanPinMessages() const
{
    return _current_group_can_pin_messages;
}

bool GroupController::currentGroupCanBanUsers() const
{
    return _current_group_can_ban_users;
}

bool GroupController::currentGroupCanManageTopics() const
{
    return _current_group_can_manage_topics;
}

QString GroupController::groupStatusText() const
{
    return _group_status_text;
}

bool GroupController::groupStatusError() const
{
    return _group_status_error;
}

bool GroupController::groupsReady() const
{
    return _groups_ready;
}

void GroupController::ensureGroupsInitialized()
{
    if (_groups_ready)
    {
        return;
    }
    if (_command_port.bootstrapGroups)
    {
        _command_port.bootstrapGroups();
    }
}

void GroupController::selectGroupIndex(int index)
{
    if (_command_port.selectGroupIndex)
    {
        _command_port.selectGroupIndex(index);
    }
}

void GroupController::refreshGroupList()
{
    const GroupCommandSnapshot snapshot = commandSnapshot();
    if (!snapshot.groupsReady)
    {
        if (_command_port.bootstrapGroups)
        {
            _command_port.bootstrapGroups();
        }
        return;
    }
    if (!requireUser(snapshot))
    {
        return;
    }
    const QJsonObject payload = memochat::group_payload::buildRefreshGroupListPayload(snapshot.selfUid);
    if (_command_port.send)
    {
        _command_port.send(ReqId::ID_GET_GROUP_LIST_REQ, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    }
}

void GroupController::requestDialogList()
{
    const GroupCommandSnapshot snapshot = commandSnapshot();
    if (!requireUser(snapshot))
    {
        return;
    }
    const QJsonObject payload = memochat::group_payload::buildRequestDialogListPayload(snapshot.selfUid);
    if (_command_port.send)
    {
        _command_port.send(ReqId::ID_GET_DIALOG_LIST_REQ, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    }
}

void GroupController::createGroup(const QString& name, const QVariantList& memberUserIdList)
{
    const GroupCommandSnapshot snapshot = commandSnapshot();
    if (!requireUser(snapshot))
    {
        return;
    }
    const QString trimmedName = memochat::group_payload::trimmedGroupName(name);
    if (!memochat::group_payload::isValidGroupName(trimmedName))
    {
        setGroupStatus("群名称长度需在1-64之间", true);
        return;
    }

    QString invalidUserId;
    const QJsonArray memberUserIds =
        memochat::group_payload::buildMemberUserIdArray(memberUserIdList, snapshot.selfUserId, &invalidUserId);
    if (!invalidUserId.isEmpty())
    {
        setGroupStatus(QString("成员ID格式非法：%1").arg(invalidUserId), true);
        return;
    }

    const QJsonObject payload =
        memochat::group_payload::buildCreateGroupPayload(snapshot.selfUid, trimmedName, memberUserIds);
    if (_command_port.send)
    {
        _command_port.send(ReqId::ID_CREATE_GROUP_REQ, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    }
    setGroupStatus("正在创建群聊...", false);
}

void GroupController::inviteGroupMember(const QString& userId, const QString& reason)
{
    const GroupCommandSnapshot snapshot = commandSnapshot();
    const QString trimmedUserId = memochat::group_payload::trimmedUserId(userId);
    if (!requireUser(snapshot) || !requireCurrentGroup(snapshot) ||
        !memochat::group_payload::isValidUserId(trimmedUserId))
    {
        setGroupStatus("邀请参数非法", true);
        return;
    }
    if (snapshot.selfUserId == trimmedUserId)
    {
        setGroupStatus("不能邀请自己", true);
        return;
    }
    if (!isFriendUserId(trimmedUserId))
    {
        setGroupStatus("仅支持邀请好友入群", true);
        return;
    }

    const QJsonObject payload = memochat::group_payload::buildInviteGroupMemberPayload(snapshot.selfUid,
                                                                                       trimmedUserId,
                                                                                       snapshot.currentGroupId,
                                                                                       reason);
    if (_command_port.send)
    {
        _command_port.send(ReqId::ID_INVITE_GROUP_MEMBER_REQ, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    }
    setGroupStatus("邀请已发送", false);
}

void GroupController::applyJoinGroup(const QString& groupCode, const QString& reason)
{
    const GroupCommandSnapshot snapshot = commandSnapshot();
    const QString trimmedGroupCode = memochat::group_payload::trimmedGroupCode(groupCode);
    if (!requireUser(snapshot) || !memochat::group_payload::isValidGroupCode(trimmedGroupCode))
    {
        setGroupStatus("申请参数非法", true);
        return;
    }
    const QJsonObject payload =
        memochat::group_payload::buildApplyJoinGroupPayload(snapshot.selfUid, trimmedGroupCode, reason);
    if (_command_port.send)
    {
        _command_port.send(ReqId::ID_APPLY_JOIN_GROUP_REQ, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    }
    setGroupStatus("申请已发送", false);
}

void GroupController::reviewGroupApply(qint64 applyId, bool agree)
{
    const GroupCommandSnapshot snapshot = commandSnapshot();
    if (!requireUser(snapshot) || applyId <= 0)
    {
        setGroupStatus("审核参数非法", true);
        return;
    }
    const QJsonObject payload = memochat::group_payload::buildReviewGroupApplyPayload(snapshot.selfUid, applyId, agree);
    if (_command_port.send)
    {
        _command_port.send(ReqId::ID_REVIEW_GROUP_APPLY_REQ, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    }
    setGroupStatus("审核请求已发送", false);
}

void GroupController::sendGroupTextMessage(const QString& text)
{
    const GroupCommandSnapshot snapshot = commandSnapshot();
    if (!requireCurrentGroup(snapshot))
    {
        return;
    }
    if (_command_port.sendText)
    {
        _command_port.sendText(text);
    }
}

void GroupController::sendGroupImageMessage()
{
    const GroupCommandSnapshot snapshot = commandSnapshot();
    if (!requireCurrentGroup(snapshot))
    {
        return;
    }
    if (_command_port.sendImage)
    {
        _command_port.sendImage();
    }
}

void GroupController::sendGroupFileMessage()
{
    const GroupCommandSnapshot snapshot = commandSnapshot();
    if (!requireCurrentGroup(snapshot))
    {
        return;
    }
    if (_command_port.sendFile)
    {
        _command_port.sendFile();
    }
}

void GroupController::editGroupMessage(const QString& msgId, const QString& text)
{
    if (_command_port.editMessage)
    {
        _command_port.editMessage(memochat::group_payload::trimmedMessageId(msgId), text);
    }
}

void GroupController::revokeGroupMessage(const QString& msgId)
{
    if (_command_port.revokeMessage)
    {
        _command_port.revokeMessage(memochat::group_payload::trimmedMessageId(msgId));
    }
}

void GroupController::forwardGroupMessage(const QString& msgId)
{
    if (_command_port.forwardMessage)
    {
        _command_port.forwardMessage(memochat::group_payload::trimmedMessageId(msgId));
    }
}

void GroupController::loadGroupHistory()
{
    if (_command_port.loadHistory)
    {
        _command_port.loadHistory();
    }
}

void GroupController::updateGroupAnnouncement(const QString& announcement)
{
    const GroupCommandSnapshot snapshot = commandSnapshot();
    if (!requireUser(snapshot) || !requireCurrentGroup(snapshot))
    {
        return;
    }
    if (!snapshot.canChangeInfo)
    {
        setGroupStatus("没有修改群资料权限", true);
        return;
    }
    const QJsonObject payload = memochat::group_payload::buildUpdateGroupAnnouncementPayload(snapshot.selfUid,
                                                                                             snapshot.currentGroupId,
                                                                                             announcement);
    if (_command_port.send)
    {
        _command_port.send(ReqId::ID_UPDATE_GROUP_ANNOUNCEMENT_REQ,
                           QJsonDocument(payload).toJson(QJsonDocument::Compact));
    }
}

void GroupController::updateGroupIcon(int source)
{
    const GroupCommandSnapshot snapshot = commandSnapshot();
    if (!requireUser(snapshot) || !requireCurrentGroup(snapshot))
    {
        return;
    }
    if (!snapshot.canChangeInfo)
    {
        setGroupStatus("没有修改群资料权限", true);
        return;
    }
    if (_group_icon_upload_in_progress)
    {
        setGroupStatus("群头像上传中，请稍候", false);
        return;
    }

    QString avatarUrl;
    QString errorText;
    if (!_command_port.pickGroupIcon || !_command_port.pickGroupIcon(source, &avatarUrl, &errorText))
    {
        if (!errorText.isEmpty())
        {
            setGroupStatus(errorText, true);
        }
        return;
    }
    if (snapshot.uploadToken.trimmed().isEmpty())
    {
        setGroupStatus("登录态失效，请重新登录", true);
        return;
    }
    if (!_command_port.uploadGroupIcon)
    {
        setGroupStatus("群头像上传服务未就绪", true);
        return;
    }

    _group_icon_upload_in_progress = true;
    setGroupStatus("群头像上传中...", false);

    auto* watcher = new QFutureWatcher<UploadedMediaInfo>(this);
    auto* uploadError = new QString();
    connect(watcher,
            &QFutureWatcher<UploadedMediaInfo>::finished,
            this,
            [this, watcher, uploadError, snapshot]()
            {
                const UploadedMediaInfo uploaded = watcher->future().result();
                watcher->deleteLater();
                const QString errorText = *uploadError;
                delete uploadError;
                _group_icon_upload_in_progress = false;
                if (uploaded.remoteUrl.isEmpty())
                {
                    setGroupStatus(errorText.isEmpty() ? "群头像上传失败" : errorText, true);
                    return;
                }

                const QJsonObject payload =
                    memochat::group_payload::buildUpdateGroupIconPayload(snapshot.selfUid,
                                                                         snapshot.currentGroupId,
                                                                         uploaded.remoteUrl);
                if (_command_port.send)
                {
                    _command_port.send(ReqId::ID_UPDATE_GROUP_ICON_REQ,
                                       QJsonDocument(payload).toJson(QJsonDocument::Compact));
                }
                setGroupStatus("群头像更新中...", false);
            });

    const QString uploadPath = avatarUrl;
    const int selfUid = snapshot.selfUid;
    const QString uploadToken = snapshot.uploadToken;
    const auto uploadGroupIcon = _command_port.uploadGroupIcon;
    watcher->setFuture(QtConcurrent::run(
        [uploadGroupIcon, uploadPath, selfUid, uploadToken, uploadError]()
        {
            UploadedMediaInfo uploaded;
            if (!uploadGroupIcon(uploadPath, selfUid, uploadToken, &uploaded, uploadError))
            {
                return UploadedMediaInfo{};
            }
            return uploaded;
        }));
}

void GroupController::setGroupAdmin(const QString& userId, bool isAdmin, qint64 permissionBits)
{
    const GroupCommandSnapshot snapshot = commandSnapshot();
    const QString trimmedUserId = memochat::group_payload::trimmedUserId(userId);
    if (!requireUser(snapshot) || !requireCurrentGroup(snapshot) ||
        !memochat::group_payload::isValidUserId(trimmedUserId))
    {
        setGroupStatus("设置管理员参数非法", true);
        return;
    }
    if (!snapshot.canManageAdmins)
    {
        setGroupStatus("没有管理管理员权限", true);
        return;
    }
    const QJsonObject payload = memochat::group_payload::buildSetGroupAdminPayload(snapshot.selfUid,
                                                                                   snapshot.currentGroupId,
                                                                                   trimmedUserId,
                                                                                   isAdmin,
                                                                                   permissionBits);
    if (_command_port.send)
    {
        _command_port.send(ReqId::ID_SET_GROUP_ADMIN_REQ, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    }
}

void GroupController::muteGroupMember(const QString& userId, int muteSeconds)
{
    const GroupCommandSnapshot snapshot = commandSnapshot();
    const QString trimmedUserId = memochat::group_payload::trimmedUserId(userId);
    if (!requireUser(snapshot) || !requireCurrentGroup(snapshot) ||
        !memochat::group_payload::isValidUserId(trimmedUserId))
    {
        setGroupStatus("禁言参数非法", true);
        return;
    }
    const QJsonObject payload = memochat::group_payload::buildMuteGroupMemberPayload(snapshot.selfUid,
                                                                                     snapshot.currentGroupId,
                                                                                     trimmedUserId,
                                                                                     muteSeconds);
    if (_command_port.send)
    {
        _command_port.send(ReqId::ID_MUTE_GROUP_MEMBER_REQ, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    }
}

void GroupController::kickGroupMember(const QString& userId)
{
    const GroupCommandSnapshot snapshot = commandSnapshot();
    const QString trimmedUserId = memochat::group_payload::trimmedUserId(userId);
    if (!requireUser(snapshot) || !requireCurrentGroup(snapshot) ||
        !memochat::group_payload::isValidUserId(trimmedUserId))
    {
        setGroupStatus("踢人参数非法", true);
        return;
    }
    const QJsonObject payload =
        memochat::group_payload::buildKickGroupMemberPayload(snapshot.selfUid, snapshot.currentGroupId, trimmedUserId);
    if (_command_port.send)
    {
        _command_port.send(ReqId::ID_KICK_GROUP_MEMBER_REQ, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    }
}

void GroupController::quitCurrentGroup()
{
    const GroupCommandSnapshot snapshot = commandSnapshot();
    if (!requireUser(snapshot) || !requireCurrentGroup(snapshot))
    {
        return;
    }
    if (snapshot.currentGroupRole >= 3)
    {
        dissolveCurrentGroup();
        return;
    }

    const QJsonObject payload = memochat::group_payload::buildGroupIdPayload(snapshot.selfUid, snapshot.currentGroupId);
    if (_command_port.send)
    {
        _command_port.send(ReqId::ID_QUIT_GROUP_REQ, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    }
}

void GroupController::dissolveCurrentGroup()
{
    const GroupCommandSnapshot snapshot = commandSnapshot();
    if (!requireUser(snapshot) || !requireCurrentGroup(snapshot))
    {
        return;
    }
    if (snapshot.currentGroupRole < 3)
    {
        setGroupStatus("只有群主可以解散群聊", true);
        return;
    }
    const QJsonObject payload = memochat::group_payload::buildGroupIdPayload(snapshot.selfUid, snapshot.currentGroupId);
    if (_command_port.send)
    {
        _command_port.send(ReqId::ID_DISSOLVE_GROUP_REQ, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    }
    setGroupStatus("正在解散群聊...", false);
}

void GroupController::clearGroupStatus()
{
    setGroupStatus(QString(), false);
}

void GroupController::syncModel(FriendListModel* groupListModel)
{
    Q_UNUSED(groupListModel)
}

void GroupController::resetGroupFeature()
{
    _group_icon_upload_in_progress = false;
    _group_list_model->clear();
    clearCurrentGroup();
    setGroupStatus(QString(), false);
    setGroupsReady(false);
}

void GroupController::setGroupsFromSnapshots(const std::vector<std::shared_ptr<GroupInfoData>>& groups,
                                             QMap<int, qint64>& groupDialogUidMap)
{
    std::vector<std::shared_ptr<FriendInfo>> converted;
    converted.reserve(groups.size());
    for (const auto& group : groups)
    {
        if (!group || group->_group_id <= 0)
        {
            continue;
        }
        const int pseudoUid = ConversationSyncService::resolveGroupDialogUid(groupDialogUidMap, group->_group_id);
        if (pseudoUid == 0)
        {
            continue;
        }
        const QString groupIcon =
            group->_icon.trimmed().isEmpty() ? QStringLiteral("qrc:/res/chat_icon.png") : group->_icon;
        auto info = std::make_shared<FriendInfo>(pseudoUid,
                                                 group->_name,
                                                 group->_name,
                                                 groupIcon,
                                                 0,
                                                 group->_announcement,
                                                 group->_announcement,
                                                 group->_last_msg);
        converted.push_back(info);
    }

    setGroups(converted);
}

void GroupController::setGroups(const std::vector<std::shared_ptr<FriendInfo>>& groups)
{
    _group_list_model->setFriends(groups);
}

void GroupController::appendGroups(const std::vector<std::shared_ptr<FriendInfo>>& groups)
{
    _group_list_model->appendFriends(groups);
}

void GroupController::upsertGroup(const std::shared_ptr<FriendInfo>& group)
{
    _group_list_model->upsertFriend(group);
}

void GroupController::removeGroupById(int groupDialogUid)
{
    _group_list_model->removeByUid(groupDialogUid);
}

void GroupController::setCurrentGroup(const GroupInfoData& group)
{
    setCurrentGroup(group._group_id, group._role, group._name, group._group_code, group._permission_bits);
}

void GroupController::setCurrentGroup(qint64 groupId,
                                      int currentGroupRole,
                                      const QString& currentGroupName,
                                      const QString& currentGroupCode,
                                      qint64 permissionBits)
{
    const bool hasCurrentGroup = groupId > 0;
    const qint64 normalizedPermissionBits = hasCurrentGroup ? permissionBits : 0;
    _current_group_id = hasCurrentGroup ? groupId : 0;
    _current_group_permission_bits = normalizedPermissionBits;
    syncCurrentGroup(hasCurrentGroup,
                     hasCurrentGroup ? currentGroupRole : 0,
                     hasCurrentGroup ? currentGroupName : QString(),
                     hasCurrentGroup ? currentGroupCode : QString(),
                     (normalizedPermissionBits & memochat::group_payload::kPermissionChangeGroupInfo) != 0,
                     (normalizedPermissionBits & memochat::group_payload::kPermissionDeleteMessages) != 0,
                     (normalizedPermissionBits & memochat::group_payload::kPermissionInviteUsers) != 0,
                     (normalizedPermissionBits & memochat::group_payload::kPermissionManageAdmins) != 0,
                     (normalizedPermissionBits & memochat::group_payload::kPermissionPinMessages) != 0,
                     (normalizedPermissionBits & memochat::group_payload::kPermissionBanUsers) != 0,
                     (normalizedPermissionBits & memochat::group_payload::kPermissionManageTopics) != 0);
}

void GroupController::clearCurrentGroup()
{
    _current_group_id = 0;
    _current_group_permission_bits = 0;
    syncCurrentGroup(false, 0, QString(), QString(), false, false, false, false, false, false, false);
}

void GroupController::setGroupStatus(const QString& text, bool isError)
{
    syncGroupStatus(text, isError);
}

void GroupController::setGroupsReady(bool ready)
{
    syncGroupsReady(ready);
}

void GroupController::syncCurrentGroup(bool hasCurrentGroup,
                                       int currentGroupRole,
                                       const QString& currentGroupName,
                                       const QString& currentGroupCode,
                                       bool canChangeInfo,
                                       bool canDeleteMessages,
                                       bool canInviteUsers,
                                       bool canManageAdmins,
                                       bool canPinMessages,
                                       bool canBanUsers,
                                       bool canManageTopics)
{
    if (_has_current_group == hasCurrentGroup && _current_group_role == currentGroupRole &&
        _current_group_name == currentGroupName && _current_group_code == currentGroupCode &&
        _current_group_can_change_info == canChangeInfo && _current_group_can_delete_messages == canDeleteMessages &&
        _current_group_can_invite_users == canInviteUsers && _current_group_can_manage_admins == canManageAdmins &&
        _current_group_can_pin_messages == canPinMessages && _current_group_can_ban_users == canBanUsers &&
        _current_group_can_manage_topics == canManageTopics)
    {
        return;
    }

    _has_current_group = hasCurrentGroup;
    _current_group_role = currentGroupRole;
    _current_group_name = currentGroupName;
    _current_group_code = currentGroupCode;
    _current_group_can_change_info = canChangeInfo;
    _current_group_can_delete_messages = canDeleteMessages;
    _current_group_can_invite_users = canInviteUsers;
    _current_group_can_manage_admins = canManageAdmins;
    _current_group_can_pin_messages = canPinMessages;
    _current_group_can_ban_users = canBanUsers;
    _current_group_can_manage_topics = canManageTopics;
    emit currentGroupChanged();
}

void GroupController::syncGroupStatus(const QString& text, bool isError)
{
    if (_group_status_text == text && _group_status_error == isError)
    {
        return;
    }

    _group_status_text = text;
    _group_status_error = isError;
    emit groupStatusChanged();
}

void GroupController::syncGroupsReady(bool ready)
{
    if (_groups_ready == ready)
    {
        return;
    }

    _groups_ready = ready;
    emit groupsReadyChanged();
}

void GroupController::notifyGroupCreated(qint64 groupId)
{
    if (groupId <= 0)
    {
        return;
    }

    emit groupCreated(groupId);
}

void GroupController::applyGroupManagementEffect(const memochat::group::GroupManagementEffect& effect,
                                                 memochat::group::GroupManagementEventEffectPort port)
{
    if (!port.setStatus)
    {
        port.setStatus = [this](const QString& text, bool isError)
        {
            setGroupStatus(text, isError);
        };
    }
    if (!port.refreshGroupList)
    {
        port.refreshGroupList = [this]()
        {
            refreshGroupList();
        };
    }
    memochat::group::GroupManagementEffectApplier::apply(effect, port);
}

void GroupController::applyGroupManagementResponseEffect(const memochat::group::GroupManagementResponseEffect& effect,
                                                         memochat::group::GroupManagementResponseEffectPort port)
{
    if (!port.setStatus)
    {
        port.setStatus = [this](const QString& text, bool isError)
        {
            setGroupStatus(text, isError);
        };
    }
    if (!port.notifyGroupCreated)
    {
        port.notifyGroupCreated = [this](qint64 groupId)
        {
            notifyGroupCreated(groupId);
        };
    }
    if (!port.refreshGroupList)
    {
        port.refreshGroupList = [this]()
        {
            refreshGroupList();
        };
    }
    memochat::group::GroupManagementEffectApplier::apply(effect, port);
}

void GroupController::setCommandPort(GroupCommandPort port)
{
    _command_port = std::move(port);
}

GroupCommandSnapshot GroupController::commandSnapshot() const
{
    if (_command_port.snapshot)
    {
        return _command_port.snapshot();
    }
    return {};
}

bool GroupController::requireUser(const GroupCommandSnapshot& snapshot)
{
    if (snapshot.selfUid <= 0)
    {
        setGroupStatus("用户状态异常，请重新登录", true);
        return false;
    }
    return true;
}

bool GroupController::requireCurrentGroup(const GroupCommandSnapshot& snapshot)
{
    if (snapshot.currentGroupId <= 0)
    {
        setGroupStatus("请选择群聊", true);
        return false;
    }
    return true;
}

bool GroupController::isFriendUserId(const QString& userId) const
{
    if (!_command_port.friendListSnapshot)
    {
        return true;
    }

    const auto friends = _command_port.friendListSnapshot();
    for (const auto& friendInfo : friends)
    {
        if (friendInfo && friendInfo->_user_id == userId)
        {
            return true;
        }
    }
    return false;
}

void GroupController::sendGroupIdCommand(ReqId reqId, const QString& statusText)
{
    const GroupCommandSnapshot snapshot = commandSnapshot();
    if (!requireUser(snapshot) || !requireCurrentGroup(snapshot))
    {
        return;
    }
    const QJsonObject payload = memochat::group_payload::buildGroupIdPayload(snapshot.selfUid, snapshot.currentGroupId);
    if (_command_port.send)
    {
        _command_port.send(reqId, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    }
    if (!statusText.isEmpty())
    {
        setGroupStatus(statusText, false);
    }
}
