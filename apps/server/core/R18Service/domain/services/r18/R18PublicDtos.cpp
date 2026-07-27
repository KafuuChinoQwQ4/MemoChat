#include "r18/R18PublicDtos.hpp"

#include "json/TypedJsonCodec.hpp"

#include <algorithm>

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
    request.sort = memochat::json::glaze_safe_get<std::string>(root, "sort", "");
    request.tag = memochat::json::glaze_safe_get<std::string>(root, "tag", "");
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

R18VideoResolveRequestDto R18VideoResolveRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    R18VideoResolveRequestDto request;
    request.source_id = memochat::json::glaze_safe_get<std::string>(root, "source_id", "");
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
    request.title = memochat::json::glaze_safe_get<std::string>(root, "title", "");
    request.cover = memochat::json::glaze_safe_get<std::string>(root, "cover", "");
    request.author = memochat::json::glaze_safe_get<std::string>(root, "author", "");
    request.subtitle = memochat::json::glaze_safe_get<std::string>(root, "subtitle", "");
    request.folder_ids.clear();
    const auto folders = memochat::json::glaze_get(root, "folder_ids");
    if (const auto* arr = memochat::json::glaze_get_array(folders))
    {
        for (const auto& entry : *arr)
        {
            memochat::json::JsonValue v(entry);
            if (!v.isString())
                continue;
            const std::string id = v.asString();
            if (!id.empty())
                request.folder_ids.push_back(id);
        }
    }
    // Also accept single folder_id for convenience.
    const std::string single = memochat::json::glaze_safe_get<std::string>(root, "folder_id", "");
    if (!single.empty() &&
        std::find(request.folder_ids.begin(), request.folder_ids.end(), single) == request.folder_ids.end())
        request.folder_ids.push_back(single);
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

R18BrowserImportStartRequestDto R18BrowserImportStartRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    R18BrowserImportStartRequestDto request;
    request.source_id = memochat::json::glaze_safe_get<std::string>(root, "source_id", "");
    request.client_kind = memochat::json::glaze_safe_get<std::string>(root, "client_kind", "");
    return request;
}

R18BrowserImportCompleteRequestDto R18BrowserImportCompleteRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    R18BrowserImportCompleteRequestDto request;
    request.ticket = memochat::json::glaze_safe_get<std::string>(root, "ticket", "");

    // Accept either structured cookies object or flat fields
    const auto cookies = memochat::json::glaze_get(root, "cookies");
    if (cookies.isObject())
    {
        request.ipb_member_id = memochat::json::glaze_safe_get<std::string>(cookies, "ipb_member_id", "");
        request.ipb_pass_hash = memochat::json::glaze_safe_get<std::string>(cookies, "ipb_pass_hash", "");
        request.igneous = memochat::json::glaze_safe_get<std::string>(cookies, "igneous", "");
        request.sk = memochat::json::glaze_safe_get<std::string>(cookies, "sk", "");
    }
    else
    {
        request.ipb_member_id = memochat::json::glaze_safe_get<std::string>(root, "ipb_member_id", "");
        request.ipb_pass_hash = memochat::json::glaze_safe_get<std::string>(root, "ipb_pass_hash", "");
        request.igneous = memochat::json::glaze_safe_get<std::string>(root, "igneous", "");
        request.sk = memochat::json::glaze_safe_get<std::string>(root, "sk", "");
    }
    return request;
}

R18SessionImportRequestDto R18SessionImportRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    R18SessionImportRequestDto request;
    request.source_id = memochat::json::glaze_safe_get<std::string>(root, "source_id", "");

    // Accept either structured cookies object or flat fields.
    const auto cookies = memochat::json::glaze_get(root, "cookies");
    if (cookies.isObject())
    {
        request.ipb_member_id = memochat::json::glaze_safe_get<std::string>(cookies, "ipb_member_id", "");
        request.ipb_pass_hash = memochat::json::glaze_safe_get<std::string>(cookies, "ipb_pass_hash", "");
        request.igneous = memochat::json::glaze_safe_get<std::string>(cookies, "igneous", "");
        request.sk = memochat::json::glaze_safe_get<std::string>(cookies, "sk", "");
        // Generic cookie_header field for non-ehentai sources.
        request.cookie_header = memochat::json::glaze_safe_get<std::string>(cookies, "cookie_header", "");
    }
    else
    {
        request.ipb_member_id = memochat::json::glaze_safe_get<std::string>(root, "ipb_member_id", "");
        request.ipb_pass_hash = memochat::json::glaze_safe_get<std::string>(root, "ipb_pass_hash", "");
        request.igneous = memochat::json::glaze_safe_get<std::string>(root, "igneous", "");
        request.sk = memochat::json::glaze_safe_get<std::string>(root, "sk", "");
    }
    // Top-level cookie_header field (used by nhentai / hanime1 clients).
    if (request.cookie_header.empty())
        request.cookie_header = memochat::json::glaze_safe_get<std::string>(root, "cookie_header", "");
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

bool DecodeR18VideoResolveRequest(std::string_view body, R18VideoResolveRequestDto* out, std::string* error_out)
{
    if (!DecodeR18PublicRequest(body, out, error_out, R18VideoResolveRequestFromJsonValue))
        return false;
    if (out->source_id.empty())
    {
        if (error_out != nullptr)
            *error_out = "source_id is required";
        return false;
    }
    if (out->chapter_id.empty())
    {
        if (error_out != nullptr)
            *error_out = "chapter_id is required";
        return false;
    }
    return true;
}

bool DecodeR18FavoriteToggleRequest(std::string_view body, R18FavoriteToggleRequestDto* out, std::string* error_out)
{
    return DecodeR18PublicRequest(body, out, error_out, R18FavoriteToggleRequestFromJsonValue);
}

bool DecodeR18HistoryUpdateRequest(std::string_view body, R18HistoryUpdateRequestDto* out, std::string* error_out)
{
    return DecodeR18PublicRequest(body, out, error_out, R18HistoryUpdateRequestFromJsonValue);
}

bool DecodeR18BrowserImportStartRequest(std::string_view body,
                                        R18BrowserImportStartRequestDto* out,
                                        std::string* error_out)
{
    return DecodeR18PublicRequest(body, out, error_out, R18BrowserImportStartRequestFromJsonValue);
}

bool DecodeR18BrowserImportCompleteRequest(std::string_view body,
                                           R18BrowserImportCompleteRequestDto* out,
                                           std::string* error_out)
{
    return DecodeR18PublicRequest(body, out, error_out, R18BrowserImportCompleteRequestFromJsonValue);
}

bool DecodeR18SessionImportRequest(std::string_view body, R18SessionImportRequestDto* out, std::string* error_out)
{
    return DecodeR18PublicRequest(body, out, error_out, R18SessionImportRequestFromJsonValue);
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

memochat::json::JsonValue R18BrowserImportStartResponseToJsonValue(const R18BrowserImportStartResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

memochat::json::JsonValue
R18BrowserImportCompleteResponseToJsonValue(const R18BrowserImportCompleteResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

memochat::json::JsonValue R18SessionImportResponseToJsonValue(const R18SessionImportResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

} // namespace memochat::r18
