#pragma once

#include "json/GlazeCompat.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace memochat::r18
{

struct R18SourceToggleRequestDto
{
    std::string source_id;
};

struct R18SearchRequestDto
{
    std::string source_id;
    std::string keyword;
    int page = 1;
    // Source-native sort key (e.g. jm: mr/mv_t, picacg: dd/ld, nhentai: popular-today).
    std::string sort;
    // Optional tag/category filter; adapters map it to upstream query/body fields.
    std::string tag;
};

struct R18ComicDetailRequestDto
{
    std::string source_id;
    std::string comic_id;
};

struct R18ChapterPagesRequestDto
{
    std::string source_id;
    std::string chapter_id;
};

struct R18VideoResolveRequestDto
{
    std::string source_id;
    std::string chapter_id;
};

struct R18FavoriteToggleRequestDto
{
    std::string source_id;
    std::string comic_id;
    bool favorited = true;
    // Optional snapshot metadata for offline library display.
    std::string title;
    std::string cover;
    std::string author;
    std::string subtitle;
    // Target folders when favoriting; empty → default folder.
    std::vector<std::string> folder_ids;
};

struct R18HistoryUpdateRequestDto
{
    std::string source_id;
    std::string comic_id;
    std::string chapter_id;
    int64_t page_index = 0;
};

struct R18SourceToggleResponseDto
{
    std::string source_id;
    bool enabled = false;
};

struct R18FavoriteToggleResponseDto
{
    std::string source_id;
    std::string comic_id;
    bool favorited = true;
};

struct R18HistoryUpdateResponseDto
{
    std::string source_id;
    std::string comic_id;
    std::string chapter_id;
    int64_t page_index = 0;
};

// Browser import DTOs
struct R18BrowserImportStartRequestDto
{
    std::string source_id;
    std::string client_kind; // "web_extension" | "qt_webengine"
};

struct R18BrowserImportStartResponseDto
{
    std::string import_id;
    std::string ticket;
    int64_t expires_at_ms = 0;
};

struct R18BrowserImportCompleteRequestDto
{
    std::string ticket;
    std::string ipb_member_id;
    std::string ipb_pass_hash;
    std::string igneous;
    std::string sk;
};

struct R18BrowserImportCompleteResponseDto
{
    bool success = false;
    std::string message;
};

struct R18SessionImportRequestDto
{
    std::string source_id;
    // E-Hentai / ExHentai specific fields.
    std::string ipb_member_id;
    std::string ipb_pass_hash;
    std::string igneous;
    std::string sk;
    // Generic cookie header for other sources (nhentai, hanime1, …).
    // Sent as a pre-formatted "name=value; name2=value2" cookie string.
    std::string cookie_header;
};

struct R18SessionImportResponseDto
{
    bool success = false;
    std::string message;
    bool ehentai_access = false;
    bool exhentai_access = false;
};

R18SourceToggleRequestDto R18SourceToggleRequestFromJsonValue(const memochat::json::JsonValue& root);
R18SearchRequestDto R18SearchRequestFromJsonValue(const memochat::json::JsonValue& root);
R18ComicDetailRequestDto R18ComicDetailRequestFromJsonValue(const memochat::json::JsonValue& root);
R18ChapterPagesRequestDto R18ChapterPagesRequestFromJsonValue(const memochat::json::JsonValue& root);
R18VideoResolveRequestDto R18VideoResolveRequestFromJsonValue(const memochat::json::JsonValue& root);
R18FavoriteToggleRequestDto R18FavoriteToggleRequestFromJsonValue(const memochat::json::JsonValue& root);
R18HistoryUpdateRequestDto R18HistoryUpdateRequestFromJsonValue(const memochat::json::JsonValue& root);
R18BrowserImportStartRequestDto R18BrowserImportStartRequestFromJsonValue(const memochat::json::JsonValue& root);
R18BrowserImportCompleteRequestDto R18BrowserImportCompleteRequestFromJsonValue(const memochat::json::JsonValue& root);
R18SessionImportRequestDto R18SessionImportRequestFromJsonValue(const memochat::json::JsonValue& root);

bool DecodeR18SourceToggleRequest(std::string_view body,
                                  R18SourceToggleRequestDto* out,
                                  std::string* error_out = nullptr);
bool DecodeR18SearchRequest(std::string_view body, R18SearchRequestDto* out, std::string* error_out = nullptr);
bool DecodeR18ComicDetailRequest(std::string_view body,
                                 R18ComicDetailRequestDto* out,
                                 std::string* error_out = nullptr);
bool DecodeR18ChapterPagesRequest(std::string_view body,
                                  R18ChapterPagesRequestDto* out,
                                  std::string* error_out = nullptr);
bool DecodeR18VideoResolveRequest(std::string_view body,
                                  R18VideoResolveRequestDto* out,
                                  std::string* error_out = nullptr);
bool DecodeR18FavoriteToggleRequest(std::string_view body,
                                    R18FavoriteToggleRequestDto* out,
                                    std::string* error_out = nullptr);
bool DecodeR18HistoryUpdateRequest(std::string_view body,
                                   R18HistoryUpdateRequestDto* out,
                                   std::string* error_out = nullptr);
bool DecodeR18BrowserImportStartRequest(std::string_view body,
                                        R18BrowserImportStartRequestDto* out,
                                        std::string* error_out = nullptr);
bool DecodeR18BrowserImportCompleteRequest(std::string_view body,
                                           R18BrowserImportCompleteRequestDto* out,
                                           std::string* error_out = nullptr);
bool DecodeR18SessionImportRequest(std::string_view body,
                                   R18SessionImportRequestDto* out,
                                   std::string* error_out = nullptr);

memochat::json::JsonValue R18SourceToggleResponseToJsonValue(const R18SourceToggleResponseDto& response);
memochat::json::JsonValue R18FavoriteToggleResponseToJsonValue(const R18FavoriteToggleResponseDto& response);
memochat::json::JsonValue R18HistoryUpdateResponseToJsonValue(const R18HistoryUpdateResponseDto& response);
memochat::json::JsonValue R18BrowserImportStartResponseToJsonValue(const R18BrowserImportStartResponseDto& response);
memochat::json::JsonValue
R18BrowserImportCompleteResponseToJsonValue(const R18BrowserImportCompleteResponseDto& response);
memochat::json::JsonValue R18SessionImportResponseToJsonValue(const R18SessionImportResponseDto& response);

} // namespace memochat::r18
