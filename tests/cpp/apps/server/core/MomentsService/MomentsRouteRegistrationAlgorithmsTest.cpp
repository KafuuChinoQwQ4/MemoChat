#include <gtest/gtest.h>

#include <array>
#include <string_view>

namespace memochat::tests::moments::route_registration
{
const char* PostMethod();
const char* PublishPath();
const char* ListPath();
const char* DetailPath();
const char* DeletePath();
const char* LikePath();
const char* CommentPath();
const char* CommentListPath();
const char* CommentLikePath();
} // namespace memochat::tests::moments::route_registration

TEST(MomentsRouteRegistrationAlgorithmsTest, ExposesStableHttpMethod)
{
    EXPECT_STREQ(memochat::tests::moments::route_registration::PostMethod(), "POST");
}

TEST(MomentsRouteRegistrationAlgorithmsTest, ExposesMomentsGatewayRouteRegistrationPaths)
{
    using memochat::tests::moments::route_registration::CommentLikePath;
    using memochat::tests::moments::route_registration::CommentListPath;
    using memochat::tests::moments::route_registration::CommentPath;
    using memochat::tests::moments::route_registration::DeletePath;
    using memochat::tests::moments::route_registration::DetailPath;
    using memochat::tests::moments::route_registration::LikePath;
    using memochat::tests::moments::route_registration::ListPath;
    using memochat::tests::moments::route_registration::PublishPath;

    constexpr std::array<std::string_view, 8> expected = {
        "/api/moments/publish",
        "/api/moments/list",
        "/api/moments/detail",
        "/api/moments/delete",
        "/api/moments/like",
        "/api/moments/comment",
        "/api/moments/comment/list",
        "/api/moments/comment/like",
    };
    const std::array<std::string_view, 8> actual = {
        PublishPath(),
        ListPath(),
        DetailPath(),
        DeletePath(),
        LikePath(),
        CommentPath(),
        CommentListPath(),
        CommentLikePath(),
    };

    EXPECT_EQ(actual, expected);
}
