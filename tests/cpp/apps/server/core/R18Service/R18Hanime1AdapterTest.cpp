#include "r18/R18Hanime1Adapter.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace
{

constexpr int64_t kNowMs = 1'900'000'000'000LL;

std::string SourceTag(const std::string& url)
{
    return "<source src=\"" + url + "\" type=\"video/mp4\">";
}

} // namespace

TEST(R18Hanime1AdapterTest, ParsesAllowedMp4SourcesInDescendingQualityOrder)
{
    const std::string url_480 = "https://vdownload.hembed.com/407339-480p.mp4?secure=token480,2000000480";
    const std::string url_720 = "https://vdownload.hembed.com/407339-720p.mp4?secure=token720,2000000720";
    const std::string url_1080 = "https://vdownload.hembed.com/407339-1080p.mp4?secure=token1080,2000001080";
    const std::string html = "<video>" + SourceTag(url_480) + SourceTag(url_1080) + SourceTag(url_720) + "</video>";

    const auto sources = memochat::r18::ParseHanime1VideoSources(html, "407339", kNowMs);

    ASSERT_EQ(sources.size(), 3U);
    EXPECT_EQ(sources[0].quality, 1080);
    EXPECT_EQ(sources[0].url, url_1080);
    EXPECT_EQ(sources[0].mime_type, "video/mp4");
    EXPECT_EQ(sources[0].expires_at_ms, 2'000'001'080'000LL);
    EXPECT_EQ(sources[1].quality, 720);
    EXPECT_EQ(sources[1].url, url_720);
    EXPECT_EQ(sources[1].expires_at_ms, 2'000'000'720'000LL);
    EXPECT_EQ(sources[2].quality, 480);
    EXPECT_EQ(sources[2].url, url_480);
    EXPECT_EQ(sources[2].expires_at_ms, 2'000'000'480'000LL);
}

TEST(R18Hanime1AdapterTest, AcceptsSecureParameterAmongHtmlEncodedQueryParameters)
{
    const std::string url = "https://vdownload.hembed.com/407339-720p.mp4?download=1&secure=token720,2000000720";
    const std::string html =
        "<source type=\"video/mp4\" src=\"https://vdownload.hembed.com/407339-720p.mp4?download=1&amp;"
        "secure=token720,2000000720\">";

    const auto sources = memochat::r18::ParseHanime1VideoSources(html, "407339", kNowMs);

    ASSERT_EQ(sources.size(), 1U);
    EXPECT_EQ(sources[0].url, url);
    EXPECT_EQ(sources[0].quality, 720);
}

TEST(R18Hanime1AdapterTest, RejectsSourcesOutsideTheFrozenPlaybackAllowlist)
{
    const std::vector<std::pair<std::string, std::string>> rejected = {
        {"http scheme", "http://vdownload.hembed.com/407339-720p.mp4?secure=token,2000000000"},
        {"foreign host", "https://cdn.example.com/407339-720p.mp4?secure=token,2000000000"},
        {"host suffix", "https://vdownload.hembed.com.example.com/407339-720p.mp4?secure=token,2000000000"},
        {"userinfo", "https://user@vdownload.hembed.com/407339-720p.mp4?secure=token,2000000000"},
        {"fragment", "https://vdownload.hembed.com/407339-720p.mp4?secure=token,2000000000#fragment"},
        {"non-default port", "https://vdownload.hembed.com:444/407339-720p.mp4?secure=token,2000000000"},
        {"wrong video id", "https://vdownload.hembed.com/407340-720p.mp4?secure=token,2000000000"},
        {"unsupported quality", "https://vdownload.hembed.com/407339-360p.mp4?secure=token,2000000000"},
        {"path suffix", "https://vdownload.hembed.com/407339-720p.mp4/extra?secure=token,2000000000"},
        {"missing secure", "https://vdownload.hembed.com/407339-720p.mp4?download=1"},
        {"empty secure token", "https://vdownload.hembed.com/407339-720p.mp4?secure=,2000000000"},
        {"missing expiry", "https://vdownload.hembed.com/407339-720p.mp4?secure=token,"},
        {"malformed expiry", "https://vdownload.hembed.com/407339-720p.mp4?secure=token,not-a-number"},
        {"trailing expiry data", "https://vdownload.hembed.com/407339-720p.mp4?secure=token,2000000000x"},
        {"expired", "https://vdownload.hembed.com/407339-720p.mp4?secure=token,1900000000"},
        {"expiry overflow", "https://vdownload.hembed.com/407339-720p.mp4?secure=token,9223372036854776"},
    };

    for (const auto& [name, url] : rejected)
    {
        SCOPED_TRACE(name);
        EXPECT_TRUE(memochat::r18::ParseHanime1VideoSources(SourceTag(url), "407339", kNowMs).empty());
    }
}

TEST(R18Hanime1AdapterTest, IgnoresMalformedAndOverflowingPaginationNumbers)
{
    const std::string overflowing_page(128, '9');
    const std::string html = "<a href=\"?page=3\">3</a><a href=\"?page=not-a-number\">bad</a>" +
                             std::string("<a href=\"?page=") + overflowing_page + "\">overflow</a>" +
                             "<a href=\"?page=12\">12</a>";

    EXPECT_EQ(memochat::r18::ParseHanime1LastPage(html), 12);
}
