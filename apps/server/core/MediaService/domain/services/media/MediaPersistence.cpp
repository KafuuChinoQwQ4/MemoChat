#include "services/media/MediaPersistence.hpp"

#include "PostgresMgr.hpp"

namespace memochat::gate::services::media
{
namespace
{

MediaAssetInfo ToStorageInfo(const MediaAssetRecord& record)
{
    MediaAssetInfo info;
    info.media_id = record.media_id;
    info.media_key = record.media_key;
    info.owner_uid = record.owner_uid;
    info.media_type = record.media_type;
    info.origin_file_name = record.origin_file_name;
    info.mime = record.mime;
    info.size_bytes = record.size_bytes;
    info.storage_provider = record.storage_provider;
    info.storage_path = record.storage_path;
    info.created_at_ms = record.created_at_ms;
    info.deleted_at_ms = record.deleted_at_ms;
    info.status = record.status;
    return info;
}

MediaAssetRecord ToAssetRecord(const MediaAssetInfo& info)
{
    MediaAssetRecord record;
    record.media_id = info.media_id;
    record.media_key = info.media_key;
    record.owner_uid = info.owner_uid;
    record.media_type = info.media_type;
    record.origin_file_name = info.origin_file_name;
    record.mime = info.mime;
    record.size_bytes = info.size_bytes;
    record.storage_provider = info.storage_provider;
    record.storage_path = info.storage_path;
    record.created_at_ms = info.created_at_ms;
    record.deleted_at_ms = info.deleted_at_ms;
    record.status = info.status;
    return record;
}

} // namespace

MediaPersistence& MediaPersistence::Instance()
{
    static MediaPersistence instance;
    return instance;
}

bool MediaPersistence::SaveAsset(const MediaAssetRecord& asset) const
{
    return PostgresMgr::GetInstance()->InsertMediaAsset(ToStorageInfo(asset));
}

bool MediaPersistence::LoadAssetByKey(const std::string& media_key, MediaAssetRecord& asset) const
{
    MediaAssetInfo info;
    if (!PostgresMgr::GetInstance()->GetMediaAssetByKey(media_key, info))
    {
        return false;
    }
    asset = ToAssetRecord(info);
    return true;
}

bool MediaPersistence::GrantAccess(int64_t media_id,
                                   int grantee_uid,
                                   const std::string& scope,
                                   int64_t created_at_ms) const
{
    return PostgresMgr::GetInstance()->GrantMediaAccess(media_id, grantee_uid, scope, created_at_ms);
}

bool MediaPersistence::GrantGroupAccess(int64_t media_id,
                                        int64_t group_id,
                                        int owner_uid,
                                        const std::string& scope,
                                        int64_t created_at_ms) const
{
    return PostgresMgr::GetInstance()->GrantMediaGroupAccess(media_id, group_id, owner_uid, scope, created_at_ms);
}

bool MediaPersistence::CanReadAsset(const MediaAssetRecord& asset, int uid) const
{
    if (uid <= 0 || asset.media_id <= 0)
    {
        return false;
    }
    if (asset.owner_uid == uid)
    {
        return true;
    }
    return PostgresMgr::GetInstance()->HasMediaAccess(asset.media_id, uid);
}

} // namespace memochat::gate::services::media
