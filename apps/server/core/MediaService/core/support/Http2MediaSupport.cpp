#include "Http2MediaSupport.hpp"
#include "json/GlazeCompat.hpp"
#include "ConfigMgr.hpp"
#include "MediaStorage.hpp"
#include "services/media/MediaPublicDtos.hpp"
#include "services/media/MediaUploadSessionDto.hpp"
#include "PostgresMgr.hpp"
#include "RedisMgr.hpp"
#include "const.hpp"
#include "logging/Logger.hpp"
#include "random/Uuid.hpp"
#include "support/UserTokenValidator.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

import memochat.media.config_algorithms;

namespace
{
template <typename Integer> bool ParseIntegerLocal(std::string_view raw, Integer* out)
{
    if (out == nullptr || raw.empty())
    {
        return false;
    }
    Integer value{};
    const auto [ptr, ec] = std::from_chars(raw.data(), raw.data() + raw.size(), value);
    if (ec != std::errc{} || ptr != raw.data() + raw.size())
    {
        return false;
    }
    *out = value;
    return true;
}

struct MediaUploadGrantSpecLocal
{
    std::vector<int> grant_uids;
    int64_t grant_group_id = 0;
    bool grant_public = false;
    bool grant_friends = false;
};

int64_t NowMsLocal()
{
    return static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count());
}

std::string SanitizeFileNameLocal(const std::string& fileName)
{
    std::string safe;
    safe.reserve(fileName.size());
    for (char c : fileName)
    {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == '.')
        {
            safe.push_back(c);
        }
        else
        {
            safe.push_back('_');
        }
    }
    if (safe.empty())
        return "file.bin";
    if (safe.size() > 96)
        safe = safe.substr(safe.size() - 96);
    return safe;
}

std::vector<int> NormalizeGrantUidsLocal(std::vector<int> values)
{
    values.erase(std::remove_if(values.begin(),
                                values.end(),
                                [](int uid)
                                {
                                    return uid <= 0;
                                }),
                 values.end());
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
    return values;
}

MediaUploadGrantSpecLocal NormalizeGrantSpecLocal(MediaUploadGrantSpecLocal grants)
{
    grants.grant_uids = NormalizeGrantUidsLocal(std::move(grants.grant_uids));
    if (grants.grant_group_id < 0)
    {
        grants.grant_group_id = 0;
    }
    return grants;
}

bool HasAnyGrantLocal(const MediaUploadGrantSpecLocal& grants)
{
    return !grants.grant_uids.empty() || grants.grant_group_id > 0 || grants.grant_public || grants.grant_friends;
}

bool IsAvatarMediaTypeLocal(const std::string& media_type)
{
    return media_type == "avatar";
}

MediaUploadGrantSpecLocal GrantSpecFromSessionLocal(const memochat::media::MediaUploadSessionDto& session)
{
    MediaUploadGrantSpecLocal grants;
    grants.grant_uids = session.grant_uids;
    grants.grant_group_id = session.grant_group_id;
    grants.grant_public = session.grant_public;
    grants.grant_friends = session.grant_friends;
    return NormalizeGrantSpecLocal(std::move(grants));
}

bool ApplyMediaUploadGrantsLocal(const std::string& media_key,
                                 int owner_uid,
                                 MediaUploadGrantSpecLocal grants,
                                 std::string* error_out)
{
    grants = NormalizeGrantSpecLocal(std::move(grants));

    MediaAssetInfo persisted;
    if (!PostgresMgr::GetInstance()->GetMediaAssetByKey(media_key, persisted) || persisted.owner_uid != owner_uid ||
        persisted.media_id <= 0)
    {
        if (error_out != nullptr)
        {
            *error_out = "load media metadata for grant failed";
        }
        return false;
    }

    if (IsAvatarMediaTypeLocal(persisted.media_type))
    {
        grants.grant_public = true;
    }
    if (!HasAnyGrantLocal(grants))
    {
        return true;
    }

    const int64_t now_ms = NowMsLocal();
    for (int grantee_uid : grants.grant_uids)
    {
        if (grantee_uid <= 0 || grantee_uid == owner_uid)
        {
            continue;
        }
        if (!PostgresMgr::GetInstance()->GrantMediaAccess(persisted.media_id, grantee_uid, "direct", now_ms))
        {
            if (error_out != nullptr)
            {
                *error_out = "grant media access failed";
            }
            return false;
        }
    }
    if (grants.grant_public && !PostgresMgr::GetInstance()->GrantMediaAccess(persisted.media_id, 0, "public", now_ms))
    {
        if (error_out != nullptr)
        {
            *error_out = "grant public media access failed";
        }
        return false;
    }
    if (grants.grant_friends && !PostgresMgr::GetInstance()->GrantMediaAccess(persisted.media_id, 0, "friends", now_ms))
    {
        if (error_out != nullptr)
        {
            *error_out = "grant friends media access failed";
        }
        return false;
    }
    if (grants.grant_group_id > 0 && !PostgresMgr::GetInstance()->GrantMediaGroupAccess(persisted.media_id,
                                                                                        grants.grant_group_id,
                                                                                        owner_uid,
                                                                                        "group",
                                                                                        now_ms))
    {
        if (error_out != nullptr)
        {
            *error_out = "grant group media access failed";
        }
        return false;
    }
    return true;
}

std::string DecodeBase64Local(const std::string& input)
{
    static const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                     "abcdefghijklmnopqrstuvwxyz"
                                     "0123456789+/";
    std::vector<int> table(256, -1);
    for (int i = 0; i < static_cast<int>(chars.size()); ++i)
        table[static_cast<unsigned char>(chars[i])] = i;
    std::string output;
    int val = 0;
    int valb = -8;
    for (unsigned char c : input)
    {
        if (std::isspace(c))
            continue;
        if (c == '=')
            break;
        if (table[c] == -1)
            return "";
        val = (val << 6) + table[c];
        valb += 6;
        if (valb >= 0)
        {
            output.push_back(static_cast<char>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return output;
}

std::string GuessContentTypeLocal(const std::string& fileName, const std::string& mimeHint)
{
    if (!mimeHint.empty())
        return mimeHint;
    static const std::unordered_map<std::string, std::string> extMap = {{".png", "image/png"},
                                                                        {".jpg", "image/jpeg"},
                                                                        {".jpeg", "image/jpeg"},
                                                                        {".webp", "image/webp"},
                                                                        {".bmp", "image/bmp"},
                                                                        {".gif", "image/gif"},
                                                                        {".txt", "text/plain"},
                                                                        {".pdf", "application/pdf"}};
    std::string ext;
    const auto dotPos = fileName.find_last_of('.');
    if (dotPos != std::string::npos)
    {
        ext = fileName.substr(dotPos);
        memochat::media::modules::LowerAsciiInPlace(ext.data(), ext.size());
    }
    auto it = extMap.find(ext);
    return (it != extMap.end()) ? it->second : "application/octet-stream";
}

std::filesystem::path UploadRootLocal()
{
    std::error_code filesystem_error;
    const auto current_path = std::filesystem::current_path(filesystem_error);
    return filesystem_error ? std::filesystem::path("uploads") : current_path / "uploads";
}

std::filesystem::path ChunkRootLocal()
{
    return UploadRootLocal() / "chunks";
}

std::filesystem::path SessionRootLocal()
{
    return UploadRootLocal() / "sessions";
}

std::filesystem::path SessionPathForLocal(const std::string& upload_id)
{
    return SessionRootLocal() / (upload_id + ".json");
}

std::filesystem::path ChunkDirForLocal(const std::string& upload_id)
{
    return ChunkRootLocal() / upload_id;
}

bool ResolveAccessTokenUidLocal(const std::string& access_token, int& uid)
{
    uid = 0;
    return memochat::auth::ResolveUserIdFromToken(access_token, uid);
}

bool ListUploadedChunkIndexesLocal(const std::filesystem::path& chunk_dir, std::set<int>* indexes, std::string* error)
{
    if (indexes == nullptr)
    {
        if (error != nullptr)
            *error = "uploaded chunk output is null";
        return false;
    }
    indexes->clear();
    if (error != nullptr)
        error->clear();

    std::error_code filesystem_error;
    const bool chunk_dir_exists = std::filesystem::exists(chunk_dir, filesystem_error);
    if (filesystem_error)
    {
        if (error != nullptr)
            *error = "check chunk directory failed: " + filesystem_error.message();
        return false;
    }
    if (!chunk_dir_exists)
        return true;

    std::filesystem::directory_iterator entry(chunk_dir, filesystem_error);
    if (filesystem_error)
    {
        if (error != nullptr)
            *error = "open chunk directory failed: " + filesystem_error.message();
        return false;
    }
    const std::filesystem::directory_iterator end;
    while (entry != end)
    {
        std::error_code entry_error;
        const bool regular_file = entry->is_regular_file(entry_error);
        if (entry_error)
        {
            if (error != nullptr)
                *error = "inspect chunk entry failed: " + entry_error.message();
            return false;
        }
        if (regular_file)
        {
            const std::string stem = entry->path().stem().string();
            const bool digits = !stem.empty() && std::all_of(stem.begin(),
                                                             stem.end(),
                                                             [](char c)
                                                             {
                                                                 return std::isdigit(static_cast<unsigned char>(c));
                                                             });
            int index = 0;
            if (digits && ParseIntegerLocal(stem, &index))
            {
                indexes->insert(index);
            }
        }
        entry.increment(filesystem_error);
        if (filesystem_error)
        {
            if (error != nullptr)
                *error = "iterate chunk directory failed: " + filesystem_error.message();
            return false;
        }
    }
    return true;
}

bool SaveJsonFileLocal(const std::filesystem::path& path, const memochat::json::JsonValue& root)
{
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    if (ec)
        return false;
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    if (!ofs.is_open())
        return false;
    ofs << memochat::json::writeString(root);
    return ofs.good();
}

bool LoadJsonFileLocal(const std::filesystem::path& path, memochat::json::JsonValue& root)
{
    std::error_code filesystem_error;
    if (!std::filesystem::exists(path, filesystem_error) || filesystem_error)
        return false;
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open())
        return false;
    const std::string json((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    if (!memochat::json::glaze_parse(root, json))
    {
        return false;
    }
    if (memochat::json::glaze_is_object(root))
    {
        return true;
    }
    if (!root.isString())
    {
        return false;
    }

    memochat::json::JsonValue decoded;
    if (!memochat::json::glaze_parse(decoded, root.asString()) || !memochat::json::glaze_is_object(decoded))
    {
        return false;
    }
    root = decoded;
    return true;
}

IMediaStorage& MediaStorageForLocal(const std::string& provider)
{
    (void) provider;
    return GetMediaStorage();
}

bool NewIdStringLocal(std::string* value, std::string* error)
{
    if (value == nullptr)
    {
        if (error != nullptr)
        {
            *error = "identifier output is null";
        }
        return false;
    }
    return memochat::random::GenerateUuid(*value, error);
}

int64_t ParseConfigInt64Local(const std::string& raw, int64_t fallback)
{
    int64_t value = 0;
    if (!ParseIntegerLocal(raw, &value) || value <= 0)
        return fallback;
    return value;
}

int ParseConfigIntLocal(const std::string& raw, int fallback)
{
    int value = 0;
    if (!ParseIntegerLocal(raw, &value) || value <= 0)
        return fallback;
    return value;
}

struct MediaConfigLocal
{
    int64_t max_image_bytes = 200LL * 1024 * 1024;
    int64_t max_file_bytes = 20480LL * 1024 * 1024;
    int chunk_size_bytes = 4 * 1024 * 1024;
    int session_expire_sec = 86400;
    std::string storage_provider = "local";
};

MediaConfigLocal LoadMediaConfigLocal()
{
    MediaConfigLocal cfg;
    auto media = ConfigMgr::Inst()["Media"];
    cfg.max_image_bytes = ParseConfigInt64Local(media["MaxImageMB"], 200) * 1024 * 1024;
    cfg.max_file_bytes = ParseConfigInt64Local(media["MaxFileMB"], 20480) * 1024 * 1024;
    cfg.chunk_size_bytes = ParseConfigIntLocal(media["ChunkSizeKB"], 4096) * 1024;
    cfg.session_expire_sec = ParseConfigIntLocal(media["SessionExpireSec"], 86400);
    const std::string provider = media["StorageProvider"];
    if (!provider.empty())
    {
        cfg.storage_provider = provider;
    }
    cfg.chunk_size_bytes = memochat::media::modules::ClampInt(cfg.chunk_size_bytes, 256 * 1024, 4 * 1024 * 1024);
    return cfg;
}

bool IsMediaTypeImageLocal(const std::string& media_type)
{
    return media_type == "image" || media_type == "moment_image";
}

uint64_t Base64EncodedLimitForDecodedBytesLocal(uint64_t decoded_limit)
{
    return ((decoded_limit + 2ULL) / 3ULL) * 4ULL;
}

bool ShouldRejectEncodedPayloadSizeLocal(uint64_t encoded_size, uint64_t decoded_limit)
{
    return encoded_size > Base64EncodedLimitForDecodedBytesLocal(decoded_limit);
}

int64_t SimpleJsonDecodedHardLimitLocal()
{
    return 64LL * 1024LL * 1024LL;
}

int64_t EffectiveSimpleJsonDecodedLimitLocal(int64_t configured_limit)
{
    if (configured_limit <= 0)
    {
        return SimpleJsonDecodedHardLimitLocal();
    }
    return configured_limit < SimpleJsonDecodedHardLimitLocal() ? configured_limit : SimpleJsonDecodedHardLimitLocal();
}

} // namespace

namespace Http2MediaSupport
{

bool ParseJsonBody(std::string_view body_sv, memochat::json::JsonValue& root)
{
    std::string body_str(body_sv);
    return memochat::json::glaze_parse(root, body_str) && memochat::json::glaze_is_object(root);
}

std::string DecodeBase64(const std::string& input)
{
    return DecodeBase64Local(input);
}

MediaResult HandleUploadMediaInit(const std::string& access_token,
                                  const std::string& media_type,
                                  const std::string& file_name,
                                  const std::string& mime,
                                  int64_t file_size,
                                  std::vector<int> grant_uids,
                                  int64_t grant_group_id,
                                  bool grant_public,
                                  bool grant_friends)
{
    MediaResult result;
    int uid = 0;
    if (file_name.empty() || file_size <= 0 || !ResolveAccessTokenUidLocal(access_token, uid))
    {
        result.error = ErrorCodes::TokenInvalid;
        result.message = "token invalid or params invalid";
        return result;
    }

    std::string safe_type = media_type.empty() ? "file" : media_type;
    std::string safe_name = SanitizeFileNameLocal(file_name);
    std::string safe_mime = mime.empty() ? GuessContentTypeLocal(safe_name, "") : mime;

    MediaConfigLocal cfg = LoadMediaConfigLocal();
    int64_t limit = IsMediaTypeImageLocal(safe_type) ? cfg.max_image_bytes : cfg.max_file_bytes;
    if (file_size > limit)
    {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "file too large";
        result.data["limit_bytes"] = static_cast<int64_t>(limit);
        return result;
    }

    std::string upload_id;
    std::string id_error;
    if (!NewIdStringLocal(&upload_id, &id_error))
    {
        memolog::LogError("media.upload.id_generation_failed",
                          "upload identifier generation failed",
                          {{"uid", std::to_string(uid)}, {"error", id_error}});
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "generate upload id failed";
        return result;
    }
    int chunk_size = cfg.chunk_size_bytes;
    int total_chunks = static_cast<int>((file_size + chunk_size - 1) / chunk_size);
    std::error_code ec;
    std::filesystem::create_directories(ChunkDirForLocal(upload_id), ec);
    if (ec)
    {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "create chunk dir failed";
        return result;
    }

    memochat::media::MediaUploadSessionDto session;
    session.uid = uid;
    session.upload_id = upload_id;
    session.media_type = safe_type;
    session.file_name = safe_name;
    session.mime = safe_mime;
    session.file_size = static_cast<int64_t>(file_size);
    session.chunk_size = chunk_size;
    session.total_chunks = total_chunks;
    session.created_at = static_cast<int64_t>(NowMsLocal());
    session.expires_at = static_cast<int64_t>(NowMsLocal() + static_cast<int64_t>(cfg.session_expire_sec) * 1000);
    session.storage_provider = cfg.storage_provider;
    MediaUploadGrantSpecLocal grants;
    grants.grant_uids = std::move(grant_uids);
    grants.grant_group_id = grant_group_id;
    grants.grant_public = grant_public;
    grants.grant_friends = grant_friends;
    grants = NormalizeGrantSpecLocal(std::move(grants));
    session.grant_uids = grants.grant_uids;
    session.grant_group_id = grants.grant_group_id;
    session.grant_public = grants.grant_public;
    session.grant_friends = grants.grant_friends;
    if (!SaveJsonFileLocal(SessionPathForLocal(upload_id), memochat::media::MediaUploadSessionToJsonValue(session)))
    {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "create upload session failed";
        return result;
    }

    memochat::media::MediaUploadInitResponseDto response_dto;
    response_dto.upload_id = upload_id;
    response_dto.chunk_size = chunk_size;
    response_dto.total_chunks = total_chunks;
    result.error = ErrorCodes::Success;
    result.data = memochat::media::MediaUploadInitResponseToJsonValue(response_dto);
    return result;
}

MediaResult HandleUploadMediaChunk(const std::string& access_token,
                                   const std::string& upload_id,
                                   int index,
                                   const std::string& chunk_data_base64)
{
    if (chunk_data_base64.empty())
    {
        MediaResult result;
        result.error = ErrorCodes::TokenInvalid;
        result.message = "token invalid or params invalid";
        return result;
    }

    const MediaConfigLocal cfg = LoadMediaConfigLocal();
    if (ShouldRejectEncodedPayloadSizeLocal(static_cast<uint64_t>(chunk_data_base64.size()),
                                            static_cast<uint64_t>(cfg.chunk_size_bytes)))
    {
        MediaResult result;
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "encoded payload too large";
        result.data["limit_bytes"] = static_cast<int64_t>(cfg.chunk_size_bytes);
        return result;
    }

    std::string binary = DecodeBase64Local(chunk_data_base64);
    if (binary.empty())
    {
        MediaResult result;
        result.error = ErrorCodes::Error_Json;
        result.message = "base64 decode failed or empty";
        return result;
    }

    return HandleUploadMediaChunkBytes(access_token, upload_id, index, binary);
}

MediaResult HandleUploadMediaChunkBytes(const std::string& access_token,
                                        const std::string& upload_id,
                                        int index,
                                        std::string_view chunk_data)
{
    MediaResult result;
    int uid = 0;
    if (upload_id.empty() || index < 0 || chunk_data.empty() || !ResolveAccessTokenUidLocal(access_token, uid))
    {
        result.error = ErrorCodes::TokenInvalid;
        result.message = "token invalid or params invalid";
        return result;
    }

    memochat::json::JsonValue session_json;
    memochat::media::MediaUploadSessionDto session;
    if (!LoadJsonFileLocal(SessionPathForLocal(upload_id), session_json) ||
        !memochat::media::MediaUploadSessionFromJsonValue(session_json, &session))
    {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "upload session not found";
        return result;
    }
    if (session.uid != uid)
    {
        result.error = ErrorCodes::TokenInvalid;
        result.message = "session uid mismatch";
        return result;
    }
    int64_t expires_at = session.expires_at;
    if (expires_at > 0 && NowMsLocal() > expires_at)
    {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "upload session expired";
        return result;
    }

    int total_chunks = session.total_chunks;
    int chunk_size = session.chunk_size;
    if (index >= total_chunks || total_chunks <= 0 || chunk_size <= 0)
    {
        result.error = ErrorCodes::Error_Json;
        result.message = "invalid chunk index";
        return result;
    }

    if (static_cast<int>(chunk_data.size()) > chunk_size)
    {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "invalid chunk size";
        return result;
    }

    std::filesystem::path chunk_dir = ChunkDirForLocal(upload_id);
    std::error_code ec;
    std::filesystem::create_directories(chunk_dir, ec);
    if (ec)
    {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "create chunk dir failed";
        return result;
    }

    std::filesystem::path chunk_path = chunk_dir / (std::to_string(index) + ".part");
    std::ofstream ofs(chunk_path, std::ios::binary | std::ios::trunc);
    if (!ofs.is_open())
    {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "write chunk failed";
        return result;
    }
    ofs.write(chunk_data.data(), static_cast<std::streamsize>(chunk_data.size()));
    ofs.close();

    memochat::media::MediaUploadChunkResponseDto response_dto;
    response_dto.upload_id = upload_id;
    response_dto.index = index;
    response_dto.size = static_cast<int64_t>(chunk_data.size());
    result.error = ErrorCodes::Success;
    result.data = memochat::media::MediaUploadChunkResponseToJsonValue(response_dto);
    return result;
}

MediaResult HandleUploadMediaStatus(const std::string& access_token, const std::string& upload_id)
{
    MediaResult result;
    int uid = 0;
    if (upload_id.empty() || !ResolveAccessTokenUidLocal(access_token, uid))
    {
        result.error = ErrorCodes::TokenInvalid;
        result.message = "token invalid or params invalid";
        return result;
    }

    memochat::json::JsonValue session_json;
    memochat::media::MediaUploadSessionDto session;
    if (!LoadJsonFileLocal(SessionPathForLocal(upload_id), session_json) ||
        !memochat::media::MediaUploadSessionFromJsonValue(session_json, &session))
    {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "upload session not found";
        return result;
    }
    if (session.uid != uid)
    {
        result.error = ErrorCodes::TokenInvalid;
        result.message = "session uid mismatch";
        return result;
    }

    memochat::media::MediaUploadStatusResponseDto response_dto;
    response_dto.upload_id = upload_id;
    response_dto.total_chunks = session.total_chunks;
    response_dto.chunk_size = session.chunk_size;
    std::set<int> uploaded;
    std::string list_error;
    if (!ListUploadedChunkIndexesLocal(ChunkDirForLocal(upload_id), &uploaded, &list_error))
    {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = list_error;
        return result;
    }
    for (int idx : uploaded)
    {
        response_dto.uploaded_chunks.push_back(idx);
    }
    result.error = ErrorCodes::Success;
    result.data = memochat::media::MediaUploadStatusResponseToJsonValue(response_dto);
    return result;
}

MediaResult HandleUploadMediaComplete(const std::string& access_token, const std::string& upload_id)
{
    MediaResult result;
    int uid = 0;
    if (upload_id.empty() || !ResolveAccessTokenUidLocal(access_token, uid))
    {
        result.error = ErrorCodes::TokenInvalid;
        result.message = "token invalid or params invalid";
        return result;
    }

    memochat::json::JsonValue session_json;
    memochat::media::MediaUploadSessionDto session;
    if (!LoadJsonFileLocal(SessionPathForLocal(upload_id), session_json) ||
        !memochat::media::MediaUploadSessionFromJsonValue(session_json, &session))
    {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "upload session not found";
        return result;
    }
    if (session.uid != uid)
    {
        result.error = ErrorCodes::TokenInvalid;
        result.message = "session uid mismatch";
        return result;
    }

    int total_chunks = session.total_chunks;
    int chunk_size = session.chunk_size;
    int64_t file_size = session.file_size;
    std::string file_name = session.file_name;
    std::string media_type = session.media_type;
    std::string mime = session.mime;
    std::string storage_provider = session.storage_provider;
    if (total_chunks <= 0 || chunk_size <= 0 || file_size <= 0 || file_name.empty())
    {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "invalid upload session";
        return result;
    }

    std::filesystem::path chunk_dir = ChunkDirForLocal(upload_id);
    std::set<int> uploaded;
    std::string list_error;
    if (!ListUploadedChunkIndexesLocal(chunk_dir, &uploaded, &list_error))
    {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = list_error;
        return result;
    }
    for (int i = 0; i < total_chunks; ++i)
    {
        if (uploaded.find(i) == uploaded.end())
        {
            result.error = ErrorCodes::MediaUploadFailed;
            result.message = "chunks not complete";
            result.data["missing_index"] = i;
            return result;
        }
    }

    std::filesystem::path merged_path = chunk_dir / "merged.tmp";
    {
        std::ofstream merged(merged_path, std::ios::binary | std::ios::trunc);
        if (!merged.is_open())
        {
            result.error = ErrorCodes::MediaUploadFailed;
            result.message = "create merged file failed";
            return result;
        }
        for (int i = 0; i < total_chunks; ++i)
        {
            std::filesystem::path one_path = chunk_dir / (std::to_string(i) + ".part");
            std::ifstream one(one_path, std::ios::binary);
            if (!one.is_open())
            {
                result.error = ErrorCodes::MediaUploadFailed;
                result.message = "open chunk failed";
                result.data["chunk_index"] = i;
                return result;
            }
            merged << one.rdbuf();
        }
        merged.flush();
        if (!merged.good())
        {
            result.error = ErrorCodes::MediaUploadFailed;
            result.message = "merge chunks failed";
            return result;
        }
    }

    std::error_code ec;
    int64_t merged_size = static_cast<int64_t>(std::filesystem::file_size(merged_path, ec));
    if (ec || merged_size != file_size)
    {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "merged file size mismatch";
        return result;
    }

    std::string media_key;
    std::string id_error;
    if (!NewIdStringLocal(&media_key, &id_error))
    {
        memolog::LogError("media.upload.id_generation_failed",
                          "media identifier generation failed",
                          {{"uid", std::to_string(uid)}, {"error", id_error}});
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "generate media id failed";
        return result;
    }
    std::string storage_path;
    std::string storage_error;
    IMediaStorage& storage = MediaStorageForLocal(storage_provider);
    if (!storage.StoreMergedFile(media_type, media_key, file_name, merged_path, storage_path, storage_error))
    {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = storage_error.empty() ? "persist media failed" : storage_error;
        return result;
    }

    MediaAssetInfo asset;
    asset.media_key = media_key;
    asset.owner_uid = uid;
    asset.media_type = media_type;
    asset.origin_file_name = file_name;
    asset.mime = mime;
    asset.size_bytes = merged_size;
    asset.storage_provider = storage_provider;
    asset.storage_path = storage_path;
    asset.created_at_ms = NowMsLocal();
    asset.deleted_at_ms = 0;
    asset.status = 1;
    if (!PostgresMgr::GetInstance()->InsertMediaAsset(asset))
    {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "save media metadata failed";
        return result;
    }
    std::string grant_error;
    if (!ApplyMediaUploadGrantsLocal(media_key, uid, GrantSpecFromSessionLocal(session), &grant_error))
    {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = grant_error;
        return result;
    }

    std::filesystem::remove(SessionPathForLocal(upload_id), ec);
    std::filesystem::remove_all(chunk_dir, ec);

    memochat::media::MediaUploadAssetResponseDto response_dto;
    response_dto.media_key = media_key;
    response_dto.media_type = media_type;
    response_dto.file_name = file_name;
    response_dto.mime = mime;
    response_dto.size = static_cast<int64_t>(merged_size);
    response_dto.url = std::string("/media/download?asset=") + media_key;
    result.error = ErrorCodes::Success;
    result.data = memochat::media::MediaUploadAssetResponseToJsonValue(response_dto);
    return result;
}

MediaResult HandleUploadMediaSimple(const std::string& access_token,
                                    const std::string& media_type,
                                    const std::string& file_name,
                                    const std::string& mime,
                                    const std::string& data_base64,
                                    std::vector<int> grant_uids,
                                    int64_t grant_group_id,
                                    bool grant_public,
                                    bool grant_friends)
{
    MediaResult result;
    int uid = 0;
    if (file_name.empty() || data_base64.empty() || !ResolveAccessTokenUidLocal(access_token, uid))
    {
        result.error = ErrorCodes::TokenInvalid;
        result.message = "token invalid or params invalid";
        return result;
    }

    MediaConfigLocal cfg = LoadMediaConfigLocal();
    std::string safe_type = media_type.empty() ? "file" : media_type;
    const int64_t configured_limit = IsMediaTypeImageLocal(safe_type) ? cfg.max_image_bytes : cfg.max_file_bytes;
    const int64_t simple_decoded_limit = EffectiveSimpleJsonDecodedLimitLocal(configured_limit);
    if (ShouldRejectEncodedPayloadSizeLocal(static_cast<uint64_t>(data_base64.size()),
                                            static_cast<uint64_t>(simple_decoded_limit)))
    {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "encoded payload too large";
        result.data["limit_bytes"] = static_cast<int64_t>(simple_decoded_limit);
        return result;
    }

    std::string binary = DecodeBase64Local(data_base64);
    if (binary.empty())
    {
        result.error = ErrorCodes::Error_Json;
        result.message = "base64 decode failed or empty";
        return result;
    }

    if (static_cast<int64_t>(binary.size()) > simple_decoded_limit)
    {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "file too large";
        result.data["limit_bytes"] = static_cast<int64_t>(simple_decoded_limit);
        return result;
    }

    std::string safe_name = SanitizeFileNameLocal(file_name);
    std::string safe_mime = mime.empty() ? GuessContentTypeLocal(safe_name, "") : mime;
    std::string media_key;
    std::string id_error;
    if (!NewIdStringLocal(&media_key, &id_error))
    {
        memolog::LogError("media.upload.id_generation_failed",
                          "media identifier generation failed",
                          {{"uid", std::to_string(uid)}, {"error", id_error}});
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "generate media id failed";
        return result;
    }
    std::filesystem::path temp_dir = ChunkRootLocal() / ("legacy_" + media_key);
    std::error_code ec;
    std::filesystem::create_directories(temp_dir, ec);
    if (ec)
    {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "create temp dir failed";
        return result;
    }
    std::filesystem::path temp_file = temp_dir / "merged.tmp";
    {
        std::ofstream ofs(temp_file, std::ios::binary | std::ios::trunc);
        if (!ofs.is_open())
        {
            result.error = ErrorCodes::MediaUploadFailed;
            result.message = "open temp file failed";
            return result;
        }
        ofs.write(binary.data(), static_cast<std::streamsize>(binary.size()));
    }

    std::string storage_path;
    std::string storage_error;
    IMediaStorage& storage = MediaStorageForLocal(cfg.storage_provider);
    if (!storage.StoreMergedFile(safe_type, media_key, safe_name, temp_file, storage_path, storage_error))
    {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = storage_error.empty() ? "persist media failed" : storage_error;
        return result;
    }

    MediaAssetInfo asset;
    asset.media_key = media_key;
    asset.owner_uid = uid;
    asset.media_type = safe_type;
    asset.origin_file_name = safe_name;
    asset.mime = safe_mime;
    asset.size_bytes = static_cast<int64_t>(binary.size());
    asset.storage_provider = cfg.storage_provider;
    asset.storage_path = storage_path;
    asset.created_at_ms = NowMsLocal();
    asset.deleted_at_ms = 0;
    asset.status = 1;
    if (!PostgresMgr::GetInstance()->InsertMediaAsset(asset))
    {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "save media metadata failed";
        return result;
    }
    MediaUploadGrantSpecLocal grants;
    grants.grant_uids = std::move(grant_uids);
    grants.grant_group_id = grant_group_id;
    grants.grant_public = grant_public;
    grants.grant_friends = grant_friends;
    std::string grant_error;
    if (!ApplyMediaUploadGrantsLocal(media_key, uid, std::move(grants), &grant_error))
    {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = grant_error;
        return result;
    }

    std::filesystem::remove_all(temp_dir, ec);
    memochat::media::MediaUploadAssetResponseDto response_dto;
    response_dto.media_key = media_key;
    response_dto.media_type = safe_type;
    response_dto.file_name = safe_name;
    response_dto.mime = safe_mime;
    response_dto.size = static_cast<int64_t>(binary.size());
    response_dto.url = std::string("/media/download?asset=") + media_key;
    result.error = ErrorCodes::Success;
    result.data = memochat::media::MediaUploadAssetResponseToJsonValue(response_dto);
    return result;
}

MediaResult HandleMediaDownloadInfo(const std::string& access_token, const std::string& media_key)
{
    MediaResult result;
    int uid = 0;
    if (media_key.empty() || !ResolveAccessTokenUidLocal(access_token, uid))
    {
        result.error = ErrorCodes::TokenInvalid;
        result.message = "token invalid or params invalid";
        return result;
    }

    MediaAssetInfo asset;
    if (!PostgresMgr::GetInstance()->GetMediaAssetByKey(media_key, asset) || asset.status != 1 ||
        asset.deleted_at_ms > 0)
    {
        result.error = ErrorCodes::UidInvalid;
        result.message = "asset not found";
        return result;
    }
    if (asset.owner_uid != uid && !PostgresMgr::GetInstance()->HasMediaAccess(asset.media_id, uid))
    {
        result.error = ErrorCodes::UidInvalid;
        result.message = "media access denied";
        return result;
    }

    IMediaStorage& storage = MediaStorageForLocal(asset.storage_provider);

    std::vector<char> data;
    std::string ct_from_storage;
    std::string storage_err;
    if (storage.ReadObject(asset.storage_path, asset.media_type, data, ct_from_storage, storage_err))
    {
        result.error = ErrorCodes::Success;
        result.data["data"] = std::string(data.begin(), data.end());
        result.data["content_type"] = ct_from_storage.empty() ? "application/octet-stream" : ct_from_storage;
        return result;
    }

    result.error = ErrorCodes::UidInvalid;
    result.message = "file not found";
    return result;
}

} // namespace Http2MediaSupport
