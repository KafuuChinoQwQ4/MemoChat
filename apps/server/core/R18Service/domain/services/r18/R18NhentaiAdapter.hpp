#pragma once

#include "json/GlazeCompat.hpp"
#include "r18/R18SourceService.hpp"

#include <filesystem>
#include <string>

namespace memochat::r18
{

extern const char* const kNhentaiSourceId;

bool NhentaiSearch(const std::string& keyword, int page, json::JsonValue* out, std::string* error);
bool NhentaiDetail(const std::string& comic_id, json::JsonValue* out, std::string* error);
bool NhentaiPages(const std::string& chapter_id, json::JsonValue* out, std::string* error);
R18ImagePayload NhentaiFetchImage(const std::filesystem::path& cache_root, const std::string& image_url);

} // namespace memochat::r18
