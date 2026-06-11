#include "GroupResponseErrorService.h"

#include <gtest/gtest.h>

namespace
{
QString actionText(int reqId)
{
    if (reqId == ID_EDIT_PRIVATE_MSG_RSP)
    {
        return QStringLiteral("编辑消息");
    }
    if (reqId == ID_FORWARD_GROUP_MSG_RSP)
    {
        return QStringLiteral("转发群消息");
    }
    return {};
}
} // namespace

TEST(GroupResponseErrorServiceTest, PrivateMutationErrorsBecomeTipDecisions)
{
    const auto decision =
        memochat::group::GroupResponseErrorService::reduce(ID_EDIT_PRIVATE_MSG_RSP, 8, {}, actionText);

    EXPECT_TRUE(decision.handled);
    EXPECT_EQ(decision.target, memochat::group::GroupResponseErrorTarget::Tip);
    EXPECT_TRUE(decision.statusIsError);
    EXPECT_EQ(decision.statusText, QStringLiteral("编辑消息失败（错误码:8）"));
}

TEST(GroupResponseErrorServiceTest, ManagementErrorsReturnManagementEffect)
{
    QJsonObject payload;
    payload.insert(QStringLiteral("message"), QStringLiteral("权限不足"));

    const auto decision =
        memochat::group::GroupResponseErrorService::reduce(ID_INVITE_GROUP_MEMBER_RSP, 9, payload, actionText);

    EXPECT_TRUE(decision.handled);
    EXPECT_EQ(decision.target, memochat::group::GroupResponseErrorTarget::ManagementEffect);
    EXPECT_TRUE(decision.managementEffect.handled);
    EXPECT_TRUE(decision.managementEffect.statusIsError);
    EXPECT_EQ(decision.managementEffect.statusText, QStringLiteral("邀请成员失败：权限不足（错误码:9）"));
}

TEST(GroupResponseErrorServiceTest, GenericErrorsBecomeGroupStatusDecisions)
{
    QJsonObject payload;
    payload.insert(QStringLiteral("message"), QStringLiteral("消息太大"));

    const auto decision =
        memochat::group::GroupResponseErrorService::reduce(ID_FORWARD_GROUP_MSG_RSP, 10, payload, actionText);

    EXPECT_TRUE(decision.handled);
    EXPECT_EQ(decision.target, memochat::group::GroupResponseErrorTarget::GroupStatus);
    EXPECT_TRUE(decision.statusIsError);
    EXPECT_EQ(decision.statusText, QStringLiteral("转发群消息失败：消息太大（错误码:10）"));
}

TEST(GroupResponseErrorServiceTest, DraftSyncErrorsAreHandledAsIgnored)
{
    const auto decision = memochat::group::GroupResponseErrorService::reduce(ID_SYNC_DRAFT_RSP, 11, {}, actionText);

    EXPECT_TRUE(decision.handled);
    EXPECT_EQ(decision.target, memochat::group::GroupResponseErrorTarget::Ignore);
    EXPECT_TRUE(decision.statusText.isEmpty());
}
