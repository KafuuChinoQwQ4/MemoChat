#pragma once

#include "json/GlazeCompat.h"

#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace memochat::r18
{

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
};

class R18SourceService
{
public:
    static R18SourceService& Instance();

    memochat::json::JsonValue ListSources();
    bool EnableSource(const std::string& id, bool enabled, std::string* error);
    R18SourceRecord ImportZip(const std::string& file_name,
                              const std::string& manifest_json,
                              const std::string& binary,
                              std::string* error);

    memochat::json::JsonValue
    Search(const std::string& source_id, const std::string& keyword, int page, int uid, const std::string& token);
    memochat::json::JsonValue
    Detail(const std::string& source_id, const std::string& comic_id, int uid, const std::string& token);
    memochat::json::JsonValue
    Pages(const std::string& source_id, const std::string& chapter_id, int uid, const std::string& token);
    R18ImagePayload FetchImage(const std::string& source_id, const std::string& image_url);

private:
    R18SourceService();

    std::optional<R18SourceRecord> SourceSnapshot(const std::string& source_id);
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

} // namespace memochat::r18
