#pragma once

#include "json/GlazeCompat.hpp"
#include "r18/R18SourceService.hpp"

#include <filesystem>
#include <string>

namespace memochat::r18
{

extern const char* const kNhentaiSourceId;

// session_cookie is optional — empty string for unauthenticated/direct access.
bool NhentaiSearch(const std::string& keyword,
                   int page,
                   const std::string& sort,
                   const std::string& tag,
                   json::JsonValue* out,
                   std::string* error,
                   const std::string& session_cookie = {});
bool NhentaiDetail(const std::string& comic_id,
                   json::JsonValue* out,
                   std::string* error,
                   const std::string& session_cookie = {});
bool NhentaiPages(const std::string& chapter_id,
                  json::JsonValue* out,
                  std::string* error,
                  const std::string& session_cookie = {});
R18ImagePayload NhentaiFetchImage(const std::filesystem::path& cache_root,
                                  const std::string& image_url,
                                  const std::string& session_cookie = {});

// Username/password login → stores session cookies (sessionid; csrftoken).
// Returns a cookie header string on success, empty on failure.
bool NhentaiLogin(const std::string& username,
                  const std::string& password,
                  std::string* session_cookie_out,
                  std::string* error);

} // namespace memochat::r18
