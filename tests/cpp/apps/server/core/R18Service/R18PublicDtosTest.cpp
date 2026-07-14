#include <gtest/gtest.h>

#include "json/GlazeCompat.hpp"
#include "r18/R18PublicDtos.hpp"
#include "reflection/StdReflectionIntrospection.hpp"

#include <array>
#include <string>
#include <string_view>

#if MEMOCHAT_ENABLE_CPP26_REFLECTION
static_assert(memochat::reflection::FieldNamesEqual<memochat::r18::R18SourceToggleRequestDto>(
    std::array<std::string_view, 1>{"source_id"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::r18::R18SearchRequestDto>(
    std::array<std::string_view, 5>{"source_id", "keyword", "page", "sort", "tag"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::r18::R18ComicDetailRequestDto>(
    std::array<std::string_view, 2>{"source_id", "comic_id"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::r18::R18ChapterPagesRequestDto>(
    std::array<std::string_view, 2>{"source_id", "chapter_id"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::r18::R18FavoriteToggleRequestDto>(
    std::array<std::string_view, 3>{"source_id", "comic_id", "favorited"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::r18::R18HistoryUpdateRequestDto>(
    std::array<std::string_view, 4>{"source_id", "comic_id", "chapter_id", "page_index"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::r18::R18SourceToggleResponseDto>(
    std::array<std::string_view, 2>{"source_id", "enabled"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::r18::R18FavoriteToggleResponseDto>(
    std::array<std::string_view, 3>{"source_id", "comic_id", "favorited"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::r18::R18HistoryUpdateResponseDto>(
    std::array<std::string_view, 4>{"source_id", "comic_id", "chapter_id", "page_index"}));
#endif

namespace
{

memochat::json::JsonValue Parse(std::string_view body)
{
    memochat::json::JsonValue root;
    EXPECT_TRUE(memochat::json::glaze_parse(root, body));
    return root;
}

} // namespace

TEST(R18PublicDtosTest, DecodesSourceToggleRequest)
{
    memochat::r18::R18SourceToggleRequestDto request;
    ASSERT_TRUE(memochat::r18::DecodeR18SourceToggleRequest(R"({"source_id":"builtin"})", &request));

    EXPECT_EQ(request.source_id, "builtin");
}

TEST(R18PublicDtosTest, DecodesSearchRequestAndDefaults)
{
    const auto full = memochat::r18::R18SearchRequestFromJsonValue(
        Parse(R"({"source_id":"builtin","keyword":"abc","page":3,"sort":"mv_t","tag":"同人"})"));
    EXPECT_EQ(full.source_id, "builtin");
    EXPECT_EQ(full.keyword, "abc");
    EXPECT_EQ(full.page, 3);
    EXPECT_EQ(full.sort, "mv_t");
    EXPECT_EQ(full.tag, "同人");

    const auto defaults = memochat::r18::R18SearchRequestFromJsonValue(Parse(R"({})"));
    EXPECT_EQ(defaults.source_id, "");
    EXPECT_EQ(defaults.keyword, "");
    EXPECT_EQ(defaults.page, 1);
    EXPECT_EQ(defaults.sort, "");
    EXPECT_EQ(defaults.tag, "");
}

TEST(R18PublicDtosTest, DecodesDetailAndPagesRequests)
{
    const auto detail = memochat::r18::R18ComicDetailRequestFromJsonValue(Parse(R"({"source_id":"s","comic_id":"c"})"));
    EXPECT_EQ(detail.source_id, "s");
    EXPECT_EQ(detail.comic_id, "c");

    const auto pages =
        memochat::r18::R18ChapterPagesRequestFromJsonValue(Parse(R"({"source_id":"s","chapter_id":"ch"})"));
    EXPECT_EQ(pages.source_id, "s");
    EXPECT_EQ(pages.chapter_id, "ch");
}

TEST(R18PublicDtosTest, DecodesFavoriteAndHistoryRequestsWithDefaults)
{
    const auto favorite = memochat::r18::R18FavoriteToggleRequestFromJsonValue(
        Parse(R"({"source_id":"s","comic_id":"c","favorited":false})"));
    EXPECT_EQ(favorite.source_id, "s");
    EXPECT_EQ(favorite.comic_id, "c");
    EXPECT_FALSE(favorite.favorited);

    const auto favorite_defaults = memochat::r18::R18FavoriteToggleRequestFromJsonValue(Parse(R"({})"));
    EXPECT_EQ(favorite_defaults.source_id, "");
    EXPECT_EQ(favorite_defaults.comic_id, "");
    EXPECT_TRUE(favorite_defaults.favorited);

    const auto history = memochat::r18::R18HistoryUpdateRequestFromJsonValue(
        Parse(R"({"source_id":"s","comic_id":"c","chapter_id":"ch","page_index":9})"));
    EXPECT_EQ(history.source_id, "s");
    EXPECT_EQ(history.comic_id, "c");
    EXPECT_EQ(history.chapter_id, "ch");
    EXPECT_EQ(history.page_index, 9);

    const auto history_defaults = memochat::r18::R18HistoryUpdateRequestFromJsonValue(Parse(R"({})"));
    EXPECT_EQ(history_defaults.page_index, 0);
}

TEST(R18PublicDtosTest, PreservesRepresentativeWrongTypeDefaults)
{
    const auto search =
        memochat::r18::R18SearchRequestFromJsonValue(Parse(R"({"source_id":false,"keyword":{},"page":"bad"})"));
    EXPECT_EQ(search.source_id, "");
    EXPECT_EQ(search.keyword, "");
    EXPECT_EQ(search.page, 1);

    const auto favorite = memochat::r18::R18FavoriteToggleRequestFromJsonValue(Parse(R"({"favorited":"bad"})"));
    EXPECT_TRUE(favorite.favorited);
}

TEST(R18PublicDtosTest, RejectsMalformedJsonAndNullOutputs)
{
    memochat::r18::R18SearchRequestDto request;
    std::string error;

    EXPECT_FALSE(memochat::r18::DecodeR18SearchRequest("not-json", &request, &error));
    EXPECT_EQ(error, "invalid json");

    error.clear();
    EXPECT_FALSE(memochat::r18::DecodeR18SearchRequest(R"({})",
                                                       static_cast<memochat::r18::R18SearchRequestDto*>(nullptr),
                                                       &error));
    EXPECT_EQ(error, "output pointer is null");
}

TEST(R18PublicDtosTest, WritesSourceToggleResponseWithExistingWireFields)
{
    memochat::r18::R18SourceToggleResponseDto response;
    response.source_id = "builtin";
    response.enabled = true;

    const memochat::json::JsonValue root = memochat::r18::R18SourceToggleResponseToJsonValue(response);
    ASSERT_TRUE(root.isObject()) << root.toStyledString();
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(root, "source_id", ""), "builtin");
    EXPECT_TRUE(memochat::json::glaze_safe_get<bool>(root, "enabled", false));

    response.enabled = false;
    const memochat::json::JsonValue disabled = memochat::r18::R18SourceToggleResponseToJsonValue(response);
    ASSERT_TRUE(disabled.isObject()) << disabled.toStyledString();
    EXPECT_FALSE(memochat::json::glaze_safe_get<bool>(disabled, "enabled", true));
}

TEST(R18PublicDtosTest, WritesFavoriteAndHistoryResponsesWithExistingWireFields)
{
    memochat::r18::R18FavoriteToggleResponseDto favorite;
    favorite.source_id = "builtin";
    favorite.comic_id = "comic-1";
    favorite.favorited = false;

    const memochat::json::JsonValue favorite_json = memochat::r18::R18FavoriteToggleResponseToJsonValue(favorite);
    ASSERT_TRUE(favorite_json.isObject()) << favorite_json.toStyledString();
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(favorite_json, "source_id", ""), "builtin");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(favorite_json, "comic_id", ""), "comic-1");
    EXPECT_FALSE(memochat::json::glaze_safe_get<bool>(favorite_json, "favorited", true));

    memochat::r18::R18HistoryUpdateResponseDto history;
    history.source_id = "builtin";
    history.comic_id = "comic-1";
    history.chapter_id = "chapter-1";
    history.page_index = 9;

    const memochat::json::JsonValue history_json = memochat::r18::R18HistoryUpdateResponseToJsonValue(history);
    ASSERT_TRUE(history_json.isObject()) << history_json.toStyledString();
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(history_json, "source_id", ""), "builtin");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(history_json, "comic_id", ""), "comic-1");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(history_json, "chapter_id", ""), "chapter-1");
    EXPECT_EQ(memochat::json::glaze_safe_get<int64_t>(history_json, "page_index", 0), 9);
}
