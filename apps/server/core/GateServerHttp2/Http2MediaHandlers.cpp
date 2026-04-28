#include "WinCompat.h"
#include "json/GlazeCompat.h"
#include "Http2MediaHandlers.h"
#include "../GateServerCore/Http2MediaSupport.h"

static memochat::json::JsonValue MakeMediaError(int error_code, const std::string& message) {
    memochat::json::JsonValue resp;
    resp["error"] = static_cast<double>(error_code);
    if (!message.empty()) resp["message"] = message;
    return resp;
}

static memochat::json::JsonValue MakeMediaOk(const memochat::json::JsonValue& data) {
    memochat::json::JsonValue resp;
    resp["error"] = 0.0;
    if (memochat::json::glaze_is_object(data)) {
        for (const auto& key : memochat::json::getMemberNames(data)) {
            resp[key] = memochat::json::glaze_get(data, key);
        }
    }
    return resp;
}

static int ExtractIntHeader(const Http2Request& req, const std::string& name, int default_val = 0) {
    auto it = req.headers.find(name);
    if (it != req.headers.end()) {
        try {
            return std::stoi(it->second);
        } catch (...) {}
    }
    return default_val;
}

void Http2MediaHandlers::HandleUploadMediaInit(const Http2Request& req, Http2Response& resp)
{
    memochat::json::JsonValue root;
    if (!Http2MediaSupport::ParseJsonBody(req.body, root)) {
        resp.SetJsonBody(memochat::json::glaze_stringify(MakeMediaError(1, "invalid json")));
        return;
    }

    const int uid = memochat::json::glaze_safe_get<int>(root, "uid", 0);
    const std::string token = memochat::json::glaze_safe_get<std::string>(root, "token", "");
    const std::string media_type = memochat::json::glaze_safe_get<std::string>(root, "media_type", "file");
    const std::string file_name = memochat::json::glaze_safe_get<std::string>(root, "file_name", "");
    const std::string mime = memochat::json::glaze_safe_get<std::string>(root, "mime", "");
    const int64_t file_size = memochat::json::glaze_safe_get<int64_t>(root, "file_size", 0LL);

    auto result = Http2MediaSupport::HandleUploadMediaInit(
        uid, token, media_type, file_name, mime, file_size);
    memochat::json::JsonValue out = MakeMediaOk(result.data);
    resp.SetJsonBody(memochat::json::glaze_stringify(out));
}

void Http2MediaHandlers::HandleUploadMediaChunk(const Http2Request& req, Http2Response& resp)
{
    std::string body_str = req.body;
    memochat::json::JsonValue root;
    int uid = 0;
    std::string token;
    std::string upload_id;
    int index = -1;
    std::string chunk_data_base64;

    if (Http2MediaSupport::ParseJsonBody(body_str, root)) {
        uid = memochat::json::glaze_safe_get<int>(root, "uid", 0);
        token = memochat::json::glaze_safe_get<std::string>(root, "token", "");
        upload_id = memochat::json::glaze_safe_get<std::string>(root, "upload_id", "");
        index = memochat::json::glaze_safe_get<int>(root, "index", -1);
        chunk_data_base64 = memochat::json::glaze_safe_get<std::string>(root, "data_base64", "");
    } else {
        uid = ExtractIntHeader(req, "X-Uid");
        auto it = req.headers.find("X-Token");
        if (it != req.headers.end()) token = it->second;
        it = req.headers.find("X-Upload-Id");
        if (it != req.headers.end()) upload_id = it->second;
        index = ExtractIntHeader(req, "X-Chunk-Index");
        chunk_data_base64 = body_str;
    }

    auto result = Http2MediaSupport::HandleUploadMediaChunk(
        uid, token, upload_id, index, chunk_data_base64);
    memochat::json::JsonValue out = MakeMediaOk(result.data);
    resp.SetJsonBody(memochat::json::glaze_stringify(out));
}

void Http2MediaHandlers::HandleUploadMediaStatus(const Http2Request& req, Http2Response& resp)
{
    int uid = 0;
    std::string token;
    std::string upload_id;

    auto it = req.headers.find("X-Uid");
    if (it != req.headers.end()) {
        try { uid = std::stoi(it->second); } catch (...) {}
    }
    it = req.headers.find("X-Token");
    if (it != req.headers.end()) token = it->second;
    it = req.headers.find("X-Upload-Id");
    if (it != req.headers.end()) upload_id = it->second;

    auto result = Http2MediaSupport::HandleUploadMediaStatus(uid, token, upload_id);
    memochat::json::JsonValue out = MakeMediaOk(result.data);
    resp.SetJsonBody(memochat::json::glaze_stringify(out));
}

void Http2MediaHandlers::HandleUploadMediaComplete(const Http2Request& req, Http2Response& resp)
{
    memochat::json::JsonValue root;
    if (!Http2MediaSupport::ParseJsonBody(req.body, root)) {
        resp.SetJsonBody(memochat::json::glaze_stringify(MakeMediaError(1, "invalid json")));
        return;
    }

    const int uid = memochat::json::glaze_safe_get<int>(root, "uid", 0);
    const std::string token = memochat::json::glaze_safe_get<std::string>(root, "token", "");
    const std::string upload_id = memochat::json::glaze_safe_get<std::string>(root, "upload_id", "");

    auto result = Http2MediaSupport::HandleUploadMediaComplete(uid, token, upload_id);
    memochat::json::JsonValue out = MakeMediaOk(result.data);
    resp.SetJsonBody(memochat::json::glaze_stringify(out));
}

void Http2MediaHandlers::HandleUploadMedia(const Http2Request& req, Http2Response& resp)
{
    memochat::json::JsonValue root;
    if (!Http2MediaSupport::ParseJsonBody(req.body, root)) {
        resp.SetJsonBody(memochat::json::glaze_stringify(MakeMediaError(1, "invalid json")));
        return;
    }

    const int uid = memochat::json::glaze_safe_get<int>(root, "uid", 0);
    const std::string token = memochat::json::glaze_safe_get<std::string>(root, "token", "");
    const std::string media_type = memochat::json::glaze_safe_get<std::string>(root, "media_type", "file");
    const std::string file_name = memochat::json::glaze_safe_get<std::string>(root, "file_name", "");
    const std::string mime = memochat::json::glaze_safe_get<std::string>(root, "mime", "");
    const std::string data_base64 = memochat::json::glaze_safe_get<std::string>(root, "data_base64", "");

    auto result = Http2MediaSupport::HandleUploadMediaSimple(
        uid, token, media_type, file_name, mime, data_base64);
    memochat::json::JsonValue out = MakeMediaOk(result.data);
    resp.SetJsonBody(memochat::json::glaze_stringify(out));
}

void Http2MediaHandlers::HandleMediaDownload(const Http2Request& req, Http2Response& resp)
{
    int uid = 0;
    std::string token;
    std::string media_key;

    auto it = req.headers.find("X-Uid");
    if (it != req.headers.end()) {
        try { uid = std::stoi(it->second); } catch (...) {}
    }
    it = req.headers.find("X-Token");
    if (it != req.headers.end()) token = it->second;
    it = req.headers.find("X-Asset");
    if (it != req.headers.end()) media_key = it->second;

    if (uid <= 0 || token.empty() || media_key.empty()) {
        resp.SetJsonBody(memochat::json::glaze_stringify(MakeMediaError(1, "missing media key or auth params")));
        return;
    }

    auto result = Http2MediaSupport::HandleMediaDownloadInfo(uid, token, media_key);
    if (result.error != 0) {
        memochat::json::JsonValue out = MakeMediaError(result.error, result.message);
        resp.SetJsonBody(memochat::json::glaze_stringify(out));
        return;
    }

    if (memochat::json::glaze_has_key(result.data, "data")) {
        resp.status_code = 200;
        resp.content_type = memochat::json::glaze_safe_get<std::string>(result.data, "content_type", "");
        resp.body = memochat::json::glaze_safe_get<std::string>(result.data, "data", "");
        return;
    }

    if (memochat::json::glaze_has_key(result.data, "path")) {
        resp.SetHeader("Content-Type", memochat::json::glaze_safe_get<std::string>(result.data, "content_type", ""));
        resp.SetHeader("X-Media-Path", memochat::json::glaze_safe_get<std::string>(result.data, "path", ""));
        memochat::json::JsonValue body;
        body["status"] = std::string("file");
        body["path"] = memochat::json::glaze_safe_get<std::string>(result.data, "path", "");
        resp.SetJsonBody(memochat::json::glaze_stringify(body));
        return;
    }

    memochat::json::JsonValue out = MakeMediaError(1, result.message.empty() ? "file not found" : result.message);
    resp.SetJsonBody(memochat::json::glaze_stringify(out));
}

