#pragma once

#include "json/GlazeCompat.hpp"
#include "r18/R18SourceService.hpp"

#include <filesystem>
#include <string>

namespace memochat::r18
{

extern const char* const kJmSourceId;

bool JmSearch(const std::string& keyword, int page, json::JsonValue* out, std::string* error);
bool JmDetail(const std::string& comic_id, json::JsonValue* out, std::string* error);
bool JmPages(const std::string& chapter_id, json::JsonValue* out, std::string* error);
R18ImagePayload JmFetchImage(const std::filesystem::path& cache_root, const std::string& image_url);
bool IsAllowedJmImageUrl(const std::string& url);

} // namespace memochat::r18
