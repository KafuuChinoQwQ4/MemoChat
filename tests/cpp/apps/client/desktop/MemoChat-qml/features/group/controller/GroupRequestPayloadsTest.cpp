#include "GroupRequestPayloads.h"

#include <gtest/gtest.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QVariantList>

namespace
{
QStringList stringsFrom(const QJsonArray& array)
{
    QStringList values;
    for (const QJsonValue& value : array)
    {
        values.push_back(value.toString());
    }
    return values;
}
} // namespace

TEST(GroupRequestPayloadTest, ValidatesGroupAndUserIdentifiers)
{
    using memochat::group_payload::isValidGroupCode;
    using memochat::group_payload::isValidUserId;

    EXPECT_TRUE(isValidGroupCode(QStringLiteral("g123456789")));
    EXPECT_TRUE(isValidGroupCode(QStringLiteral("  g987654321  ")));
    EXPECT_FALSE(isValidGroupCode(QStringLiteral("123456789")));
    EXPECT_FALSE(isValidGroupCode(QStringLiteral("g023456789")));
    EXPECT_FALSE(isValidGroupCode(QStringLiteral("g12345678")));

    EXPECT_TRUE(isValidUserId(QStringLiteral("u123456789")));
    EXPECT_FALSE(isValidUserId(QStringLiteral("u023456789")));
    EXPECT_FALSE(isValidUserId(QStringLiteral("U123456789")));
}

TEST(GroupRequestPayloadTest, BuildsMemberUserIdArrayWithTrimmedNonSelfMembers)
{
    QString invalidUserId;
    const QVariantList members{
        QStringLiteral(" u123456789 "), QStringLiteral("u111111111"), QStringLiteral(""), QStringLiteral("u222222222")};

    const QJsonArray payloadMembers =
        memochat::group_payload::buildMemberUserIdArray(members, QStringLiteral("u123456789"), &invalidUserId);

    EXPECT_TRUE(invalidUserId.isEmpty());
    EXPECT_EQ(stringsFrom(payloadMembers), QStringList({QStringLiteral("u111111111"), QStringLiteral("u222222222")}));
}

TEST(GroupRequestPayloadTest, BuildsMemberUserIdArrayStopsOnInvalidMember)
{
    QString invalidUserId;
    const QVariantList members{QStringLiteral("u111111111"), QStringLiteral("bad")};

    const QJsonArray payloadMembers =
        memochat::group_payload::buildMemberUserIdArray(members, QStringLiteral("u123456789"), &invalidUserId);

    EXPECT_TRUE(payloadMembers.isEmpty());
    EXPECT_EQ(invalidUserId, QStringLiteral("bad"));
}

TEST(GroupRequestPayloadTest, CreateGroupPayloadTrimsNameAndKeepsMemberShape)
{
    QJsonArray members;
    members.append(QStringLiteral("u111111111"));

    const QJsonObject payload =
        memochat::group_payload::buildCreateGroupPayload(1001, QStringLiteral("  Team  "), members, 64);

    EXPECT_EQ(payload.value(QStringLiteral("fromuid")).toInt(), 1001);
    EXPECT_EQ(payload.value(QStringLiteral("name")).toString(), QStringLiteral("Team"));
    EXPECT_EQ(payload.value(QStringLiteral("member_limit")).toInt(), 64);
    EXPECT_EQ(stringsFrom(payload.value(QStringLiteral("member_user_ids")).toArray()),
                          QStringList({QStringLiteral("u111111111")}));
}

TEST(GroupRequestPayloadTest, SetGroupAdminPayloadNormalizesPermissionBits)
{
    const qint64 oversizedPermissions = memochat::group_payload::kOwnerPermissionBits | (1LL << 20);
    const QJsonObject payload = memochat::group_payload::buildSetGroupAdminPayload(
        1001,
        42,
        QStringLiteral("  u111111111  "), true, oversizedPermissions);

    EXPECT_EQ(payload.value(QStringLiteral("fromuid")).toInt(), 1001);
    EXPECT_EQ(payload.value(QStringLiteral("groupid")).toVariant().toLongLong(), 42);
    EXPECT_EQ(payload.value(QStringLiteral("target_user_id")).toString(), QStringLiteral("u111111111"));
    EXPECT_TRUE(payload.value(QStringLiteral("is_admin")).toBool());
    EXPECT_EQ(payload.value(QStringLiteral("permission_bits")).toVariant().toLongLong(),
                            memochat::group_payload::kOwnerPermissionBits);
    EXPECT_TRUE(payload.value(QStringLiteral("can_manage_admins")).toBool());

    const QJsonObject demotePayload =
        memochat::group_payload::buildSetGroupAdminPayload(1001, 42, QStringLiteral("u111111111"), false, 123);
    EXPECT_EQ(demotePayload.value(QStringLiteral("permission_bits")).toVariant().toLongLong(), 0);
    EXPECT_FALSE(demotePayload.value(QStringLiteral("can_invite_users")).toBool());
}

TEST(GroupRequestPayloadTest, TrimsMessageIdsAndLimitsMutableTextPayloads)
{
    const QString longAnnouncement(1200, QLatin1Char('a'));
    const QString longContent(5000, QLatin1Char('b'));

    const QJsonObject announcement =
        memochat::group_payload::buildUpdateGroupAnnouncementPayload(1001, 42, longAnnouncement);
    EXPECT_EQ(announcement.value(QStringLiteral("announcement")).toString().size(), 1000);

    const QJsonObject icon =
        memochat::group_payload::buildUpdateGroupIconPayload(1001, 42, QStringLiteral("https://cdn/group.png"));
    EXPECT_EQ(icon.value(QStringLiteral("fromuid")).toInt(), 1001);
    EXPECT_EQ(icon.value(QStringLiteral("groupid")).toVariant().toLongLong(), 42);
    EXPECT_EQ(icon.value(QStringLiteral("icon")).toString(), QStringLiteral("https://cdn/group.png"));

    const QJsonObject edit =
        memochat::group_payload::buildGroupEditMessagePayload(1001, 42, QStringLiteral(" msg-1 "), longContent);
    EXPECT_EQ(edit.value(QStringLiteral("msgid")).toString(), QStringLiteral("msg-1"));
    EXPECT_EQ(edit.value(QStringLiteral("content")).toString().size(), 4096);

    const QJsonObject mute =
        memochat::group_payload::buildMuteGroupMemberPayload(1001, 42, QStringLiteral(" u111111111 "), -1);
    EXPECT_EQ(mute.value(QStringLiteral("target_user_id")).toString(), QStringLiteral("u111111111"));
    EXPECT_EQ(mute.value(QStringLiteral("mute_seconds")).toInt(), 0);
}
