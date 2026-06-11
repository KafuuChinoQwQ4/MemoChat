#include "ApplyRequestModel.h"
#include "ContactController.h"
#include "FriendListModel.h"
#include "SearchResultModel.h"
#include "userdata.h"

#include <gtest/gtest.h>

#include <QString>
#include <memory>
#include <vector>

namespace
{
std::shared_ptr<FriendInfo> makeFriend(int uid, const QString& name)
{
    const QString nick = name + QStringLiteral("Nick");
    const QString desc = QStringLiteral("desc ") + name;
    const QString remark = QStringLiteral("remark ") + name;
    const QString lastMessage = QStringLiteral("last ") + name;
    auto friendInfo = std::make_shared<FriendInfo>(
        uid,
        name,
        nick,
        QStringLiteral("head.png"), 1, desc, remark, lastMessage, QStringLiteral("u123456789"));
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
    int nextPageCalls = 0;
    int markPageLoadedCalls = 0;
    int loadFinishedCalls = 0;

    ContactBootstrapPort port;
    port.ensureChatListInitialized = [&ensureChatListCalls]()
    {
        ++ensureChatListCalls;
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
    EXPECT_EQ(nextPageCalls, 1);
    EXPECT_EQ(markPageLoadedCalls, 1);
    EXPECT_EQ(loadFinishedCalls, 1);
    EXPECT_TRUE(controller.contactsReady());
    EXPECT_TRUE(controller.canLoadMoreContacts());
    ASSERT_EQ(controller.contactListModel()->rowCount(), 2);
    EXPECT_EQ(row(controller.contactListModel(), 0).value(QStringLiteral("uid")).toInt(), 501);
    EXPECT_EQ(row(controller.contactListModel(), 1).value(QStringLiteral("uid")).toInt(), 502);
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
