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

R18SourceToggleRequestDto R18SourceToggleRequestFromJsonValue(const memochat::json::JsonValue& root);
R18SearchRequestDto R18SearchRequestFromJsonValue(const memochat::json::JsonValue& root);
R18ComicDetailRequestDto R18ComicDetailRequestFromJsonValue(const memochat::json::JsonValue& root);
R18ChapterPagesRequestDto R18ChapterPagesRequestFromJsonValue(const memochat::json::JsonValue& root);
R18FavoriteToggleRequestDto R18FavoriteToggleRequestFromJsonValue(const memochat::json::JsonValue& root);
R18HistoryUpdateRequestDto R18HistoryUpdateRequestFromJsonValue(const memochat::json::JsonValue& root);

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
bool DecodeR18FavoriteToggleRequest(std::string_view body,
                                    R18FavoriteToggleRequestDto* out,
                                    std::string* error_out = nullptr);
bool DecodeR18HistoryUpdateRequest(std::string_view body,
                                   R18HistoryUpdateRequestDto* out,
                                   std::string* error_out = nullptr);

memochat::json::JsonValue R18SourceToggleResponseToJsonValue(const R18SourceToggleResponseDto& response);
memochat::json::JsonValue R18FavoriteToggleResponseToJsonValue(const R18FavoriteToggleResponseDto& response);
memochat::json::JsonValue R18HistoryUpdateResponseToJsonValue(const R18HistoryUpdateResponseDto& response);

} // namespace memochat::r18
