#include <gtest/gtest.h>

#include <array>
#include <string_view>

namespace memochat::tests::media::route_registration
{
const char* GetMethod();
const char* PostMethod();
const char* UploadInitPath();
const char* UploadChunkPath();
const char* UploadStatusPath();
const char* UploadCompletePath();
const char* UploadSimplePath();
const char* MediaDownloadPath();
} // namespace memochat::tests::media::route_registration

TEST(MediaRouteRegistrationAlgorithmsTest, ExposesOnlyStableHttpMethods)
{
    EXPECT_STREQ(memochat::tests::media::route_registration::GetMethod(), "GET");
    EXPECT_STREQ(memochat::tests::media::route_registration::PostMethod(), "POST");
}

TEST(MediaRouteRegistrationAlgorithmsTest, ExposesMediaGatewayRouteRegistrationPaths)
{
    using memochat::tests::media::route_registration::MediaDownloadPath;
    using memochat::tests::media::route_registration::UploadChunkPath;
    using memochat::tests::media::route_registration::UploadCompletePath;
    using memochat::tests::media::route_registration::UploadInitPath;
    using memochat::tests::media::route_registration::UploadSimplePath;
    using memochat::tests::media::route_registration::UploadStatusPath;

    constexpr std::array<std::string_view, 6> expected = {
        "/upload_media_init",
        "/upload_media_chunk",
        "/upload_media_status",
        "/upload_media_complete",
        "/upload_media",
        "/media/download",
    };
    const std::array<std::string_view, 6> actual = {
        UploadInitPath(),
        UploadChunkPath(),
        UploadStatusPath(),
        UploadCompletePath(),
        UploadSimplePath(),
        MediaDownloadPath(),
    };

    EXPECT_EQ(actual, expected);
}
