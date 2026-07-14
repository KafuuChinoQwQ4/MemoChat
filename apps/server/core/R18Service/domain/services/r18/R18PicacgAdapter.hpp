#pragma once

#include "json/GlazeCompat.hpp"
#include "r18/R18SourceService.hpp"

#include <filesystem>
#include <string>

namespace memochat::r18
{

extern const char* const kPicacgSourceId;

bool PicacgSearch(const std::string& keyword,
                  int page,
                  const std::string& sort,
                  const std::string& tag,
                  json::JsonValue* out,
                  std::string* error);
bool PicacgSearchWithToken(const std::string& keyword,
                           int page,
                           const std::string& sort,
                           const std::string& tag,
                           const std::string& token,
                           json::JsonValue* out,
                           std::string* error);
bool PicacgDetail(const std::string& comic_id, json::JsonValue* out, std::string* error);
bool PicacgDetailWithToken(const std::string& comic_id,
                           const std::string& token,
                           json::JsonValue* out,
                           std::string* error);
bool PicacgPages(const std::string& comic_id, const std::string& chapter_id, json::JsonValue* out, std::string* error);
bool PicacgPagesWithToken(const std::string& comic_id,
                          const std::string& chapter_id,
                          const std::string& token,
                          json::JsonValue* out,
                          std::string* error);
bool PicacgLogin(const std::string& email, const std::string& password, std::string* token_out, std::string* error);
R18ImagePayload PicacgFetchImage(const std::filesystem::path& cache_root, const std::string& image_url);

} // namespace memochat::r18
