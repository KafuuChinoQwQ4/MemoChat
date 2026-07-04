#include <gtest/gtest.h>

#include <set>
#include <string_view>
#include <vector>

namespace memochat::tests::gate::h3::legacy
{
int GetRouteCount();
const char* GetRoutePathAt(int index);
int PostRouteCount();
const char* PostRoutePathAt(int index);
int RouteNotFoundStatus();
const char* RouteNotFoundBody();
const char* JsonContentType();
} // namespace memochat::tests::gate::h3::legacy

namespace
{
std::vector<std::string_view> CollectGetRoutes()
{
    std::vector<std::string_view> routes;
    for (int index = 0; index < memochat::tests::gate::h3::legacy::GetRouteCount(); ++index)
    {
        routes.emplace_back(memochat::tests::gate::h3::legacy::GetRoutePathAt(index));
    }
    return routes;
}

std::vector<std::string_view> CollectPostRoutes()
{
    std::vector<std::string_view> routes;
    for (int index = 0; index < memochat::tests::gate::h3::legacy::PostRouteCount(); ++index)
    {
        routes.emplace_back(memochat::tests::gate::h3::legacy::PostRoutePathAt(index));
    }
    return routes;
}
} // namespace

TEST(H3LegacyRouteAlgorithmsTest, ExposesStableGetRoutes)
{
    const std::vector<std::string_view> expected{
        "/healthz",
        "/readyz",
        "/upload_media_status",
        "/media/download",
    };

    EXPECT_EQ(CollectGetRoutes(), expected);
    EXPECT_STREQ(memochat::tests::gate::h3::legacy::GetRoutePathAt(-1), "");
    EXPECT_STREQ(memochat::tests::gate::h3::legacy::GetRoutePathAt(memochat::tests::gate::h3::legacy::GetRouteCount()),
                 "");
}

TEST(H3LegacyRouteAlgorithmsTest, ExposesStablePostRoutes)
{
    const std::vector<std::string_view> expected{
        "/get_varifycode",
        "/user_register",
        "/reset_pwd",
        "/user_login",
        "/user_update_profile",
        "/get_user_info",
        "/api/call/token",
        "/api/call/start",
        "/api/call/accept",
        "/api/call/reject",
        "/api/call/cancel",
        "/api/call/hangup",
        "/upload_media_init",
        "/upload_media_chunk",
        "/upload_media_complete",
        "/upload_media",
        "/api/moments/publish",
        "/api/moments/list",
        "/api/moments/detail",
        "/api/moments/delete",
        "/api/moments/like",
        "/api/moments/comment",
        "/api/moments/comment/list",
        "/api/moments/comment/like",
    };

    EXPECT_EQ(CollectPostRoutes(), expected);
    EXPECT_STREQ(memochat::tests::gate::h3::legacy::PostRoutePathAt(-1), "");
    EXPECT_STREQ(
        memochat::tests::gate::h3::legacy::PostRoutePathAt(memochat::tests::gate::h3::legacy::PostRouteCount()),
        "");
}

TEST(H3LegacyRouteAlgorithmsTest, KeepsRoutesUniqueAndResponseMetadataStable)
{
    const auto get_routes = CollectGetRoutes();
    const auto post_routes = CollectPostRoutes();

    std::set<std::string_view> unique_routes;
    unique_routes.insert(get_routes.begin(), get_routes.end());
    unique_routes.insert(post_routes.begin(), post_routes.end());

    EXPECT_EQ(unique_routes.size(), get_routes.size() + post_routes.size());
    EXPECT_EQ(memochat::tests::gate::h3::legacy::RouteNotFoundStatus(), 404);
    EXPECT_STREQ(memochat::tests::gate::h3::legacy::RouteNotFoundBody(),
                 R"({"error":404,"message":"route not found"})");
    EXPECT_STREQ(memochat::tests::gate::h3::legacy::JsonContentType(), "application/json");
}
