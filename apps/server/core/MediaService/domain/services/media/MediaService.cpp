#include "services/media/MediaService.h"

#include "ConfigMgr.h"
#include "MediaStorage.h"
#include "services/media/MediaPublicDtos.h"
#include "services/media/MediaUploadSessionDto.h"
#include "PostgresMgr.h"
#include "RedisMgr.h"
#include "const.h"
#include "json/GlazeCompat.h"
#include "logging/Logger.h"
#include "support/UserTokenValidator.h"

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

namespace memochat::gate::services::media
{
namespace
{

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
    {
        return "file.bin";
    }
    if (safe.size() > 96)
    {
        safe = safe.substr(safe.size() - 96);
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
        std::transform(ext.begin(),
                       ext.end(),
                       ext.begin(),
                       [](unsigned char c)
                       {
                           return static_cast<char>(std::tolower(c));
                       });
    }
    const auto it = extMap.find(ext);
    if (it != extMap.end())
    {
        return it->second;
    }
    return "application/octet-stream";
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
    if (cfg.chunk_size_bytes < 256 * 1024)
    {
        cfg.chunk_size_bytes = 256 * 1024;
    }
    if (cfg.chunk_size_bytes > 4 * 1024 * 1024)
    {
        cfg.chunk_size_bytes = 4 * 1024 * 1024;
    }
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

bool ResolveLegacyMediaPathLocal(const std::string& legacy_file, std::filesystem::path& full_path)
{
    if (legacy_file.empty())
    {
        return false;
    }

    const std::filesystem::path rel = std::filesystem::path(legacy_file).lexically_normal();
    if (rel.is_absolute())
    {
        return false;
    }
    const std::string rel_str = rel.generic_string();
    if (rel_str.find("..") != std::string::npos)
    {
        return false;
    }

    std::error_code ec;
    const std::filesystem::path upload_root = UploadRootLocal();
    const std::filesystem::path by_relative = upload_root / rel;
    if (std::filesystem::exists(by_relative, ec) && !ec && std::filesystem::is_regular_file(by_relative, ec))
    {
        full_path = by_relative;
        return true;
    }
    ec.clear();

    const std::filesystem::path by_filename = upload_root / rel.filename();
    if (std::filesystem::exists(by_filename, ec) && !ec && std::filesystem::is_regular_file(by_filename, ec))
    {
        full_path = by_filename;
        return true;
    }
    ec.clear();

    const std::filesystem::path asset_root = upload_root / "assets";
    if (!std::filesystem::exists(asset_root, ec) || ec)
    {
        return false;
    }
    for (const auto& entry : std::filesystem::recursive_directory_iterator(asset_root, ec))
    {
        if (ec)
        {
            return false;
        }
        if (!entry.is_regular_file())
        {
            continue;
        }
        if (entry.path().filename() == rel.filename())
        {
            full_path = entry.path();
            return true;
        }
    }
    return false;
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

bool ValidateUserTokenLocal(int uid, const std::string& token)
{
    return memochat::auth::ValidateUserToken(uid, token);
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

std::string NewIdStringLocal()
{
    return boost::uuids::to_string(boost::uuids::random_generator()());
}

void WriteJson(memochat::gate::routing::GateResponse& response, const memochat::json::JsonValue& root)
{
    response.status = 200;
    response.content_type = "text/json";
    response.body_kind = memochat::gate::routing::GateResponseBodyKind::Inline;
    response.body = root.toStyledString();
}

std::string LowercaseAscii(std::string value)
{
    std::transform(value.begin(),
                   value.end(),
                   value.begin(),
                   [](unsigned char c)
                   {
                       return static_cast<char>(std::tolower(c));
                   });
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

    const int uid = upload_request.uid;
    const std::string token = upload_request.token;
    std::string media_type = upload_request.media_type;
    const std::string file_name = SanitizeFileNameLocal(upload_request.file_name);
    std::string mime = upload_request.mime;
    const int64_t file_size = upload_request.file_size;
    if (uid <= 0 || file_name.empty() || file_size <= 0 || !ValidateUserTokenLocal(uid, token))
    {
        root["error"] = ErrorCodes::TokenInvalid;
        root["message"] = "token invalid or params invalid";
        WriteJson(response, root);
        return true;
    }

    const MediaConfigLocal media_cfg = LoadMediaConfigLocal();
    const int64_t limit = IsMediaTypeImageLocal(media_type) ? media_cfg.max_image_bytes : media_cfg.max_file_bytes;
    if (file_size > limit)
    {
        root["error"] = ErrorCodes::MediaUploadFailed;
        root["message"] = "file too large";
        root["limit_bytes"] = static_cast<int64_t>(limit);
        WriteJson(response, root);
        return true;
    }

    if (mime.empty())
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
    std::string token;
    std::string upload_id;
    std::string binary;

    std::string content_type = HeaderValue(request, "Content-Type");
    if (memochat::media::IsJsonContentType(content_type))
    {
        memochat::media::MediaUploadChunkJsonRequestDto chunk_request;
        if (!memochat::media::DecodeMediaUploadChunkJsonRequest(request.body, &chunk_request))
        {
            root["error"] = ErrorCodes::Error_Json;
            root["message"] = "invalid json";
            WriteJson(response, root);
            return true;
        }

        uid = chunk_request.uid;
        token = chunk_request.token;
        upload_id = chunk_request.upload_id;
        index = chunk_request.index;
        const std::string encoded = chunk_request.data_base64;
        if (encoded.empty() || !DecodeBase64Local(encoded, binary))
        {
            root["error"] = ErrorCodes::Error_Json;
            root["message"] = "base64 decode failed";
            WriteJson(response, root);
            return true;
        }
    }
    else
    {
        uid = std::atoi(HeaderValue(request, "X-Uid").c_str());
        token = HeaderValue(request, "X-Token");
        upload_id = HeaderValue(request, "X-Upload-Id");
        index = std::atoi(HeaderValue(request, "X-Chunk-Index").c_str());
        binary = request.body;
    }

    if (uid <= 0 || upload_id.empty() || index < 0 || binary.empty() || !ValidateUserTokenLocal(uid, token))
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
    if (expires_at > 0 && NowMsLocal() > expires_at)
    {
        root["error"] = ErrorCodes::MediaUploadFailed;
        root["message"] = "upload session expired";
        WriteJson(response, root);
        return true;
    }

    const int total_chunks = session.total_chunks;
    const int chunk_size = session.chunk_size;
    if (index >= total_chunks || total_chunks <= 0 || chunk_size <= 0)
    {
        root["error"] = ErrorCodes::Error_Json;
        root["message"] = "invalid chunk index";
        WriteJson(response, root);
        return true;
    }

    if (static_cast<int>(binary.size()) > chunk_size)
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
    const int uid = std::atoi(QueryValue(request, "uid").c_str());
    const std::string token = QueryValue(request, "token");
    const std::string upload_id = QueryValue(request, "upload_id");
    if (uid <= 0 || token.empty() || upload_id.empty() || !ValidateUserTokenLocal(uid, token))
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

    const int uid = upload_request.uid;
    const std::string token = upload_request.token;
    const std::string upload_id = upload_request.upload_id;
    if (uid <= 0 || token.empty() || upload_id.empty() || !ValidateUserTokenLocal(uid, token))
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
    if (total_chunks <= 0 || chunk_size <= 0 || file_size <= 0 || file_name.empty())
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
        root["message"] = storage_error.empty() ? "persist media failed" : storage_error;
        WriteJson(response, root);
        return true;
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
        root["error"] = ErrorCodes::MediaUploadFailed;
        root["message"] = "save media metadata failed";
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
    response_dto.url = std::string("/media/download?asset=") + media_key;
    root = memochat::media::MediaUploadAssetResponseToJsonValue(response_dto);
    root["error"] = ErrorCodes::Success;
    WriteJson(response, root);
    return true;
}

bool MediaService::HandleUploadMediaSimple(const memochat::gate::routing::GateRequest& request,
                                           memochat::gate::routing::GateResponse& response)
{
    memochat::json::JsonValue root;
    memochat::media::MediaUploadSimpleRequestDto upload_request;
    if (!memochat::media::DecodeMediaUploadSimpleRequest(request.body, &upload_request))
    {
        root["error"] = ErrorCodes::Error_Json;
        root["message"] = "invalid json";
        WriteJson(response, root);
        return true;
    }

    const int uid = upload_request.uid;
    const std::string token = upload_request.token;
    std::string media_type = upload_request.media_type;
    const std::string file_name = SanitizeFileNameLocal(upload_request.file_name);
    std::string mime = upload_request.mime;
    const std::string encoded = upload_request.data_base64;
    if (uid <= 0 || file_name.empty() || encoded.empty() || !ValidateUserTokenLocal(uid, token))
    {
        root["error"] = ErrorCodes::TokenInvalid;
        root["message"] = "token invalid or params invalid";
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
    if (binary.empty())
    {
        root["error"] = ErrorCodes::MediaUploadFailed;
        root["message"] = "file empty";
        WriteJson(response, root);
        return true;
    }

    const MediaConfigLocal media_cfg = LoadMediaConfigLocal();
    const int64_t limit = IsMediaTypeImageLocal(media_type) ? media_cfg.max_image_bytes : media_cfg.max_file_bytes;
    if (static_cast<int64_t>(binary.size()) > limit)
    {
        root["error"] = ErrorCodes::MediaUploadFailed;
        root["message"] = "file too large";
        root["limit_bytes"] = static_cast<int64_t>(limit);
        WriteJson(response, root);
        return true;
    }
    if (mime.empty())
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
        root["message"] = storage_error.empty() ? "persist media failed" : storage_error;
        WriteJson(response, root);
        return true;
    }

    MediaAssetInfo asset;
    asset.media_key = media_key;
    asset.owner_uid = uid;
    asset.media_type = media_type;
    asset.origin_file_name = file_name;
    asset.mime = mime;
    asset.size_bytes = static_cast<int64_t>(binary.size());
    asset.storage_provider = media_cfg.storage_provider;
    asset.storage_path = storage_path;
    asset.created_at_ms = NowMsLocal();
    asset.deleted_at_ms = 0;
    asset.status = 1;
    if (!PostgresMgr::GetInstance()->InsertMediaAsset(asset))
    {
        root["error"] = ErrorCodes::MediaUploadFailed;
        root["message"] = "save media metadata failed";
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
    response_dto.url = std::string("/media/download?asset=") + media_key;
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
    const std::string uid_raw = QueryValue(request, "uid");
    const std::string token = QueryValue(request, "token");
    if ((media_key.empty() && legacy_file.empty()) || uid_raw.empty() || token.empty())
    {
        root["error"] = ErrorCodes::Error_Json;
        root["message"] = "missing media key or auth params";
        WriteJson(response, root);
        return true;
    }

    const int uid = std::atoi(uid_raw.c_str());
    if (uid <= 0 || !ValidateUserTokenLocal(uid, token))
    {
        root["error"] = ErrorCodes::TokenInvalid;
        root["message"] = "token invalid";
        WriteJson(response, root);
        return true;
    }

    std::filesystem::path full_path;
    std::string content_type = "application/octet-stream";
    if (!media_key.empty())
    {
        MediaAssetInfo asset;
        if (!PostgresMgr::GetInstance()->GetMediaAssetByKey(media_key, asset) || asset.status != 1 ||
            asset.deleted_at_ms > 0)
        {
            root["error"] = ErrorCodes::UidInvalid;
            root["message"] = "asset not found";
            WriteJson(response, root);
            return true;
        }

        // Defense-in-depth audit: media_key is an unguessable UUIDv4 capability and chat/moments
        // recipients legitimately fetch the sender's asset using their own uid/token (there is no
        // share table, only owner_uid). A mismatch is audited, NOT blocked — blocking would break
        // every received image. See docs/media-access-control.md.
        if (asset.owner_uid != uid)
        {
            memolog::LogWarn("media.download.cross_owner",
                             "cross-owner media download",
                             {{"uid", std::to_string(uid)},
                              {"owner_uid", std::to_string(asset.owner_uid)},
                              {"media_key", media_key}});
        }

        IMediaStorage& storage = MediaStorageForLocal(asset.storage_provider);

        std::vector<char> data;
        std::string ct_from_storage;
        std::string storage_err;
        memolog::LogInfo("media.download.attempt",
                         "MediaService /media/download ReadObject attempt: media_key=" + media_key +
                             " storage_provider=" + asset.storage_provider + " storage_path=" + asset.storage_path);
        if (storage.ReadObject(asset.storage_path, asset.media_type, data, ct_from_storage, storage_err))
        {
            memolog::LogInfo("media.download.ok",
                             "MediaService /media/download ReadObject success: media_key=" + media_key +
                                 " data_size=" + std::to_string(data.size()) + " content_type=" + ct_from_storage);
            response.status = 200;
            response.content_type = ct_from_storage.empty() ? "application/octet-stream" : ct_from_storage;
            response.body_kind = memochat::gate::routing::GateResponseBodyKind::Inline;
            response.body.assign(data.data(), data.size());
            return true;
        }

        memolog::LogWarn("media.download.read_failed",
                         "MediaService /media/download ReadObject failed: media_key=" + media_key +
                             " storage_err=" + storage_err + " storage_path=" + asset.storage_path);

        std::string public_url;
        if (storage.ResolvePublicUrl(asset.storage_path, asset.media_type, public_url) && !public_url.empty())
        {
            response.status = 307;
            response.headers["Location"] = public_url;
            response.content_type = "text/plain";
            response.body_kind = memochat::gate::routing::GateResponseBodyKind::Inline;
            response.body = "redirecting to object storage";
            return true;
        }

        if (!storage.ResolveReadPath(asset.storage_path, full_path) || !std::filesystem::exists(full_path))
        {
            root["error"] = ErrorCodes::UidInvalid;
            root["message"] = storage_err.empty() ? "file not found" : storage_err;
            WriteJson(response, root);
            return true;
        }
        content_type = GuessContentTypeLocal(asset.origin_file_name, asset.mime);
    }
    else
    {
        if (!ResolveLegacyMediaPathLocal(legacy_file, full_path) || !std::filesystem::exists(full_path))
        {
            root["error"] = ErrorCodes::UidInvalid;
            root["message"] = "legacy file not found";
            WriteJson(response, root);
            return true;
        }
        content_type = GuessContentTypeLocal(full_path.filename().string(), "");
    }

    response.status = 200;
    response.content_type = content_type;
    response.body_kind = memochat::gate::routing::GateResponseBodyKind::File;
    response.file_path = full_path.string();
    return true;
}

} // namespace memochat::gate::services::media
