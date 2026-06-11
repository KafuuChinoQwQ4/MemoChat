#include "ContactRequestPayloads.h"

#include <gtest/gtest.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QVariantList>

namespace
{
QStringList labelsFrom(const QJsonObject& payload)
{
    QStringList labels;
    const QJsonArray array = payload.value(QStringLiteral("labels")).toArray();
    for (const QJsonValue& value : array)
    {
        labels.push_back(value.toString());
    }
    return labels;
}
} // namespace

TEST(ContactRequestPayloadTest, ValidatesUserIdFormat)
{
    using memochat::contact_payload::isValidUserId;

    EXPECT_TRUE(isValidUserId(QStringLiteral("u123456789")));
    EXPECT_TRUE(isValidUserId(QStringLiteral("  u987654321  ")));

    EXPECT_FALSE(isValidUserId(QString()));
    EXPECT_FALSE(isValidUserId(QStringLiteral("123456789")));
    EXPECT_FALSE(isValidUserId(QStringLiteral("u023456789")));
    EXPECT_FALSE(isValidUserId(QStringLiteral("u12345678")));
    EXPECT_FALSE(isValidUserId(QStringLiteral("u1234567890")));
    EXPECT_FALSE(isValidUserId(QStringLiteral("U123456789")));
}

TEST(ContactRequestPayloadTest, SearchPayloadTrimsUserId)
{
    const QJsonObject payload = memochat::contact_payload::buildSearchUserPayload(QStringLiteral("  u123456789  "));

    EXPECT_EQ(payload.size(), 1);
    EXPECT_EQ(payload.value(QStringLiteral("user_id")).toString(), QStringLiteral("u123456789"));
}

TEST(ContactRequestPayloadTest, AddFriendPayloadTrimsLabelsAndUsesSelfNameWhenRemarkIsBlank)
{
    const QVariantList labels{QStringLiteral(" classmates "),
                                             QStringLiteral(""), QStringLiteral("  "), QStringLiteral("project")};

    const QJsonObject payload =
        memochat::contact_payload::buildAddFriendPayload(1001,
                                                         QStringLiteral("Self"), 2002, QStringLiteral("  "), labels);

    EXPECT_EQ(payload.value(QStringLiteral("uid")).toInt(), 1001);
    EXPECT_EQ(payload.value(QStringLiteral("applyname")).toString(), QStringLiteral("Self"));
    EXPECT_EQ(payload.value(QStringLiteral("touid")).toInt(), 2002);
    EXPECT_EQ(payload.value(QStringLiteral("bakname")).toString(), QStringLiteral("Self"));
    EXPECT_EQ(labelsFrom(payload), QStringList({QStringLiteral("classmates"), QStringLiteral("project")}));
}

TEST(ContactRequestPayloadTest, AddFriendPayloadUsesTrimmedRemarkWhenPresent)
{
    const QJsonObject payload =
        memochat::contact_payload::buildAddFriendPayload(1001,
                                                         QStringLiteral("Self"), 2002, QStringLiteral("  Buddy  "), {});

    EXPECT_EQ(payload.value(QStringLiteral("bakname")).toString(), QStringLiteral("Buddy"));
}

TEST(ContactRequestPayloadTest, ApproveFriendPayloadShape)
{
    const QVariantList labels{QStringLiteral(" friends "), QStringLiteral("work")};

    const QJsonObject payload =
        memochat::contact_payload::buildApproveFriendPayload(1001, 2002, QStringLiteral("Remark"), labels);

    EXPECT_EQ(payload.value(QStringLiteral("fromuid")).toInt(), 1001);
    EXPECT_EQ(payload.value(QStringLiteral("touid")).toInt(), 2002);
    EXPECT_EQ(payload.value(QStringLiteral("back")).toString(), QStringLiteral("Remark"));
    EXPECT_EQ(labelsFrom(payload), QStringList({QStringLiteral("friends"), QStringLiteral("work")}));
}
