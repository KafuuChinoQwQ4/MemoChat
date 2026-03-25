#include "WinCompat.h"
#include "Http2MediaHandlers.h"
#include "../GateServerCore/Http2MediaSupport.h"

static Json::Value MakeMediaError(int error_code, const std::string& message) {
    Json::Value resp;
    resp["error"] = error_code;
    if (!message.empty()) resp["message"] = message;
    return resp;
}

static Json::Value MakeMediaOk(const Json::Value& data) {
    Json::Value resp;
    resp["error"] = 0;
    if (data.isObject()) {
        for (const auto& key : data.getMemberNames()) {
            resp[key] = data[key];
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
    Json::Value root;
    if (!Http2MediaSupport::ParseJsonBody(req.body, root)) {
        resp.SetJsonBody(MakeMediaError(1, "invalid json").toStyledString());
        return;
    }

    const int uid = root.get("uid", 0).asInt();
    const std::string token = root.get("token", "").asString();
    const std::string media_type = root.get("media_type", "file").asString();
    const std::string file_name = root.get("file_name", "").asString();
    const std::string mime = root.get("mime", "").asString();
    const int64_t file_size = root.get("file_size", 0).asInt64();

    auto result = Http2MediaSupport::HandleUploadMediaInit(
        uid, token, media_type, file_name, mime, file_size);
    Json::Value out = MakeMediaOk(result.data);
    resp.SetJsonBody(out.toStyledString());
}

void Http2MediaHandlers::HandleUploadMediaChunk(const Http2Request& req, Http2Response& resp)
{
    std::string body_str = req.body;
    Json::Value root;
    int uid = 0;
    std::string token;
    std::string upload_id;
    int index = -1;
    std::string chunk_data_base64;

    if (Http2MediaSupport::ParseJsonBody(body_str, root)) {
        uid = root.get("uid", 0).asInt();
        token = root.get("token", "").asString();
        upload_id = root.get("upload_id", "").asString();
        index = root.get("index", -1).asInt();
        chunk_data_base64 = root.get("data_base64", "").asString();
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
    Json::Value out = MakeMediaOk(result.data);
    resp.SetJsonBody(out.toStyledString());
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
    Json::Value out = MakeMediaOk(result.data);
    resp.SetJsonBody(out.toStyledString());
}

void Http2MediaHandlers::HandleUploadMediaComplete(const Http2Request& req, Http2Response& resp)
{
    Json::Value root;
    if (!Http2MediaSupport::ParseJsonBody(req.body, root)) {
        resp.SetJsonBody(MakeMediaError(1, "invalid json").toStyledString());
        return;
    }

    const int uid = root.get("uid", 0).asInt();
    const std::string token = root.get("token", "").asString();
    const std::string upload_id = root.get("upload_id", "").asString();

    auto result = Http2MediaSupport::HandleUploadMediaComplete(uid, token, upload_id);
    Json::Value out = MakeMediaOk(result.data);
    resp.SetJsonBody(out.toStyledString());
}

void Http2MediaHandlers::HandleUploadMedia(const Http2Request& req, Http2Response& resp)
{
    Json::Value root;
    if (!Http2MediaSupport::ParseJsonBody(req.body, root)) {
        resp.SetJsonBody(MakeMediaError(1, "invalid json").toStyledString());
        return;
    }

    const int uid = root.get("uid", 0).asInt();
    const std::string token = root.get("token", "").asString();
    const std::string media_type = root.get("media_type", "file").asString();
    const std::string file_name = root.get("file_name", "").asString();
    const std::string mime = root.get("mime", "").asString();
    const std::string data_base64 = root.get("data_base64", "").asString();

    auto result = Http2MediaSupport::HandleUploadMediaSimple(
        uid, token, media_type, file_name, mime, data_base64);
    Json::Value out = MakeMediaOk(result.data);
    resp.SetJsonBody(out.toStyledString());
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
        resp.SetJsonBody(MakeMediaError(1, "missing media key or auth params").toStyledString());
        return;
    }

    auto result = Http2MediaSupport::HandleMediaDownloadInfo(uid, token, media_key);
    if (result.error != 0) {
        Json::Value out = MakeMediaError(result.error, result.message);
        resp.SetJsonBody(out.toStyledString());
        return;
    }

    if (result.data.isMember("redirect")) {
        resp.SetStatus(302, "Found");
        resp.SetHeader("Location", result.data["redirect"].asString());
        resp.SetJsonBody(R"({})");
        return;
    }

    if (result.data.isMember("path")) {
        resp.SetHeader("Content-Type", result.data["content_type"].asString());
        resp.SetHeader("X-Media-Path", result.data["path"].asString());
        resp.SetJsonBody(R"({"status":"file","path":""})".replace(26, 5, result.data["path"].asString()));
        return;
    }

    Json::Value out = MakeMediaError(1, "file not found");
    resp.SetJsonBody(out.toStyledString());
}
