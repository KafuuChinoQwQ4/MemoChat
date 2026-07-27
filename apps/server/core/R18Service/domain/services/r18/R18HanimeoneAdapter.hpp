#pragma once

#include "json/GlazeCompat.hpp"
#include "r18/R18SourceService.hpp"

#include <filesystem>
#include <string>

namespace memochat::r18
{

extern const char* const kHanimeoneSourceId;

// hanimeone.me — H-manga / doujinshi comic source (sister site of hanime1.me).
// No session required for public content; same CDN infrastructure.
bool HanimeoneSearch(const std::string& keyword,
                     int page,
                     const std::string& sort,
                     const std::string& tag,
                     const std::string& session_cookie,
                     json::JsonValue* out,
                     std::string* error);

bool HanimeoneDetail(const std::string& comic_id,
                     const std::string& session_cookie,
                     json::JsonValue* out,
                     std::string* error);

bool HanimeonePages(const std::string& chapter_id,
                    const std::string& session_cookie,
                    json::JsonValue* out,
                    std::string* error);

R18ImagePayload HanimeoneFootImage(const std::filesystem::path& cache_root,
                                   const std::string& image_url,
                                   const std::string& session_cookie);

} // namespace memochat::r18
