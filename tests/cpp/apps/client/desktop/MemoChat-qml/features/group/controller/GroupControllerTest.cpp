#include "FriendListModel.h"
#include "GroupController.h"
#include "GroupManagementEventService.h"
#include "GroupManagementResponseService.h"
#include "GroupRequestPayloads.h"
#include "MediaUploadResult.h"
#include "UserGroupData.h"
#include "userdata.h"

#include <gtest/gtest.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTest>
#include <QString>
#include <QVariant>
#include <memory>
#include <vector>

namespace
{
std::shared_ptr<FriendInfo> makeGroupDialog(int dialogUid, const QString& name)
{
    auto group = std::make_shared<FriendInfo>(
        dialogUid,
        name,
        name,
        QStringLiteral("group.png"),
            0,
            QStringLiteral("group desc"), name, QStringLiteral("last ") + name, QStringLiteral("g123456789"));
    group->_dialog_type = QStringLiteral("group");
    return group;
}

std::shared_ptr<FriendInfo> makeFriendUser(int uid, const QString& userId, const QString& name)
{
    auto friendInfo = std::make_shared<FriendInfo>(
        uid,
        name,
        name,
        QStringLiteral("friend.png"), 0, QStringLiteral("friend desc"), name, QStringLiteral("last ") + name, userId);
    friendInfo->_dialog_type = QStringLiteral("private");
    return friendInfo;
}

QVariantMap row(FriendListModel* model, int index)
{
    return model->get(index);
}

GroupCommandSnapshot groupSnapshot()
{
    GroupCommandSnapshot snapshot;
    snapshot.selfUid = 1001;
    snapshot.selfUserId = QStringLiteral("u123456789");
    snapshot.uploadToken = QStringLiteral("token");
    snapshot.currentGroupId = 42;
    snapshot.groupsReady = true;
    snapshot.canChangeInfo = true;
    snapshot.canManageAdmins = true;
    return snapshot;
}
} // namespace

TEST(GroupControllerTest, OwnsGroupListModelAndIgnoresExternalSyncModel)
{
    GroupController controller(nullptr);

    ASSERT_NE(controller.groupListModel(), nullptr);
    EXPECT_EQ(controller.groupListModel()->parent(), &controller);
    EXPECT_EQ(controller.groupListModel()->rowCount(), 0);

    FriendListModel externalModel;
    controller.syncModel(&externalModel);

    EXPECT_NE(controller.groupListModel(), &externalModel);
    EXPECT_EQ(controller.groupListModel()->parent(), &controller);
}

TEST(GroupControllerTest, UpdatesGroupListModelWithSetAppendUpsertAndRemove)
{
    GroupController controller(nullptr);

    controller.setGroups(
        {makeGroupDialog(1001, QStringLiteral("Alpha")), makeGroupDialog(1002, QStringLiteral("Beta"))});
    ASSERT_EQ(controller.groupListModel()->rowCount(), 2);
    EXPECT_EQ(row(controller.groupListModel(), 0).value(QStringLiteral("uid")).toInt(), 1001);
    EXPECT_EQ(row(controller.groupListModel(), 1).value(QStringLiteral("name")).toString(), QStringLiteral("Beta"));
    EXPECT_EQ(
        row(controller.groupListModel(), 1).value(QStringLiteral("dialogType")).toString(), QStringLiteral("group"));

    controller.appendGroups({makeGroupDialog(1003, QStringLiteral("Gamma"))});
    ASSERT_EQ(controller.groupListModel()->rowCount(), 3);
    EXPECT_EQ(row(controller.groupListModel(), 2).value(QStringLiteral("uid")).toInt(), 1003);

    controller.upsertGroup(makeGroupDialog(1002, QStringLiteral("Beta Renamed")));
    ASSERT_EQ(controller.groupListModel()->rowCount(), 3);
    const int betaIndex = controller.groupListModel()->indexOfUid(1002);
    ASSERT_GE(betaIndex, 0);
    EXPECT_EQ(row(controller.groupListModel(), betaIndex)
                  .value(QStringLiteral("name")).toString(), QStringLiteral("Beta Renamed"));

    controller.removeGroupById(1001);
    ASSERT_EQ(controller.groupListModel()->rowCount(), 2);
    EXPECT_EQ(controller.groupListModel()->indexOfUid(1001), -1);
}

TEST(GroupControllerTest, UpdatesCurrentGroupFromGroupInfoData)
{
    GroupController controller(nullptr);

    GroupInfoData group;
    group._group_id = 42;
    group._group_code = QStringLiteral("g123456789");
    group._name = QStringLiteral("Ops");
    group._role = 2;
    group._permission_bits = memochat::group_payload::kPermissionChangeGroupInfo |
                             memochat::group_payload::kPermissionInviteUsers |
                             memochat::group_payload::kPermissionManageTopics;

    controller.setCurrentGroup(group);

    EXPECT_TRUE(controller.hasCurrentGroup());
    EXPECT_EQ(controller.currentGroupRole(), 2);
    EXPECT_EQ(controller.currentGroupName(), QStringLiteral("Ops"));
    EXPECT_EQ(controller.currentGroupCode(), QStringLiteral("g123456789"));
    EXPECT_TRUE(controller.currentGroupCanChangeInfo());
    EXPECT_FALSE(controller.currentGroupCanDeleteMessages());
    EXPECT_TRUE(controller.currentGroupCanInviteUsers());
    EXPECT_FALSE(controller.currentGroupCanManageAdmins());
    EXPECT_FALSE(controller.currentGroupCanPinMessages());
    EXPECT_FALSE(controller.currentGroupCanBanUsers());
    EXPECT_TRUE(controller.currentGroupCanManageTopics());

    controller.clearCurrentGroup();
    EXPECT_FALSE(controller.hasCurrentGroup());
    EXPECT_EQ(controller.currentGroupRole(), 0);
    EXPECT_TRUE(controller.currentGroupName().isEmpty());
    EXPECT_TRUE(controller.currentGroupCode().isEmpty());
}

TEST(GroupControllerTest, UpdatesStatusReadinessAndResetState)
{
    GroupController controller(nullptr);

    controller.setGroups({makeGroupDialog(1001, QStringLiteral("Alpha"))});
    controller.setCurrentGroup(
        9,
        1,
        QStringLiteral("Alpha"), QStringLiteral("g123456789"), memochat::group_payload::kOwnerPermissionBits);
    controller.setGroupStatus(QStringLiteral("Ready"), false);
    controller.setGroupsReady(true);

    EXPECT_TRUE(controller.hasCurrentGroup());
    EXPECT_EQ(controller.groupStatusText(), QStringLiteral("Ready"));
    EXPECT_FALSE(controller.groupStatusError());
    EXPECT_TRUE(controller.groupsReady());

    controller.setGroupStatus(QStringLiteral("Failed"), true);
    EXPECT_EQ(controller.groupStatusText(), QStringLiteral("Failed"));
    EXPECT_TRUE(controller.groupStatusError());

    controller.resetGroupFeature();
    EXPECT_EQ(controller.groupListModel()->rowCount(), 0);
    EXPECT_FALSE(controller.hasCurrentGroup());
    EXPECT_TRUE(controller.groupStatusText().isEmpty());
    EXPECT_FALSE(controller.groupStatusError());
    EXPECT_FALSE(controller.groupsReady());
}

TEST(GroupControllerTest, RequestDialogListSendsDialogListPayloadFromGroupController)
{
    GroupController controller(nullptr);
    int sendCalls = 0;
    ReqId sentReqId = ID_GET_VARIFY_CODE;
    QByteArray sentPayload;

    GroupCommandPort port;
    port.snapshot = []()
    {
        return groupSnapshot();
    };
    port.send = [&sendCalls, &sentReqId, &sentPayload](ReqId reqId, const QByteArray& payload)
    {
        ++sendCalls;
        sentReqId = reqId;
        sentPayload = payload;
    };
    controller.setCommandPort(std::move(port));

    controller.requestDialogList();

    EXPECT_EQ(sendCalls, 1);
    EXPECT_EQ(sentReqId, ReqId::ID_GET_DIALOG_LIST_REQ);
    const QJsonObject payload = QJsonDocument::fromJson(sentPayload).object();
    EXPECT_EQ(payload.value(QStringLiteral("fromuid")).toInt(), 1001);
    EXPECT_EQ(payload.size(), 1);
}

TEST(GroupControllerTest, RequestDialogListRequiresLoggedInUser)
{
    GroupController controller(nullptr);
    int sendCalls = 0;

    GroupCommandPort port;
    port.snapshot = []()
    {
        GroupCommandSnapshot snapshot = groupSnapshot();
        snapshot.selfUid = 0;
        return snapshot;
    };
    port.send = [&sendCalls](ReqId, const QByteArray&)
    {
        ++sendCalls;
    };
    controller.setCommandPort(std::move(port));

    controller.requestDialogList();

    EXPECT_EQ(sendCalls, 0);
    EXPECT_TRUE(controller.groupStatusError());
    EXPECT_EQ(controller.groupStatusText(), QStringLiteral("用户状态异常，请重新登录"));
}

TEST(GroupControllerTest, InviteGroupMemberUsesFriendSnapshotInsideGroupController)
{
    GroupController controller(nullptr);
    int snapshotCalls = 0;
    ReqId sentReqId = ID_GET_VARIFY_CODE;
    QByteArray sentPayload;

    GroupCommandPort port;
    port.snapshot = []()
    {
        return groupSnapshot();
    };
    port.friendListSnapshot = [&snapshotCalls]()
    {
        ++snapshotCalls;
        return std::vector<std::shared_ptr<FriendInfo>>{
            makeFriendUser(2001, QStringLiteral("u223456789"), QStringLiteral("Ada"))};
    };
    port.send = [&sentReqId, &sentPayload](ReqId reqId, const QByteArray& payload)
    {
        sentReqId = reqId;
        sentPayload = payload;
    };
    controller.setCommandPort(std::move(port));

    controller.inviteGroupMember(QStringLiteral(" u223456789 "), QStringLiteral("join"));

    EXPECT_EQ(snapshotCalls, 1);
    EXPECT_EQ(sentReqId, ReqId::ID_INVITE_GROUP_MEMBER_REQ);
    const QJsonObject payload = QJsonDocument::fromJson(sentPayload).object();
    EXPECT_EQ(payload.value(QStringLiteral("fromuid")).toInt(), 1001);
    EXPECT_EQ(payload.value(QStringLiteral("target_user_id")).toString(), QStringLiteral("u223456789"));
    EXPECT_EQ(payload.value(QStringLiteral("groupid")).toVariant().toLongLong(), 42);
    EXPECT_EQ(payload.value(QStringLiteral("reason")).toString(), QStringLiteral("join"));
    EXPECT_EQ(controller.groupStatusText(), QStringLiteral("邀请已发送"));
    EXPECT_FALSE(controller.groupStatusError());
}

TEST(GroupControllerTest, InviteGroupMemberRejectsUnknownFriendUserIdFromSnapshot)
{
    GroupController controller(nullptr);
    int sendCalls = 0;

    GroupCommandPort port;
    port.snapshot = []()
    {
        return groupSnapshot();
    };
    port.friendListSnapshot = []()
    {
        return std::vector<std::shared_ptr<FriendInfo>>{
            makeFriendUser(2002, QStringLiteral("u987654321"), QStringLiteral("Ben"))};
    };
    port.send = [&sendCalls](ReqId, const QByteArray&)
    {
        ++sendCalls;
    };
    controller.setCommandPort(std::move(port));

    controller.inviteGroupMember(QStringLiteral("u223456789"), QStringLiteral("join"));

    EXPECT_EQ(sendCalls, 0);
    EXPECT_EQ(controller.groupStatusText(), QStringLiteral("仅支持邀请好友入群"));
    EXPECT_TRUE(controller.groupStatusError());
}

TEST(GroupControllerTest, AppliesGroupManagementEventEffectsThroughControllerDefaults)
{
    GroupController controller(nullptr);
    int refreshListCalls = 0;
    int removedGroupId = 0;
    int clearedConversationGroupId = 0;

    GroupCommandPort commandPort;
    commandPort.snapshot = []()
    {
        return groupSnapshot();
    };
    commandPort.send = [&refreshListCalls](ReqId reqId, const QByteArray& payload)
    {
        ++refreshListCalls;
        EXPECT_EQ(reqId, ReqId::ID_GET_GROUP_LIST_REQ);
        const QJsonObject object = QJsonDocument::fromJson(payload).object();
        EXPECT_EQ(object.value(QStringLiteral("fromuid")).toInt(), 1001);
    };
    controller.setCommandPort(std::move(commandPort));

    memochat::group::GroupManagementEffect effect;
    effect.handled = true;
    effect.hasStatus = true;
    effect.statusText = QStringLiteral("group changed");
    effect.statusIsError = false;
    effect.requestGroupList = true;
    effect.removeGroupIds = {42};
    effect.clearGroupConversationIds = {42};

    memochat::group::GroupManagementEventEffectPort port;
    port.removeGroup = [&removedGroupId](qint64 groupId)
    {
        removedGroupId = static_cast<int>(groupId);
    };
    port.clearGroupConversation = [&clearedConversationGroupId](qint64 groupId)
    {
        clearedConversationGroupId = static_cast<int>(groupId);
    };

    controller.applyGroupManagementEffect(effect, std::move(port));

    EXPECT_EQ(controller.groupStatusText(), QStringLiteral("group changed"));
    EXPECT_FALSE(controller.groupStatusError());
    EXPECT_EQ(refreshListCalls, 1);
    EXPECT_EQ(removedGroupId, 42);
    EXPECT_EQ(clearedConversationGroupId, 42);
}

TEST(GroupControllerTest, AppliesGroupManagementResponseEffectsThroughControllerDefaults)
{
    GroupController controller(nullptr);
    QSignalSpy createdSpy(&controller, &GroupController::groupCreated);
    int refreshListCalls = 0;
    int selectedDialogUid = 0;

    GroupCommandPort commandPort;
    commandPort.snapshot = []()
    {
        return groupSnapshot();
    };
    commandPort.send = [&refreshListCalls](ReqId reqId, const QByteArray& payload)
    {
        ++refreshListCalls;
        EXPECT_EQ(reqId, ReqId::ID_GET_GROUP_LIST_REQ);
        const QJsonObject object = QJsonDocument::fromJson(payload).object();
        EXPECT_EQ(object.value(QStringLiteral("fromuid")).toInt(), 1001);
    };
    controller.setCommandPort(std::move(commandPort));

    memochat::group::GroupManagementResponseEffect effect;
    effect.handled = true;
    effect.hasStatus = true;
    effect.statusText = QStringLiteral("created");
    effect.statusIsError = false;
    effect.refreshGroupList = true;
    effect.selectDialogUid = true;
    effect.dialogUid = 9001;
    effect.emitGroupCreatedId = true;
    effect.createdGroupId = 42;

    memochat::group::GroupManagementResponseEffectPort port;
    port.selectDialogByUid = [&selectedDialogUid](int dialogUid)
    {
        selectedDialogUid = dialogUid;
    };

    controller.applyGroupManagementResponseEffect(effect, std::move(port));

    EXPECT_EQ(controller.groupStatusText(), QStringLiteral("created"));
    EXPECT_FALSE(controller.groupStatusError());
    EXPECT_EQ(refreshListCalls, 1);
    EXPECT_EQ(selectedDialogUid, 9001);
    ASSERT_EQ(createdSpy.count(), 1);
    EXPECT_EQ(createdSpy.takeFirst().at(0).toLongLong(), 42);
}

TEST(GroupControllerTest, UploadsGroupIconAndDispatchesUpdatePayload)
{
    GroupController controller(nullptr);
    QByteArray sentPayload;
    ReqId sentReqId = ID_GET_VARIFY_CODE;
    int pickCalls = 0;
    int uploadCalls = 0;
    int pickedSource = -1;
    QString uploadedPath;
    int uploadedUid = 0;
    QString uploadedToken;

    GroupCommandPort port;
    port.snapshot = []()
    {
        return groupSnapshot();
    };
    port.pickGroupIcon = [&pickCalls, &pickedSource](int source, QString* avatarUrl, QString* errorText)
    {
        Q_UNUSED(errorText);
        ++pickCalls;
        pickedSource = source;
        *avatarUrl = QStringLiteral("file:///tmp/group.png");
        return true;
    };
    port.uploadGroupIcon = [&uploadCalls, &uploadedPath, &uploadedUid, &uploadedToken](const QString& path,
                                                                                       int uid,
                                                                                       const QString& token,
                                                                                       UploadedMediaInfo* uploaded,
                                                                                       QString* errorText)
    {
        Q_UNUSED(errorText);
        ++uploadCalls;
        uploadedPath = path;
        uploadedUid = uid;
        uploadedToken = token;
        uploaded->remoteUrl = QStringLiteral("https://cdn.example/group.png");
        return true;
    };
    port.send = [&sentReqId, &sentPayload](ReqId reqId, const QByteArray& payload)
    {
        sentReqId = reqId;
        sentPayload = payload;
    };
    controller.setCommandPort(std::move(port));

    controller.updateGroupIcon(1);

    QTRY_VERIFY_WITH_TIMEOUT(sentReqId == ID_UPDATE_GROUP_ICON_REQ, 1000);
    EXPECT_EQ(pickCalls, 1);
    EXPECT_EQ(pickedSource, 1);
    EXPECT_EQ(uploadCalls, 1);
    EXPECT_EQ(uploadedPath, QStringLiteral("file:///tmp/group.png"));
    EXPECT_EQ(uploadedUid, 1001);
    EXPECT_EQ(uploadedToken, QStringLiteral("token"));
    const QJsonObject payload = QJsonDocument::fromJson(sentPayload).object();
    EXPECT_EQ(payload.value(QStringLiteral("fromuid")).toInt(), 1001);
    EXPECT_EQ(payload.value(QStringLiteral("groupid")).toVariant().toLongLong(), 42);
    EXPECT_EQ(payload.value(QStringLiteral("icon")).toString(), QStringLiteral("https://cdn.example/group.png"));
    EXPECT_EQ(controller.groupStatusText(), QStringLiteral("群头像更新中..."));
    EXPECT_FALSE(controller.groupStatusError());
}

TEST(GroupControllerTest, GroupIconUploadFailureResetsAndAllowsRetry)
{
    GroupController controller(nullptr);
    int uploadCalls = 0;
    GroupCommandPort port;
    port.snapshot = []()
    {
        return groupSnapshot();
    };
    port.pickGroupIcon = [](int, QString* avatarUrl, QString*)
    {
        *avatarUrl = QStringLiteral("file:///tmp/group.png");
        return true;
    };
    port.uploadGroupIcon = [&uploadCalls](const QString&, int, const QString&, UploadedMediaInfo*, QString* errorText)
    {
        ++uploadCalls;
        if (errorText)
        {
            *errorText = QStringLiteral("upload failed");
        }
        return false;
    };
    controller.setCommandPort(std::move(port));

    controller.updateGroupIcon(0);
    QTRY_VERIFY_WITH_TIMEOUT(controller.groupStatusError(), 1000);
    EXPECT_EQ(controller.groupStatusText(), QStringLiteral("upload failed"));
    EXPECT_EQ(uploadCalls, 1);

    controller.updateGroupIcon(0);
    QTRY_VERIFY_WITH_TIMEOUT(uploadCalls == 2, 1000);
}
