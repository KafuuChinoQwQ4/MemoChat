#pragma once

#include "json/GlazeCompat.hpp"
#include "r18/R18SourceService.hpp"

#include <filesystem>
#include <string>

namespace memochat::r18
{

extern const char* const kEhentaiSourceId;
extern const char* const kExhentaiSourceId;

// e-hentai: public listing works without login; cookie optional for restricted content.
// exhentai: requires a valid e-hentai family cookie (ipb_* + usually igneous).
bool EhentaiSearch(const std::string& keyword,
                   int page,
                   const std::string& sort,
                   const std::string& tag,
                   const std::string& session_cookie,
                   json::JsonValue* out,
                   std::string* error);
bool ExhentaiSearch(const std::string& keyword,
                    int page,
                    const std::string& sort,
                    const std::string& tag,
                    const std::string& session_cookie,
                    json::JsonValue* out,
                    std::string* error);

bool EhentaiDetail(const std::string& comic_id,
                   const std::string& session_cookie,
                   json::JsonValue* out,
                   std::string* error);
bool ExhentaiDetail(const std::string& comic_id,
                    const std::string& session_cookie,
                    json::JsonValue* out,
                    std::string* error);

bool EhentaiPages(const std::string& chapter_id,
                  const std::string& session_cookie,
                  json::JsonValue* out,
                  std::string* error);
bool ExhentaiPages(const std::string& chapter_id,
                   const std::string& session_cookie,
                   json::JsonValue* out,
                   std::string* error);

R18ImagePayload EhentaiFetchImage(const std::filesystem::path& cache_root,
                                  const std::string& image_url,
                                  const std::string& session_cookie);
R18ImagePayload ExhentaiFetchImage(const std::filesystem::path& cache_root,
                                   const std::string& image_url,
                                   const std::string& session_cookie);

// Forums account/password login → cookie string (ipb_member_id / ipb_pass_hash / …).
// When for_exhentai is true, also visits exhentai.org to obtain igneous and validates access.
bool EhentaiForumLogin(const std::string& username,
                       const std::string& password,
                       bool for_exhentai,
                       std::string* session_cookie,
                       std::string* error);

// Normalize raw cookie paste / password field into a cookie header value.
// Accepts "ipb_member_id=…; ipb_pass_hash=…" style strings.
bool LooksLikeEhentaiCookie(const std::string& value);

// Validate that a cookie grants exhentai access (non-sadpanda HTML listing).
bool ExhentaiValidateSession(const std::string& session_cookie, std::string* error);

} // namespace memochat::r18
