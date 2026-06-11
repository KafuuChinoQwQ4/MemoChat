#include "GroupManagementEffectApplier.h"

#include <gtest/gtest.h>

#include <QString>
#include <vector>

namespace
{
struct RecordedCall
{
    QString name;
    qint64 groupId = 0;
    int uid = 0;
    QString text;
    QString extra;
    bool flag = false;
};

bool operator==(const RecordedCall& lhs, const RecordedCall& rhs)
{
    return lhs.name == rhs.name && lhs.groupId == rhs.groupId && lhs.uid == rhs.uid && lhs.text == rhs.text &&
           lhs.extra == rhs.extra && lhs.flag == rhs.flag;
}

std::ostream& operator<<(std::ostream& os, const RecordedCall& call)
{
    os << call.name.toStdString() << "{groupId=" << call.groupId << ", uid=" << call.uid
       << ", text=" << call.text.toStdString() << ", extra=" << call.extra.toStdString() << ", flag=" << call.flag
       << "}";
    return os;
}
} // namespace

TEST(GroupManagementEffectApplierTest, EventEffectDispatchesGroupOwnedActionsInOrder)
{
    std::vector<RecordedCall> calls;
    memochat::group::GroupManagementEffect effect;
    effect.handled = true;
    effect.hasStatus = true;
    effect.statusText = QStringLiteral("ok");
    effect.statusIsError = true;
    effect.removeGroupIds = {0, 42};
    effect.clearGroupConversationIds = {77};
    effect.refreshGroupModel = true;
    effect.refreshDialogModel = true;
    effect.requestDialogList = true;
    effect.selectCurrentGroup = true;
    effect.selectedGroupId = 88;
    effect.selectedGroupName = QStringLiteral("Ops");
    effect.selectedGroupCode = QStringLiteral("g123456789");
    effect.updateCurrentChatPeerIcon = true;
    effect.currentChatPeerIcon = QStringLiteral("qrc:/res/group.png");
    effect.openGroupConversation = true;
    effect.openGroupId = 99;
    effect.openGroupResetHistory = true;
    effect.syncCurrentDialogDraft = true;
    effect.clearCurrentGroup = true;
    effect.loadCurrentChatMessages = true;
    effect.clearMessageModel = true;
    effect.resetCurrentChatProjection = true;
    effect.requestGroupList = true;

    memochat::group::GroupManagementEventEffectPort port;
    port.setStatus = [&calls](const QString& text, bool isError)
    {
        calls.push_back({"setStatus", 0, 0, text, {}, isError});
    };
    port.removeGroup = [&calls](qint64 groupId)
    {
        calls.push_back({"removeGroup", groupId});
    };
    port.clearGroupConversation = [&calls](qint64 groupId)
    {
        calls.push_back({"clearGroupConversation", groupId});
    };
    port.refreshGroupModel = [&calls]()
    {
        calls.push_back({"refreshGroupModel"});
    };
    port.refreshDialogModel = [&calls]()
    {
        calls.push_back({"refreshDialogModel"});
    };
    port.requestDialogList = [&calls]()
    {
        calls.push_back({"requestDialogList"});
    };
    port.selectCurrentGroup = [&calls](qint64 groupId, const QString& name, const QString& code)
    {
        calls.push_back({"selectCurrentGroup", groupId, 0, name, code});
    };
    port.setCurrentChatPeerIcon = [&calls](const QString& icon)
    {
        calls.push_back({"setCurrentChatPeerIcon", 0, 0, icon});
    };
    port.openGroupConversation = [&calls](qint64 groupId, bool resetHistory)
    {
        calls.push_back({"openGroupConversation", groupId, 0, {}, {}, resetHistory});
    };
    port.syncCurrentDialogDraft = [&calls]()
    {
        calls.push_back({"syncCurrentDialogDraft"});
    };
    port.clearCurrentGroup = [&calls]()
    {
        calls.push_back({"clearCurrentGroup"});
    };
    port.loadCurrentChatMessages = [&calls]()
    {
        calls.push_back({"loadCurrentChatMessages"});
    };
    port.clearMessageModel = [&calls]()
    {
        calls.push_back({"clearMessageModel"});
    };
    port.resetCurrentChatProjection = [&calls]()
    {
        calls.push_back({"resetCurrentChatProjection"});
    };
    port.refreshGroupList = [&calls]()
    {
        calls.push_back({"refreshGroupList"});
    };

    memochat::group::GroupManagementEffectApplier::apply(effect, port);

    const std::vector<RecordedCall> expected = {
        {"setStatus", 0, 0, QStringLiteral("ok"), {}, true},
         {"removeGroup", 42},
         {"clearGroupConversation", 77},
         {"refreshGroupModel"},
         {"refreshDialogModel"},
         {"requestDialogList"},
         {"selectCurrentGroup",
          88,
          0,
          QStringLiteral("Ops"), QStringLiteral("g123456789")},
          {"setCurrentChatPeerIcon", 0, 0, QStringLiteral("qrc:/res/group.png")},
           {"openGroupConversation", 99, 0, {}, {}, true},
           {"syncCurrentDialogDraft"},
           {"clearCurrentGroup"},
           {"loadCurrentChatMessages"},
           {"clearMessageModel"},
           {"resetCurrentChatProjection"},
           {"refreshGroupList"},
         };
    EXPECT_EQ(calls, expected);
}

TEST(GroupManagementEffectApplierTest, EventEffectIgnoresUnhandledEffects)
{
    std::vector<RecordedCall> calls;
    memochat::group::GroupManagementEffect effect;
    effect.hasStatus = true;
    effect.statusText = QStringLiteral("ignored");

    memochat::group::GroupManagementEventEffectPort port;
    port.setStatus = [&calls](const QString& text, bool isError)
    {
        calls.push_back({"setStatus", 0, 0, text, {}, isError});
    };

    memochat::group::GroupManagementEffectApplier::apply(effect, port);

    EXPECT_TRUE(calls.empty());
}

TEST(GroupManagementEffectApplierTest, ResponseEffectDispatchesNarrowCallbacks)
{
    std::vector<RecordedCall> calls;
    memochat::group::GroupManagementResponseEffect effect;
    effect.handled = true;
    effect.hasStatus = true;
    effect.statusText = QStringLiteral("created");
    effect.statusIsError = false;
    effect.removeGroupIds = {11};
    effect.clearGroupConversationIds = {11};
    effect.refreshGroupModel = true;
    effect.refreshDialogModel = true;
    effect.selectDialogUid = true;
    effect.dialogUid = 90011;
    effect.emitGroupCreatedId = true;
    effect.createdGroupId = 11;
    effect.updateCurrentChatPeerIcon = true;
    effect.currentChatPeerIcon = QStringLiteral("qrc:/res/group.png");
    effect.refreshGroupList = true;

    memochat::group::GroupManagementResponseEffectPort port;
    port.setStatus = [&calls](const QString& text, bool isError)
    {
        calls.push_back({"setStatus", 0, 0, text, {}, isError});
    };
    port.removeGroup = [&calls](qint64 groupId)
    {
        calls.push_back({"removeGroup", groupId});
    };
    port.clearGroupConversation = [&calls](qint64 groupId)
    {
        calls.push_back({"clearGroupConversation", groupId});
    };
    port.refreshGroupModel = [&calls]()
    {
        calls.push_back({"refreshGroupModel"});
    };
    port.refreshDialogModel = [&calls]()
    {
        calls.push_back({"refreshDialogModel"});
    };
    port.selectDialogByUid = [&calls](int uid)
    {
        calls.push_back({"selectDialogByUid", 0, uid});
    };
    port.notifyGroupCreated = [&calls](qint64 groupId)
    {
        calls.push_back({"notifyGroupCreated", groupId});
    };
    port.setCurrentChatPeerIcon = [&calls](const QString& icon)
    {
        calls.push_back({"setCurrentChatPeerIcon", 0, 0, icon});
    };
    port.refreshGroupList = [&calls]()
    {
        calls.push_back({"refreshGroupList"});
    };

    memochat::group::GroupManagementEffectApplier::apply(effect, port);

    const std::vector<RecordedCall> expected = {
        {"setStatus", 0, 0, QStringLiteral("created")},
         {"removeGroup", 11},
         {"clearGroupConversation", 11},
         {"refreshGroupModel"},
         {"refreshDialogModel"},
         {"selectDialogByUid", 0, 90011},
         {"notifyGroupCreated", 11},
         {"setCurrentChatPeerIcon", 0, 0, QStringLiteral("qrc:/res/group.png")}, {"refreshGroupList"},
        };
    EXPECT_EQ(calls, expected);
}

TEST(GroupManagementEffectApplierTest, ResponseEffectToleratesMissingCallbacks)
{
    memochat::group::GroupManagementResponseEffect effect;
    effect.handled = true;
    effect.hasStatus = true;
    effect.refreshGroupList = true;
    effect.emitGroupCreatedId = true;
    effect.createdGroupId = 42;

    memochat::group::GroupManagementEffectApplier::apply(effect, {});
}
