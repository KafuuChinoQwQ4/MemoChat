#include "r18/R18PublicDtos.hpp"

#include "json/TypedJsonCodec.hpp"

import memochat.r18.public_dto_algorithms;

namespace
{

constexpr const char* kDefaultR18SourceId = "";
constexpr int kDefaultR18Page = 1;
constexpr bool kDefaultR18Favorited = true;
constexpr int64_t kDefaultR18PageIndex = 0;

bool ParseJsonForR18Public(std::string_view body, memochat::json::JsonValue* out, std::string* error_out)
{
    if (out == nullptr)
    {
        if (error_out != nullptr)
        {
            *error_out = "output pointer is null";
        }
        return false;
    }
    if (!memochat::json::glaze_parse(*out, body))
    {
        if (error_out != nullptr)
        {
            *error_out = "invalid json";
        }
        return false;
    }
    return true;
}

template <typename Dto>
bool DecodeR18PublicRequest(std::string_view body,
                            Dto* out,
                            std::string* error_out,
                            Dto (*from_json)(const memochat::json::JsonValue&))
{
    if (out == nullptr)
    {
        if (error_out != nullptr)
        {
            *error_out = "output pointer is null";
        }
        return false;
    }

    memochat::json::JsonValue root;
    if (!ParseJsonForR18Public(body, &root, error_out))
    {
        return false;
    }
    *out = from_json(root);
    return true;
}

template <typename T> bool WriteTypedJsonNoThrow(const T& value, std::string* out, std::string* error_out)
{
    return memochat::json::WriteTypedJson(value, out, error_out);
}

template <typename T> memochat::json::JsonValue TypedJsonToJsonValue(const T& value)
{
    std::string body;
    if (!WriteTypedJsonNoThrow(value, &body, nullptr))
    {
        return memochat::json::JsonValue(memochat::json::object_t{});
    }

    memochat::json::JsonValue root;
    if (!memochat::json::glaze_parse(root, body))
    {
        return memochat::json::JsonValue(memochat::json::object_t{});
    }
    return root;
}

} // namespace

namespace memochat::r18
{

R18SourceToggleRequestDto R18SourceToggleRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    R18SourceToggleRequestDto request;
    request.source_id = memochat::json::glaze_safe_get<std::string>(root, "source_id", "");
    return request;
}

R18SearchRequestDto R18SearchRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    R18SearchRequestDto request;
    const bool has_source_id = root.isMember("source_id");
    request.source_id = memochat::json::glaze_safe_get<std::string>(root, "source_id", kDefaultR18SourceId);
    if (public_dto::modules::ShouldUseDefaultSourceId(has_source_id))
    {
        request.source_id = kDefaultR18SourceId;
    }
    request.keyword = memochat::json::glaze_safe_get<std::string>(root, "keyword", "");
    const bool has_page = root.isMember("page");
    const auto page = static_cast<int>(memochat::json::glaze_safe_get<int64_t>(root, "page", kDefaultR18Page));
    request.page = public_dto::modules::SelectPageOrDefault(has_page, page, kDefaultR18Page);
    return request;
}

R18ComicDetailRequestDto R18ComicDetailRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    R18ComicDetailRequestDto request;
    const bool has_source_id = root.isMember("source_id");
    request.source_id = memochat::json::glaze_safe_get<std::string>(root, "source_id", kDefaultR18SourceId);
    if (public_dto::modules::ShouldUseDefaultSourceId(has_source_id))
    {
        request.source_id = kDefaultR18SourceId;
    }
    request.comic_id = memochat::json::glaze_safe_get<std::string>(root, "comic_id", "");
    return request;
}

R18ChapterPagesRequestDto R18ChapterPagesRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    R18ChapterPagesRequestDto request;
    const bool has_source_id = root.isMember("source_id");
    request.source_id = memochat::json::glaze_safe_get<std::string>(root, "source_id", kDefaultR18SourceId);
    if (public_dto::modules::ShouldUseDefaultSourceId(has_source_id))
    {
        request.source_id = kDefaultR18SourceId;
    }
    request.chapter_id = memochat::json::glaze_safe_get<std::string>(root, "chapter_id", "");
    return request;
}

R18FavoriteToggleRequestDto R18FavoriteToggleRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    R18FavoriteToggleRequestDto request;
    request.source_id = memochat::json::glaze_safe_get<std::string>(root, "source_id", "");
    request.comic_id = memochat::json::glaze_safe_get<std::string>(root, "comic_id", "");
    const bool has_favorited = root.isMember("favorited");
    const bool favorited = memochat::json::glaze_safe_get<bool>(root, "favorited", kDefaultR18Favorited);
    request.favorited =
        public_dto::modules::SelectFavoriteStateOrDefault(has_favorited, favorited, kDefaultR18Favorited);
    return request;
}

R18HistoryUpdateRequestDto R18HistoryUpdateRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    R18HistoryUpdateRequestDto request;
    request.source_id = memochat::json::glaze_safe_get<std::string>(root, "source_id", "");
    request.comic_id = memochat::json::glaze_safe_get<std::string>(root, "comic_id", "");
    request.chapter_id = memochat::json::glaze_safe_get<std::string>(root, "chapter_id", "");
    const bool has_page_index = root.isMember("page_index");
    const auto page_index = memochat::json::glaze_safe_get<int64_t>(root, "page_index", kDefaultR18PageIndex);
    request.page_index =
        public_dto::modules::SelectPageIndexOrDefault(has_page_index, page_index, kDefaultR18PageIndex);
    return request;
}

bool DecodeR18SourceToggleRequest(std::string_view body, R18SourceToggleRequestDto* out, std::string* error_out)
{
    return DecodeR18PublicRequest(body, out, error_out, R18SourceToggleRequestFromJsonValue);
}

bool DecodeR18SearchRequest(std::string_view body, R18SearchRequestDto* out, std::string* error_out)
{
    return DecodeR18PublicRequest(body, out, error_out, R18SearchRequestFromJsonValue);
}

bool DecodeR18ComicDetailRequest(std::string_view body, R18ComicDetailRequestDto* out, std::string* error_out)
{
    return DecodeR18PublicRequest(body, out, error_out, R18ComicDetailRequestFromJsonValue);
}

bool DecodeR18ChapterPagesRequest(std::string_view body, R18ChapterPagesRequestDto* out, std::string* error_out)
{
    return DecodeR18PublicRequest(body, out, error_out, R18ChapterPagesRequestFromJsonValue);
}

bool DecodeR18FavoriteToggleRequest(std::string_view body, R18FavoriteToggleRequestDto* out, std::string* error_out)
{
    return DecodeR18PublicRequest(body, out, error_out, R18FavoriteToggleRequestFromJsonValue);
}

bool DecodeR18HistoryUpdateRequest(std::string_view body, R18HistoryUpdateRequestDto* out, std::string* error_out)
{
    return DecodeR18PublicRequest(body, out, error_out, R18HistoryUpdateRequestFromJsonValue);
}

memochat::json::JsonValue R18SourceToggleResponseToJsonValue(const R18SourceToggleResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

memochat::json::JsonValue R18FavoriteToggleResponseToJsonValue(const R18FavoriteToggleResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

memochat::json::JsonValue R18HistoryUpdateResponseToJsonValue(const R18HistoryUpdateResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

} // namespace memochat::r18
