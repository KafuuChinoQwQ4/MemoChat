#include "ChatDialogListResponseService.h"
#include "userdata.h"

#include <gtest/gtest.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QStringList>

namespace
{
std::shared_ptr<FriendInfo> makeDialog(int uid)
{
    return std::make_shared<FriendInfo>(
        uid,
        QStringLiteral("name"), QStringLiteral("nick"), QStringLiteral("icon"), 0, QString(), QString());
}

QJsonObject privateDialog(int peerUid, const QString& title, int unreadCount, qint64 lastMsgTs)
{
    QJsonObject dialog;
    dialog.insert(QStringLiteral("dialog_type"), QStringLiteral("private"));
    dialog.insert(QStringLiteral("peer_uid"), peerUid);
    dialog.insert(QStringLiteral("title"), title);
    dialog.insert(QStringLiteral("avatar"), QStringLiteral("qrc:/res/head_1.png"));
    dialog.insert(QStringLiteral("last_msg_preview"), QStringLiteral("private preview"));
    dialog.insert(QStringLiteral("unread_count"), unreadCount);
    dialog.insert(QStringLiteral("pinned_rank"), 3);
    dialog.insert(QStringLiteral("draft_text"), QStringLiteral("draft"));
    dialog.insert(QStringLiteral("last_msg_ts"), lastMsgTs);
    dialog.insert(QStringLiteral("mute_state"), 1);
    return dialog;
}

QJsonObject groupDialog(qint64 groupId, const QString& title, int unreadCount, qint64 lastMsgTs)
{
    QJsonObject dialog;
    dialog.insert(QStringLiteral("dialog_type"), QStringLiteral("group"));
    dialog.insert(QStringLiteral("group_id"), groupId);
    dialog.insert(QStringLiteral("title"), title);
    dialog.insert(QStringLiteral("avatar"), QString());
    dialog.insert(QStringLiteral("last_msg_preview"), QStringLiteral("group preview"));
    dialog.insert(QStringLiteral("unread_count"), unreadCount);
    dialog.insert(QStringLiteral("pinned_rank"), 2);
    dialog.insert(QStringLiteral("draft_text"), QStringLiteral("group draft"));
    dialog.insert(QStringLiteral("last_msg_ts"), lastMsgTs);
    dialog.insert(QStringLiteral("mute_state"), 0);
    return dialog;
}

int countDialogsWithUid(const std::vector<std::shared_ptr<FriendInfo>>& dialogs, int uid)
{
    int count = 0;
    for (const auto& dialog : dialogs)
    {
        if (dialog && dialog->_uid == uid)
        {
            ++count;
        }
    }
    return count;
}
} // namespace

TEST(ChatDialogListResponseServiceTest, AppliesDialogListEffectsThroughPortInStableOrder)
{
    ChatDialogListResponseEffect effect;
    effect.dialogsReady = true;
    effect.chatListInitialized = true;
    effect.resetServerDialogMeta = true;
    effect.serverMetaSeeds.push_back(ChatDialogServerMetaSeed{7, 2, 1});
    effect.upsertGroups.push_back(std::make_shared<GroupInfoData>());
    effect.upsertChatDialogs.push_back(makeDialog(7));
    effect.upsertGroupDialogs.push_back(makeDialog(-9));
    effect.refreshDialogModel = true;
    effect.replaceDialogList.push_back(makeDialog(7));
    effect.clearUnreadDialogUids.push_back(7);
    effect.clearMentionDialogUids.push_back(-9);
    effect.syncCurrentDialogDraft = true;
    effect.selectDialogUid = 7;
    effect.bootstrapPrivateHistoryUids.push_back(11);
    effect.currentPrivateHistoryUid = 12;
    effect.bootstrapGroupHistoryIds.push_back(99);
    effect.flushIncomingMessages = true;

    QStringList calls;
    ChatDialogListResponsePort featurePort;
    ChatDialogListAppPort appPort;
    appPort.setDialogBootstrapLoading = [&calls](bool loading)
    {
        calls.push_back(QStringLiteral("bootstrap:%1").arg(loading));
    };
    appPort.setDialogsReady = [&calls](bool ready)
    {
        calls.push_back(QStringLiteral("dialogs:%1").arg(ready));
    };
    appPort.setChatListInitialized = [&calls](bool initialized)
    {
        calls.push_back(QStringLiteral("chatList:%1").arg(initialized));
    };
    featurePort.resetServerDialogMeta = [&calls]()
    {
        calls.push_back(QStringLiteral("resetMeta"));
    };
    featurePort.seedServerDialogMeta = [&calls](int dialogUid, int pinnedRank, int muteState)
    {
        calls.push_back(QStringLiteral("seed:%1:%2:%3").arg(dialogUid).arg(pinnedRank).arg(muteState));
    };
    appPort.upsertGroup = [&calls](std::shared_ptr<GroupInfoData>)
    {
        calls.push_back(QStringLiteral("upsertGroup"));
    };
    featurePort.upsertChatDialog = [&calls](std::shared_ptr<FriendInfo>)
    {
        calls.push_back(QStringLiteral("upsertChat"));
    };
    appPort.upsertGroupDialog = [&calls](std::shared_ptr<FriendInfo>)
    {
        calls.push_back(QStringLiteral("upsertGroupDialog"));
    };
    appPort.refreshDialogModel = [&calls]()
    {
        calls.push_back(QStringLiteral("refreshDialogModel"));
    };
    featurePort.replaceDialogList = [&calls](const std::vector<std::shared_ptr<FriendInfo>>& dialogs)
    {
        calls.push_back(QStringLiteral("replace:%1").arg(static_cast<int>(dialogs.size())));
    };
    featurePort.clearUnreadDialog = [&calls](int dialogUid)
    {
        calls.push_back(QStringLiteral("clearUnread:%1").arg(dialogUid));
    };
    featurePort.clearMentionDialog = [&calls](int dialogUid)
    {
        calls.push_back(QStringLiteral("clearMention:%1").arg(dialogUid));
    };
    featurePort.removeMentionForDialog = [&calls](int dialogUid)
    {
        calls.push_back(QStringLiteral("removeMention:%1").arg(dialogUid));
    };
    appPort.syncCurrentDialogDraft = [&calls]()
    {
        calls.push_back(QStringLiteral("syncDraft"));
    };
    appPort.selectDialogByUid = [&calls](int dialogUid)
    {
        calls.push_back(QStringLiteral("select:%1").arg(dialogUid));
    };
    appPort.selectFirstChat = [&calls]()
    {
        calls.push_back(QStringLiteral("selectFirst"));
    };
    appPort.requestPrivateHistoryForBootstrap = [&calls](int peerUid)
    {
        calls.push_back(QStringLiteral("privateBootstrap:%1").arg(peerUid));
    };
    appPort.requestCurrentPrivateHistory = [&calls](int peerUid)
    {
        calls.push_back(QStringLiteral("privateCurrent:%1").arg(peerUid));
    };
    appPort.requestGroupHistoryForBootstrap = [&calls](qint64 groupId)
    {
        calls.push_back(QStringLiteral("groupBootstrap:%1").arg(groupId));
    };
    appPort.flushIncomingMessages = [&calls]()
    {
        calls.push_back(QStringLiteral("flush"));
    };

    ChatDialogListResponseService::apply(effect, featurePort, appPort);

    const QStringList expected{
        QStringLiteral("bootstrap:0"),
            QStringLiteral("dialogs:1"),
                QStringLiteral("chatList:1"),
                    QStringLiteral("resetMeta"),
                        QStringLiteral("seed:7:2:1"),
                            QStringLiteral("upsertGroup"),
                                QStringLiteral("upsertChat"),
                                    QStringLiteral("upsertGroupDialog"),
                                        QStringLiteral("refreshDialogModel"),
                                            QStringLiteral("replace:1"),
                                                QStringLiteral("clearUnread:7"),
                                                    QStringLiteral("clearMention:-9"),
                                                        QStringLiteral("removeMention:-9"),
                                                            QStringLiteral("syncDraft"),
                                                                QStringLiteral("select:7"),
                                                                    QStringLiteral("privateBootstrap:11"),
                                                                        QStringLiteral("privateCurrent:12"),
                                                                            QStringLiteral("groupBootstrap:99"),
                                                                                           QStringLiteral("flush"),
                                                                            };
    EXPECT_EQ(expected, calls);
}

TEST(ChatDialogListResponseServiceTest, SelectFirstChatIsFallbackWhenNoExplicitDialogSelected)
{
    ChatDialogListResponseEffect effect;
    effect.selectFirstChat = true;
    effect.selectDialogUid = 0;

    QStringList calls;
    ChatDialogListResponsePort featurePort;
    ChatDialogListAppPort appPort;
    appPort.setDialogBootstrapLoading = [&calls](bool)
    {
        calls.push_back(QStringLiteral("bootstrap"));
    };
    appPort.selectDialogByUid = [&calls](int)
    {
        calls.push_back(QStringLiteral("select"));
    };
    appPort.selectFirstChat = [&calls]()
    {
        calls.push_back(QStringLiteral("selectFirst"));
    };

    ChatDialogListResponseService::apply(effect, featurePort, appPort);

    EXPECT_EQ(QStringList({QStringLiteral("bootstrap"), QStringLiteral("selectFirst")}), calls);
}

TEST(ChatDialogListResponseServiceTest, ReducesDialogPayloadWithoutDuplicatingUpsertEffects)
{
    QJsonArray dialogs;
    dialogs.push_back(privateDialog(7, QStringLiteral("Server Friend"), 2, 1700000000));
    dialogs.push_back(groupDialog(88, QStringLiteral("Memo Group"), 4, 1700000001000LL));

    QJsonObject payload;
    payload.insert(QStringLiteral("error"), 0);
    payload.insert(QStringLiteral("dialogs"), dialogs);

    ChatDialogListResponseContext context;
    context.bootstrappingDialog = true;
    context.hasCurrentDialog = false;

    QMap<int, qint64> groupDialogUidMap;
    const ChatDialogListResponseEffect effect =
        ChatDialogListResponseService::reduce(payload, std::move(context), groupDialogUidMap);

    EXPECT_TRUE(effect.dialogsReady);
    EXPECT_TRUE(effect.chatListInitialized);
    EXPECT_TRUE(effect.resetServerDialogMeta);
    EXPECT_TRUE(effect.syncCurrentDialogDraft);
    EXPECT_TRUE(effect.flushIncomingMessages);
    EXPECT_EQ(effect.replaceDialogList.size(), 2U);
    EXPECT_EQ(effect.upsertGroups.size(), 1U);
    EXPECT_EQ(effect.upsertGroups.front()->_group_id, 88);
    EXPECT_EQ(effect.upsertChatDialogs.size(), 1U);
    EXPECT_EQ(effect.upsertGroupDialogs.size(), 1U);
    EXPECT_EQ(countDialogsWithUid(effect.upsertChatDialogs, 7), 1);
    EXPECT_EQ(countDialogsWithUid(effect.upsertGroupDialogs, -88), 1);
    EXPECT_EQ(groupDialogUidMap.value(-88), 88);
    EXPECT_EQ(effect.selectDialogUid, 7);
    EXPECT_TRUE(effect.clearUnreadDialogUids.contains(7));
    EXPECT_TRUE(effect.clearMentionDialogUids.contains(7));
    EXPECT_TRUE(effect.bootstrapGroupHistoryIds.contains(88));
    EXPECT_EQ(effect.currentPrivateHistoryUid, 0);
    EXPECT_TRUE(effect.bootstrapPrivateHistoryUids.contains(7));
}
