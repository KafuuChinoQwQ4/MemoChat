#include "H1MediaService.h"

#include "H1Connection.h"
#include "H1Globals.h"
#include "H1LogicSystem.h"
#include "ConfigMgr.h"
#include "MediaStorage.h"
#include "PostgresMgr.h"
#include "RedisMgr.h"
#include "const.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <algorithm>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <json/json.h>
#include <set>
#include <unordered_map>
#include <vector>

namespace beast = boost::beast;
namespace http = beast::http;

namespace {
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
    if (safe.empty()) {
        return "file.bin";
    }
    if (safe.size() > 96) {
        safe = safe.substr(safe.size() - 96);
    }
    return safe;
}

bool DecodeBase64Local(const std::string& input, std::string& output) {
    static const std::string chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::vector<int> table(256, -1);
    for (int i = 0; i < static_cast<int>(chars.size()); ++i) {
        table[static_cast<unsigned char>(chars[i])] = i;
    }

    output.clear();
    int val = 0;
    int valb = -8;
    for (unsigned char c : input) {
        if (std::isspace(c)) continue;
        if (c == '=') break;
        if (table[c] == -1) return false;
        val = (val << 6) + table[c];
        valb += 6;
        if (valb >= 0) {
            output.push_back(static_cast<char>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return true;
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
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
    }
    const auto it = extMap.find(ext);
    return (it != extMap.end()) ? it->second : "application/octet-stream";
}

struct MediaConfigLocal {
    int64_t max_image_bytes = 200LL * 1024 * 1024;
    int64_t max_file_bytes = 20480LL * 1024 * 1024;
    int chunk_size_bytes = 4 * 1024 * 1024;
    int session_expire_sec = 86400;
    std::string storage_provider = "local";
};

int64_t ParseConfigInt64Local(const std::string& raw, int64_t fallback) {
    if (raw.empty()) return fallback;
    try {
        const int64_t value = std::stoll(raw);
        return value <= 0 ? fallback : value;
    } catch (...) { return fallback; }
}

int ParseConfigIntLocal(const std::string& raw, int fallback) {
    if (raw.empty()) return fallback;
    try {
        const int value = std::stoi(raw);
        return value <= 0 ? fallback : value;
    } catch (...) { return fallback; }
}

MediaConfigLocal LoadMediaConfigLocal() {
    MediaConfigLocal cfg;
    auto media = ConfigMgr::Inst()["Media"];
    cfg.max_image_bytes = ParseConfigInt64Local(media["MaxImageMB"], 200) * 1024 * 1024;
    cfg.max_file_bytes = ParseConfigInt64Local(media["MaxFileMB"], 20480) * 1024 * 1024;
    cfg.chunk_size_bytes = ParseConfigIntLocal(media["ChunkSizeKB"], 4096) * 1024;
    cfg.session_expire_sec = ParseConfigIntLocal(media["SessionExpireSec"], 86400);
    const std::string provider = media["StorageProvider"];
    if (!provider.empty()) cfg.storage_provider = provider;
    cfg.chunk_size_bytes = std::clamp(cfg.chunk_size_bytes, 256 * 1024, 4 * 1024 * 1024);
    return cfg;
}

int64_t NowMsLocal() {
    return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

std::filesystem::path UploadRootLocal() {
    return std::filesystem::current_path() / "uploads";
}

std::filesystem::path SessionRootLocal() {
    return UploadRootLocal() / "sessions";
}

std::filesystem::path ChunkRootLocal() {
    return UploadRootLocal() / "chunks";
}

std::filesystem::path SessionPathForLocal(const std::string& upload_id) {
    return SessionRootLocal() / (upload_id + ".json");
}

std::filesystem::path ChunkDirForLocal(const std::string& upload_id) {
    return ChunkRootLocal() / upload_id;
}

bool SaveJsonFileLocal(const std::filesystem::path& path, const Json::Value& root) {
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    if (ec) return false;
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    return ofs.is_open() && ofs.good() && (ofs << root.toStyledString(), true);
}

bool LoadJsonFileLocal(const std::filesystem::path& path, Json::Value& root) {
    if (!std::filesystem::exists(path)) return false;
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) return false;
    std::string json((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    Json::Reader reader;
    return reader.parse(json, root);
}

bool ValidateUserTokenLocal(int uid, const std::string& token) {
    if (uid <= 0 || token.empty()) return false;
    const std::string token_key = USERTOKENPREFIX + std::to_string(uid);
    std::string token_value;
    return RedisMgr::GetInstance()->Get(token_key, token_value) && !token_value.empty() && token_value == token;
}

std::set<int> ListUploadedChunkIndexesLocal(const std::filesystem::path& chunk_dir) {
    std::set<int> indexes;
    if (!std::filesystem::exists(chunk_dir)) return indexes;
    for (const auto& entry : std::filesystem::directory_iterator(chunk_dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string stem = entry.path().stem().string();
        if (stem.empty()) continue;
        bool digits = std::all_of(stem.begin(), stem.end(), [](char c) { return std::isdigit(static_cast<unsigned char>(c)); });
        if (!digits) continue;
        try { indexes.insert(std::stoi(stem)); } catch (...) {}
    }
    return indexes;
}

IMediaStorage& MediaStorageForLocal(const std::string& provider) {
    static LocalMediaStorage local_storage;
    (void)provider;
    return local_storage;
}

bool IsMediaTypeImageLocal(const std::string& media_type) {
    return media_type == "image";
}

std::string NewIdStringLocal() {
    return boost::uuids::to_string(boost::uuids::random_generator()());
}

bool ResolveLegacyMediaPathLocal(const std::string& legacy_file, std::filesystem::path& full_path) {
    if (legacy_file.empty()) return false;
    const std::filesystem::path rel = std::filesystem::path(legacy_file).lexically_normal();
    if (rel.is_absolute() || rel.generic_string().find("..") != std::string::npos) return false;
    std::error_code ec;
    const std::filesystem::path upload_root = UploadRootLocal();
    const std::filesystem::path by_relative = upload_root / rel;
    if (std::filesystem::exists(by_relative, ec) && !ec && std::filesystem::is_regular_file(by_relative, ec)) {
        full_path = by_relative; return true;
    }
    ec.clear();
    const std::filesystem::path by_filename = upload_root / rel.filename();
    if (std::filesystem::exists(by_filename, ec) && !ec && std::filesystem::is_regular_file(by_filename, ec)) {
        full_path = by_filename; return true;
    }
    return false;
}
} // namespace

void H1MediaService::RegisterRoutes(H1LogicSystem& logic)
{
    logic.RegPost("/upload_media_init", [](std::shared_ptr<H1Connection> connection) {
        const auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
        connection->_response.set(http::field::content_type, "text/json");
        Json::Value root;
        Json::Value src_root;
        Json::Reader reader;
        if (!reader.parse(body_str, src_root)) {
            root["error"] = ErrorCodes::Error_Json;
            root["message"] = "invalid json";
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        const int uid = src_root.get("uid", 0).asInt();
        const std::string token = src_root.get("token", "").asString();
        std::string media_type = src_root.get("media_type", "file").asString();
        if (media_type.empty()) media_type = "file";
        const std::string file_name = SanitizeFileNameLocal(src_root.get("file_name", "").asString());
        std::string mime = src_root.get("mime", "").asString();
        const int64_t file_size = src_root.get("file_size", 0).asInt64();
        if (uid <= 0 || file_name.empty() || file_size <= 0 || !ValidateUserTokenLocal(uid, token)) {
            root["error"] = ErrorCodes::TokenInvalid;
            root["message"] = "token invalid or params invalid";
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        const MediaConfigLocal media_cfg = LoadMediaConfigLocal();
        const int64_t limit = IsMediaTypeImageLocal(media_type) ? media_cfg.max_image_bytes : media_cfg.max_file_bytes;
        if (file_size > limit) {
            root["error"] = ErrorCodes::MediaUploadFailed;
            root["message"] = "file too large";
            root["limit_bytes"] = static_cast<Json::Int64>(limit);
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        if (mime.empty()) mime = GuessContentTypeLocal(file_name, "");
        const std::string upload_id = NewIdStringLocal();
        const int chunk_size = media_cfg.chunk_size_bytes;
        const int total_chunks = static_cast<int>((file_size + chunk_size - 1) / chunk_size);
        std::error_code ec;
        std::filesystem::create_directories(ChunkDirForLocal(upload_id), ec);
        if (ec) {
            root["error"] = ErrorCodes::MediaUploadFailed;
            root["message"] = "create chunk dir failed";
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        Json::Value session;
        session["uid"] = uid;
        session["upload_id"] = upload_id;
        session["media_type"] = media_type;
        session["file_name"] = file_name;
        session["mime"] = mime;
        session["file_size"] = static_cast<Json::Int64>(file_size);
        session["chunk_size"] = chunk_size;
        session["total_chunks"] = total_chunks;
        session["created_at"] = static_cast<Json::Int64>(NowMsLocal());
        session["expires_at"] = static_cast<Json::Int64>(NowMsLocal() + static_cast<int64_t>(media_cfg.session_expire_sec) * 1000);
        session["storage_provider"] = media_cfg.storage_provider;
        if (!SaveJsonFileLocal(SessionPathForLocal(upload_id), session)) {
            root["error"] = ErrorCodes::MediaUploadFailed;
            root["message"] = "create upload session failed";
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        root["error"] = ErrorCodes::Success;
        root["upload_id"] = upload_id;
        root["chunk_size"] = chunk_size;
        root["total_chunks"] = total_chunks;
        root["uploaded_chunks"] = Json::arrayValue;
        beast::ostream(connection->_response.body()) << root.toStyledString();
        return true;
    });

    logic.RegPost("/upload_media_chunk", [](std::shared_ptr<H1Connection> connection) {
        connection->_response.set(http::field::content_type, "text/json");
        Json::Value root;
        int uid = 0;
        int index = -1;
        std::string token;
        std::string upload_id;
        std::string binary;

        std::string content_type;
        const auto ct_it = connection->_request.find(http::field::content_type);
        if (ct_it != connection->_request.end()) {
            content_type = std::string(ct_it->value().data(), ct_it->value().size());
            std::transform(content_type.begin(), content_type.end(), content_type.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        }

        if (content_type.find("application/json") != std::string::npos) {
            const auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
            Json::Value src_root;
            Json::Reader reader;
            if (!reader.parse(body_str, src_root)) {
                root["error"] = ErrorCodes::Error_Json;
                root["message"] = "invalid json";
                beast::ostream(connection->_response.body()) << root.toStyledString();
                return true;
            }
            uid = src_root.get("uid", 0).asInt();
            token = src_root.get("token", "").asString();
            upload_id = src_root.get("upload_id", "").asString();
            index = src_root.get("index", -1).asInt();
            const std::string encoded = src_root.get("data_base64", "").asString();
            if (encoded.empty() || !DecodeBase64Local(encoded, binary)) {
                root["error"] = ErrorCodes::Error_Json;
                root["message"] = "base64 decode failed";
                beast::ostream(connection->_response.body()) << root.toStyledString();
                return true;
            }
        } else {
            const auto read_header = [&connection](const char* key) -> std::string {
                const auto it = connection->_request.find(key);
                if (it == connection->_request.end()) return "";
                return std::string(it->value().data(), it->value().size());
            };
            uid = std::atoi(read_header("X-Uid").c_str());
            token = read_header("X-Token");
            upload_id = read_header("X-Upload-Id");
            index = std::atoi(read_header("X-Chunk-Index").c_str());
            binary = boost::beast::buffers_to_string(connection->_request.body().data());
        }

        if (uid <= 0 || upload_id.empty() || index < 0 || binary.empty() || !ValidateUserTokenLocal(uid, token)) {
            root["error"] = ErrorCodes::TokenInvalid;
            root["message"] = "token invalid or params invalid";
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        Json::Value session;
        if (!LoadJsonFileLocal(SessionPathForLocal(upload_id), session)) {
            root["error"] = ErrorCodes::MediaUploadFailed;
            root["message"] = "upload session not found";
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }
        if (session.get("uid", 0).asInt() != uid) {
            root["error"] = ErrorCodes::TokenInvalid;
            root["message"] = "session uid mismatch";
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }
        const int64_t expires_at = session.get("expires_at", 0).asInt64();
        if (expires_at > 0 && NowMsLocal() > expires_at) {
            root["error"] = ErrorCodes::MediaUploadFailed;
            root["message"] = "upload session expired";
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        const int total_chunks = session.get("total_chunks", 0).asInt();
        const int chunk_size = session.get("chunk_size", 0).asInt();
        if (index >= total_chunks || total_chunks <= 0 || chunk_size <= 0 ||
            static_cast<int>(binary.size()) > chunk_size) {
            root["error"] = ErrorCodes::Error_Json;
            root["message"] = "invalid chunk index or size";
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        const std::filesystem::path chunk_dir = ChunkDirForLocal(upload_id);
        std::error_code ec;
        std::filesystem::create_directories(chunk_dir, ec);
        if (ec) {
            root["error"] = ErrorCodes::MediaUploadFailed;
            root["message"] = "create chunk dir failed";
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        const std::filesystem::path chunk_path = chunk_dir / (std::to_string(index) + ".part");
        std::ofstream ofs(chunk_path, std::ios::binary | std::ios::trunc);
        if (!ofs.is_open()) {
            root["error"] = ErrorCodes::MediaUploadFailed;
            root["message"] = "write chunk failed";
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }
        ofs.write(binary.data(), static_cast<std::streamsize>(binary.size()));
        ofs.close();

        root["error"] = ErrorCodes::Success;
        root["upload_id"] = upload_id;
        root["index"] = index;
        root["size"] = static_cast<Json::Int64>(binary.size());
        beast::ostream(connection->_response.body()) << root.toStyledString();
        return true;
    });

    logic.RegGet("/upload_media_status", [](std::shared_ptr<H1Connection> connection) {
        connection->_response.set(http::field::content_type, "text/json");
        Json::Value root;
        const int uid = std::atoi(connection->_get_params["uid"].c_str());
        const std::string token = connection->_get_params["token"];
        const std::string upload_id = connection->_get_params["upload_id"];
        if (uid <= 0 || token.empty() || upload_id.empty() || !ValidateUserTokenLocal(uid, token)) {
            root["error"] = ErrorCodes::TokenInvalid;
            root["message"] = "token invalid or params invalid";
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        Json::Value session;
        if (!LoadJsonFileLocal(SessionPathForLocal(upload_id), session)) {
            root["error"] = ErrorCodes::MediaUploadFailed;
            root["message"] = "upload session not found";
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }
        if (session.get("uid", 0).asInt() != uid) {
            root["error"] = ErrorCodes::TokenInvalid;
            root["message"] = "session uid mismatch";
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        root["error"] = ErrorCodes::Success;
        root["upload_id"] = upload_id;
        root["total_chunks"] = session.get("total_chunks", 0).asInt();
        root["chunk_size"] = session.get("chunk_size", 0).asInt();
        root["uploaded_chunks"] = Json::arrayValue;
        for (int idx : ListUploadedChunkIndexesLocal(ChunkDirForLocal(upload_id))) {
            root["uploaded_chunks"].append(idx);
        }
        beast::ostream(connection->_response.body()) << root.toStyledString();
        return true;
    });

    logic.RegPost("/upload_media_complete", [](std::shared_ptr<H1Connection> connection) {
        const auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
        connection->_response.set(http::field::content_type, "text/json");
        Json::Value root;
        Json::Value src_root;
        Json::Reader reader;
        if (!reader.parse(body_str, src_root)) {
            root["error"] = ErrorCodes::Error_Json;
            root["message"] = "invalid json";
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        const int uid = src_root.get("uid", 0).asInt();
        const std::string token = src_root.get("token", "").asString();
        const std::string upload_id = src_root.get("upload_id", "").asString();
        if (uid <= 0 || token.empty() || upload_id.empty() || !ValidateUserTokenLocal(uid, token)) {
            root["error"] = ErrorCodes::TokenInvalid;
            root["message"] = "token invalid or params invalid";
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        Json::Value session;
        if (!LoadJsonFileLocal(SessionPathForLocal(upload_id), session)) {
            root["error"] = ErrorCodes::MediaUploadFailed;
            root["message"] = "upload session not found";
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }
        if (session.get("uid", 0).asInt() != uid) {
            root["error"] = ErrorCodes::TokenInvalid;
            root["message"] = "session uid mismatch";
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        const int total_chunks = session.get("total_chunks", 0).asInt();
        const int chunk_size = session.get("chunk_size", 0).asInt();
        const int64_t file_size = session.get("file_size", 0).asInt64();
        const std::string file_name = session.get("file_name", "").asString();
        const std::string media_type = session.get("media_type", "file").asString();
        const std::string mime = session.get("mime", "").asString();
        const std::string storage_provider = session.get("storage_provider", "local").asString();
        if (total_chunks <= 0 || chunk_size <= 0 || file_size <= 0 || file_name.empty()) {
            root["error"] = ErrorCodes::MediaUploadFailed;
            root["message"] = "invalid upload session";
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        const std::filesystem::path chunk_dir = ChunkDirForLocal(upload_id);
        const std::set<int> uploaded = ListUploadedChunkIndexesLocal(chunk_dir);
        for (int i = 0; i < total_chunks; ++i) {
            if (uploaded.find(i) == uploaded.end()) {
                root["error"] = ErrorCodes::MediaUploadFailed;
                root["message"] = "chunks not complete";
                root["missing_index"] = i;
                beast::ostream(connection->_response.body()) << root.toStyledString();
                return true;
            }
        }

        const std::filesystem::path merged_path = chunk_dir / "merged.tmp";
        {
            std::ofstream merged(merged_path, std::ios::binary | std::ios::trunc);
            if (!merged.is_open()) {
                root["error"] = ErrorCodes::MediaUploadFailed;
                root["message"] = "create merged file failed";
                beast::ostream(connection->_response.body()) << root.toStyledString();
                return true;
            }
            for (int i = 0; i < total_chunks; ++i) {
                const std::filesystem::path one_path = chunk_dir / (std::to_string(i) + ".part");
                std::ifstream one(one_path, std::ios::binary);
                if (!one.is_open()) {
                    root["error"] = ErrorCodes::MediaUploadFailed;
                    root["message"] = "open chunk failed";
                    root["chunk_index"] = i;
                    beast::ostream(connection->_response.body()) << root.toStyledString();
                    return true;
                }
                merged << one.rdbuf();
            }
            merged.flush();
            if (!merged.good()) {
                root["error"] = ErrorCodes::MediaUploadFailed;
                root["message"] = "merge chunks failed";
                beast::ostream(connection->_response.body()) << root.toStyledString();
                return true;
            }
        }

        std::error_code ec;
        const int64_t merged_size = static_cast<int64_t>(std::filesystem::file_size(merged_path, ec));
        if (ec || merged_size != file_size) {
            root["error"] = ErrorCodes::MediaUploadFailed;
            root["message"] = "merged file size mismatch";
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        const std::string media_key = NewIdStringLocal();
        std::string storage_path;
        std::string storage_error;
        IMediaStorage& storage = MediaStorageForLocal(storage_provider);
        if (!storage.StoreMergedFile(media_key, file_name, merged_path, storage_path, storage_error)) {
            root["error"] = ErrorCodes::MediaUploadFailed;
            root["message"] = storage_error.empty() ? "persist media failed" : storage_error;
            beast::ostream(connection->_response.body()) << root.toStyledString();
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
        if (!PostgresMgr::GetInstance()->InsertMediaAsset(asset)) {
            root["error"] = ErrorCodes::MediaUploadFailed;
            root["message"] = "save media metadata failed";
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        std::filesystem::remove(SessionPathForLocal(upload_id), ec);
        std::filesystem::remove_all(chunk_dir, ec);

        root["error"] = ErrorCodes::Success;
        root["media_key"] = media_key;
        root["media_type"] = media_type;
        root["file_name"] = file_name;
        root["mime"] = mime;
        root["size"] = static_cast<Json::Int64>(merged_size);
        root["url"] = std::string("/media/download?asset=") + media_key;
        beast::ostream(connection->_response.body()) << root.toStyledString();
        return true;
    });

    logic.RegPost("/upload_media", [](std::shared_ptr<H1Connection> connection) {
        const auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
        connection->_response.set(http::field::content_type, "text/json");
        Json::Value root;
        Json::Value src_root;
        Json::Reader reader;
        if (!reader.parse(body_str, src_root)) {
            root["error"] = ErrorCodes::Error_Json;
            root["message"] = "invalid json";
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        const int uid = src_root.get("uid", 0).asInt();
        const std::string token = src_root.get("token", "").asString();
        std::string media_type = src_root.get("media_type", "file").asString();
        if (media_type.empty()) media_type = "file";
        const std::string file_name = SanitizeFileNameLocal(src_root.get("file_name", "").asString());
        std::string mime = src_root.get("mime", "").asString();
        const std::string encoded = src_root.get("data_base64", "").asString();
        if (uid <= 0 || file_name.empty() || encoded.empty() || !ValidateUserTokenLocal(uid, token)) {
            root["error"] = ErrorCodes::TokenInvalid;
            root["message"] = "token invalid or params invalid";
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        std::string binary;
        if (!DecodeBase64Local(encoded, binary) || binary.empty()) {
            root["error"] = ErrorCodes::Error_Json;
            root["message"] = binary.empty() ? "file empty" : "base64 decode failed";
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        const MediaConfigLocal media_cfg = LoadMediaConfigLocal();
        const int64_t limit = IsMediaTypeImageLocal(media_type) ? media_cfg.max_image_bytes : media_cfg.max_file_bytes;
        if (static_cast<int64_t>(binary.size()) > limit) {
            root["error"] = ErrorCodes::MediaUploadFailed;
            root["message"] = "file too large";
            root["limit_bytes"] = static_cast<Json::Int64>(limit);
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }
        if (mime.empty()) mime = GuessContentTypeLocal(file_name, "");

        const std::string media_key = NewIdStringLocal();
        const std::filesystem::path temp_dir = ChunkRootLocal() / ("legacy_" + media_key);
        std::error_code ec;
        std::filesystem::create_directories(temp_dir, ec);
        if (ec) {
            root["error"] = ErrorCodes::MediaUploadFailed;
            root["message"] = "create temp dir failed";
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }
        const std::filesystem::path temp_file = temp_dir / "merged.tmp";
        {
            std::ofstream ofs(temp_file, std::ios::binary | std::ios::trunc);
            if (!ofs.is_open()) {
                root["error"] = ErrorCodes::MediaUploadFailed;
                root["message"] = "open temp file failed";
                beast::ostream(connection->_response.body()) << root.toStyledString();
                return true;
            }
            ofs.write(binary.data(), static_cast<std::streamsize>(binary.size()));
        }

        std::string storage_path;
        std::string storage_error;
        IMediaStorage& storage = MediaStorageForLocal(media_cfg.storage_provider);
        if (!storage.StoreMergedFile(media_key, file_name, temp_file, storage_path, storage_error)) {
            root["error"] = ErrorCodes::MediaUploadFailed;
            root["message"] = storage_error.empty() ? "persist media failed" : storage_error;
            beast::ostream(connection->_response.body()) << root.toStyledString();
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
        if (!PostgresMgr::GetInstance()->InsertMediaAsset(asset)) {
            root["error"] = ErrorCodes::MediaUploadFailed;
            root["message"] = "save media metadata failed";
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        std::filesystem::remove_all(temp_dir, ec);
        root["error"] = ErrorCodes::Success;
        root["media_key"] = media_key;
        root["media_type"] = media_type;
        root["file_name"] = file_name;
        root["mime"] = mime;
        root["size"] = static_cast<Json::Int64>(binary.size());
        root["url"] = std::string("/media/download?asset=") + media_key;
        beast::ostream(connection->_response.body()) << root.toStyledString();
        return true;
    });

    logic.RegGet("/media/download", [](std::shared_ptr<H1Connection> connection) {
        connection->_response.set(http::field::content_type, "text/json");
        Json::Value root;
        const auto asset_it = connection->_get_params.find("asset");
        const auto file_it = connection->_get_params.find("file");
        const auto uid_it = connection->_get_params.find("uid");
        const auto token_it = connection->_get_params.find("token");
        if ((asset_it == connection->_get_params.end() && file_it == connection->_get_params.end()) ||
            uid_it == connection->_get_params.end() || token_it == connection->_get_params.end()) {
            root["error"] = ErrorCodes::Error_Json;
            root["message"] = "missing media key or auth params";
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        const std::string media_key = (asset_it != connection->_get_params.end()) ? asset_it->second : "";
        const std::string legacy_file = (file_it != connection->_get_params.end()) ? file_it->second : "";
        const int uid = std::atoi(uid_it->second.c_str());
        const std::string token = token_it->second;
        if (uid <= 0 || !ValidateUserTokenLocal(uid, token)) {
            root["error"] = ErrorCodes::TokenInvalid;
            root["message"] = "token invalid";
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        std::filesystem::path full_path;
        std::string content_type = "application/octet-stream";
        if (!media_key.empty()) {
            MediaAssetInfo asset;
            if (!PostgresMgr::GetInstance()->GetMediaAssetByKey(media_key, asset)
                || asset.status != 1 || asset.deleted_at_ms > 0) {
                root["error"] = ErrorCodes::UidInvalid;
                root["message"] = "asset not found";
                beast::ostream(connection->_response.body()) << root.toStyledString();
                return true;
            }

            IMediaStorage& storage = MediaStorageForLocal(asset.storage_provider);
            std::string public_url;
            if (storage.ResolvePublicUrl(asset.storage_path, public_url) && !public_url.empty()) {
                connection->_response.result(http::status::temporary_redirect);
                connection->_response.set(http::field::location, public_url);
                connection->_response.set(http::field::content_type, "text/plain");
                beast::ostream(connection->_response.body()) << "redirecting to object storage";
                return true;
            }

            if (!storage.ResolveReadPath(asset.storage_path, full_path) || !std::filesystem::exists(full_path)) {
                root["error"] = ErrorCodes::UidInvalid;
                root["message"] = "file not found";
                beast::ostream(connection->_response.body()) << root.toStyledString();
                return true;
            }
            content_type = GuessContentTypeLocal(asset.origin_file_name, asset.mime);
        } else {
            if (!ResolveLegacyMediaPathLocal(legacy_file, full_path) || !std::filesystem::exists(full_path)) {
                root["error"] = ErrorCodes::UidInvalid;
                root["message"] = "legacy file not found";
                beast::ostream(connection->_response.body()) << root.toStyledString();
                return true;
            }
            content_type = GuessContentTypeLocal(full_path.filename().string(), "");
        }

        connection->_response.set(http::field::content_type, content_type);
        connection->SetFileResponse(full_path.string(), content_type);
        return true;
    });
}
