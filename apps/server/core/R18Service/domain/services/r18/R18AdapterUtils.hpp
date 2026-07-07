#pragma once

#include "json/GlazeCompat.hpp"
#include "r18/R18SourceService.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace memochat::r18::detail
{

struct ParsedUrl
{
    std::string scheme;
    std::string host;
    std::string port;
    std::string target;
};

struct HttpResult
{
    int status = 0;
    std::string content_type;
    std::string body;
};

ParsedUrl ParseUrl(const std::string& url);
HttpResult HttpGet(const std::string& url,
                   const std::vector<std::pair<std::string, std::string>>& headers,
                   int timeout_seconds = 20);
std::string Md5Hex(const std::string& input);
std::string Aes256EcbDecrypt(const std::string& cipher_text, const std::string& key);
std::string UrlEncode(const std::string& input);
std::string EscapeXml(std::string value);

std::string StringValue(const json::JsonValue& value);
std::string FieldString(const json::JsonValue& object, const std::string& key, const std::string& fallback = "");
int64_t FieldInt(const json::JsonValue& object, const std::string& key, int64_t fallback = 0);
json::JsonValue MakeTags(const std::vector<std::string>& tags);
std::string ImageProxyUrl(const std::string& source_id, const std::string& image_url);

R18ImagePayload PlaceholderImage(const std::string& line1, const std::string& line2 = "");
bool ReadCachedImage(const std::filesystem::path& cache_root, const std::string& cache_key, R18ImagePayload* payload);
void WriteCachedImage(const std::filesystem::path& cache_root,
                      const std::string& cache_key,
                      const R18ImagePayload& payload);

} // namespace memochat::r18::detail
