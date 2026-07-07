#include "CallRequestPayloads.h"

#include <gtest/gtest.h>

#include <QJsonObject>
#include <QString>

TEST(CallRequestPayloadsTest, BuildsStartCallPayloadShape)
{
    const QJsonObject payload = memochat::call_request_payload::buildStartCallPayload(2002, QStringLiteral("video"));

    EXPECT_EQ(payload.size(), 2);
    EXPECT_FALSE(payload.contains(QStringLiteral("uid")));
    EXPECT_FALSE(payload.contains(QStringLiteral("token")));
    EXPECT_EQ(payload.value(QStringLiteral("peer_uid")).toInt(), 2002);
    EXPECT_EQ(payload.value(QStringLiteral("call_type")).toString(), QStringLiteral("video"));
}

TEST(CallRequestPayloadsTest, BuildsCallIdPayloadShape)
{
    const QJsonObject payload = memochat::call_request_payload::buildCallIdPayload(QStringLiteral("call-9"));

    EXPECT_EQ(payload.size(), 1);
    EXPECT_FALSE(payload.contains(QStringLiteral("uid")));
    EXPECT_FALSE(payload.contains(QStringLiteral("token")));
    EXPECT_EQ(payload.value(QStringLiteral("call_id")).toString(), QStringLiteral("call-9"));
}

TEST(CallRequestPayloadsTest, BuildsFetchTokenPayloadShape)
{
    const QJsonObject payload =
        memochat::call_request_payload::buildFetchTokenPayload(QStringLiteral("call-10"), QStringLiteral("caller"));

    EXPECT_EQ(payload.size(), 2);
    EXPECT_FALSE(payload.contains(QStringLiteral("uid")));
    EXPECT_FALSE(payload.contains(QStringLiteral("token")));
    EXPECT_EQ(payload.value(QStringLiteral("call_id")).toString(), QStringLiteral("call-10"));
    EXPECT_EQ(payload.value(QStringLiteral("role")).toString(), QStringLiteral("caller"));
}
