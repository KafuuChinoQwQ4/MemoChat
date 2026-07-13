#pragma once

#include "json/GlazeCompat.hpp"

#include <cstddef>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace memochat::r18
{

std::size_t SourceImportLimitBytes();

struct R18SourceRecord
{
    std::string id;
    std::string name;
    std::string version;
    std::string path;
    std::string format = "native";
    std::string source_url;
    std::string catalog_url;
    bool enabled = true;
    bool builtin = false;
    std::string status = "ok";
    std::string message;
};

struct R18ImagePayload
{
    std::string content_type = "application/octet-stream";
    std::string body;
    bool ok = true;
    std::string error;
};

class R18SourceService
{
public:
    static R18SourceService& Instance();

    memochat::json::JsonValue ListSources();
    memochat::json::JsonValue ListSourcesForUser(int uid);
    bool EnableSource(const std::string& id, bool enabled, std::string* error);
    bool DeleteSource(const std::string& id, std::string* error);
    R18SourceRecord ImportZip(const std::string& file_name,
                              const std::string& manifest_json,
                              const std::string& binary,
                              std::string* error);

    // Unified account manager (per MemoChat uid).
    memochat::json::JsonValue ListAccounts(int uid);
    bool SaveAccount(int uid,
                     const std::string& source_id,
                     const std::string& username,
                     const std::string& password,
                     std::string* error);
    bool LoginAccount(int uid, const std::string& source_id, std::string* error);
    bool ClearAccount(int uid, const std::string& source_id, std::string* error);

    memochat::json::JsonValue Search(const std::string& source_id, const std::string& keyword, int page);
    memochat::json::JsonValue
    SearchForUser(int uid, const std::string& source_id, const std::string& keyword, int page);
    memochat::json::JsonValue Detail(const std::string& source_id, const std::string& comic_id);
    memochat::json::JsonValue DetailForUser(int uid, const std::string& source_id, const std::string& comic_id);
    memochat::json::JsonValue Pages(const std::string& source_id, const std::string& chapter_id);
    memochat::json::JsonValue PagesForUser(int uid, const std::string& source_id, const std::string& chapter_id);
    R18ImagePayload FetchImage(const std::string& source_id, const std::string& image_url);
    R18ImagePayload FetchImageForUser(int uid, const std::string& source_id, const std::string& image_url);

    // Daily check-in for sources that support it (currently JMComic).
    // Returns a JSON payload with status/message; errors stored on the credential.
    memochat::json::JsonValue CheckinForUser(int uid, const std::string& source_id);

private:
    R18SourceService();

    bool CanDispatchSource(const std::string& source_id, std::string* error);
    bool CanDispatchSourceForUser(int uid, const std::string& source_id, std::string* error);
    std::optional<R18SourceRecord> SourceSnapshot(const std::string& source_id);
    std::string SessionTokenFor(int uid, const std::string& source_id);
    std::string SessionCookieFor(int uid, const std::string& source_id);
    void LoadLocked();
    void LoadManifestLocked(const std::filesystem::path& manifest_path);
    void SaveLocked();
    void InstallBuiltinSourcesLocked();

    std::mutex mu_;
    std::filesystem::path data_root_;
    std::filesystem::path image_cache_root_;
    std::unordered_map<std::string, R18SourceRecord> sources_;
};

bool DecodeBase64(const std::string& input, std::string& out);
bool DecodeBase64Bounded(const std::string& input, std::string& out, std::size_t max_output_bytes);

} // namespace memochat::r18
