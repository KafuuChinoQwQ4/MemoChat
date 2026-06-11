#include "ChatProtocolRouter.h"

#include <gtest/gtest.h>

#include <QJsonObject>

using memochat::chat::ChatProtocolRouter;
using memochat::chat::GroupMemberEventRoute;
using memochat::chat::GroupResponseRoute;
using memochat::chat::MessageStatusRoute;

TEST(ChatProtocolRouterTest, RoutesGroupResponsesByFeatureResponsibility)
{
    EXPECT_EQ(ChatProtocolRouter::routeGroupResponse(ID_CREATE_GROUP_RSP), GroupResponseRoute::Management);
    EXPECT_EQ(ChatProtocolRouter::routeGroupResponse(ID_INVITE_GROUP_MEMBER_RSP), GroupResponseRoute::Management);
    EXPECT_EQ(ChatProtocolRouter::routeGroupResponse(ID_UPDATE_GROUP_ICON_RSP), GroupResponseRoute::Management);
    EXPECT_EQ(ChatProtocolRouter::routeGroupResponse(ID_DISSOLVE_GROUP_RSP), GroupResponseRoute::Management);
    EXPECT_EQ(ChatProtocolRouter::routeGroupResponse(ID_GROUP_HISTORY_RSP), GroupResponseRoute::History);
    EXPECT_EQ(ChatProtocolRouter::routeGroupResponse(ID_EDIT_GROUP_MSG_RSP), GroupResponseRoute::GroupMessageMutation);
    EXPECT_EQ(ChatProtocolRouter::routeGroupResponse(ID_REVOKE_GROUP_MSG_RSP),
              GroupResponseRoute::GroupMessageMutation);
    EXPECT_EQ(ChatProtocolRouter::routeGroupResponse(ID_EDIT_PRIVATE_MSG_RSP),
              GroupResponseRoute::PrivateMessageMutation);
    EXPECT_EQ(ChatProtocolRouter::routeGroupResponse(ID_REVOKE_PRIVATE_MSG_RSP),
              GroupResponseRoute::PrivateMessageMutation);
    EXPECT_EQ(ChatProtocolRouter::routeGroupResponse(ID_FORWARD_PRIVATE_MSG_RSP), GroupResponseRoute::PrivateForward);
    EXPECT_EQ(ChatProtocolRouter::routeGroupResponse(ID_GROUP_CHAT_MSG_RSP), GroupResponseRoute::GroupMessageAck);
    EXPECT_EQ(ChatProtocolRouter::routeGroupResponse(ID_FORWARD_GROUP_MSG_RSP), GroupResponseRoute::GroupMessageAck);
    EXPECT_EQ(ChatProtocolRouter::routeGroupResponse(ID_SYNC_DRAFT_RSP), GroupResponseRoute::DialogMeta);
    EXPECT_EQ(ChatProtocolRouter::routeGroupResponse(ID_PIN_DIALOG_RSP), GroupResponseRoute::DialogMeta);
    EXPECT_EQ(ChatProtocolRouter::routeGroupResponse(ID_GET_GROUP_LIST_RSP), GroupResponseRoute::Ignore);
}

TEST(ChatProtocolRouterTest, RoutesMessageStatusByPayloadScope)
{
    QJsonObject missingClientMsg;
    missingClientMsg[QStringLiteral("scope")] = QStringLiteral("group");
    EXPECT_EQ(ChatProtocolRouter::routeMessageStatus(missingClientMsg), MessageStatusRoute::Ignore);

    QJsonObject groupByScope;
    groupByScope[QStringLiteral("client_msg_id")] = QStringLiteral("m1");
    groupByScope[QStringLiteral("scope")] = QStringLiteral("group");
    EXPECT_EQ(ChatProtocolRouter::routeMessageStatus(groupByScope), MessageStatusRoute::GroupMessageStatus);

    QJsonObject groupById;
    groupById[QStringLiteral("client_msg_id")] = QStringLiteral("m2");
    groupById[QStringLiteral("groupid")] = 7001;
    EXPECT_EQ(ChatProtocolRouter::routeMessageStatus(groupById), MessageStatusRoute::GroupMessageStatus);

    QJsonObject privateStatus;
    privateStatus[QStringLiteral("client_msg_id")] = QStringLiteral("m3");
    EXPECT_EQ(ChatProtocolRouter::routeMessageStatus(privateStatus), MessageStatusRoute::PrivateMessageStatus);
}

TEST(ChatProtocolRouterTest, RoutesGroupMemberEventsByEventName)
{
    QJsonObject readAck;
    readAck[QStringLiteral("event")] = QStringLiteral("group_read_ack");
    EXPECT_EQ(ChatProtocolRouter::routeGroupMemberEvent(readAck), GroupMemberEventRoute::GroupReadAck);

    QJsonObject edited;
    edited[QStringLiteral("event")] = QStringLiteral("group_msg_edited");
    EXPECT_EQ(ChatProtocolRouter::routeGroupMemberEvent(edited), GroupMemberEventRoute::GroupMessageMutation);

    QJsonObject revoked;
    revoked[QStringLiteral("event")] = QStringLiteral("group_msg_revoked");
    EXPECT_EQ(ChatProtocolRouter::routeGroupMemberEvent(revoked), GroupMemberEventRoute::GroupMessageMutation);

    QJsonObject management;
    management[QStringLiteral("event")] = QStringLiteral("member_joined");
    EXPECT_EQ(ChatProtocolRouter::routeGroupMemberEvent(management), GroupMemberEventRoute::GroupManagement);
}
