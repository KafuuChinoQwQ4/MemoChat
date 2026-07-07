#pragma once

#include "json/GlazeCompat.hpp"
#include "r18/R18SourceService.hpp"

#include <filesystem>
#include <string>

namespace memochat::r18
{

extern const char* const kJmSourceId;

json::JsonValue JmSearch(const std::string& keyword, int page);
json::JsonValue JmDetail(const std::string& comic_id);
json::JsonValue JmPages(const std::string& chapter_id);
R18ImagePayload JmFetchImage(const std::filesystem::path& cache_root, const std::string& image_url);
bool IsAllowedJmImageUrl(const std::string& url);

} // namespace memochat::r18
