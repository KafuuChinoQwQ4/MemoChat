#include <gtest/gtest.h>

#include "ConfigMgr.h"
#include "RelationQueryGrpcClient.h"
#include "RelationQueryServiceConfig.h"
#include "RelationQueryServiceFactory.h"
#include "const.h"
#include "json/GlazeCompat.h"

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <string>

namespace
{
const char* EnvValue(const char* name)
{
    const char* value = std::getenv(name);
    return value == nullptr ? "" : value;
}

int SmokeUid()
{
    const std::string raw = EnvValue("MEMOCHAT_RELATION_QUERY_SMOKE_UID");
    if (raw.empty())
    {
        return 1;
    }
    return std::max(1, std::atoi(raw.c_str()));
}

class FakeInProcessRelationQueryService final : public IRelationQueryService
{
public:
    void AppendRelationBootstrapJson(int, memochat::json::JsonValue& out) override
    {
        append_called = true;
        out["inprocess_called"] = true;
    }

    void BuildDialogListJson(int, memochat::json::JsonValue& out) override
    {
        dialog_called = true;
        out["inprocess_called"] = true;
    }

    bool append_called = false;
    bool dialog_called = false;
};

void ExpectNoRemoteError(const memochat::json::JsonValue& payload)
{
    EXPECT_FALSE(payload.isMember("relation_query_remote_error"));
    EXPECT_FALSE(payload.isMember("relation_query_remote_status_code"));
    EXPECT_FALSE(payload.isMember("relation_query_remote_error_code"));
}
} // namespace

TEST(RelationQueryRemoteSmokeTest, ConfiguredGrpcBackendTalksToRelationQueryServiceProcess)
{
    const std::string config_path = EnvValue("MEMOCHAT_RELATION_QUERY_SMOKE_CONFIG");
    if (config_path.empty())
    {
        GTEST_SKIP() << "MEMOCHAT_RELATION_QUERY_SMOKE_CONFIG is not set";
    }

    ConfigMgr::InitConfigPath(config_path);
    RelationQueryServiceConfig config;
    EXPECT_EQ(config.RelationQueryServiceBackend(), "grpc");

    const auto endpoint = config.RelationQueryServiceEndpoint();
    ASSERT_FALSE(endpoint.empty());
    const std::string expected_endpoint = EnvValue("MEMOCHAT_RELATION_QUERY_SMOKE_ENDPOINT");
    if (!expected_endpoint.empty())
    {
        EXPECT_EQ(endpoint, expected_endpoint);
    }

    FakeInProcessRelationQueryService inprocess;
    std::unique_ptr<IRelationQueryService> remote;
    auto* selected = SelectRelationQueryService(config, &inprocess, remote);

    ASSERT_NE(selected, nullptr);
    ASSERT_NE(selected, &inprocess);
    ASSERT_NE(remote, nullptr);
    ASSERT_EQ(selected, remote.get());

    const int uid = SmokeUid();
    memochat::json::JsonValue bootstrap(memochat::json::object_t{});
    bootstrap["error"] = ErrorCodes::Success;
    selected->AppendRelationBootstrapJson(uid, bootstrap);

    ExpectNoRemoteError(bootstrap);
    EXPECT_EQ(bootstrap["error"].asInt(), ErrorCodes::Success);
    EXPECT_EQ(bootstrap["uid"].asInt(), uid);
    EXPECT_TRUE(bootstrap.isMember("apply_list"));
    EXPECT_TRUE(bootstrap.isMember("friend_list"));
    EXPECT_FALSE(inprocess.append_called);

    memochat::json::JsonValue dialogs(memochat::json::object_t{});
    dialogs["error"] = ErrorCodes::Success;
    selected->BuildDialogListJson(uid, dialogs);

    ExpectNoRemoteError(dialogs);
    EXPECT_EQ(dialogs["error"].asInt(), ErrorCodes::Success);
    EXPECT_EQ(dialogs["uid"].asInt(), uid);
    EXPECT_FALSE(inprocess.dialog_called);
}
