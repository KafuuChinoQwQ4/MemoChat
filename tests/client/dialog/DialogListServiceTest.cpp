#include "DialogListService.h"
#include "FriendListModel.h"

#include <gtest/gtest.h>

#include <QHash>
#include <QObject>
#include <QSet>

QString gate_url_prefix;
QString gate_media_url_prefix;

namespace {
std::shared_ptr<FriendInfo> makeFriend(int uid, const QString &name, const QString &lastMsg = QString())
{
    auto info = std::make_shared<FriendInfo>(uid,
                                             name,
                                             name,
                                             QStringLiteral("qrc:/res/head_1.jpg"),
                                             0,
                                             QStringLiteral("desc"),
                                             name,
                                             lastMsg,
                                             QStringLiteral("u000000001"));
    info->_dialog_type = QStringLiteral("private");
    return info;
}

std::shared_ptr<GroupInfoData> makeGroup(qint64 groupId, const QString &name, const QString &lastMsg = QString())
{
    auto group = std::make_shared<GroupInfoData>();
    group->_group_id = groupId;
    group->_name = name;
    group->_icon = QStringLiteral("qrc:/res/chat_icon.png");
    group->_announcement = QStringLiteral("announcement");
    group->_last_msg = lastMsg;
    return group;
}
}

TEST(DialogListServiceTest, AppendsLocalFriendsMissingFromDialogResponse)
{
    std::vector<std::shared_ptr<FriendInfo>> dialogs;
    dialogs.push_back(makeFriend(10, QStringLiteral("Server Friend")));

    const std::vector<std::shared_ptr<FriendInfo>> localFriends = {
        makeFriend(10, QStringLiteral("Duplicate Friend")),
        makeFriend(20, QStringLiteral("Local Friend"), QStringLiteral("cached preview"))
    };

    QHash<int, QString> drafts;
    drafts.insert(20, QStringLiteral("draft text"));
    QHash<int, int> mentions;
    mentions.insert(20, 2);
    QSet<int> localPinned;
    localPinned.insert(20);
    const DialogDecorationState decorationState {
        &drafts,
        &mentions,
        nullptr,
        nullptr,
        &localPinned
    };

    DialogListService::appendMissingPrivateDialogs(dialogs,
                                                   localFriends,
                                                   QSet<int> {10},
                                                   decorationState);

    ASSERT_EQ(dialogs.size(), 2U);
    ASSERT_NE(dialogs[1], nullptr);
    EXPECT_EQ(dialogs[1]->_uid, 20);
    EXPECT_EQ(dialogs[1]->_dialog_type, QStringLiteral("private"));
    EXPECT_EQ(dialogs[1]->_name, QStringLiteral("Local Friend"));
    EXPECT_EQ(dialogs[1]->_last_msg, QStringLiteral("cached preview"));
    EXPECT_EQ(dialogs[1]->_draft_text, QStringLiteral("draft text"));
    EXPECT_EQ(dialogs[1]->_mention_count, 2);
    EXPECT_GT(dialogs[1]->_pinned_rank, 0);
}

TEST(DialogListServiceTest, AppliesLocalFriendProfileToPrivateDialogSeed)
{
    DialogEntrySeed seed;
    seed.dialogUid = 1060;
    seed.dialogType = QStringLiteral("private");
    seed.name = QStringLiteral("1060");
    seed.nick = QStringLiteral("1060");
    seed.icon = QStringLiteral("qrc:/res/head_1.jpg");
    seed.previewText = QStringLiteral("server preview");
    seed.unreadCount = 3;
    seed.pinnedRank = 4;

    auto friendInfo = makeFriend(1060, QStringLiteral("yangksl"));
    friendInfo->_nick = QStringLiteral("yangksl");
    friendInfo->_icon = QStringLiteral("http://127.0.0.1:8080/media/download?asset=avatar");
    friendInfo->_user_id = QStringLiteral("u955561854");

    DialogListService::applyFriendProfileToPrivateSeed(seed, friendInfo);

    const DialogDecorationState decorationState {};
    auto item = DialogListService::buildDialogEntry(seed, decorationState);

    ASSERT_NE(item, nullptr);
    EXPECT_EQ(item->_uid, 1060);
    EXPECT_EQ(item->_name, QStringLiteral("yangksl"));
    EXPECT_EQ(item->_nick, QStringLiteral("yangksl"));
    EXPECT_EQ(item->_icon, QStringLiteral("http://127.0.0.1:8080/media/download?asset=avatar"));
    EXPECT_EQ(item->_user_id, QStringLiteral("u955561854"));
    EXPECT_EQ(item->_last_msg, QStringLiteral("server preview"));
    EXPECT_EQ(item->_unread_count, 3);
    EXPECT_EQ(item->_pinned_rank, 4);
}

TEST(DialogListServiceTest, AppendsLocalGroupsMissingFromDialogResponse)
{
    std::vector<std::shared_ptr<FriendInfo>> dialogs;
    dialogs.push_back(makeFriend(1060, QStringLiteral("yangksl"), QStringLiteral("private preview")));

    QMap<int, qint64> groupUidMap;
    groupUidMap.insert(-88, 88);
    const std::vector<std::shared_ptr<GroupInfoData>> localGroups = {
        makeGroup(88, QStringLiteral("Memo Group"), QStringLiteral("group preview"))
    };

    QHash<int, QString> drafts;
    drafts.insert(-88, QStringLiteral("group draft"));
    QSet<int> localPinned;
    localPinned.insert(-88);
    const DialogDecorationState decorationState {
        &drafts,
        nullptr,
        nullptr,
        nullptr,
        &localPinned
    };

    DialogListService::appendMissingGroupDialogs(dialogs,
                                                 localGroups,
                                                 groupUidMap,
                                                 QSet<qint64> {},
                                                 decorationState);

    ASSERT_EQ(dialogs.size(), 2U);
    ASSERT_NE(dialogs[1], nullptr);
    EXPECT_EQ(dialogs[1]->_uid, -88);
    EXPECT_EQ(dialogs[1]->_dialog_type, QStringLiteral("group"));
    EXPECT_EQ(dialogs[1]->_name, QStringLiteral("Memo Group"));
    EXPECT_EQ(dialogs[1]->_icon, QStringLiteral("qrc:/res/chat_icon.png"));
    EXPECT_EQ(dialogs[1]->_last_msg, QStringLiteral("group preview"));
    EXPECT_EQ(dialogs[1]->_draft_text, QStringLiteral("group draft"));
    EXPECT_GT(dialogs[1]->_pinned_rank, 0);
}

TEST(DialogListServiceTest, PreservesExistingPrivateDialogWhenPartialResponseHasOnlyGroup)
{
    DialogEntrySeed groupSeed;
    groupSeed.dialogUid = -88;
    groupSeed.dialogType = QStringLiteral("group");
    groupSeed.name = QStringLiteral("test");
    groupSeed.nick = QStringLiteral("test");
    groupSeed.icon = QStringLiteral("qrc:/res/chat_icon.png");
    groupSeed.previewText = QStringLiteral("group preview");

    const DialogDecorationState decorationState {};
    std::vector<std::shared_ptr<FriendInfo>> dialogs {
        DialogListService::buildDialogEntry(groupSeed, decorationState)
    };

    QVariantMap existingPrivate;
    existingPrivate.insert(QStringLiteral("uid"), 1060);
    existingPrivate.insert(QStringLiteral("dialogType"), QStringLiteral("private"));
    existingPrivate.insert(QStringLiteral("userId"), QStringLiteral("u955561854"));
    existingPrivate.insert(QStringLiteral("name"), QStringLiteral("yangksl"));
    existingPrivate.insert(QStringLiteral("nick"), QStringLiteral("yangksl"));
    existingPrivate.insert(QStringLiteral("icon"), QStringLiteral("http://127.0.0.1:8080/media/download?asset=avatar"));
    existingPrivate.insert(QStringLiteral("lastMsg"), QStringLiteral("111"));
    existingPrivate.insert(QStringLiteral("unreadCount"), 2);
    existingPrivate.insert(QStringLiteral("lastMsgTs"), static_cast<qint64>(1234567890000));

    DialogListService::appendExistingDialogs(dialogs,
                                             std::vector<QVariantMap> {existingPrivate},
                                             decorationState);

    ASSERT_EQ(dialogs.size(), 2U);
    ASSERT_NE(dialogs[1], nullptr);
    EXPECT_EQ(dialogs[1]->_uid, 1060);
    EXPECT_EQ(dialogs[1]->_dialog_type, QStringLiteral("private"));
    EXPECT_EQ(dialogs[1]->_name, QStringLiteral("yangksl"));
    EXPECT_EQ(dialogs[1]->_icon, QStringLiteral("http://127.0.0.1:8080/media/download?asset=avatar"));
    EXPECT_EQ(dialogs[1]->_last_msg, QStringLiteral("111"));
    EXPECT_EQ(dialogs[1]->_unread_count, 2);
    EXPECT_EQ(dialogs[1]->_last_msg_ts, static_cast<qint64>(1234567890000));
}

TEST(FriendListModelTest, UpsertBatchEmitsInsertBeforeRowsAreVisible)
{
    FriendListModel model;
    std::vector<int> rowCountsBeforeInsert;

    QObject::connect(&model,
                     &QAbstractItemModel::rowsAboutToBeInserted,
                     [&model, &rowCountsBeforeInsert](const QModelIndex &, int, int) {
                         rowCountsBeforeInsert.push_back(model.rowCount());
                     });

    const std::vector<std::shared_ptr<FriendInfo>> friends = {
        makeFriend(1060, QStringLiteral("yangksl")),
        makeFriend(1059, QStringLiteral("friend"))
    };

    model.upsertBatch(friends);

    ASSERT_EQ(rowCountsBeforeInsert.size(), 2U);
    EXPECT_EQ(rowCountsBeforeInsert[0], 0);
    EXPECT_EQ(rowCountsBeforeInsert[1], 1);
    EXPECT_EQ(model.rowCount(), 2);
    EXPECT_EQ(model.get(0).value(QStringLiteral("name")).toString(), QStringLiteral("yangksl"));
    EXPECT_EQ(model.get(1).value(QStringLiteral("name")).toString(), QStringLiteral("friend"));
}
