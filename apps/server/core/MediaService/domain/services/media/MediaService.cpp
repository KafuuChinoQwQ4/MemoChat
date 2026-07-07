#include "services/media/MediaService.hpp"

#include "ConfigMgr.hpp"
#include "MediaStorage.hpp"
#include "services/media/MediaPersistence.hpp"
#include "services/media/MediaPublicDtos.hpp"
#include "services/media/MediaUploadSessionDto.hpp"
#include "RedisMgr.hpp"
#include "const.hpp"
#include "json/GlazeCompat.hpp"
#include "logging/Logger.hpp"
#include "support/BearerAccessAuth.hpp"

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

import memochat.media.config_algorithms;
import memochat.media.service_algorithms;

namespace memochat::gate::services::media
{
namespace
{

namespace service_modules = memochat::media::service::modules;

struct MediaUploadGrantSpecLocal
{
    std::vector<int> grant_uids;
    int64_t grant_group_id = 0;
    bool grant_public = false;
    bool grant_friends = false;
};

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
    if (service_modules::ShouldUseFallbackFileName(safe.empty()))
    {
        return service_modules::FallbackFileName();
    }
    if (service_modules::ShouldTrimSanitizedFileName(safe.size()))
    {
        safe = safe.substr(safe.size() - service_modules::MaxSanitizedFileNameLength());
    }
    return safe;
}

bool DecodeBase64Local(const std::string& input, std::string& output)
{
    static const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                     "abcdefghijklmnopqrstuvwxyz"
                                     "0123456789+/";

    std::vector<int> table(256, -1);
    for (int i = 0; i < static_cast<int>(chars.size()); ++i)
    {
        table[static_cast<unsigned char>(chars[i])] = i;
    }

    output.clear();
    int val = 0;
    int valb = -8;
    for (unsigned char c : input)
    {
        if (std::isspace(c))
        {
            continue;
        }
        if (c == '=')
        {
            break;
        }
        if (table[c] == -1)
        {
            return false;
        }
        val = (val << 6) + table[c];
        valb += 6;
        if (valb >= 0)
        {
            output.push_back(static_cast<char>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return true;
}

std::string GuessContentTypeLocal(const std::string& fileName, const std::string& mimeHint)
{
    if (!mimeHint.empty())
    {
        return mimeHint;
    }
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
    const auto it = extMap.find(ext);
    if (it != extMap.end())
    {
        return it->second;
    }
    return service_modules::DefaultContentType();
}

struct MediaConfigLocal
{
    int64_t max_image_bytes = 200LL * 1024 * 1024;
    int64_t max_file_bytes = 20480LL * 1024 * 1024;
    int chunk_size_bytes = 4 * 1024 * 1024;
    int session_expire_sec = 86400;
    std::string storage_provider = "local";
};

int64_t ParseConfigInt64Local(const std::string& raw, int64_t fallback)
{
    if (raw.empty())
    {
        return fallback;
    }
    try
    {
        const int64_t value = std::stoll(raw);
        if (value <= 0)
        {
            return fallback;
        }
        return value;
    }
    catch (...)
    {
        return fallback;
    }
}

int ParseConfigIntLocal(const std::string& raw, int fallback)
{
    if (raw.empty())
    {
        return fallback;
    }
    try
    {
        const int value = std::stoi(raw);
        if (value <= 0)
        {
            return fallback;
        }
        return value;
    }
    catch (...)
    {
        return fallback;
    }
}

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

int64_t NowMsLocal()
{
    return static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count());
}

std::filesystem::path UploadRootLocal()
{
    return std::filesystem::current_path() / "uploads";
}

std::filesystem::path SessionRootLocal()
{
    return UploadRootLocal() / "sessions";
}

std::filesystem::path ChunkRootLocal()
{
    return UploadRootLocal() / "chunks";
}

std::filesystem::path SessionPathForLocal(const std::string& upload_id)
{
    return SessionRootLocal() / (upload_id + ".json");
}

std::filesystem::path ChunkDirForLocal(const std::string& upload_id)
{
    return ChunkRootLocal() / upload_id;
}

bool SaveJsonFileLocal(const std::filesystem::path& path, const memochat::json::JsonValue& root)
{
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    if (ec)
    {
        return false;
    }
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    if (!ofs.is_open())
    {
        return false;
    }
    ofs << memochat::json::writeString(root);
    return ofs.good();
}

bool LoadJsonFileLocal(const std::filesystem::path& path, memochat::json::JsonValue& root)
{
    if (!std::filesystem::exists(path))
    {
        return false;
    }
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open())
    {
        return false;
    }
    const std::string json((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    memochat::json::JsonReader reader;
    if (!reader.parse(json, root))
    {
        return false;
    }
    if (root.isObject())
    {
        return true;
    }
    if (!root.isString())
    {
        return false;
    }

    memochat::json::JsonValue decoded;
    if (!reader.parse(root.asString(), decoded) || !decoded.isObject())
    {
        return false;
    }
    root = decoded;
    return true;
}

std::set<int> ListUploadedChunkIndexesLocal(const std::filesystem::path& chunk_dir)
{
    std::set<int> indexes;
    if (!std::filesystem::exists(chunk_dir))
    {
        return indexes;
    }
    for (const auto& entry : std::filesystem::directory_iterator(chunk_dir))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }
        const std::string stem = entry.path().stem().string();
        if (stem.empty())
        {
            continue;
        }
        bool digits = true;
        for (char c : stem)
        {
            if (!std::isdigit(static_cast<unsigned char>(c)))
            {
                digits = false;
                break;
            }
        }
        if (!digits)
        {
            continue;
        }
        try
        {
            indexes.insert(std::stoi(stem));
        }
        catch (...)
        {
        }
    }
    return indexes;
}

IMediaStorage& MediaStorageForLocal(const std::string& provider)
{
    (void) provider;
    return GetMediaStorage();
}

bool IsMediaTypeImageLocal(const std::string& media_type)
{
    return media_type == "image" || media_type == "moment_image";
}

bool IsAvatarMediaTypeLocal(const std::string& media_type)
{
    return media_type == "avatar";
}

std::string NewIdStringLocal()
{
    return boost::uuids::to_string(boost::uuids::random_generator()());
}

void WriteJson(memochat::gate::routing::GateResponse& response, const memochat::json::JsonValue& root)
{
    response.status = service_modules::SuccessHttpStatus();
    response.content_type = service_modules::JsonContentType();
    response.body_kind = memochat::gate::routing::GateResponseBodyKind::Inline;
    response.body = root.toStyledString();
}

bool ResolveBearerUidLocal(const memochat::gate::routing::GateRequest& request,
                           memochat::json::JsonValue& root,
                           int& uid,
                           const std::string& message = "token invalid or params invalid")
{
    if (!memochat::auth::ResolveBearerAccessUserId(request, uid))
    {
        root["error"] = ErrorCodes::TokenInvalid;
        root["message"] = message;
        return false;
    }
    return true;
}

std::string LowercaseAscii(std::string value)
{
    memochat::media::modules::LowerAsciiInPlace(value.data(), value.size());
    return value;
}

std::string HeaderValue(const memochat::gate::routing::GateRequest& request, const std::string& name)
{
    for (const auto& [key, value] : request.headers)
    {
        if (LowercaseAscii(key) == LowercaseAscii(name))
        {
            return value;
        }
    }
    return "";
}

std::string QueryValue(const memochat::gate::routing::GateRequest& request, const std::string& name)
{
    const auto it = request.query.find(name);
    if (it == request.query.end())
    {
        return "";
    }
    return it->second;
}

bool IsAllowedDownloadContentTypeLocal(const std::string& content_type)
{
    std::string lower = content_type;
    const auto semicolon = lower.find(';');
    if (semicolon != std::string::npos)
    {
        lower = lower.substr(0, semicolon);
    }
    lower = LowercaseAscii(lower);

    static const std::set<std::string> allowed = {"application/octet-stream",
                                                  "application/pdf",
                                                  "audio/mpeg",
                                                  "audio/mp4",
                                                  "audio/ogg",
                                                  "audio/wav",
                                                  "image/bmp",
                                                  "image/gif",
                                                  "image/jpeg",
                                                  "image/png",
                                                  "image/webp",
                                                  "text/plain",
                                                  "video/mp4",
                                                  "video/webm"};
    return allowed.find(lower) != allowed.end();
}

std::string ResolveDownloadContentTypeLocal(const std::string& content_type,
                                            const std::string& file_name,
                                            const std::string& mime_hint)
{
    const std::string chosen = service_modules::ShouldUseFallbackContentType(content_type.empty())
                                   ? GuessContentTypeLocal(file_name, mime_hint)
                                   : content_type;
    return IsAllowedDownloadContentTypeLocal(chosen) ? chosen : service_modules::DefaultContentType();
}

std::string ContentDispositionLocal(const std::string& file_name)
{
    return "attachment; filename=\"" + SanitizeFileNameLocal(file_name) + "\"";
}

void ApplySecureDownloadHeadersLocal(memochat::gate::routing::GateResponse& response, const std::string& file_name)
{
    response.headers["X-Content-Type-Options"] = "nosniff";
    response.headers["Content-Disposition"] = ContentDispositionLocal(file_name);
    response.headers["Cache-Control"] = "private, no-store";
}

MediaUploadGrantSpecLocal GrantSpecFromRequestLocal(const memochat::media::MediaUploadInitRequestDto& request)
{
    MediaUploadGrantSpecLocal grants;
    grants.grant_uids = request.grant_uids;
    grants.grant_group_id = request.grant_group_id;
    grants.grant_public = request.grant_public;
    grants.grant_friends = request.grant_friends;
    return grants;
}

MediaUploadGrantSpecLocal GrantSpecFromRequestLocal(const memochat::media::MediaUploadSimpleRequestDto& request)
{
    MediaUploadGrantSpecLocal grants;
    grants.grant_uids = request.grant_uids;
    grants.grant_group_id = request.grant_group_id;
    grants.grant_public = request.grant_public;
    grants.grant_friends = request.grant_friends;
    return grants;
}

MediaUploadGrantSpecLocal GrantSpecFromSessionLocal(const memochat::media::MediaUploadSessionDto& session)
{
    MediaUploadGrantSpecLocal grants;
    grants.grant_uids = session.grant_uids;
    grants.grant_group_id = session.grant_group_id;
    grants.grant_public = session.grant_public;
    grants.grant_friends = session.grant_friends;
    return grants;
}

bool HasAnyGrantLocal(const MediaUploadGrantSpecLocal& grants)
{
    return !grants.grant_uids.empty() || grants.grant_group_id > 0 || grants.grant_public || grants.grant_friends;
}

bool ApplyMediaUploadGrantsLocal(const std::string& media_key,
                                 int owner_uid,
                                 MediaUploadGrantSpecLocal grants,
                                 std::string* error_out)
{
    MediaAssetRecord persisted;
    if (!MediaPersistence::Instance().LoadAssetByKey(media_key, persisted) || persisted.owner_uid != owner_uid ||
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
        if (!MediaPersistence::Instance().GrantAccess(persisted.media_id, grantee_uid, "direct", now_ms))
        {
            if (error_out != nullptr)
            {
                *error_out = "grant media access failed";
            }
            return false;
        }
    }
    if (grants.grant_public && !MediaPersistence::Instance().GrantAccess(persisted.media_id, 0, "public", now_ms))
    {
        if (error_out != nullptr)
        {
            *error_out = "grant public media access failed";
        }
        return false;
    }
    // TODO(security/arch): grant_friends and grant_group_id resolution currently
    // relies on relation tables/projections being co-located with (or accessible
    // from) the MediaService database.  In a split-schema deployment these joins
    // may become unavailable, causing friend/group media to silently fall back to
    // no-grant.  Mitigation options (choose one at deployment design time):
    //   a) Keep relation projections in the MediaService schema via CDC replication.
    //   b) Introduce a RelationService RPC call to validate access at download time.
    //   c) Issue a signed, short-lived download token at upload-grant time and
    //      verify the token signature at download time (no cross-service join).
    // Track progress in: .ai/security-audit/a/plan.md §Residual Risks.
    if (grants.grant_friends && !MediaPersistence::Instance().GrantAccess(persisted.media_id, 0, "friends", now_ms))
    {
        if (error_out != nullptr)
        {
            *error_out = "grant friends media access failed";
        }
        return false;
    }
    if (grants.grant_group_id > 0 && !MediaPersistence::Instance().GrantGroupAccess(persisted.media_id,
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

} // namespace

MediaService& MediaService::Instance()
{
    static MediaService instance;
    return instance;
}

bool MediaService::HandleUploadMediaInit(const memochat::gate::routing::GateRequest& request,
                                         memochat::gate::routing::GateResponse& response)
{
    memochat::json::JsonValue root;
    memochat::media::MediaUploadInitRequestDto upload_request;
    if (!memochat::media::DecodeMediaUploadInitRequest(request.body, &upload_request))
    {
        root["error"] = ErrorCodes::Error_Json;
        root["message"] = "invalid json";
        WriteJson(response, root);
        return true;
    }

    int uid = 0;
    if (!ResolveBearerUidLocal(request, root, uid))
    {
        WriteJson(response, root);
        return true;
    }
    std::string media_type = upload_request.media_type;
    const std::string file_name = SanitizeFileNameLocal(upload_request.file_name);
    std::string mime = upload_request.mime;
    const int64_t file_size = upload_request.file_size;
    if (!service_modules::HasValidUploadInitRequest(uid, file_name.empty(), file_size, true))
    {
        root["error"] = ErrorCodes::TokenInvalid;
        root["message"] = "token invalid or params invalid";
        WriteJson(response, root);
        return true;
    }

    const MediaConfigLocal media_cfg = LoadMediaConfigLocal();
    const int64_t limit = IsMediaTypeImageLocal(media_type) ? media_cfg.max_image_bytes : media_cfg.max_file_bytes;
    if (service_modules::ShouldRejectFileSize(file_size, limit))
    {
        root["error"] = ErrorCodes::MediaUploadFailed;
        root["message"] = "file too large";
        root["limit_bytes"] = static_cast<int64_t>(limit);
        WriteJson(response, root);
        return true;
    }

    if (service_modules::ShouldGuessMime(mime.empty()))
    {
        mime = GuessContentTypeLocal(file_name, "");
    }
    const std::string upload_id = NewIdStringLocal();
    const int chunk_size = media_cfg.chunk_size_bytes;
    const int total_chunks = static_cast<int>((file_size + chunk_size - 1) / chunk_size);
    std::error_code ec;
    std::filesystem::create_directories(ChunkDirForLocal(upload_id), ec);
    if (ec)
    {
        root["error"] = ErrorCodes::MediaUploadFailed;
        root["message"] = "create chunk dir failed";
        WriteJson(response, root);
        return true;
    }

    memochat::media::MediaUploadSessionDto session;
    session.uid = uid;
    session.upload_id = upload_id;
    session.media_type = media_type;
    session.file_name = file_name;
    session.mime = mime;
    session.file_size = static_cast<int64_t>(file_size);
    session.chunk_size = chunk_size;
    session.total_chunks = total_chunks;
    session.created_at = static_cast<int64_t>(NowMsLocal());
    session.expires_at = static_cast<int64_t>(NowMsLocal() + static_cast<int64_t>(media_cfg.session_expire_sec) * 1000);
    session.storage_provider = media_cfg.storage_provider;
    const MediaUploadGrantSpecLocal grants = GrantSpecFromRequestLocal(upload_request);
    session.grant_uids = grants.grant_uids;
    session.grant_group_id = grants.grant_group_id;
    session.grant_public = grants.grant_public;
    session.grant_friends = grants.grant_friends;
    if (!SaveJsonFileLocal(SessionPathForLocal(upload_id), memochat::media::MediaUploadSessionToJsonValue(session)))
    {
        root["error"] = ErrorCodes::MediaUploadFailed;
        root["message"] = "create upload session failed";
        WriteJson(response, root);
        return true;
    }

    memochat::media::MediaUploadInitResponseDto response_dto;
    response_dto.upload_id = upload_id;
    response_dto.chunk_size = chunk_size;
    response_dto.total_chunks = total_chunks;
    root = memochat::media::MediaUploadInitResponseToJsonValue(response_dto);
    root["error"] = ErrorCodes::Success;
    WriteJson(response, root);
    return true;
}

bool MediaService::HandleUploadMediaChunk(const memochat::gate::routing::GateRequest& request,
                                          memochat::gate::routing::GateResponse& response)
{
    memochat::json::JsonValue root;
    int uid = 0;
    int index = -1;
    std::string upload_id;
    std::string binary;
    std::string encoded;
    bool json_payload = false;

    if (!ResolveBearerUidLocal(request, root, uid))
    {
        WriteJson(response, root);
        return true;
    }

    std::string content_type = HeaderValue(request, "Content-Type");
    if (memochat::media::IsJsonContentType(content_type))
    {
        json_payload = true;
        const MediaConfigLocal media_cfg = LoadMediaConfigLocal();
        if (service_modules::ShouldRejectRequestBodySize(static_cast<unsigned long long>(request.body.size()),
                                                         static_cast<unsigned long long>(media_cfg.chunk_size_bytes)))
        {
            root["error"] = ErrorCodes::MediaUploadFailed;
            root["message"] = service_modules::RequestBodyTooLargeMessage();
            root["limit_bytes"] = static_cast<int64_t>(media_cfg.chunk_size_bytes);
            WriteJson(response, root);
            return true;
        }

        memochat::media::MediaUploadChunkJsonRequestDto chunk_request;
        if (!memochat::media::DecodeMediaUploadChunkJsonRequest(request.body, &chunk_request))
        {
            root["error"] = ErrorCodes::Error_Json;
            root["message"] = "invalid json";
            WriteJson(response, root);
            return true;
        }

        upload_id = chunk_request.upload_id;
        index = chunk_request.index;
        encoded = chunk_request.data_base64;
    }
    else
    {
        upload_id = HeaderValue(request, "X-Upload-Id");
        index = std::atoi(HeaderValue(request, "X-Chunk-Index").c_str());
        binary = request.body;
    }

    const bool payload_empty = json_payload ? encoded.empty() : binary.empty();
    if (!service_modules::HasValidChunkUploadRequest(uid, upload_id.empty(), index, payload_empty, true))
    {
        root["error"] = ErrorCodes::TokenInvalid;
        root["message"] = "token invalid or params invalid";
        WriteJson(response, root);
        return true;
    }

    memochat::json::JsonValue session_json;
    memochat::media::MediaUploadSessionDto session;
    if (!LoadJsonFileLocal(SessionPathForLocal(upload_id), session_json) ||
        !memochat::media::MediaUploadSessionFromJsonValue(session_json, &session))
    {
        root["error"] = ErrorCodes::MediaUploadFailed;
        root["message"] = "upload session not found";
        WriteJson(response, root);
        return true;
    }
    if (session.uid != uid)
    {
        root["error"] = ErrorCodes::TokenInvalid;
        root["message"] = "session uid mismatch";
        WriteJson(response, root);
        return true;
    }
    const int64_t expires_at = session.expires_at;
    if (service_modules::IsExpiredSession(expires_at, NowMsLocal()))
    {
        root["error"] = ErrorCodes::MediaUploadFailed;
        root["message"] = "upload session expired";
        WriteJson(response, root);
        return true;
    }

    const int total_chunks = session.total_chunks;
    const int chunk_size = session.chunk_size;
    if (!service_modules::HasValidChunkIndex(index, total_chunks, chunk_size))
    {
        root["error"] = ErrorCodes::Error_Json;
        root["message"] = "invalid chunk index";
        WriteJson(response, root);
        return true;
    }

    if (json_payload)
    {
        if (service_modules::ShouldRejectEncodedPayloadSize(static_cast<unsigned long long>(encoded.size()),
                                                            static_cast<unsigned long long>(chunk_size)))
        {
            root["error"] = ErrorCodes::MediaUploadFailed;
            root["message"] = service_modules::EncodedPayloadTooLargeMessage();
            root["limit_bytes"] = static_cast<int64_t>(chunk_size);
            WriteJson(response, root);
            return true;
        }

        const bool decode_ok = DecodeBase64Local(encoded, binary);
        if (service_modules::IsInvalidEncodedPayload(encoded.empty(), decode_ok) || binary.empty())
        {
            root["error"] = ErrorCodes::Error_Json;
            root["message"] = "base64 decode failed";
            WriteJson(response, root);
            return true;
        }
    }

    if (service_modules::ExceedsChunkSize(static_cast<int>(binary.size()), chunk_size))
    {
        root["error"] = ErrorCodes::MediaUploadFailed;
        root["message"] = "invalid chunk size";
        WriteJson(response, root);
        return true;
    }

    const std::filesystem::path chunk_dir = ChunkDirForLocal(upload_id);
    std::error_code ec;
    std::filesystem::create_directories(chunk_dir, ec);
    if (ec)
    {
        root["error"] = ErrorCodes::MediaUploadFailed;
        root["message"] = "create chunk dir failed";
        WriteJson(response, root);
        return true;
    }

    const std::filesystem::path chunk_path = chunk_dir / (std::to_string(index) + ".part");
    std::ofstream ofs(chunk_path, std::ios::binary | std::ios::trunc);
    if (!ofs.is_open())
    {
        root["error"] = ErrorCodes::MediaUploadFailed;
        root["message"] = "write chunk failed";
        WriteJson(response, root);
        return true;
    }
    ofs.write(binary.data(), static_cast<std::streamsize>(binary.size()));
    ofs.close();

    memochat::media::MediaUploadChunkResponseDto response_dto;
    response_dto.upload_id = upload_id;
    response_dto.index = index;
    response_dto.size = static_cast<int64_t>(binary.size());
    root = memochat::media::MediaUploadChunkResponseToJsonValue(response_dto);
    root["error"] = ErrorCodes::Success;
    WriteJson(response, root);
    return true;
}

bool MediaService::HandleUploadMediaStatus(const memochat::gate::routing::GateRequest& request,
                                           memochat::gate::routing::GateResponse& response)
{
    memochat::json::JsonValue root;
    int uid = 0;
    if (!ResolveBearerUidLocal(request, root, uid))
    {
        WriteJson(response, root);
        return true;
    }
    const std::string upload_id = QueryValue(request, "upload_id");
    if (!service_modules::HasValidStatusAuth(uid, false, upload_id.empty(), true))
    {
        root["error"] = ErrorCodes::TokenInvalid;
        root["message"] = "token invalid or params invalid";
        WriteJson(response, root);
        return true;
    }

    memochat::json::JsonValue session_json;
    memochat::media::MediaUploadSessionDto session;
    if (!LoadJsonFileLocal(SessionPathForLocal(upload_id), session_json) ||
        !memochat::media::MediaUploadSessionFromJsonValue(session_json, &session))
    {
        root["error"] = ErrorCodes::MediaUploadFailed;
        root["message"] = "upload session not found";
        WriteJson(response, root);
        return true;
    }
    if (session.uid != uid)
    {
        root["error"] = ErrorCodes::TokenInvalid;
        root["message"] = "session uid mismatch";
        WriteJson(response, root);
        return true;
    }

    memochat::media::MediaUploadStatusResponseDto response_dto;
    response_dto.upload_id = upload_id;
    response_dto.total_chunks = session.total_chunks;
    response_dto.chunk_size = session.chunk_size;
    for (int idx : ListUploadedChunkIndexesLocal(ChunkDirForLocal(upload_id)))
    {
        response_dto.uploaded_chunks.push_back(idx);
    }
    root = memochat::media::MediaUploadStatusResponseToJsonValue(response_dto);
    root["error"] = ErrorCodes::Success;
    WriteJson(response, root);
    return true;
}

bool MediaService::HandleUploadMediaComplete(const memochat::gate::routing::GateRequest& request,
                                             memochat::gate::routing::GateResponse& response)
{
    memochat::json::JsonValue root;
    memochat::media::MediaUploadCompleteRequestDto upload_request;
    if (!memochat::media::DecodeMediaUploadCompleteRequest(request.body, &upload_request))
    {
        root["error"] = ErrorCodes::Error_Json;
        root["message"] = "invalid json";
        WriteJson(response, root);
        return true;
    }

    int uid = 0;
    if (!ResolveBearerUidLocal(request, root, uid))
    {
        WriteJson(response, root);
        return true;
    }
    const std::string upload_id = upload_request.upload_id;
    if (!service_modules::HasValidCompleteAuth(uid, false, upload_id.empty(), true))
    {
        root["error"] = ErrorCodes::TokenInvalid;
        root["message"] = "token invalid or params invalid";
        WriteJson(response, root);
        return true;
    }

    memochat::json::JsonValue session_json;
    memochat::media::MediaUploadSessionDto session;
    if (!LoadJsonFileLocal(SessionPathForLocal(upload_id), session_json) ||
        !memochat::media::MediaUploadSessionFromJsonValue(session_json, &session))
    {
        root["error"] = ErrorCodes::MediaUploadFailed;
        root["message"] = "upload session not found";
        WriteJson(response, root);
        return true;
    }
    if (session.uid != uid)
    {
        root["error"] = ErrorCodes::TokenInvalid;
        root["message"] = "session uid mismatch";
        WriteJson(response, root);
        return true;
    }

    const int total_chunks = session.total_chunks;
    const int chunk_size = session.chunk_size;
    const int64_t file_size = session.file_size;
    const std::string file_name = session.file_name;
    const std::string media_type = session.media_type;
    const std::string mime = session.mime;
    const std::string storage_provider = session.storage_provider;
    if (!service_modules::HasValidUploadSessionShape(total_chunks, chunk_size, file_size, file_name.empty()))
    {
        root["error"] = ErrorCodes::MediaUploadFailed;
        root["message"] = "invalid upload session";
        WriteJson(response, root);
        return true;
    }

    const std::filesystem::path chunk_dir = ChunkDirForLocal(upload_id);
    const std::set<int> uploaded = ListUploadedChunkIndexesLocal(chunk_dir);
    for (int i = 0; i < total_chunks; ++i)
    {
        if (uploaded.find(i) == uploaded.end())
        {
            root["error"] = ErrorCodes::MediaUploadFailed;
            root["message"] = "chunks not complete";
            root["missing_index"] = i;
            WriteJson(response, root);
            return true;
        }
    }

    const std::filesystem::path merged_path = chunk_dir / "merged.tmp";
    {
        std::ofstream merged(merged_path, std::ios::binary | std::ios::trunc);
        if (!merged.is_open())
        {
            root["error"] = ErrorCodes::MediaUploadFailed;
            root["message"] = "create merged file failed";
            WriteJson(response, root);
            return true;
        }
        for (int i = 0; i < total_chunks; ++i)
        {
            const std::filesystem::path one_path = chunk_dir / (std::to_string(i) + ".part");
            std::ifstream one(one_path, std::ios::binary);
            if (!one.is_open())
            {
                root["error"] = ErrorCodes::MediaUploadFailed;
                root["message"] = "open chunk failed";
                root["chunk_index"] = i;
                WriteJson(response, root);
                return true;
            }
            merged << one.rdbuf();
        }
        merged.flush();
        if (!merged.good())
        {
            root["error"] = ErrorCodes::MediaUploadFailed;
            root["message"] = "merge chunks failed";
            WriteJson(response, root);
            return true;
        }
    }

    std::error_code ec;
    const int64_t merged_size = static_cast<int64_t>(std::filesystem::file_size(merged_path, ec));
    if (ec || merged_size != file_size)
    {
        root["error"] = ErrorCodes::MediaUploadFailed;
        root["message"] = "merged file size mismatch";
        WriteJson(response, root);
        return true;
    }

    const std::string media_key = NewIdStringLocal();
    std::string storage_path;
    std::string storage_error;
    IMediaStorage& storage = MediaStorageForLocal(storage_provider);
    if (!storage.StoreMergedFile(media_type, media_key, file_name, merged_path, storage_path, storage_error))
    {
        root["error"] = ErrorCodes::MediaUploadFailed;
        root["message"] = service_modules::ShouldUseFallbackStorageMessage(storage_error.empty())
                              ? service_modules::PersistMediaFailedMessage()
                              : storage_error;
        WriteJson(response, root);
        return true;
    }

    MediaAssetRecord asset;
    asset.media_key = media_key;
    asset.owner_uid = uid;
    asset.media_type = media_type;
    asset.origin_file_name = file_name;
    asset.mime = mime;
    asset.size_bytes = merged_size;
    asset.storage_provider = storage_provider;
    asset.storage_path = storage_path;
    asset.created_at_ms = NowMsLocal();
    asset.deleted_at_ms = service_modules::NotDeletedTimestamp();
    asset.status = service_modules::ActiveAssetStatus();
    if (!MediaPersistence::Instance().SaveAsset(asset))
    {
        root["error"] = ErrorCodes::MediaUploadFailed;
        root["message"] = "save media metadata failed";
        WriteJson(response, root);
        return true;
    }
    std::string grant_error;
    if (!ApplyMediaUploadGrantsLocal(media_key, uid, GrantSpecFromSessionLocal(session), &grant_error))
    {
        root["error"] = ErrorCodes::MediaUploadFailed;
        root["message"] = grant_error;
        WriteJson(response, root);
        return true;
    }

    std::filesystem::remove(SessionPathForLocal(upload_id), ec);
    std::filesystem::remove_all(chunk_dir, ec);

    memochat::media::MediaUploadAssetResponseDto response_dto;
    response_dto.media_key = media_key;
    response_dto.media_type = media_type;
    response_dto.file_name = file_name;
    response_dto.mime = mime;
    response_dto.size = static_cast<int64_t>(merged_size);
    response_dto.url = std::string(service_modules::MediaDownloadUrlPrefix()) + media_key;
    root = memochat::media::MediaUploadAssetResponseToJsonValue(response_dto);
    root["error"] = ErrorCodes::Success;
    WriteJson(response, root);
    return true;
}

bool MediaService::HandleUploadMediaSimple(const memochat::gate::routing::GateRequest& request,
                                           memochat::gate::routing::GateResponse& response)
{
    memochat::json::JsonValue root;
    const MediaConfigLocal media_cfg = LoadMediaConfigLocal();
    const int64_t simple_body_decoded_limit =
        service_modules::EffectiveSimpleJsonDecodedLimit(media_cfg.max_file_bytes);
    if (service_modules::ShouldRejectRequestBodySize(static_cast<unsigned long long>(request.body.size()),
                                                     static_cast<unsigned long long>(simple_body_decoded_limit)))
    {
        root["error"] = ErrorCodes::MediaUploadFailed;
        root["message"] = service_modules::RequestBodyTooLargeMessage();
        root["limit_bytes"] = static_cast<int64_t>(simple_body_decoded_limit);
        WriteJson(response, root);
        return true;
    }

    memochat::media::MediaUploadSimpleRequestDto upload_request;
    if (!memochat::media::DecodeMediaUploadSimpleRequest(request.body, &upload_request))
    {
        root["error"] = ErrorCodes::Error_Json;
        root["message"] = "invalid json";
        WriteJson(response, root);
        return true;
    }

    int uid = 0;
    if (!ResolveBearerUidLocal(request, root, uid))
    {
        WriteJson(response, root);
        return true;
    }
    std::string media_type = upload_request.media_type;
    const std::string file_name = SanitizeFileNameLocal(upload_request.file_name);
    std::string mime = upload_request.mime;
    const std::string encoded = upload_request.data_base64;
    if (!service_modules::HasValidSimpleUploadRequest(uid, file_name.empty(), encoded.empty(), true))
    {
        root["error"] = ErrorCodes::TokenInvalid;
        root["message"] = "token invalid or params invalid";
        WriteJson(response, root);
        return true;
    }

    const int64_t configured_limit =
        IsMediaTypeImageLocal(media_type) ? media_cfg.max_image_bytes : media_cfg.max_file_bytes;
    const int64_t simple_decoded_limit = service_modules::EffectiveSimpleJsonDecodedLimit(configured_limit);
    if (service_modules::ShouldRejectEncodedPayloadSize(static_cast<unsigned long long>(encoded.size()),
                                                        static_cast<unsigned long long>(simple_decoded_limit)))
    {
        root["error"] = ErrorCodes::MediaUploadFailed;
        root["message"] = service_modules::EncodedPayloadTooLargeMessage();
        root["limit_bytes"] = static_cast<int64_t>(simple_decoded_limit);
        WriteJson(response, root);
        return true;
    }

    std::string binary;
    if (!DecodeBase64Local(encoded, binary))
    {
        root["error"] = ErrorCodes::Error_Json;
        root["message"] = "base64 decode failed";
        WriteJson(response, root);
        return true;
    }
    if (service_modules::ShouldRejectEmptyBinary(binary.empty()))
    {
        root["error"] = ErrorCodes::MediaUploadFailed;
        root["message"] = "file empty";
        WriteJson(response, root);
        return true;
    }

    if (service_modules::ShouldRejectFileSize(static_cast<int64_t>(binary.size()), simple_decoded_limit))
    {
        root["error"] = ErrorCodes::MediaUploadFailed;
        root["message"] = "file too large";
        root["limit_bytes"] = static_cast<int64_t>(simple_decoded_limit);
        WriteJson(response, root);
        return true;
    }
    if (service_modules::ShouldGuessMime(mime.empty()))
    {
        mime = GuessContentTypeLocal(file_name, "");
    }

    const std::string media_key = NewIdStringLocal();
    const std::filesystem::path temp_dir = ChunkRootLocal() / ("legacy_" + media_key);
    std::error_code ec;
    std::filesystem::create_directories(temp_dir, ec);
    if (ec)
    {
        root["error"] = ErrorCodes::MediaUploadFailed;
        root["message"] = "create temp dir failed";
        WriteJson(response, root);
        return true;
    }
    const std::filesystem::path temp_file = temp_dir / "merged.tmp";
    {
        std::ofstream ofs(temp_file, std::ios::binary | std::ios::trunc);
        if (!ofs.is_open())
        {
            root["error"] = ErrorCodes::MediaUploadFailed;
            root["message"] = "open temp file failed";
            WriteJson(response, root);
            return true;
        }
        ofs.write(binary.data(), static_cast<std::streamsize>(binary.size()));
    }

    std::string storage_path;
    std::string storage_error;
    IMediaStorage& storage = MediaStorageForLocal(media_cfg.storage_provider);
    if (!storage.StoreMergedFile(media_type, media_key, file_name, temp_file, storage_path, storage_error))
    {
        root["error"] = ErrorCodes::MediaUploadFailed;
        root["message"] = service_modules::ShouldUseFallbackStorageMessage(storage_error.empty())
                              ? service_modules::PersistMediaFailedMessage()
                              : storage_error;
        WriteJson(response, root);
        return true;
    }

    MediaAssetRecord asset;
    asset.media_key = media_key;
    asset.owner_uid = uid;
    asset.media_type = media_type;
    asset.origin_file_name = file_name;
    asset.mime = mime;
    asset.size_bytes = static_cast<int64_t>(binary.size());
    asset.storage_provider = media_cfg.storage_provider;
    asset.storage_path = storage_path;
    asset.created_at_ms = NowMsLocal();
    asset.deleted_at_ms = service_modules::NotDeletedTimestamp();
    asset.status = service_modules::ActiveAssetStatus();
    if (!MediaPersistence::Instance().SaveAsset(asset))
    {
        root["error"] = ErrorCodes::MediaUploadFailed;
        root["message"] = "save media metadata failed";
        WriteJson(response, root);
        return true;
    }
    std::string grant_error;
    if (!ApplyMediaUploadGrantsLocal(media_key, uid, GrantSpecFromRequestLocal(upload_request), &grant_error))
    {
        root["error"] = ErrorCodes::MediaUploadFailed;
        root["message"] = grant_error;
        WriteJson(response, root);
        return true;
    }

    std::filesystem::remove_all(temp_dir, ec);
    memochat::media::MediaUploadAssetResponseDto response_dto;
    response_dto.media_key = media_key;
    response_dto.media_type = media_type;
    response_dto.file_name = file_name;
    response_dto.mime = mime;
    response_dto.size = static_cast<int64_t>(binary.size());
    response_dto.url = std::string(service_modules::MediaDownloadUrlPrefix()) + media_key;
    root = memochat::media::MediaUploadAssetResponseToJsonValue(response_dto);
    root["error"] = ErrorCodes::Success;
    WriteJson(response, root);
    return true;
}

bool MediaService::HandleMediaDownload(const memochat::gate::routing::GateRequest& request,
                                       memochat::gate::routing::GateResponse& response)
{
    memochat::json::JsonValue root;
    const std::string media_key = QueryValue(request, "asset");
    const std::string legacy_file = QueryValue(request, "file");
    if (service_modules::ShouldRejectLegacyFileDownload(legacy_file.empty()))
    {
        root["error"] = ErrorCodes::Error_Json;
        root["message"] = service_modules::LegacyFileDownloadDisabledMessage();
        WriteJson(response, root);
        return true;
    }

    if (!service_modules::HasDownloadLocator(media_key.empty(), legacy_file.empty()))
    {
        root["error"] = ErrorCodes::Error_Json;
        root["message"] = "missing media key";
        WriteJson(response, root);
        return true;
    }

    int uid = 0;
    if (!ResolveBearerUidLocal(request, root, uid, "token invalid") ||
        !service_modules::HasValidDownloadAuth(uid, true))
    {
        root["error"] = ErrorCodes::TokenInvalid;
        root["message"] = "token invalid";
        WriteJson(response, root);
        return true;
    }

    std::filesystem::path full_path;
    std::string content_type = service_modules::DefaultContentType();
    if (!media_key.empty())
    {
        MediaAssetRecord asset;
        const bool asset_loaded = MediaPersistence::Instance().LoadAssetByKey(media_key, asset);
        if (!service_modules::IsReadableAsset(asset_loaded, asset.status, asset.deleted_at_ms))
        {
            root["error"] = ErrorCodes::UidInvalid;
            root["message"] = "asset not found";
            WriteJson(response, root);
            return true;
        }

        if (!MediaPersistence::Instance().CanReadAsset(asset, uid))
        {
            memolog::LogWarn("media.download.access_denied",
                             "media download denied",
                             {{"uid", std::to_string(uid)},
                              {"owner_uid", std::to_string(asset.owner_uid)},
                              {"media_id", std::to_string(asset.media_id)}});
            root["error"] = ErrorCodes::UidInvalid;
            root["message"] = "media access denied";
            WriteJson(response, root);
            return true;
        }

        IMediaStorage& storage = MediaStorageForLocal(asset.storage_provider);

        std::vector<char> data;
        std::string ct_from_storage;
        std::string storage_err;
        memolog::LogInfo("media.download.attempt",
                         "MediaService /media/download ReadObject attempt: media_id=" + std::to_string(asset.media_id) +
                             " storage_provider=" + asset.storage_provider);
        if (storage.ReadObject(asset.storage_path, asset.media_type, data, ct_from_storage, storage_err))
        {
            memolog::LogInfo(
                "media.download.ok",
                "MediaService /media/download ReadObject success: media_id=" + std::to_string(asset.media_id) +
                    " data_size=" + std::to_string(data.size()) + " content_type=" + ct_from_storage);
            response.status = service_modules::SuccessHttpStatus();
            response.content_type =
                ResolveDownloadContentTypeLocal(ct_from_storage, asset.origin_file_name, asset.mime);
            response.body_kind = memochat::gate::routing::GateResponseBodyKind::Inline;
            response.body.assign(data.data(), data.size());
            ApplySecureDownloadHeadersLocal(response, asset.origin_file_name);
            return true;
        }

        memolog::LogWarn("media.download.read_failed",
                         "MediaService /media/download ReadObject failed: media_id=" + std::to_string(asset.media_id) +
                             " storage_err=" + storage_err);

        std::string public_url;
        const bool public_url_resolved = storage.ResolvePublicUrl(asset.storage_path, asset.media_type, public_url);
        if (service_modules::ShouldRedirectToPublicUrl(public_url_resolved, public_url.empty()))
        {
            response.status = service_modules::RedirectHttpStatus();
            response.headers["Location"] = public_url;
            response.content_type = service_modules::PlainTextContentType();
            response.body_kind = memochat::gate::routing::GateResponseBodyKind::Inline;
            response.body = service_modules::RedirectBody();
            return true;
        }

        if (!storage.ResolveReadPath(asset.storage_path, full_path) || !std::filesystem::exists(full_path))
        {
            root["error"] = ErrorCodes::UidInvalid;
            root["message"] = service_modules::FileNotFoundMessage();
            WriteJson(response, root);
            return true;
        }
        content_type = ResolveDownloadContentTypeLocal("", asset.origin_file_name, asset.mime);
        ApplySecureDownloadHeadersLocal(response, asset.origin_file_name);
    }

    response.status = service_modules::SuccessHttpStatus();
    response.content_type = content_type;
    response.body_kind = memochat::gate::routing::GateResponseBodyKind::File;
    response.file_path = full_path.string();
    return true;
}

} // namespace memochat::gate::services::media
