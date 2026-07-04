#pragma once

#include <cstdint>
#include <string>

namespace memochat::gate::services::media
{

struct MediaAssetRecord
{
    int64_t media_id = 0;
    std::string media_key;
    int owner_uid = 0;
    std::string media_type;
    std::string origin_file_name;
    std::string mime;
    int64_t size_bytes = 0;
    std::string storage_provider;
    std::string storage_path;
    int64_t created_at_ms = 0;
    int64_t deleted_at_ms = 0;
    int status = 1;
};

class MediaPersistence
{
public:
    static MediaPersistence& Instance();

    bool SaveAsset(const MediaAssetRecord& asset) const;
    bool LoadAssetByKey(const std::string& media_key, MediaAssetRecord& asset) const;
    bool GrantAccess(int64_t media_id, int grantee_uid, const std::string& scope, int64_t created_at_ms) const;
    bool GrantGroupAccess(int64_t media_id,
                          int64_t group_id,
                          int owner_uid,
                          const std::string& scope,
                          int64_t created_at_ms) const;
    bool CanReadAsset(const MediaAssetRecord& asset, int uid) const;

private:
    MediaPersistence() = default;
};

} // namespace memochat::gate::services::media
