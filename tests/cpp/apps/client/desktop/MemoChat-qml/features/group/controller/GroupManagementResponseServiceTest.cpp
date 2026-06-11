#include "GroupManagementResponseService.h"

#include <gtest/gtest.h>

#include <QJsonObject>
#include <QString>

namespace
{
QJsonObject groupPayload(qint64 groupId, const QString& name = QString())
{
    QJsonObject payload;
    payload.insert(QStringLiteral("groupid"), groupId);
    if (!name.isNull())
    {
        payload.insert(QStringLiteral("name"), name);
    }
    return payload;
}
} // namespace

TEST(GroupManagementResponseServiceTest, CreateSuccessWithGroupIdNameRefreshesAndSelectsDialog)
{
    memochat::group::GroupManagementResponseContext context;
    context.dialogUidForCreatedGroup = 90042;

    const auto effect = memochat::group::GroupManagementResponseService::reduceSuccess(
        ID_CREATE_GROUP_RSP,
        groupPayload(42, QStringLiteral("Ops")), context);

    EXPECT_TRUE(effect.handled);
    EXPECT_TRUE(effect.hasStatus);
    EXPECT_EQ(effect.statusText, QStringLiteral("群聊创建成功：Ops"));
    EXPECT_FALSE(effect.statusIsError);
    EXPECT_TRUE(effect.refreshGroupList);
    EXPECT_TRUE(effect.refreshGroupModel);
    EXPECT_TRUE(effect.refreshDialogModel);
    EXPECT_TRUE(effect.selectDialogUid);
    EXPECT_EQ(effect.dialogUid, 90042);
    EXPECT_TRUE(effect.emitGroupCreatedId);
    EXPECT_EQ(effect.createdGroupId, 42);
}

TEST(GroupManagementResponseServiceTest, CreateSuccessWithoutGroupIdStillRefreshesButDoesNotEmit)
{
    const auto effect = memochat::group::GroupManagementResponseService::reduceSuccess(ID_CREATE_GROUP_RSP,
                                                                                       groupPayload(0, QString()),
                                                                                       {});

    EXPECT_TRUE(effect.handled);
    EXPECT_EQ(effect.statusText, QStringLiteral("群聊创建成功"));
    EXPECT_TRUE(effect.refreshGroupList);
    EXPECT_TRUE(effect.refreshGroupModel);
    EXPECT_TRUE(effect.refreshDialogModel);
    EXPECT_FALSE(effect.selectDialogUid);
    EXPECT_FALSE(effect.emitGroupCreatedId);
}

TEST(GroupManagementResponseServiceTest, UpdateIconForCurrentGroupUpdatesProjectedPeerIcon)
{
    memochat::group::GroupManagementResponseContext context;
    context.currentGroupId = 42;
    QJsonObject payload = groupPayload(42);
    payload.insert(QStringLiteral("icon"), QStringLiteral(" qrc:/res/group.png "));

    const auto effect =
        memochat::group::GroupManagementResponseService::reduceSuccess(ID_UPDATE_GROUP_ICON_RSP, payload, context);

    EXPECT_TRUE(effect.handled);
    EXPECT_EQ(effect.statusText, QStringLiteral("群头像已更新"));
    EXPECT_TRUE(effect.refreshGroupList);
    EXPECT_TRUE(effect.updateCurrentChatPeerIcon);
    EXPECT_EQ(effect.currentChatPeerIcon, QStringLiteral(" qrc:/res/group.png "));
}

TEST(GroupManagementResponseServiceTest, UpdateIconForCurrentGroupUsesDefaultWhenEmpty)
{
    memochat::group::GroupManagementResponseContext context;
    context.currentGroupId = 42;
    QJsonObject payload = groupPayload(42);
    payload.insert(QStringLiteral("icon"), QStringLiteral("   "));

    const auto effect =
        memochat::group::GroupManagementResponseService::reduceSuccess(ID_UPDATE_GROUP_ICON_RSP, payload, context);

    EXPECT_TRUE(effect.updateCurrentChatPeerIcon);
    EXPECT_EQ(effect.currentChatPeerIcon, memochat::group::GroupManagementResponseService::defaultGroupIcon());
}

TEST(GroupManagementResponseServiceTest, UpdateIconForOtherGroupOnlyRefreshes)
{
    memochat::group::GroupManagementResponseContext context;
    context.currentGroupId = 42;

    const auto effect = memochat::group::GroupManagementResponseService::reduceSuccess(ID_UPDATE_GROUP_ICON_RSP,
                                                                                       groupPayload(77),
                                                                                       context);

    EXPECT_TRUE(effect.handled);
    EXPECT_TRUE(effect.refreshGroupList);
    EXPECT_FALSE(effect.updateCurrentChatPeerIcon);
}

TEST(GroupManagementResponseServiceTest, QuitAndDissolveCleanupEffectsRemoveGroupAndConversation)
{
    const auto quitEffect =
        memochat::group::GroupManagementResponseService::reduceSuccess(ID_QUIT_GROUP_RSP, groupPayload(42), {});

    EXPECT_TRUE(quitEffect.handled);
    EXPECT_EQ(quitEffect.statusText, QStringLiteral("已退出当前群聊"));
    EXPECT_TRUE(quitEffect.refreshGroupList);
    ASSERT_EQ(quitEffect.removeGroupIds.size(), 1U);
    EXPECT_EQ(quitEffect.removeGroupIds.front(), 42);
    ASSERT_EQ(quitEffect.clearGroupConversationIds.size(), 1U);
    EXPECT_EQ(quitEffect.clearGroupConversationIds.front(), 42);

    const auto dissolveEffect =
        memochat::group::GroupManagementResponseService::reduceSuccess(ID_DISSOLVE_GROUP_RSP, groupPayload(77), {});

    EXPECT_TRUE(dissolveEffect.handled);
    EXPECT_EQ(dissolveEffect.statusText, QStringLiteral("群聊已解散"));
    EXPECT_TRUE(dissolveEffect.refreshGroupList);
    ASSERT_EQ(dissolveEffect.removeGroupIds.size(), 1U);
    EXPECT_EQ(dissolveEffect.removeGroupIds.front(), 77);
    ASSERT_EQ(dissolveEffect.clearGroupConversationIds.size(), 1U);
    EXPECT_EQ(dissolveEffect.clearGroupConversationIds.front(), 77);
}

TEST(GroupManagementResponseServiceTest, GenericSuccessStatusAndRefreshDecisions)
{
    const auto invite =
        memochat::group::GroupManagementResponseService::reduceSuccess(ID_INVITE_GROUP_MEMBER_RSP, {}, {});
    EXPECT_EQ(invite.statusText, QStringLiteral("邀请成员成功"));
    EXPECT_TRUE(invite.refreshGroupList);

    const auto apply = memochat::group::GroupManagementResponseService::reduceSuccess(ID_APPLY_JOIN_GROUP_RSP, {}, {});
    EXPECT_EQ(apply.statusText, QStringLiteral("入群申请已提交"));
    EXPECT_FALSE(apply.refreshGroupList);

    const auto review =
        memochat::group::GroupManagementResponseService::reduceSuccess(ID_REVIEW_GROUP_APPLY_RSP, {}, {});
    EXPECT_EQ(review.statusText, QStringLiteral("审核操作已完成"));
    EXPECT_TRUE(review.refreshGroupList);

    const auto announcement =
        memochat::group::GroupManagementResponseService::reduceSuccess(ID_UPDATE_GROUP_ANNOUNCEMENT_RSP, {}, {});
    EXPECT_EQ(announcement.statusText, QStringLiteral("群公告已更新"));
    EXPECT_TRUE(announcement.refreshGroupList);

    const auto mute = memochat::group::GroupManagementResponseService::reduceSuccess(ID_MUTE_GROUP_MEMBER_RSP, {}, {});
    EXPECT_EQ(mute.statusText, QStringLiteral("禁言设置已生效"));
    EXPECT_FALSE(mute.refreshGroupList);

    const auto admin = memochat::group::GroupManagementResponseService::reduceSuccess(ID_SET_GROUP_ADMIN_RSP, {}, {});
    EXPECT_EQ(admin.statusText, QStringLiteral("管理员设置已更新"));
    EXPECT_TRUE(admin.refreshGroupList);

    const auto kick = memochat::group::GroupManagementResponseService::reduceSuccess(ID_KICK_GROUP_MEMBER_RSP, {}, {});
    EXPECT_EQ(kick.statusText, QStringLiteral("成员已移出群聊"));
    EXPECT_TRUE(kick.refreshGroupList);
}

TEST(GroupManagementResponseServiceTest, ErrorUsesServerMessageOrActionTextFallback)
{
    QJsonObject serverPayload;
    serverPayload.insert(QStringLiteral("message"), QStringLiteral("权限不足"));

    const auto withServerMessage =
        memochat::group::GroupManagementResponseService::reduceError(ID_INVITE_GROUP_MEMBER_RSP, 9, serverPayload);

    EXPECT_TRUE(withServerMessage.handled);
    EXPECT_TRUE(withServerMessage.hasStatus);
    EXPECT_TRUE(withServerMessage.statusIsError);
    EXPECT_EQ(withServerMessage.statusText, QStringLiteral("邀请成员失败：权限不足（错误码:9）"));

    const auto fallback = memochat::group::GroupManagementResponseService::reduceError(ID_DISSOLVE_GROUP_RSP, 12, {});

    EXPECT_TRUE(fallback.handled);
    EXPECT_EQ(fallback.statusText, QStringLiteral("解散群聊失败（错误码:12）"));
}

TEST(GroupManagementResponseServiceTest, NonManagementResponsesAreIgnored)
{
    const auto success = memochat::group::GroupManagementResponseService::reduceSuccess(ID_GROUP_CHAT_MSG_RSP, {}, {});
    EXPECT_FALSE(success.handled);
    EXPECT_FALSE(success.hasStatus);

    const auto error = memochat::group::GroupManagementResponseService::reduceError(ID_GROUP_HISTORY_RSP, 3, {});
    EXPECT_FALSE(error.handled);
    EXPECT_FALSE(error.hasStatus);
}
