#include <gtest/gtest.h>

#include <array>
#include <string_view>

namespace memochat::tests::r18::route_registration
{
const char* GetMethod();
const char* PostMethod();
const char* SourcesPath();
const char* SourceImportPath();
const char* SourceEnablePath();
const char* SourceDisablePath();
const char* SourceDeletePath();
const char* SearchPath();
const char* ComicDetailPath();
const char* ChapterPagesPath();
const char* FavoriteTogglePath();
const char* HistoryUpdatePath();
const char* HistoryPath();
const char* ImagePath();
} // namespace memochat::tests::r18::route_registration

TEST(R18RouteRegistrationAlgorithmsTest, ExposesStableHttpMethods)
{
    EXPECT_STREQ(memochat::tests::r18::route_registration::GetMethod(), "GET");
    EXPECT_STREQ(memochat::tests::r18::route_registration::PostMethod(), "POST");
}

TEST(R18RouteRegistrationAlgorithmsTest, ExposesR18GatewayRouteRegistrationPaths)
{
    using memochat::tests::r18::route_registration::ChapterPagesPath;
    using memochat::tests::r18::route_registration::ComicDetailPath;
    using memochat::tests::r18::route_registration::FavoriteTogglePath;
    using memochat::tests::r18::route_registration::HistoryPath;
    using memochat::tests::r18::route_registration::HistoryUpdatePath;
    using memochat::tests::r18::route_registration::ImagePath;
    using memochat::tests::r18::route_registration::SearchPath;
    using memochat::tests::r18::route_registration::SourceDeletePath;
    using memochat::tests::r18::route_registration::SourceDisablePath;
    using memochat::tests::r18::route_registration::SourceEnablePath;
    using memochat::tests::r18::route_registration::SourceImportPath;
    using memochat::tests::r18::route_registration::SourcesPath;

    constexpr std::array<std::string_view, 12> expected = {
        "/api/r18/sources",
        "/api/r18/source/import",
        "/api/r18/source/enable",
        "/api/r18/source/disable",
        "/api/r18/source/delete",
        "/api/r18/search",
        "/api/r18/comic/detail",
        "/api/r18/chapter/pages",
        "/api/r18/favorite/toggle",
        "/api/r18/history/update",
        "/api/r18/history",
        "/api/r18/image",
    };
    const std::array<std::string_view, 12> actual = {
        SourcesPath(),
        SourceImportPath(),
        SourceEnablePath(),
        SourceDisablePath(),
        SourceDeletePath(),
        SearchPath(),
        ComicDetailPath(),
        ChapterPagesPath(),
        FavoriteTogglePath(),
        HistoryUpdatePath(),
        HistoryPath(),
        ImagePath(),
    };

    EXPECT_EQ(actual, expected);
}
