#ifndef GROUPCONTROLLER_H
#define GROUPCONTROLLER_H

#include "global.h"
#include "GroupManagementEffectPorts.h"

#include <QMap>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QtGlobal>
#include <functional>
#include <memory>
#include <vector>

class FriendListModel;
struct FriendInfo;
struct GroupInfoData;
struct UploadedMediaInfo;

struct GroupCommandSnapshot
{
    int selfUid = 0;
    QString selfUserId;
    QString uploadToken;
    qint64 currentGroupId = 0;
    int currentGroupRole = 0;
    bool groupsReady = false;
    bool canChangeInfo = false;
    bool canManageAdmins = false;
};

struct GroupCommandPort
{
    std::function<GroupCommandSnapshot()> snapshot;
    std::function<std::vector<std::shared_ptr<FriendInfo>>()> friendListSnapshot;
    std::function<void(ReqId, const QByteArray&)> send;
    std::function<void()> bootstrapGroups;
    std::function<void(const QString&, bool)> setStatus;
    std::function<void(const QString&)> sendText;
    std::function<void()> sendImage;
    std::function<void()> sendFile;
    std::function<bool(int, QString*, QString*)> pickGroupIcon;
    std::function<bool(const QString&, int, const QString&, UploadedMediaInfo*, QString*)> uploadGroupIcon;
    std::function<void(int)> selectGroupIndex;
    std::function<void(const QString&, const QString&)> editMessage;
    std::function<void(const QString&)> revokeMessage;
    std::function<void(const QString&)> forwardMessage;
    std::function<void()> loadHistory;
};

namespace memochat::group
{
struct GroupManagementEffect;
struct GroupManagementResponseEffect;
} // namespace memochat::group

class GroupController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(FriendListModel* groupListModel READ groupListModel NOTIFY modelChanged)
    Q_PROPERTY(bool hasCurrentGroup READ hasCurrentGroup NOTIFY currentGroupChanged)
    Q_PROPERTY(int currentGroupRole READ currentGroupRole NOTIFY currentGroupChanged)
    Q_PROPERTY(QString currentGroupName READ currentGroupName NOTIFY currentGroupChanged)
    Q_PROPERTY(QString currentGroupCode READ currentGroupCode NOTIFY currentGroupChanged)
    Q_PROPERTY(bool currentGroupCanChangeInfo READ currentGroupCanChangeInfo NOTIFY currentGroupChanged)
    Q_PROPERTY(bool currentGroupCanDeleteMessages READ currentGroupCanDeleteMessages NOTIFY currentGroupChanged)
    Q_PROPERTY(bool currentGroupCanInviteUsers READ currentGroupCanInviteUsers NOTIFY currentGroupChanged)
    Q_PROPERTY(bool currentGroupCanManageAdmins READ currentGroupCanManageAdmins NOTIFY currentGroupChanged)
    Q_PROPERTY(bool currentGroupCanPinMessages READ currentGroupCanPinMessages NOTIFY currentGroupChanged)
    Q_PROPERTY(bool currentGroupCanBanUsers READ currentGroupCanBanUsers NOTIFY currentGroupChanged)
    Q_PROPERTY(bool currentGroupCanManageTopics READ currentGroupCanManageTopics NOTIFY currentGroupChanged)
    Q_PROPERTY(QString groupStatusText READ groupStatusText NOTIFY groupStatusChanged)
    Q_PROPERTY(bool groupStatusError READ groupStatusError NOTIFY groupStatusChanged)
    Q_PROPERTY(bool groupsReady READ groupsReady NOTIFY groupsReadyChanged)

public:
    explicit GroupController(QObject* parent = nullptr);

    FriendListModel* groupListModel() const;
    qint64 currentGroupId() const;
    bool hasCurrentGroup() const;
    int currentGroupRole() const;
    QString currentGroupName() const;
    QString currentGroupCode() const;
    qint64 currentGroupPermissionBitsRaw() const;
    bool currentGroupCanChangeInfo() const;
    bool currentGroupCanDeleteMessages() const;
    bool currentGroupCanInviteUsers() const;
    bool currentGroupCanManageAdmins() const;
    bool currentGroupCanPinMessages() const;
    bool currentGroupCanBanUsers() const;
    bool currentGroupCanManageTopics() const;
    QString groupStatusText() const;
    bool groupStatusError() const;
    bool groupsReady() const;

    Q_INVOKABLE void ensureGroupsInitialized();
    Q_INVOKABLE void selectGroupIndex(int index);
    Q_INVOKABLE void refreshGroupList();
    Q_INVOKABLE void requestDialogList();
    Q_INVOKABLE void createGroup(const QString& name, const QVariantList& memberUserIdList = QVariantList());
    Q_INVOKABLE void inviteGroupMember(const QString& userId, const QString& reason = QString());
    Q_INVOKABLE void applyJoinGroup(const QString& groupCode, const QString& reason = QString());
    Q_INVOKABLE void reviewGroupApply(qint64 applyId, bool agree);
    Q_INVOKABLE void sendGroupTextMessage(const QString& text);
    Q_INVOKABLE void sendGroupImageMessage();
    Q_INVOKABLE void sendGroupFileMessage();
    Q_INVOKABLE void editGroupMessage(const QString& msgId, const QString& text);
    Q_INVOKABLE void revokeGroupMessage(const QString& msgId);
    Q_INVOKABLE void forwardGroupMessage(const QString& msgId);
    Q_INVOKABLE void loadGroupHistory();
    Q_INVOKABLE void updateGroupAnnouncement(const QString& announcement);
    Q_INVOKABLE void updateGroupIcon(int source = 0);
    Q_INVOKABLE void setGroupAdmin(const QString& userId, bool isAdmin, qint64 permissionBits = 0);
    Q_INVOKABLE void muteGroupMember(const QString& userId, int muteSeconds);
    Q_INVOKABLE void kickGroupMember(const QString& userId);
    Q_INVOKABLE void quitCurrentGroup();
    Q_INVOKABLE void dissolveCurrentGroup();
    Q_INVOKABLE void clearGroupStatus();

    void syncModel(FriendListModel* groupListModel);
    void resetGroupFeature();
    void setGroupsFromSnapshots(const std::vector<std::shared_ptr<GroupInfoData>>& groups,
                                QMap<int, qint64>& groupDialogUidMap);
    void setGroups(const std::vector<std::shared_ptr<FriendInfo>>& groups);
    void appendGroups(const std::vector<std::shared_ptr<FriendInfo>>& groups);
    void upsertGroup(const std::shared_ptr<FriendInfo>& group);
    void removeGroupById(int groupDialogUid);
    void setCurrentGroup(const GroupInfoData& group);
    void setCurrentGroup(qint64 groupId,
                         int currentGroupRole,
                         const QString& currentGroupName,
                         const QString& currentGroupCode,
                         qint64 permissionBits);
    void clearCurrentGroup();
    void setGroupStatus(const QString& text, bool isError);
    void setGroupsReady(bool ready);
    void syncCurrentGroup(bool hasCurrentGroup,
                          int currentGroupRole,
                          const QString& currentGroupName,
                          const QString& currentGroupCode,
                          bool canChangeInfo,
                          bool canDeleteMessages,
                          bool canInviteUsers,
                          bool canManageAdmins,
                          bool canPinMessages,
                          bool canBanUsers,
                          bool canManageTopics);
    void syncGroupStatus(const QString& text, bool isError);
    void syncGroupsReady(bool ready);
    void notifyGroupCreated(qint64 groupId);
    void applyGroupManagementEffect(const memochat::group::GroupManagementEffect& effect,
                                    memochat::group::GroupManagementEventEffectPort port);
    void applyGroupManagementResponseEffect(const memochat::group::GroupManagementResponseEffect& effect,
                                            memochat::group::GroupManagementResponseEffectPort port);
    void setCommandPort(GroupCommandPort port);

signals:
    void modelChanged();
    void currentGroupChanged();
    void groupStatusChanged();
    void groupsReadyChanged();
    void groupCreated(qint64 groupId);

private:
    GroupCommandSnapshot commandSnapshot() const;
    bool requireUser(const GroupCommandSnapshot& snapshot);
    bool requireCurrentGroup(const GroupCommandSnapshot& snapshot);
    bool isFriendUserId(const QString& userId) const;
    void sendGroupIdCommand(ReqId reqId, const QString& statusText);

    FriendListModel* _group_list_model = nullptr;
    GroupCommandPort _command_port;
    bool _group_icon_upload_in_progress = false;
    qint64 _current_group_id = 0;
    bool _has_current_group = false;
    int _current_group_role = 0;
    QString _current_group_name;
    QString _current_group_code;
    qint64 _current_group_permission_bits = 0;
    bool _current_group_can_change_info = false;
    bool _current_group_can_delete_messages = false;
    bool _current_group_can_invite_users = false;
    bool _current_group_can_manage_admins = false;
    bool _current_group_can_pin_messages = false;
    bool _current_group_can_ban_users = false;
    bool _current_group_can_manage_topics = false;
    QString _group_status_text;
    bool _group_status_error = false;
    bool _groups_ready = false;
};

#endif // GROUPCONTROLLER_H
