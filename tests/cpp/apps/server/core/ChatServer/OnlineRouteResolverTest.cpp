// OnlineRouteResolver 纯决策函数单测
//
// 验证 DecideOnlineRoute 覆盖全部四个分支(Offline/Local/Remote/Stale)，
// 且任何分支都不调用写方法(RepairOnlineRoute/ClearTrackedOnlineRoute/SetUserServer)。
// 自愈写操作由 ApplyOnlineRouteSelfHeal 单独负责；此处只测决策面。

#include <gtest/gtest.h>

#include "domain/ports/OnlineRouteResolver.hpp"
#include "domain/ports/ISessionRegistry.hpp"
#include "domain/ports/IOnlineRouteStore.hpp"
#include "CSession.hpp"

#include <boost/asio.hpp>

using namespace memochat::chat::routing;

// ── fakes ──────────────────────────────────────────────────────────────────

struct FakeSessionRegistry final : ISessionRegistry
{
    std::shared_ptr<IChatSession> session; // nullptr → 没有本地 session

    std::shared_ptr<IChatSession> FindSession(int) override
    {
        return session;
    }
    void BindSession(int, std::shared_ptr<IChatSession>) override
    {
    }
    void UnbindSession(int, const std::string&) override
    {
    }
};

struct FakeRouteStore final : IOnlineRouteStore
{
    std::string self_name = "chatserver1";
    std::string user_server;       // 空 → Redis 里没记录
    std::string online_set_server; // ResolveServerFromOnlineSets 的回答

    int repair_calls = 0;
    int clear_calls = 0;
    int set_calls = 0;

    std::string SelfServerName() const override
    {
        return self_name;
    }
    std::string FindUserServer(int) override
    {
        return user_server;
    }
    std::string ResolveServerFromOnlineSets(int) override
    {
        return online_set_server;
    }
    void RepairOnlineRoute(int, const std::shared_ptr<IChatSession>&) override
    {
        ++repair_calls;
    }
    void ClearTrackedOnlineRoute(int, const std::string&) override
    {
        ++clear_calls;
    }
    void SetUserServer(int, const std::string&) override
    {
        ++set_calls;
    }
};

// ── helpers ─────────────────────────────────────────────────────────────────

// DecideOnlineRoute 不得调用任何写方法
static void ExpectNoWrites(const FakeRouteStore& s)
{
    EXPECT_EQ(s.repair_calls, 0) << "DecideOnlineRoute 不应调用 RepairOnlineRoute";
    EXPECT_EQ(s.clear_calls, 0) << "DecideOnlineRoute 不应调用 ClearTrackedOnlineRoute";
    EXPECT_EQ(s.set_calls, 0) << "DecideOnlineRoute 不应调用 SetUserServer";
}

// ── tests ────────────────────────────────────────────────────────────────────

TEST(DecideOnlineRoute, Offline_NoSessionNoRedis)
{
    FakeSessionRegistry reg; // session = nullptr
    FakeRouteStore store;    // user_server = "", online_set_server = ""

    auto d = DecideOnlineRoute(1, &reg, &store);

    EXPECT_EQ(d.kind, OnlineRouteKind::Offline);
    EXPECT_EQ(d.session, nullptr);
    ExpectNoWrites(store);
}

TEST(DecideOnlineRoute, Local_SessionOnThisNode)
{
    // FindSession 返回非 null 时应判定为 Local，不查 Redis。
    FakeSessionRegistry reg;
    // 构造一个最小 CSession——仅用作身份指针，测试期间不发起任何 I/O。
    boost::asio::io_context dummy_ioc;
    reg.session = std::make_shared<CSession>(dummy_ioc, /*server=*/nullptr);

    FakeRouteStore store;
    store.user_server = ""; // 即使 Redis 有值也应在 FindSession 之后短路

    auto d = DecideOnlineRoute(1, &reg, &store);

    EXPECT_EQ(d.kind, OnlineRouteKind::Local);
    EXPECT_EQ(d.session, reg.session);
    EXPECT_EQ(d.redis_server, store.self_name);
    EXPECT_TRUE(d.local_session_found);
    ExpectNoWrites(store);
}

TEST(DecideOnlineRoute, Remote_UserOnOtherNode)
{
    FakeSessionRegistry reg; // 本节点无 session
    FakeRouteStore store;
    store.user_server = "chatserver2"; // Redis 指向另一节点

    auto d = DecideOnlineRoute(1, &reg, &store);

    EXPECT_EQ(d.kind, OnlineRouteKind::Remote);
    EXPECT_EQ(d.redis_server, "chatserver2");
    ExpectNoWrites(store);
}

TEST(DecideOnlineRoute, Stale_RedisPointsHereButNoSession)
{
    FakeSessionRegistry reg; // 本节点无 session
    FakeRouteStore store;
    store.user_server = "chatserver1"; // Redis 指回本节点，但 session 不存在

    auto d = DecideOnlineRoute(1, &reg, &store);

    EXPECT_EQ(d.kind, OnlineRouteKind::Stale);
    ExpectNoWrites(store);
}
