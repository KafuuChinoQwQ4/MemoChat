#include "CallRequestPayloads.h"

#include <gtest/gtest.h>

#include <QJsonObject>
#include <QString>

TEST(CallRequestPayloadsTest, BuildsStartCallPayloadShape)
{
    const QJsonObject payload =
        memochat::call_request_payload::buildStartCallPayload(1001,
                                                              QStringLiteral("token-a"), 2002, QStringLiteral("video"));

    EXPECT_EQ(payload.size(), 4);
    EXPECT_EQ(payload.value(QStringLiteral("uid")).toInt(), 1001);
    EXPECT_EQ(payload.value(QStringLiteral("token")).toString(), QStringLiteral("token-a"));
    EXPECT_EQ(payload.value(QStringLiteral("peer_uid")).toInt(), 2002);
    EXPECT_EQ(payload.value(QStringLiteral("call_type")).toString(), QStringLiteral("video"));
}

TEST(CallRequestPayloadsTest, BuildsCallIdPayloadShape)
{
    const QJsonObject payload =
        memochat::call_request_payload::buildCallIdPayload(1001, QStringLiteral("token-b"), QStringLiteral("call-9"));

    EXPECT_EQ(payload.size(), 3);
    EXPECT_EQ(payload.value(QStringLiteral("uid")).toInt(), 1001);
    EXPECT_EQ(payload.value(QStringLiteral("token")).toString(), QStringLiteral("token-b"));
    EXPECT_EQ(payload.value(QStringLiteral("call_id")).toString(), QStringLiteral("call-9"));
}

TEST(CallRequestPayloadsTest, BuildsFetchTokenPayloadShape)
{
    const QJsonObject payload = memochat::call_request_payload::buildFetchTokenPayload(
        1001,
        QStringLiteral("token-c"), QStringLiteral("call-10"), QStringLiteral("caller"));

    EXPECT_EQ(payload.size(), 4);
    EXPECT_EQ(payload.value(QStringLiteral("uid")).toInt(), 1001);
    EXPECT_EQ(payload.value(QStringLiteral("token")).toString(), QStringLiteral("token-c"));
    EXPECT_EQ(payload.value(QStringLiteral("call_id")).toString(), QStringLiteral("call-10"));
    EXPECT_EQ(payload.value(QStringLiteral("role")).toString(), QStringLiteral("caller"));
}
