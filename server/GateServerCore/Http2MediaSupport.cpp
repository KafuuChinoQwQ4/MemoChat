#include "Http2MediaSupport.h"
#include "ConfigMgr.h"
#include "MediaStorage.h"
#include "PostgresMgr.h"
#include "RedisMgr.h"
#include "const.h"
#include "logging/Logger.h"

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace {

int64_t NowMsLocal() {
    return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

std::string SanitizeFileNameLocal(const std::string& fileName) {
    std::string safe;
    safe.reserve(fileName.size());
    for (char c : fileName) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == '.') {
            safe.push_back(c);
        } else {
            safe.push_back('_');
        }
    }
    if (safe.empty()) return "file.bin";
    if (safe.size() > 96) safe = safe.substr(safe.size() - 96);
    return safe;
}

std::string DecodeBase64Local(const std::string& input) {
    static const std::string chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
    std::vector<int> table(256, -1);
    for (int i = 0; i < static_cast<int>(chars.size()); ++i)
        table[static_cast<unsigned char>(chars[i])] = i;
    std::string output;
    int val = 0;
    int valb = -8;
    for (unsigned char c : input) {
        if (std::isspace(c)) continue;
        if (c == '=') break;
        if (table[c] == -1) return "";
        val = (val << 6) + table[c];
        valb += 6;
        if (valb >= 0) {
            output.push_back(static_cast<char>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return output;
}

std::string GuessContentTypeLocal(const std::string& fileName, const std::string& mimeHint) {
    if (!mimeHint.empty()) return mimeHint;
    static const std::unordered_map<std::string, std::string> extMap = {
        {".png", "image/png"}, {".jpg", "image/jpeg"}, {".jpeg", "image/jpeg"},
        {".webp", "image/webp"}, {".bmp", "image/bmp"}, {".gif", "image/gif"},
        {".txt", "text/plain"}, {".pdf", "application/pdf"}
    };
    std::string ext;
    const auto dotPos = fileName.find_last_of('.');
    if (dotPos != std::string::npos) {
        ext = fileName.substr(dotPos);
        std::transform(ext.begin(), ext.end(), ext.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    }
    auto it = extMap.find(ext);
    return (it != extMap.end()) ? it->second : "application/octet-stream";
}

std::filesystem::path UploadRootLocal() {
    return std::filesystem::current_path() / "uploads";
}

std::filesystem::path ChunkRootLocal() {
    return UploadRootLocal() / "chunks";
}

std::filesystem::path SessionRootLocal() {
    return UploadRootLocal() / "sessions";
}

std::filesystem::path SessionPathForLocal(const std::string& upload_id) {
    return SessionRootLocal() / (upload_id + ".json");
}

std::filesystem::path ChunkDirForLocal(const std::string& upload_id) {
    return ChunkRootLocal() / upload_id;
}

bool ValidateUserTokenLocal(int uid, const std::string& token) {
    if (uid <= 0 || token.empty()) return false;
    const std::string token_key = USERTOKENPREFIX + std::to_string(uid);
    std::string token_value;
    return RedisMgr::GetInstance()->Get(token_key, token_value)
        && !token_value.empty() && token_value == token;
}

std::set<int> ListUploadedChunkIndexesLocal(const std::filesystem::path& chunk_dir) {
    std::set<int> indexes;
    if (!std::filesystem::exists(chunk_dir)) return indexes;
    for (const auto& entry : std::filesystem::directory_iterator(chunk_dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string stem = entry.path().stem().string();
        if (stem.empty()) continue;
        bool digits = std::all_of(stem.begin(), stem.end(),
            [](char c) { return std::isdigit(static_cast<unsigned char>(c)); });
        if (!digits) continue;
        try { indexes.insert(std::stoi(stem)); } catch (...) {}
    }
    return indexes;
}

bool SaveJsonFileLocal(const std::filesystem::path& path, const Json::Value& root) {
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    if (ec) return false;
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) return false;
    ofs << root.toStyledString();
    return ofs.good();
}

bool LoadJsonFileLocal(const std::filesystem::path& path, Json::Value& root) {
    if (!std::filesystem::exists(path)) return false;
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) return false;
    std::string json((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    Json::Reader reader;
    return reader.parse(json, root);
}

IMediaStorage& MediaStorageForLocal(const std::string& provider) {
    static LocalMediaStorage local_storage;
    (void)provider;
    return local_storage;
}

std::string NewIdStringLocal() {
    return boost::uuids::to_string(boost::uuids::random_generator()());
}

int64_t ParseConfigInt64Local(const std::string& raw, int64_t fallback) {
    if (raw.empty()) return fallback;
    try {
        int64_t v = std::stoll(raw);
        return v <= 0 ? fallback : v;
    } catch (...) { return fallback; }
}

int ParseConfigIntLocal(const std::string& raw, int fallback) {
    if (raw.empty()) return fallback;
    try {
        int v = std::stoi(raw);
        return v <= 0 ? fallback : v;
    } catch (...) { return fallback; }
}

struct MediaConfigLocal {
    int64_t max_image_bytes = 200LL * 1024 * 1024;
    int64_t max_file_bytes = 20480LL * 1024 * 1024;
    int chunk_size_bytes = 4 * 1024 * 1024;
    int session_expire_sec = 86400;
    std::string storage_provider = "local";
};

MediaConfigLocal LoadMediaConfigLocal() {
    MediaConfigLocal cfg;
    auto media = ConfigMgr::Inst()["Media"];
    cfg.max_image_bytes = ParseConfigInt64Local(media["MaxImageMB"], 200) * 1024 * 1024;
    cfg.max_file_bytes = ParseConfigInt64Local(media["MaxFileMB"], 20480) * 1024 * 1024;
    cfg.chunk_size_bytes = ParseConfigIntLocal(media["ChunkSizeKB"], 4096) * 1024;
    cfg.session_expire_sec = ParseConfigIntLocal(media["SessionExpireSec"], 86400);
    cfg.chunk_size_bytes = std::clamp(cfg.chunk_size_bytes, 256 * 1024, 4 * 1024 * 1024);
    return cfg;
}

bool IsMediaTypeImageLocal(const std::string& media_type) {
    return media_type == "image";
}

}  // namespace

namespace Http2MediaSupport {

bool ParseJsonBody(std::string_view body_sv, Json::Value& root) {
    std::string body_str(body_sv);
    Json::Reader reader;
    return reader.parse(body_str, root) && root.isObject();
}

std::string DecodeBase64(const std::string& input) {
    return DecodeBase64Local(input);
}

bool ValidateUserToken(int uid, const std::string& token) {
    return ValidateUserTokenLocal(uid, token);
}

MediaResult HandleUploadMediaInit(int uid, const std::string& token,
    const std::string& media_type, const std::string& file_name,
    const std::string& mime, int64_t file_size) {
    MediaResult result;
    if (uid <= 0 || file_name.empty() || file_size <= 0 || !ValidateUserTokenLocal(uid, token)) {
        result.error = ErrorCodes::TokenInvalid;
        result.message = "token invalid or params invalid";
        return result;
    }

    std::string safe_type = media_type.empty() ? "file" : media_type;
    std::string safe_name = SanitizeFileNameLocal(file_name);
    std::string safe_mime = mime.empty() ? GuessContentTypeLocal(safe_name, "") : mime;

    MediaConfigLocal cfg = LoadMediaConfigLocal();
    int64_t limit = IsMediaTypeImageLocal(safe_type) ? cfg.max_image_bytes : cfg.max_file_bytes;
    if (file_size > limit) {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "file too large";
        result.data["limit_bytes"] = static_cast<Json::Int64>(limit);
        return result;
    }

    const std::string upload_id = NewIdStringLocal();
    int chunk_size = cfg.chunk_size_bytes;
    int total_chunks = static_cast<int>((file_size + chunk_size - 1) / chunk_size);
    std::error_code ec;
    std::filesystem::create_directories(ChunkDirForLocal(upload_id), ec);
    if (ec) {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "create chunk dir failed";
        return result;
    }

    Json::Value session;
    session["uid"] = uid;
    session["upload_id"] = upload_id;
    session["media_type"] = safe_type;
    session["file_name"] = safe_name;
    session["mime"] = safe_mime;
    session["file_size"] = static_cast<Json::Int64>(file_size);
    session["chunk_size"] = chunk_size;
    session["total_chunks"] = total_chunks;
    session["created_at"] = static_cast<Json::Int64>(NowMsLocal());
    session["expires_at"] = static_cast<Json::Int64>(NowMsLocal() + static_cast<int64_t>(cfg.session_expire_sec) * 1000);
    session["storage_provider"] = cfg.storage_provider;
    if (!SaveJsonFileLocal(SessionPathForLocal(upload_id), session)) {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "create upload session failed";
        return result;
    }

    result.error = ErrorCodes::Success;
    result.data["upload_id"] = upload_id;
    result.data["chunk_size"] = chunk_size;
    result.data["total_chunks"] = total_chunks;
    result.data["uploaded_chunks"] = Json::arrayValue;
    return result;
}

MediaResult HandleUploadMediaChunk(int uid, const std::string& token,
    const std::string& upload_id, int index,
    const std::string& chunk_data_base64) {
    MediaResult result;
    if (uid <= 0 || upload_id.empty() || index < 0 || chunk_data_base64.empty()
        || !ValidateUserTokenLocal(uid, token)) {
        result.error = ErrorCodes::TokenInvalid;
        result.message = "token invalid or params invalid";
        return result;
    }

    Json::Value session;
    if (!LoadJsonFileLocal(SessionPathForLocal(upload_id), session)) {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "upload session not found";
        return result;
    }
    if (session.get("uid", 0).asInt() != uid) {
        result.error = ErrorCodes::TokenInvalid;
        result.message = "session uid mismatch";
        return result;
    }
    int64_t expires_at = session.get("expires_at", 0).asInt64();
    if (expires_at > 0 && NowMsLocal() > expires_at) {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "upload session expired";
        return result;
    }

    int total_chunks = session.get("total_chunks", 0).asInt();
    int chunk_size = session.get("chunk_size", 0).asInt();
    if (index >= total_chunks || total_chunks <= 0 || chunk_size <= 0) {
        result.error = ErrorCodes::Error_Json;
        result.message = "invalid chunk index";
        return result;
    }

    std::string binary = DecodeBase64Local(chunk_data_base64);
    if (static_cast<int>(binary.size()) > chunk_size) {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "invalid chunk size";
        return result;
    }

    std::filesystem::path chunk_dir = ChunkDirForLocal(upload_id);
    std::error_code ec;
    std::filesystem::create_directories(chunk_dir, ec);
    if (ec) {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "create chunk dir failed";
        return result;
    }

    std::filesystem::path chunk_path = chunk_dir / (std::to_string(index) + ".part");
    std::ofstream ofs(chunk_path, std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "write chunk failed";
        return result;
    }
    ofs.write(binary.data(), static_cast<std::streamsize>(binary.size()));
    ofs.close();

    result.error = ErrorCodes::Success;
    result.data["upload_id"] = upload_id;
    result.data["index"] = index;
    result.data["size"] = static_cast<Json::Int64>(binary.size());
    return result;
}

MediaResult HandleUploadMediaStatus(int uid, const std::string& token,
    const std::string& upload_id) {
    MediaResult result;
    if (uid <= 0 || upload_id.empty() || !ValidateUserTokenLocal(uid, token)) {
        result.error = ErrorCodes::TokenInvalid;
        result.message = "token invalid or params invalid";
        return result;
    }

    Json::Value session;
    if (!LoadJsonFileLocal(SessionPathForLocal(upload_id), session)) {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "upload session not found";
        return result;
    }
    if (session.get("uid", 0).asInt() != uid) {
        result.error = ErrorCodes::TokenInvalid;
        result.message = "session uid mismatch";
        return result;
    }

    result.error = ErrorCodes::Success;
    result.data["upload_id"] = upload_id;
    result.data["total_chunks"] = session.get("total_chunks", 0).asInt();
    result.data["chunk_size"] = session.get("chunk_size", 0).asInt();
    result.data["uploaded_chunks"] = Json::arrayValue;
    for (int idx : ListUploadedChunkIndexesLocal(ChunkDirForLocal(upload_id))) {
        result.data["uploaded_chunks"].append(idx);
    }
    return result;
}

MediaResult HandleUploadMediaComplete(int uid, const std::string& token,
    const std::string& upload_id) {
    MediaResult result;
    if (uid <= 0 || upload_id.empty() || !ValidateUserTokenLocal(uid, token)) {
        result.error = ErrorCodes::TokenInvalid;
        result.message = "token invalid or params invalid";
        return result;
    }

    Json::Value session;
    if (!LoadJsonFileLocal(SessionPathForLocal(upload_id), session)) {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "upload session not found";
        return result;
    }
    if (session.get("uid", 0).asInt() != uid) {
        result.error = ErrorCodes::TokenInvalid;
        result.message = "session uid mismatch";
        return result;
    }

    int total_chunks = session.get("total_chunks", 0).asInt();
    int chunk_size = session.get("chunk_size", 0).asInt();
    int64_t file_size = session.get("file_size", 0).asInt64();
    std::string file_name = session.get("file_name", "").asString();
    std::string media_type = session.get("media_type", "file").asString();
    std::string mime = session.get("mime", "").asString();
    std::string storage_provider = session.get("storage_provider", "local").asString();
    if (total_chunks <= 0 || chunk_size <= 0 || file_size <= 0 || file_name.empty()) {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "invalid upload session";
        return result;
    }

    std::filesystem::path chunk_dir = ChunkDirForLocal(upload_id);
    std::set<int> uploaded = ListUploadedChunkIndexesLocal(chunk_dir);
    for (int i = 0; i < total_chunks; ++i) {
        if (uploaded.find(i) == uploaded.end()) {
            result.error = ErrorCodes::MediaUploadFailed;
            result.message = "chunks not complete";
            result.data["missing_index"] = i;
            return result;
        }
    }

    std::filesystem::path merged_path = chunk_dir / "merged.tmp";
    {
        std::ofstream merged(merged_path, std::ios::binary | std::ios::trunc);
        if (!merged.is_open()) {
            result.error = ErrorCodes::MediaUploadFailed;
            result.message = "create merged file failed";
            return result;
        }
        for (int i = 0; i < total_chunks; ++i) {
            std::filesystem::path one_path = chunk_dir / (std::to_string(i) + ".part");
            std::ifstream one(one_path, std::ios::binary);
            if (!one.is_open()) {
                result.error = ErrorCodes::MediaUploadFailed;
                result.message = "open chunk failed";
                result.data["chunk_index"] = i;
                return result;
            }
            merged << one.rdbuf();
        }
        merged.flush();
        if (!merged.good()) {
            result.error = ErrorCodes::MediaUploadFailed;
            result.message = "merge chunks failed";
            return result;
        }
    }

    std::error_code ec;
    int64_t merged_size = static_cast<int64_t>(std::filesystem::file_size(merged_path, ec));
    if (ec || merged_size != file_size) {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "merged file size mismatch";
        return result;
    }

    std::string media_key = NewIdStringLocal();
    std::string storage_path;
    std::string storage_error;
    IMediaStorage& storage = MediaStorageForLocal(storage_provider);
    if (!storage.StoreMergedFile(media_key, file_name, merged_path, storage_path, storage_error)) {
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
    if (!PostgresMgr::GetInstance()->InsertMediaAsset(asset)) {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "save media metadata failed";
        return result;
    }

    std::filesystem::remove(SessionPathForLocal(upload_id), ec);
    std::filesystem::remove_all(chunk_dir, ec);

    result.error = ErrorCodes::Success;
    result.data["media_key"] = media_key;
    result.data["media_type"] = media_type;
    result.data["file_name"] = file_name;
    result.data["mime"] = mime;
    result.data["size"] = static_cast<Json::Int64>(merged_size);
    result.data["url"] = std::string("/media/download?asset=") + media_key;
    return result;
}

MediaResult HandleUploadMediaSimple(int uid, const std::string& token,
    const std::string& media_type, const std::string& file_name,
    const std::string& mime, const std::string& data_base64) {
    MediaResult result;
    if (uid <= 0 || file_name.empty() || data_base64.empty()
        || !ValidateUserTokenLocal(uid, token)) {
        result.error = ErrorCodes::TokenInvalid;
        result.message = "token invalid or params invalid";
        return result;
    }

    std::string binary = DecodeBase64Local(data_base64);
    if (binary.empty()) {
        result.error = ErrorCodes::Error_Json;
        result.message = "base64 decode failed or empty";
        return result;
    }

    MediaConfigLocal cfg = LoadMediaConfigLocal();
    std::string safe_type = media_type.empty() ? "file" : media_type;
    int64_t limit = IsMediaTypeImageLocal(safe_type) ? cfg.max_image_bytes : cfg.max_file_bytes;
    if (static_cast<int64_t>(binary.size()) > limit) {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "file too large";
        result.data["limit_bytes"] = static_cast<Json::Int64>(limit);
        return result;
    }

    std::string safe_name = SanitizeFileNameLocal(file_name);
    std::string safe_mime = mime.empty() ? GuessContentTypeLocal(safe_name, "") : mime;
    std::string media_key = NewIdStringLocal();
    std::filesystem::path temp_dir = ChunkRootLocal() / ("legacy_" + media_key);
    std::error_code ec;
    std::filesystem::create_directories(temp_dir, ec);
    if (ec) {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "create temp dir failed";
        return result;
    }
    std::filesystem::path temp_file = temp_dir / "merged.tmp";
    {
        std::ofstream ofs(temp_file, std::ios::binary | std::ios::trunc);
        if (!ofs.is_open()) {
            result.error = ErrorCodes::MediaUploadFailed;
            result.message = "open temp file failed";
            return result;
        }
        ofs.write(binary.data(), static_cast<std::streamsize>(binary.size()));
    }

    std::string storage_path;
    std::string storage_error;
    IMediaStorage& storage = MediaStorageForLocal(cfg.storage_provider);
    if (!storage.StoreMergedFile(media_key, safe_name, temp_file, storage_path, storage_error)) {
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
    if (!PostgresMgr::GetInstance()->InsertMediaAsset(asset)) {
        result.error = ErrorCodes::MediaUploadFailed;
        result.message = "save media metadata failed";
        return result;
    }

    std::filesystem::remove_all(temp_dir, ec);
    result.error = ErrorCodes::Success;
    result.data["media_key"] = media_key;
    result.data["media_type"] = safe_type;
    result.data["file_name"] = safe_name;
    result.data["mime"] = safe_mime;
    result.data["size"] = static_cast<Json::Int64>(binary.size());
    result.data["url"] = std::string("/media/download?asset=") + media_key;
    return result;
}

MediaResult HandleMediaDownloadInfo(int uid, const std::string& token,
    const std::string& media_key) {
    MediaResult result;
    if (uid <= 0 || media_key.empty() || !ValidateUserTokenLocal(uid, token)) {
        result.error = ErrorCodes::TokenInvalid;
        result.message = "token invalid or params invalid";
        return result;
    }

    MediaAssetInfo asset;
    if (!PostgresMgr::GetInstance()->GetMediaAssetByKey(media_key, asset)
        || asset.status != 1 || asset.deleted_at_ms > 0) {
        result.error = ErrorCodes::UidInvalid;
        result.message = "asset not found";
        return result;
    }

    IMediaStorage& storage = MediaStorageForLocal(asset.storage_provider);
    std::string public_url;
    if (storage.ResolvePublicUrl(asset.storage_path, public_url) && !public_url.empty()) {
        result.error = ErrorCodes::Success;
        result.data["redirect"] = public_url;
        return result;
    }

    std::filesystem::path full_path;
    if (!storage.ResolveReadPath(asset.storage_path, full_path)
        || !std::filesystem::exists(full_path)) {
        result.error = ErrorCodes::UidInvalid;
        result.message = "file not found";
        return result;
    }

    result.error = ErrorCodes::Success;
    result.data["path"] = full_path.string();
    result.data["content_type"] = GuessContentTypeLocal(asset.origin_file_name, asset.mime);
    return result;
}

}  // namespace Http2MediaSupport
