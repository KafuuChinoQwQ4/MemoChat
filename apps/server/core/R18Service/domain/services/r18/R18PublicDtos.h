#pragma once

#include "json/GlazeCompat.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace memochat::r18
{

struct R18AuthRequestDto
{
    int uid = 0;
    std::string token;
};

struct R18SourceToggleRequestDto
{
    int uid = 0;
    std::string token;
    std::string source_id;
};

struct R18SearchRequestDto
{
    int uid = 0;
    std::string token;
    std::string source_id;
    std::string keyword;
    int page = 1;
};

struct R18ComicDetailRequestDto
{
    int uid = 0;
    std::string token;
    std::string source_id;
    std::string comic_id;
};

struct R18ChapterPagesRequestDto
{
    int uid = 0;
    std::string token;
    std::string source_id;
    std::string chapter_id;
};

struct R18FavoriteToggleRequestDto
{
    int uid = 0;
    std::string token;
    std::string source_id;
    std::string comic_id;
    bool favorited = true;
};

struct R18HistoryUpdateRequestDto
{
    int uid = 0;
    std::string token;
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

R18AuthRequestDto R18AuthRequestFromJsonValue(const memochat::json::JsonValue& root);
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
