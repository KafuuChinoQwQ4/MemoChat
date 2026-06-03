#include "GroupController.h"

GroupController::GroupController(QObject* parent)
    : QObject(parent)
{
}

FriendListModel* GroupController::groupListModel() const
{
    return _group_list_model;
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
    emit ensureGroupsInitializedRequested();
}

void GroupController::selectGroupIndex(int index)
{
    emit selectGroupIndexRequested(index);
}

void GroupController::refreshGroupList()
{
    emit refreshGroupListRequested();
}

void GroupController::createGroup(const QString& name, const QVariantList& memberUserIdList)
{
    emit createGroupRequested(name, memberUserIdList);
}

void GroupController::inviteGroupMember(const QString& userId, const QString& reason)
{
    emit inviteGroupMemberRequested(userId, reason);
}

void GroupController::applyJoinGroup(const QString& groupCode, const QString& reason)
{
    emit applyJoinGroupRequested(groupCode, reason);
}

void GroupController::reviewGroupApply(qint64 applyId, bool agree)
{
    emit reviewGroupApplyRequested(applyId, agree);
}

void GroupController::sendGroupTextMessage(const QString& text)
{
    emit sendGroupTextMessageRequested(text);
}

void GroupController::sendGroupImageMessage()
{
    emit sendGroupImageMessageRequested();
}

void GroupController::sendGroupFileMessage()
{
    emit sendGroupFileMessageRequested();
}

void GroupController::editGroupMessage(const QString& msgId, const QString& text)
{
    emit editGroupMessageRequested(msgId, text);
}

void GroupController::revokeGroupMessage(const QString& msgId)
{
    emit revokeGroupMessageRequested(msgId);
}

void GroupController::forwardGroupMessage(const QString& msgId)
{
    emit forwardGroupMessageRequested(msgId);
}

void GroupController::loadGroupHistory()
{
    emit loadGroupHistoryRequested();
}

void GroupController::updateGroupAnnouncement(const QString& announcement)
{
    emit updateGroupAnnouncementRequested(announcement);
}

void GroupController::updateGroupIcon(int source)
{
    emit updateGroupIconRequested(source);
}

void GroupController::setGroupAdmin(const QString& userId, bool isAdmin, qint64 permissionBits)
{
    emit setGroupAdminRequested(userId, isAdmin, permissionBits);
}

void GroupController::muteGroupMember(const QString& userId, int muteSeconds)
{
    emit muteGroupMemberRequested(userId, muteSeconds);
}

void GroupController::kickGroupMember(const QString& userId)
{
    emit kickGroupMemberRequested(userId);
}

void GroupController::quitCurrentGroup()
{
    emit quitCurrentGroupRequested();
}

void GroupController::dissolveCurrentGroup()
{
    emit dissolveCurrentGroupRequested();
}

void GroupController::clearGroupStatus()
{
    emit clearGroupStatusRequested();
}

void GroupController::syncModel(FriendListModel* groupListModel)
{
    if (_group_list_model == groupListModel)
    {
        return;
    }

    _group_list_model = groupListModel;
    emit modelChanged();
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
