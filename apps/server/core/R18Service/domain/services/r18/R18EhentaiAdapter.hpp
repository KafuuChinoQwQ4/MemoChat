#pragma once

#include "json/GlazeCompat.hpp"
#include "r18/R18SourceService.hpp"

#include <filesystem>
#include <string>

namespace memochat::r18
{

extern const char* const kEhentaiSourceId;

// Optional cookie string for exhentai / restricted galleries.
bool EhentaiSearch(const std::string& keyword,
                   int page,
                   const std::string& session_cookie,
                   json::JsonValue* out,
                   std::string* error);
bool EhentaiDetail(const std::string& comic_id,
                   const std::string& session_cookie,
                   json::JsonValue* out,
                   std::string* error);
bool EhentaiPages(const std::string& chapter_id,
                  const std::string& session_cookie,
                  json::JsonValue* out,
                  std::string* error);
R18ImagePayload EhentaiFetchImage(const std::filesystem::path& cache_root,
                                  const std::string& image_url,
                                  const std::string& session_cookie);

} // namespace memochat::r18
