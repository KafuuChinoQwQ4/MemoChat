#include "ApplyRequestModel.h"
#include "ClientGateway.h"
#include "ContactController.h"
#include "FriendListModel.h"
#include "SearchResultModel.h"
#include "userdata.h"
#include "usermgr.h"

#include <gtest/gtest.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <memory>
#include <vector>

namespace
{
std::shared_ptr<FriendInfo>
makeFriend(int uid, const QString& name, const QString& userId = QStringLiteral("u123456789"))
{
    const QString nick = name + QStringLiteral("Nick");
    const QString desc = QStringLiteral("desc ") + name;
    const QString remark = QStringLiteral("remark ") + name;
    const QString lastMessage = QStringLiteral("last ") + name;
    auto friendInfo =
        std::make_shared<FriendInfo>(uid, name, nick, QStringLiteral("head.png"), 1, desc, remark, lastMessage, userId);
    friendInfo->_dialog_type = QStringLiteral("private");
    return friendInfo;
}

std::shared_ptr<SearchInfo> makeSearchInfo(int uid, const QString& name)
{
    const QString nick = name + QStringLiteral("Nick");
    return std::make_shared<SearchInfo>(
        uid,
        name,
        nick,
        QStringLiteral("search desc"), 2, QStringLiteral("search.png"), QStringLiteral("u987654321"));
}

std::shared_ptr<ApplyInfo> makeApply(int uid, const QString& name, int status = 0)
{
    const QString nick = name + QStringLiteral("Nick");
    return std::make_shared<ApplyInfo>(
        uid,
        name,
        QStringLiteral("apply desc"), QStringLiteral("apply.png"), nick, 1, status, QStringLiteral("u223456789"));
}

QVariantMap row(FriendListModel* model, int index)
{
    return model->get(index);
}

QVariantMap row(ApplyRequestModel* model, int index)
{
    return model->get(index);
}

QJsonObject friendJson(int uid, const QString& name, const QString& userId)
{
    QJsonObject item;
    item.insert(QStringLiteral("uid"), uid);
    item.insert(QStringLiteral("name"), name);
    item.insert(QStringLiteral("nick"), name + QStringLiteral("Nick"));
    item.insert(QStringLiteral("icon"), QStringLiteral("head.png"));
    item.insert(QStringLiteral("sex"), 1);
    item.insert(QStringLiteral("desc"), QStringLiteral("desc ") + name);
    item.insert(QStringLiteral("back"), QStringLiteral("remark ") + name);
    item.insert(QStringLiteral("user_id"), userId);
    return item;
}

void setCurrentFriend(ContactController& controller, int uid, const QString& name, const QString& userId = QString())
{
    const QString nick = name + QStringLiteral("Nick");
    const QString icon = QStringLiteral("head.png");
    const QString remark = QStringLiteral("remark ") + name;
    controller.setCurrentContact(uid, name, nick, icon, remark, 1, userId);
}
} // namespace

TEST(ContactControllerTest, OwnsModelsAndStartsWithNonNullModels)
{
    ContactController controller(nullptr);

    ASSERT_NE(controller.contactListModel(), nullptr);
    ASSERT_NE(controller.searchResultModel(), nullptr);
    ASSERT_NE(controller.applyRequestModel(), nullptr);
    EXPECT_EQ(controller.contactListModel()->rowCount(), 0);
    EXPECT_EQ(controller.searchResultModel()->rowCount(), 0);
    EXPECT_EQ(controller.applyRequestModel()->rowCount(), 0);
    EXPECT_FALSE(controller.hasPendingApply());
}

TEST(ContactControllerTest, UpdatesContactListModelWithSetAppendUpsertAndRemove)
{
    ContactController controller(nullptr);

    controller.setContacts({makeFriend(101, QStringLiteral("Alice")), makeFriend(102, QStringLiteral("Bob"))});
    ASSERT_EQ(controller.contactListModel()->rowCount(), 2);
    EXPECT_EQ(row(controller.contactListModel(), 0).value(QStringLiteral("uid")).toInt(), 101);
    EXPECT_EQ(row(controller.contactListModel(), 1).value(QStringLiteral("name")).toString(), QStringLiteral("Bob"));

    controller.appendContacts({makeFriend(103, QStringLiteral("Carol"))});
    ASSERT_EQ(controller.contactListModel()->rowCount(), 3);
    EXPECT_EQ(row(controller.contactListModel(), 2).value(QStringLiteral("uid")).toInt(), 103);

    controller.upsertContact(makeFriend(102, QStringLiteral("Bobby")));
    ASSERT_EQ(controller.contactListModel()->rowCount(), 3);
    const int bobIndex = controller.contactListModel()->indexOfUid(102);
    ASSERT_GE(bobIndex, 0);
    EXPECT_EQ(
        row(controller.contactListModel(), bobIndex).value(QStringLiteral("name")).toString(), QStringLiteral("Bobby"));

    controller.removeContactByUid(101);
    ASSERT_EQ(controller.contactListModel()->rowCount(), 2);
    EXPECT_EQ(controller.contactListModel()->indexOfUid(101), -1);
}

TEST(ContactControllerTest, ContactListModelSortsBySectionKeyAndGroupsNonLettersUnderHash)
{
    ContactController controller(nullptr);

    std::vector<std::shared_ptr<FriendInfo>> contacts;
    contacts.push_back(makeFriend(301, QStringLiteral("_zed")));
    contacts.push_back(makeFriend(302, QStringLiteral("bob")));
    contacts.push_back(makeFriend(303, QStringLiteral("Alice")));
    contacts.push_back(makeFriend(304, QStringLiteral("3rd")));
    contacts.push_back(makeFriend(305, QStringLiteral("carol")));
    controller.setContacts(contacts);

    ASSERT_EQ(controller.contactListModel()->rowCount(), 5);
    EXPECT_EQ(row(controller.contactListModel(), 0).value(QStringLiteral("name")).toString(), QStringLiteral("Alice"));
    EXPECT_EQ(
        row(controller.contactListModel(), 0).value(QStringLiteral("sectionKey")).toString(), QStringLiteral("A"));
    EXPECT_EQ(row(controller.contactListModel(), 1).value(QStringLiteral("name")).toString(), QStringLiteral("bob"));
    EXPECT_EQ(
        row(controller.contactListModel(), 1).value(QStringLiteral("sectionKey")).toString(), QStringLiteral("B"));
    EXPECT_EQ(row(controller.contactListModel(), 2).value(QStringLiteral("name")).toString(), QStringLiteral("carol"));
    EXPECT_EQ(
        row(controller.contactListModel(), 2).value(QStringLiteral("sectionKey")).toString(), QStringLiteral("C"));
    EXPECT_EQ(row(controller.contactListModel(), 3).value(QStringLiteral("name")).toString(), QStringLiteral("3rd"));
    EXPECT_EQ(
        row(controller.contactListModel(), 3).value(QStringLiteral("sectionKey")).toString(), QStringLiteral("#"));
    EXPECT_EQ(row(controller.contactListModel(), 4).value(QStringLiteral("name")).toString(), QStringLiteral("_zed"));
    EXPECT_EQ(
        row(controller.contactListModel(), 4).value(QStringLiteral("sectionKey")).toString(), QStringLiteral("#"));

    controller.upsertContact(makeFriend(306, QStringLiteral("Aaron")));

    ASSERT_EQ(controller.contactListModel()->rowCount(), 6);
    EXPECT_EQ(row(controller.contactListModel(), 0).value(QStringLiteral("name")).toString(), QStringLiteral("Aaron"));
    EXPECT_EQ(row(controller.contactListModel(), 1).value(QStringLiteral("name")).toString(), QStringLiteral("Alice"));
    EXPECT_EQ(
        row(controller.contactListModel(), 5).value(QStringLiteral("sectionKey")).toString(), QStringLiteral("#"));
}

TEST(ContactControllerTest, SelectContactIndexUpdatesCurrentContactAndPane)
{
    ContactController controller(nullptr);
    controller.setContacts({makeFriend(201, QStringLiteral("Dana")), makeFriend(202, QStringLiteral("Eli"))});
    controller.setContactsReady(true);

    controller.selectContactIndex(1);

    EXPECT_EQ(controller.currentContactUid(), 202);
    EXPECT_TRUE(controller.hasCurrentContact());
    EXPECT_EQ(controller.currentContactName(), QStringLiteral("Eli"));
    EXPECT_EQ(controller.currentContactNick(), QStringLiteral("EliNick"));
    EXPECT_EQ(controller.currentContactBack(), QStringLiteral("remark Eli"));
    EXPECT_EQ(controller.contactPane(), 1);

    controller.selectContactIndex(10);
    EXPECT_EQ(controller.currentContactUid(), 202);
    EXPECT_EQ(controller.contactPane(), 1);
}

TEST(ContactControllerTest, RefreshesSelectedContactPublicUserIdWhenStoreUpdates)
{
    ClientGateway gateway;
    ContactController controller(&gateway);
    gateway.userMgr()->ResetSession();

    gateway.userMgr()->AppendFriendList(QJsonArray{friendJson(203, QStringLiteral("Faye"), QString())});
    auto stale = gateway.userMgr()->GetFriendById(203);
    ASSERT_NE(stale, nullptr);

    controller.setContacts({stale});
    setCurrentFriend(controller, 203, QStringLiteral("Faye"));
    ASSERT_TRUE(controller.currentContactUserId().isEmpty());

    gateway.userMgr()->AppendFriendList(
        QJsonArray{friendJson(203, QStringLiteral("Faye"), QStringLiteral("u203203203"))});
    auto fresh = gateway.userMgr()->GetFriendById(203);
    ASSERT_NE(fresh, nullptr);
    controller.upsertContact(fresh);

    EXPECT_EQ(controller.currentContactUid(), 203);
    EXPECT_EQ(controller.currentContactUserId(), QStringLiteral("u203203203"));
}

TEST(ContactControllerTest, RefreshContactProfilesSyncsListRowsFromStoreSnapshot)
{
    ClientGateway gateway;
    ContactController controller(&gateway);
    gateway.userMgr()->ResetSession();

    gateway.userMgr()->AppendFriendList(QJsonArray{friendJson(204, QStringLiteral("Gia"), QString())});
    auto stale = gateway.userMgr()->GetFriendById(204);
    ASSERT_NE(stale, nullptr);
    controller.setContacts({stale});
    setCurrentFriend(controller, 204, QStringLiteral("Gia"));
    ASSERT_EQ(controller.contactListModel()->rowCount(), 1);
    EXPECT_TRUE(row(controller.contactListModel(), 0).value(QStringLiteral("userId")).toString().isEmpty());

    gateway.userMgr()->AppendFriendList(QJsonArray{
        friendJson(204,
                   QStringLiteral("Gia"), QStringLiteral("u204204204")),
                   friendJson(205, QStringLiteral("Hidden"), QStringLiteral("u205205205")),
                   });

    controller.refreshContactProfiles();

    ASSERT_EQ(controller.contactListModel()->rowCount(), 1);
    EXPECT_EQ(
        row(controller.contactListModel(), 0).value(QStringLiteral("userId")).toString(), QStringLiteral("u204204204"));
    EXPECT_EQ(controller.currentContactUid(), 204);
    EXPECT_EQ(controller.currentContactUserId(), QStringLiteral("u204204204"));
    EXPECT_EQ(controller.contactListModel()->indexOfUid(205), -1);
}

TEST(ContactControllerTest, ContactHttpProfileLookupCachesPublicUserIdForNonFriendProfile)
{
    ContactController controller(nullptr);

    controller.handleContactHttpFinished(
        ReqId::ID_GET_USER_INFO,
        QStringLiteral(
            R"({"error":0,"uid":206,"user_id":"u206206206","name":"Hera","nick":"HeraNick","icon":"","desc":"public desc","sex":2})"),
            ErrorCodes::SUCCESS);

    const QVariantMap profile = controller.contactProfileByUid(206);
    EXPECT_EQ(profile.value(QStringLiteral("uid")).toInt(), 206);
    EXPECT_FALSE(profile.value(QStringLiteral("isFriend")).toBool());
    EXPECT_EQ(profile.value(QStringLiteral("userId")).toString(), QStringLiteral("u206206206"));
    EXPECT_EQ(profile.value(QStringLiteral("name")).toString(), QStringLiteral("Hera"));
    EXPECT_EQ(profile.value(QStringLiteral("nick")).toString(), QStringLiteral("HeraNick"));
    EXPECT_EQ(profile.value(QStringLiteral("desc")).toString(), QStringLiteral("public desc"));
    EXPECT_EQ(profile.value(QStringLiteral("sex")).toInt(), 2);
}

TEST(ContactControllerTest, ContactHttpProfileLookupBackfillsFriendListAndCurrentContact)
{
    ClientGateway gateway;
    ContactController controller(&gateway);
    gateway.userMgr()->ResetSession();

    gateway.userMgr()->AppendFriendList(QJsonArray{friendJson(207, QStringLiteral("Ira"), QString())});
    auto stale = gateway.userMgr()->GetFriendById(207);
    ASSERT_NE(stale, nullptr);
    controller.setContacts({stale});
    setCurrentFriend(controller, 207, QStringLiteral("Ira"));
    ASSERT_TRUE(controller.currentContactUserId().isEmpty());
    ASSERT_TRUE(row(controller.contactListModel(), 0).value(QStringLiteral("userId")).toString().isEmpty());

    controller.handleContactHttpFinished(
        ReqId::ID_GET_USER_INFO,
        QStringLiteral(
            R"({"error":0,"uid":207,"user_id":"u207207207","name":"Ira","nick":"IraNick","icon":"","desc":"fresh desc","sex":1})"),
            ErrorCodes::SUCCESS);

    const QVariantMap profile = controller.contactProfileByUid(207);
    EXPECT_TRUE(profile.value(QStringLiteral("isFriend")).toBool());
    EXPECT_EQ(profile.value(QStringLiteral("userId")).toString(), QStringLiteral("u207207207"));
    EXPECT_EQ(
        row(controller.contactListModel(), 0).value(QStringLiteral("userId")).toString(), QStringLiteral("u207207207"));
    EXPECT_EQ(controller.currentContactUid(), 207);
    EXPECT_EQ(controller.currentContactUserId(), QStringLiteral("u207207207"));
}

TEST(ContactControllerTest, UpdatesSearchModelPendingAndStatus)
{
    ContactController controller(nullptr);

    controller.setSearchPending(true);
    controller.setSearchStatus(QStringLiteral("Searching"), false);
    controller.setSearchResult(makeSearchInfo(301, QStringLiteral("Finn")));

    ASSERT_EQ(controller.searchResultModel()->rowCount(), 1);
    EXPECT_TRUE(controller.searchPending());
    EXPECT_EQ(controller.searchStatusText(), QStringLiteral("Searching"));
    EXPECT_FALSE(controller.searchStatusError());
    EXPECT_EQ(controller.searchResultModel()
                  ->data(controller.searchResultModel()->index(0, 0), SearchResultModel::UidRole)
                  .toInt(),
              301);

    controller.setSearchStatus(QStringLiteral("Missing"), true);
    EXPECT_EQ(controller.searchStatusText(), QStringLiteral("Missing"));
    EXPECT_TRUE(controller.searchStatusError());

    controller.clearSearchState();
    EXPECT_FALSE(controller.searchPending());
    EXPECT_TRUE(controller.searchStatusText().isEmpty());
    EXPECT_FALSE(controller.searchStatusError());
    EXPECT_EQ(controller.searchResultModel()->rowCount(), 0);
}

TEST(ContactControllerTest, UpdatesApplyModelPendingApprovedAndHasPendingApply)
{
    ContactController controller(nullptr);

    controller.setApplies({makeApply(401, QStringLiteral("Gina"), 0), makeApply(402, QStringLiteral("Hank"), 1)});
    ASSERT_EQ(controller.applyRequestModel()->rowCount(), 2);
    EXPECT_TRUE(controller.hasPendingApply());
    EXPECT_EQ(controller.applyRequestModel()->unapprovedCount(), 1);

    controller.upsertApply(makeApply(403, QStringLiteral("Ivy"), 0));
    ASSERT_EQ(controller.applyRequestModel()->rowCount(), 3);
    EXPECT_TRUE(controller.hasPendingApply());
    EXPECT_EQ(controller.applyRequestModel()->unapprovedCount(), 2);

    controller.setApplyPending(403, true);
    const int ivyIndex = controller.applyRequestModel()->indexOfUid(403);
    ASSERT_GE(ivyIndex, 0);
    EXPECT_TRUE(row(controller.applyRequestModel(), ivyIndex).value(QStringLiteral("pending")).toBool());

    controller.markApplyApproved(403);
    EXPECT_TRUE(row(controller.applyRequestModel(), ivyIndex).value(QStringLiteral("approved")).toBool());
    EXPECT_FALSE(row(controller.applyRequestModel(), ivyIndex).value(QStringLiteral("pending")).toBool());
    EXPECT_EQ(controller.applyRequestModel()->unapprovedCount(), 1);
    EXPECT_TRUE(controller.hasPendingApply());

    controller.markApplyApproved(401);
    EXPECT_EQ(controller.applyRequestModel()->unapprovedCount(), 0);
    EXPECT_FALSE(controller.hasPendingApply());
}

TEST(ContactControllerTest, UpdatesLoadingAndReadinessProperties)
{
    ContactController controller(nullptr);

    controller.setContactLoadingMore(true);
    controller.setCanLoadMoreContacts(true);
    controller.setContactsReady(true);
    controller.setApplyReady(true);

    EXPECT_TRUE(controller.contactLoadingMore());
    EXPECT_TRUE(controller.canLoadMoreContacts());
    EXPECT_TRUE(controller.contactsReady());
    EXPECT_TRUE(controller.applyReady());

    controller.setContactLoadingMore(false);
    controller.setCanLoadMoreContacts(false);
    controller.setContactsReady(false);
    controller.setApplyReady(false);

    EXPECT_FALSE(controller.contactLoadingMore());
    EXPECT_FALSE(controller.canLoadMoreContacts());
    EXPECT_FALSE(controller.contactsReady());
    EXPECT_FALSE(controller.applyReady());
}

TEST(ContactControllerTest, EnsureContactsInitializedOwnsInitialPageAndLoadMoreProjection)
{
    ContactController controller(nullptr);

    int ensureChatListCalls = 0;
    int friendSnapshotCalls = 0;
    int nextPageCalls = 0;
    int markPageLoadedCalls = 0;
    int loadFinishedCalls = 0;

    ContactBootstrapPort port;
    port.ensureChatListInitialized = [&ensureChatListCalls]()
    {
        ++ensureChatListCalls;
    };
    port.friendSnapshot = [&friendSnapshotCalls]()
    {
        ++friendSnapshotCalls;
        return std::vector<std::shared_ptr<FriendInfo>>{};
    };
    port.nextPage = [&nextPageCalls]()
    {
        ++nextPageCalls;
        return std::vector<std::shared_ptr<FriendInfo>>{makeFriend(501, QStringLiteral("Iris")),
                                                                   makeFriend(502, QStringLiteral("Jules"))};
    };
    port.markPageLoaded = [&markPageLoadedCalls]()
    {
        ++markPageLoadedCalls;
    };
    port.loadFinished = [&loadFinishedCalls]()
    {
        ++loadFinishedCalls;
        return false;
    };
    controller.setBootstrapPort(std::move(port));

    controller.ensureContactsInitialized();

    EXPECT_EQ(ensureChatListCalls, 1);
    EXPECT_EQ(friendSnapshotCalls, 1);
    EXPECT_EQ(nextPageCalls, 1);
    EXPECT_EQ(markPageLoadedCalls, 1);
    EXPECT_EQ(loadFinishedCalls, 1);
    EXPECT_TRUE(controller.contactsReady());
    EXPECT_TRUE(controller.canLoadMoreContacts());
    ASSERT_EQ(controller.contactListModel()->rowCount(), 2);
    EXPECT_EQ(row(controller.contactListModel(), 0).value(QStringLiteral("uid")).toInt(), 501);
    EXPECT_EQ(row(controller.contactListModel(), 1).value(QStringLiteral("uid")).toInt(), 502);
}

TEST(ContactControllerTest, EnsureContactsInitializedUsesLoginFriendSnapshotBeforeRelationBootstrap)
{
    ContactController controller(nullptr);

    int ensureChatListCalls = 0;
    int friendSnapshotCalls = 0;
    int nextPageCalls = 0;
    int markPageLoadedCalls = 0;
    int loadFinishedCalls = 0;

    ContactBootstrapPort port;
    port.ensureChatListInitialized = [&ensureChatListCalls]()
    {
        ++ensureChatListCalls;
    };
    port.friendSnapshot = [&friendSnapshotCalls]()
    {
        ++friendSnapshotCalls;
        return std::vector<std::shared_ptr<FriendInfo>>{makeFriend(521, QStringLiteral("Nina")),
                                                                   makeFriend(522, QStringLiteral("Omar"))};
    };
    port.nextPage = [&nextPageCalls]()
    {
        ++nextPageCalls;
        return std::vector<std::shared_ptr<FriendInfo>>{makeFriend(523, QStringLiteral("Paged"))};
    };
    port.markPageLoaded = [&markPageLoadedCalls]()
    {
        ++markPageLoadedCalls;
    };
    port.loadFinished = [&loadFinishedCalls]()
    {
        ++loadFinishedCalls;
        return false;
    };
    controller.setBootstrapPort(std::move(port));

    controller.ensureContactsInitialized();

    EXPECT_EQ(ensureChatListCalls, 1);
    EXPECT_EQ(friendSnapshotCalls, 1);
    EXPECT_EQ(nextPageCalls, 0);
    EXPECT_EQ(markPageLoadedCalls, 0);
    EXPECT_EQ(loadFinishedCalls, 0);
    EXPECT_TRUE(controller.contactsReady());
    EXPECT_FALSE(controller.canLoadMoreContacts());
    ASSERT_EQ(controller.contactListModel()->rowCount(), 2);
    EXPECT_EQ(row(controller.contactListModel(), 0).value(QStringLiteral("uid")).toInt(), 521);
    EXPECT_EQ(row(controller.contactListModel(), 1).value(QStringLiteral("uid")).toInt(), 522);
}

TEST(ContactControllerTest, EnsureContactsInitializedSkipsWhenAlreadyReady)
{
    ContactController controller(nullptr);
    controller.setContacts({makeFriend(601, QStringLiteral("Kai"))});
    controller.setContactsReady(true);

    int ensureChatListCalls = 0;
    int nextPageCalls = 0;
    ContactBootstrapPort port;
    port.ensureChatListInitialized = [&ensureChatListCalls]()
    {
        ++ensureChatListCalls;
    };
    port.nextPage = [&nextPageCalls]()
    {
        ++nextPageCalls;
        return std::vector<std::shared_ptr<FriendInfo>>{makeFriend(602, QStringLiteral("Lina"))};
    };
    controller.setBootstrapPort(std::move(port));

    controller.ensureContactsInitialized();

    EXPECT_EQ(ensureChatListCalls, 0);
    EXPECT_EQ(nextPageCalls, 0);
    ASSERT_EQ(controller.contactListModel()->rowCount(), 1);
    EXPECT_EQ(row(controller.contactListModel(), 0).value(QStringLiteral("uid")).toInt(), 601);
}

TEST(ContactControllerTest, EnsureContactsInitializedRepairsReadyRowsMissingPublicUserIdFromSnapshot)
{
    ContactController controller(nullptr);
    controller.setContacts({makeFriend(641, QStringLiteral("Sid"), QString())});
    controller.setContactsReady(true);

    int friendSnapshotCalls = 0;
    int nextPageCalls = 0;
    ContactBootstrapPort port;
    port.friendSnapshot = [&friendSnapshotCalls]()
    {
        ++friendSnapshotCalls;
        return std::vector<std::shared_ptr<FriendInfo>>{
            makeFriend(641, QStringLiteral("Sid"), QStringLiteral("u641641641"))};
    };
    port.nextPage = [&nextPageCalls]()
    {
        ++nextPageCalls;
        return std::vector<std::shared_ptr<FriendInfo>>{makeFriend(642, QStringLiteral("Paged"))};
    };
    controller.setBootstrapPort(std::move(port));

    controller.ensureContactsInitialized();

    EXPECT_EQ(friendSnapshotCalls, 1);
    EXPECT_EQ(nextPageCalls, 0);
    ASSERT_EQ(controller.contactListModel()->rowCount(), 1);
    EXPECT_EQ(
        row(controller.contactListModel(), 0).value(QStringLiteral("userId")).toString(), QStringLiteral("u641641641"));
    EXPECT_TRUE(controller.contactsReady());
}

TEST(ContactControllerTest, EnsureContactsInitializedRepairsEmptyReadyModelFromSnapshot)
{
    ContactController controller(nullptr);
    controller.setContactsReady(true);

    int ensureChatListCalls = 0;
    int friendSnapshotCalls = 0;
    int nextPageCalls = 0;
    ContactBootstrapPort port;
    port.ensureChatListInitialized = [&ensureChatListCalls]()
    {
        ++ensureChatListCalls;
    };
    port.friendSnapshot = [&friendSnapshotCalls]()
    {
        ++friendSnapshotCalls;
        return std::vector<std::shared_ptr<FriendInfo>>{makeFriend(621, QStringLiteral("Pia"))};
    };
    port.nextPage = [&nextPageCalls]()
    {
        ++nextPageCalls;
        return std::vector<std::shared_ptr<FriendInfo>>{makeFriend(622, QStringLiteral("Quin"))};
    };
    controller.setBootstrapPort(std::move(port));

    controller.ensureContactsInitialized();

    EXPECT_EQ(ensureChatListCalls, 0);
    EXPECT_EQ(friendSnapshotCalls, 1);
    EXPECT_EQ(nextPageCalls, 0);
    ASSERT_EQ(controller.contactListModel()->rowCount(), 1);
    EXPECT_EQ(row(controller.contactListModel(), 0).value(QStringLiteral("uid")).toInt(), 621);
    EXPECT_TRUE(controller.contactsReady());
}

TEST(ContactControllerTest, EnsureContactsInitializedKeepsReadyEmptySnapshotWithoutPaging)
{
    ContactController controller(nullptr);
    controller.setContactsReady(true);

    int ensureChatListCalls = 0;
    int friendSnapshotCalls = 0;
    int nextPageCalls = 0;
    ContactBootstrapPort port;
    port.ensureChatListInitialized = [&ensureChatListCalls]()
    {
        ++ensureChatListCalls;
    };
    port.friendSnapshot = [&friendSnapshotCalls]()
    {
        ++friendSnapshotCalls;
        return std::vector<std::shared_ptr<FriendInfo>>{};
    };
    port.nextPage = [&nextPageCalls]()
    {
        ++nextPageCalls;
        return std::vector<std::shared_ptr<FriendInfo>>{makeFriend(631, QStringLiteral("Rui"))};
    };
    controller.setBootstrapPort(std::move(port));

    controller.ensureContactsInitialized();

    EXPECT_EQ(ensureChatListCalls, 0);
    EXPECT_EQ(friendSnapshotCalls, 1);
    EXPECT_EQ(nextPageCalls, 0);
    EXPECT_TRUE(controller.contactsReady());
    EXPECT_EQ(controller.contactListModel()->rowCount(), 0);
}

TEST(ContactControllerTest, EnsureContactsInitializedDoesNotMarkReadyWhenInitialStoreIsEmpty)
{
    ContactController controller(nullptr);

    int friendSnapshotCalls = 0;
    int nextPageCalls = 0;
    int markPageLoadedCalls = 0;
    int relationBootstrapCalls = 0;
    ContactBootstrapPort port;
    port.friendSnapshot = [&friendSnapshotCalls]()
    {
        ++friendSnapshotCalls;
        return std::vector<std::shared_ptr<FriendInfo>>{};
    };
    port.nextPage = [&nextPageCalls]()
    {
        ++nextPageCalls;
        return std::vector<std::shared_ptr<FriendInfo>>{};
    };
    port.markPageLoaded = [&markPageLoadedCalls]()
    {
        ++markPageLoadedCalls;
    };
    port.loadFinished = []()
    {
        return true;
    };
    controller.setBootstrapPort(std::move(port));
    ContactCommandPort commandPort;
    commandPort.requestRelationBootstrap = [&relationBootstrapCalls]()
    {
        ++relationBootstrapCalls;
    };
    controller.setCommandPort(std::move(commandPort));

    controller.ensureContactsInitialized();

    EXPECT_EQ(friendSnapshotCalls, 1);
    EXPECT_EQ(nextPageCalls, 1);
    EXPECT_EQ(markPageLoadedCalls, 1);
    EXPECT_EQ(relationBootstrapCalls, 1);
    EXPECT_FALSE(controller.contactsReady());
    EXPECT_EQ(controller.contactListModel()->rowCount(), 0);
}

TEST(ContactControllerTest, FullSnapshotContactsDisableLoadMoreAndAvoidDuplicateAppend)
{
    ClientGateway gateway;
    ContactController controller(&gateway);
    gateway.userMgr()->ResetSession();
    QJsonArray friends;
    friends.push_back(friendJson(611, QStringLiteral("Mia"), QStringLiteral("u611611611")));
    friends.push_back(friendJson(612, QStringLiteral("Noah"), QStringLiteral("u612612612")));
    gateway.userMgr()->AppendFriendList(friends);
    const auto fullSnapshot = gateway.userMgr()->GetFriendListSnapshot();
    controller.setContacts(fullSnapshot);
    controller.setCanLoadMoreContacts(true);

    controller.loadMoreContacts();

    ASSERT_EQ(controller.contactListModel()->rowCount(), 2);
    EXPECT_EQ(controller.contactListModel()->indexOfUid(611), 0);
    EXPECT_EQ(controller.contactListModel()->indexOfUid(612), 1);
    EXPECT_FALSE(controller.canLoadMoreContacts());
}

TEST(ContactControllerTest, EnsureApplyInitializedOwnsSnapshotAndReadyProjection)
{
    ContactController controller(nullptr);

    int snapshotCalls = 0;
    ContactApplyBootstrapPort port;
    port.applySnapshot = [&snapshotCalls]()
    {
        ++snapshotCalls;
        return std::vector<std::shared_ptr<ApplyInfo>>{makeApply(701, QStringLiteral("Mona")),
                                                                 makeApply(702, QStringLiteral("Niko"), 1)};
    };
    controller.setApplyBootstrapPort(std::move(port));

    controller.ensureApplyInitialized();

    EXPECT_EQ(snapshotCalls, 1);
    EXPECT_TRUE(controller.applyReady());
    ASSERT_EQ(controller.applyRequestModel()->rowCount(), 2);
    EXPECT_TRUE(controller.hasPendingApply());
    EXPECT_EQ(row(controller.applyRequestModel(), 0).value(QStringLiteral("uid")).toInt(), 701);
    EXPECT_EQ(row(controller.applyRequestModel(), 1).value(QStringLiteral("uid")).toInt(), 702);
}

TEST(ContactControllerTest, EnsureApplyInitializedSkipsWhenAlreadyReady)
{
    ContactController controller(nullptr);
    controller.setApplies({makeApply(801, QStringLiteral("Ola"))});
    controller.setApplyReady(true);

    int snapshotCalls = 0;
    ContactApplyBootstrapPort port;
    port.applySnapshot = [&snapshotCalls]()
    {
        ++snapshotCalls;
        return std::vector<std::shared_ptr<ApplyInfo>>{makeApply(802, QStringLiteral("Pia"))};
    };
    controller.setApplyBootstrapPort(std::move(port));

    controller.ensureApplyInitialized();

    EXPECT_EQ(snapshotCalls, 0);
    ASSERT_EQ(controller.applyRequestModel()->rowCount(), 1);
    EXPECT_EQ(row(controller.applyRequestModel(), 0).value(QStringLiteral("uid")).toInt(), 801);
}

TEST(ContactControllerTest, RefreshApplySnapshotReloadsWhenAlreadyReady)
{
    ContactController controller(nullptr);
    controller.setApplies({makeApply(901, QStringLiteral("Quin"))});
    controller.setApplyReady(true);

    int snapshotCalls = 0;
    ContactApplyBootstrapPort port;
    port.applySnapshot = [&snapshotCalls]()
    {
        ++snapshotCalls;
        return std::vector<std::shared_ptr<ApplyInfo>>{makeApply(902, QStringLiteral("Ria"), 1),
                                                                 makeApply(903, QStringLiteral("Sol"))};
    };
    controller.setApplyBootstrapPort(std::move(port));

    controller.refreshApplySnapshot();

    EXPECT_EQ(snapshotCalls, 1);
    EXPECT_TRUE(controller.applyReady());
    ASSERT_EQ(controller.applyRequestModel()->rowCount(), 2);
    EXPECT_EQ(row(controller.applyRequestModel(), 0).value(QStringLiteral("uid")).toInt(), 902);
    EXPECT_EQ(row(controller.applyRequestModel(), 1).value(QStringLiteral("uid")).toInt(), 903);
    EXPECT_TRUE(controller.hasPendingApply());
}
