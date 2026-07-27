#pragma once

#include "json/GlazeCompat.hpp"
#include "r18/R18SourceService.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace memochat::r18
{

extern const char* const kHanime1SourceId;

struct Hanime1VideoSource
{
    std::string url;
    std::string mime_type;
    int quality = 0;
    int64_t expires_at_ms = 0;
};

std::vector<Hanime1VideoSource>
ParseHanime1VideoSources(const std::string& html, const std::string& video_id, int64_t now_ms);
int ParseHanime1LastPage(const std::string& html);

// hanime1.me — video hentai source.
// Cookie auth is optional; without it only public content is accessible.
bool Hanime1Search(const std::string& keyword,
                   int page,
                   const std::string& sort,
                   const std::string& tag,
                   const std::string& session_cookie,
                   json::JsonValue* out,
                   std::string* error);

bool Hanime1Detail(const std::string& comic_id,
                   const std::string& session_cookie,
                   json::JsonValue* out,
                   std::string* error);

bool Hanime1Pages(const std::string& chapter_id,
                  const std::string& session_cookie,
                  json::JsonValue* out,
                  std::string* error);

bool Hanime1ResolveVideo(const std::string& chapter_id,
                         const std::string& session_cookie,
                         json::JsonValue* out,
                         std::string* error);

R18ImagePayload Hanime1FetchImage(const std::filesystem::path& cache_root,
                                  const std::string& image_url,
                                  const std::string& session_cookie);

// Username/password login → JWT/session cookie header on success.
bool Hanime1Login(const std::string& username,
                  const std::string& password,
                  std::string* session_cookie_out,
                  std::string* error);

} // namespace memochat::r18
