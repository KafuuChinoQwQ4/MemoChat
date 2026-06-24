#pragma once

#include "json/GlazeCompat.h"
#include "r18/R18SourceService.h"

#include <filesystem>
#include <string>

namespace memochat::r18
{

extern const char* const kPicacgSourceId;

json::JsonValue PicacgSearch(const std::string& keyword, int page, int uid, const std::string& token);
json::JsonValue PicacgDetail(const std::string& comic_id, int uid, const std::string& token);
json::JsonValue
PicacgPages(const std::string& comic_id, const std::string& chapter_id, int uid, const std::string& token);
R18ImagePayload PicacgFetchImage(const std::filesystem::path& cache_root, const std::string& image_url);

} // namespace memochat::r18
