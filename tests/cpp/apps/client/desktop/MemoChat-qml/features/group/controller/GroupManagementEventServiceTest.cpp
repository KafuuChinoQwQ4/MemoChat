#include "GroupManagementEventService.h"

#include <gtest/gtest.h>

#include <QJsonObject>
#include <QString>

namespace
{
memochat::group::GroupManagementEventContext contextWithSelf()
{
    memochat::group::GroupManagementEventContext context;
    context.selfUid = 10001;
    return context;
}

QJsonObject memberEvent(const QString& event, qint64 groupId = 0)
{
    QJsonObject payload;
    payload.insert(QStringLiteral("event"), event);
    if (groupId > 0)
    {
        payload.insert(QStringLiteral("groupid"), groupId);
    }
    return payload;
}
} // namespace

TEST(GroupManagementEventServiceTest, InviteShowsStatusRequestsGroupListAndSelectsWhenNoCurrentGroup)
{
    const auto effect = memochat::group::GroupManagementEventService::reduceGroupInvite(
        42,
        QStringLiteral("g123456789"), QStringLiteral("Ops"), 7, contextWithSelf());

    EXPECT_TRUE(effect.handled);
    EXPECT_TRUE(effect.hasStatus);
    EXPECT_EQ(effect.statusText, QStringLiteral("收到群邀请：Ops"));
    EXPECT_FALSE(effect.statusIsError);
    EXPECT_TRUE(effect.requestGroupList);
    EXPECT_TRUE(effect.selectCurrentGroup);
    EXPECT_EQ(effect.selectedGroupId, 42);
    EXPECT_EQ(effect.selectedGroupName, QStringLiteral("Ops"));
    EXPECT_EQ(effect.selectedGroupCode, QStringLiteral("g123456789"));
}

TEST(GroupManagementEventServiceTest, InviteDoesNotRequestGroupListWithoutLoggedInUserOrReplaceCurrentGroup)
{
    memochat::group::GroupManagementEventContext context;
    context.currentGroup.groupId = 9;

    const auto effect = memochat::group::GroupManagementEventService::reduceGroupInvite(
        42,
        QStringLiteral("g123456789"), QStringLiteral("Ops"), 7, context);

    EXPECT_TRUE(effect.handled);
    EXPECT_FALSE(effect.requestGroupList);
    EXPECT_FALSE(effect.selectCurrentGroup);
}

TEST(GroupManagementEventServiceTest, ApplyShowsApplicantUserIdOrUid)
{
    const auto withUserId = memochat::group::GroupManagementEventService::reduceGroupApply(
        42,
        7,
        QStringLiteral("memo_7"), QStringLiteral("please"));
    EXPECT_TRUE(withUserId.handled);
    EXPECT_TRUE(withUserId.hasStatus);
    EXPECT_EQ(withUserId.statusText, QStringLiteral("收到入群申请：memo_7 please"));
    EXPECT_FALSE(withUserId.statusIsError);

    const auto withoutUserId =
        memochat::group::GroupManagementEventService::reduceGroupApply(42, 7, QString(), QStringLiteral("please"));
    EXPECT_EQ(withoutUserId.statusText, QStringLiteral("收到入群申请：7 please"));
}

TEST(GroupManagementEventServiceTest, DissolveRemovesGroupRefreshesModelsAndClearsConversation)
{
    QJsonObject payload = memberEvent(QStringLiteral("group_dissolved"), 42);

    const auto effect =
        memochat::group::GroupManagementEventService::reduceGroupMemberChanged(payload, contextWithSelf());

    EXPECT_TRUE(effect.handled);
    EXPECT_TRUE(effect.hasStatus);
    EXPECT_EQ(effect.statusText, QStringLiteral("群事件：group_dissolved"));
    EXPECT_TRUE(effect.requestGroupList);
    EXPECT_TRUE(effect.refreshGroupModel);
    EXPECT_TRUE(effect.refreshDialogModel);
    ASSERT_EQ(effect.removeGroupIds.size(), 1U);
    EXPECT_EQ(effect.removeGroupIds.front(), 42);
    ASSERT_EQ(effect.clearGroupConversationIds.size(), 1U);
    EXPECT_EQ(effect.clearGroupConversationIds.front(), 42);
    EXPECT_TRUE(effect.clearCurrentChatIfGroupId);
    EXPECT_EQ(effect.currentChatGroupIdToClear, 42);
}

TEST(GroupManagementEventServiceTest, IconUpdatedProjectsCurrentGroupIconAndRefreshesGroups)
{
    auto context = contextWithSelf();
    context.currentGroup.groupId = 42;
    QJsonObject payload = memberEvent(QStringLiteral("group_icon_updated"), 42);
    payload.insert(QStringLiteral("icon"), QStringLiteral(" qrc:/res/new_group.png "));

    const auto effect = memochat::group::GroupManagementEventService::reduceGroupMemberChanged(payload, context);

    EXPECT_TRUE(effect.handled);
    EXPECT_TRUE(effect.hasStatus);
    EXPECT_EQ(effect.statusText, QStringLiteral("群事件：group_icon_updated"));
    EXPECT_TRUE(effect.requestGroupList);
    EXPECT_TRUE(effect.updateCurrentChatPeerIcon);
    EXPECT_EQ(effect.currentChatPeerIcon, QStringLiteral(" qrc:/res/new_group.png "));
}

TEST(GroupManagementEventServiceTest, IconUpdatedUsesDefaultIconForEmptyCurrentGroupIcon)
{
    auto context = contextWithSelf();
    context.currentGroup.groupId = 42;
    QJsonObject payload = memberEvent(QStringLiteral("group_icon_updated"), 42);
    payload.insert(QStringLiteral("icon"), QStringLiteral("   "));

    const auto effect = memochat::group::GroupManagementEventService::reduceGroupMemberChanged(payload, context);

    EXPECT_TRUE(effect.updateCurrentChatPeerIcon);
    EXPECT_EQ(effect.currentChatPeerIcon, memochat::group::GroupManagementEventService::defaultGroupIcon());
}

TEST(GroupManagementEventServiceTest, IconUpdatedForOtherGroupOnlyRefreshesGroupList)
{
    auto context = contextWithSelf();
    context.currentGroup.groupId = 42;
    const QJsonObject payload = memberEvent(QStringLiteral("group_icon_updated"), 77);

    const auto effect = memochat::group::GroupManagementEventService::reduceGroupMemberChanged(payload, context);

    EXPECT_TRUE(effect.handled);
    EXPECT_TRUE(effect.requestGroupList);
    EXPECT_FALSE(effect.updateCurrentChatPeerIcon);
}

TEST(GroupManagementEventServiceTest, GenericMemberAdminStatusEventsOnlyRequestGroupRefresh)
{
    const auto memberEffect = memochat::group::GroupManagementEventService::reduceGroupMemberChanged(
        memberEvent(QStringLiteral("group_member_kicked"), 42), contextWithSelf());
    EXPECT_TRUE(memberEffect.handled);
    EXPECT_TRUE(memberEffect.hasStatus);
    EXPECT_EQ(memberEffect.statusText, QStringLiteral("群事件：group_member_kicked"));
    EXPECT_TRUE(memberEffect.requestGroupList);

    const auto adminEffect = memochat::group::GroupManagementEventService::reduceGroupMemberChanged(
        memberEvent(QStringLiteral("group_admin_updated"), 42), contextWithSelf());
    EXPECT_TRUE(adminEffect.handled);
    EXPECT_TRUE(adminEffect.hasStatus);
    EXPECT_EQ(adminEffect.statusText, QStringLiteral("群事件：group_admin_updated"));
    EXPECT_TRUE(adminEffect.requestGroupList);
}

TEST(GroupManagementEventServiceTest, ChatConversationEventsAreNotHandledByGroupManagement)
{
    for (const QString& event :
         {QStringLiteral("group_read_ack"), QStringLiteral("group_msg_edited"), QStringLiteral("group_msg_revoked")})
    {
        const auto effect =
            memochat::group::GroupManagementEventService::reduceGroupMemberChanged(memberEvent(event, 42),
                                                                                   contextWithSelf());
        EXPECT_FALSE(effect.handled);
        EXPECT_TRUE(effect.chatConversationEvent);
        EXPECT_FALSE(effect.hasStatus);
        EXPECT_FALSE(effect.requestGroupList);
    }
}

TEST(GroupManagementEventServiceTest, GroupListUpdatedReopensRefreshedCurrentGroup)
{
    auto context = contextWithSelf();
    context.currentGroup.groupId = 42;
    context.refreshedCurrentGroup.exists = true;
    context.refreshedCurrentGroup.groupId = 42;
    context.refreshedCurrentGroup.name = QStringLiteral("Ops");
    context.refreshedCurrentGroup.code = QStringLiteral("g123456789");
    context.refreshedCurrentGroup.icon = QString();

    const auto effect = memochat::group::GroupManagementEventService::reduceGroupListUpdated(context);

    EXPECT_TRUE(effect.handled);
    EXPECT_TRUE(effect.refreshGroupModel);
    EXPECT_TRUE(effect.refreshDialogModel);
    EXPECT_TRUE(effect.requestDialogList);
    EXPECT_TRUE(effect.selectCurrentGroup);
    EXPECT_EQ(effect.selectedGroupId, 42);
    EXPECT_TRUE(effect.updateCurrentChatPeerIcon);
    EXPECT_EQ(effect.currentChatPeerIcon, memochat::group::GroupManagementEventService::defaultGroupIcon());
    EXPECT_TRUE(effect.openGroupConversation);
    EXPECT_EQ(effect.openGroupId, 42);
    EXPECT_FALSE(effect.openGroupResetHistory);
    EXPECT_TRUE(effect.syncCurrentDialogDraft);
}

TEST(GroupManagementEventServiceTest, GroupListUpdatedClearsMissingCurrentGroup)
{
    auto context = contextWithSelf();
    context.currentGroup.groupId = 42;

    const auto effect = memochat::group::GroupManagementEventService::reduceGroupListUpdated(context);

    EXPECT_TRUE(effect.handled);
    EXPECT_TRUE(effect.clearCurrentGroup);
    EXPECT_TRUE(effect.clearMessageModel);
    EXPECT_TRUE(effect.resetCurrentChatProjection);
    EXPECT_TRUE(effect.updateCurrentChatPeerIcon);
    EXPECT_EQ(effect.currentChatPeerIcon, memochat::group::GroupManagementEventService::defaultPrivatePeerIcon());
}
