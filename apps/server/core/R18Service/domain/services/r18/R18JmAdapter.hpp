#pragma once

#include "json/GlazeCompat.hpp"
#include "r18/R18SourceService.hpp"

#include <filesystem>
#include <string>

namespace memochat::r18
{

extern const char* const kJmSourceId;

bool JmSearch(const std::string& keyword,
              int page,
              const std::string& sort,
              const std::string& tag,
              json::JsonValue* out,
              std::string* error);
bool JmSearchWithToken(const std::string& keyword,
                       int page,
                       const std::string& sort,
                       const std::string& tag,
                       const std::string& uid,
                       json::JsonValue* out,
                       std::string* error);
bool JmDetail(const std::string& comic_id, json::JsonValue* out, std::string* error);
bool JmDetailWithToken(const std::string& comic_id, const std::string& uid, json::JsonValue* out, std::string* error);
bool JmPages(const std::string& chapter_id, json::JsonValue* out, std::string* error);
bool JmPagesWithToken(const std::string& chapter_id, const std::string& uid, json::JsonValue* out, std::string* error);
R18ImagePayload
JmFetchImage(const std::filesystem::path& cache_root, const std::string& image_url, long long scramble_id = 0);
bool IsAllowedJmImageUrl(const std::string& url);

// Optional login: stores uid in *uid_out on success.
bool JmLogin(const std::string& username, const std::string& password, std::string* uid_out, std::string* error);

// Daily check-in for logged-in users; uid from JmLogin.
bool JmCheckin(const std::string& uid, json::JsonValue* out, std::string* error);

} // namespace memochat::r18
