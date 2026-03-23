#include "WinCompat.h"
#include "DrogonMediaHandlers.h"
#include "DrogonMediaSupport.h"
#include <drogon/HttpResponse.h>

using namespace drogon;

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

void DrogonMediaHandlers::HandleUploadMediaInit(const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    Json::Value root;
    if (!DrogonMediaSupport::ParseJsonBody(body_str, root)) {
        callback(HttpResponse::newHttpJsonResponse(MakeMediaError(1, "invalid json")));
        return;
    }

    const int uid = root.get("uid", 0).asInt();
    const std::string token = root.get("token", "").asString();
    const std::string media_type = root.get("media_type", "file").asString();
    const std::string file_name = root.get("file_name", "").asString();
    const std::string mime = root.get("mime", "").asString();
    const int64_t file_size = root.get("file_size", 0).asInt64();

    auto result = DrogonMediaSupport::HandleUploadMediaInit(
        uid, token, media_type, file_name, mime, file_size);
    Json::Value out = MakeMediaOk(result.data);
    callback(HttpResponse::newHttpJsonResponse(out));
}

void DrogonMediaHandlers::HandleUploadMediaChunk(const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    std::string body_str{req->getBody()};
    Json::Value root;
    int uid = 0;
    std::string token;
    std::string upload_id;
    int index = -1;
    std::string chunk_data_base64;

    if (DrogonMediaSupport::ParseJsonBody(body_str, root)) {
        uid = root.get("uid", 0).asInt();
        token = root.get("token", "").asString();
        upload_id = root.get("upload_id", "").asString();
        index = root.get("index", -1).asInt();
        chunk_data_base64 = root.get("data_base64", "").asString();
    } else {
        uid = std::atoi(req->getHeader("X-Uid").c_str());
        token = req->getHeader("X-Token");
        upload_id = req->getHeader("X-Upload-Id");
        index = std::atoi(req->getHeader("X-Chunk-Index").c_str());
        chunk_data_base64 = body_str;
    }

    auto result = DrogonMediaSupport::HandleUploadMediaChunk(
        uid, token, upload_id, index, chunk_data_base64);
    Json::Value out = MakeMediaOk(result.data);
    callback(HttpResponse::newHttpJsonResponse(out));
}

void DrogonMediaHandlers::HandleUploadMediaStatus(const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    // Parse query params from URL
    const std::string query = req->getQuery();
    int uid = 0;
    std::string token;
    std::string upload_id;
    
    // Simple query string parsing
    if (!query.empty()) {
        size_t uid_pos = query.find("uid=");
        if (uid_pos != std::string::npos) {
            uid = std::atoi(query.c_str() + uid_pos + 4);
        }
        size_t token_pos = query.find("token=");
        if (token_pos != std::string::npos) {
            size_t end = query.find('&', token_pos);
            token = query.substr(token_pos + 6, end - token_pos - 6);
        }
        size_t upload_pos = query.find("upload_id=");
        if (upload_pos != std::string::npos) {
            size_t end = query.find('&', upload_pos);
            upload_id = query.substr(upload_pos + 10, end - upload_pos - 10);
        }
    }

    auto result = DrogonMediaSupport::HandleUploadMediaStatus(uid, token, upload_id);
    Json::Value out = MakeMediaOk(result.data);
    callback(HttpResponse::newHttpJsonResponse(out));
}

void DrogonMediaHandlers::HandleUploadMediaComplete(const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    Json::Value root;
    if (!DrogonMediaSupport::ParseJsonBody(body_str, root)) {
        callback(HttpResponse::newHttpJsonResponse(MakeMediaError(1, "invalid json")));
        return;
    }

    const int uid = root.get("uid", 0).asInt();
    const std::string token = root.get("token", "").asString();
    const std::string upload_id = root.get("upload_id", "").asString();

    auto result = DrogonMediaSupport::HandleUploadMediaComplete(uid, token, upload_id);
    Json::Value out = MakeMediaOk(result.data);
    callback(HttpResponse::newHttpJsonResponse(out));
}

void DrogonMediaHandlers::HandleUploadMedia(const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    Json::Value root;
    if (!DrogonMediaSupport::ParseJsonBody(body_str, root)) {
        callback(HttpResponse::newHttpJsonResponse(MakeMediaError(1, "invalid json")));
        return;
    }

    const int uid = root.get("uid", 0).asInt();
    const std::string token = root.get("token", "").asString();
    const std::string media_type = root.get("media_type", "file").asString();
    const std::string file_name = root.get("file_name", "").asString();
    const std::string mime = root.get("mime", "").asString();
    const std::string data_base64 = root.get("data_base64", "").asString();

    auto result = DrogonMediaSupport::HandleUploadMediaSimple(
        uid, token, media_type, file_name, mime, data_base64);
    Json::Value out = MakeMediaOk(result.data);
    callback(HttpResponse::newHttpJsonResponse(out));
}

void DrogonMediaHandlers::HandleMediaDownload(const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    // Parse query params from URL
    const std::string query = req->getQuery();
    int uid = 0;
    std::string token;
    std::string media_key;
    
    // Simple query string parsing
    if (!query.empty()) {
        size_t uid_pos = query.find("uid=");
        if (uid_pos != std::string::npos) {
            uid = std::atoi(query.c_str() + uid_pos + 4);
        }
        size_t token_pos = query.find("token=");
        if (token_pos != std::string::npos) {
            size_t end = query.find('&', token_pos);
            token = query.substr(token_pos + 6, end - token_pos - 6);
        }
        size_t asset_pos = query.find("asset=");
        if (asset_pos != std::string::npos) {
            size_t end = query.find('&', asset_pos);
            media_key = query.substr(asset_pos + 6, end - asset_pos - 6);
        }
    }

    if (uid <= 0 || token.empty() || media_key.empty()) {
        callback(HttpResponse::newHttpJsonResponse(
            MakeMediaError(1, "missing media key or auth params")));
        return;
    }

    auto result = DrogonMediaSupport::HandleMediaDownloadInfo(uid, token, media_key);
    if (result.error != 0) {
        Json::Value out = MakeMediaError(result.error, result.message);
        callback(HttpResponse::newHttpJsonResponse(out));
        return;
    }

    if (result.data.isMember("redirect")) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k302Found);
        resp->addHeader("Location", result.data["redirect"].asString());
        callback(resp);
        return;
    }

    if (result.data.isMember("path")) {
        auto resp = HttpResponse::newFileResponse(result.data["path"].asString());
        std::string content_type = result.data["content_type"].asString();
        resp->setContentTypeCode(CT_CUSTOM);
        resp->addHeader("Content-Type", content_type);
        callback(resp);
        return;
    }

    Json::Value out = MakeMediaError(1, "file not found");
    callback(HttpResponse::newHttpJsonResponse(out));
}
