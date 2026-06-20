#include "WinCompat.h"
#include "json/GlazeCompat.h"
#include "Http2MediaHandlers.h"
#include "Http2MediaSupport.h"
#include "services/media/MediaPublicDtos.h"

static memochat::json::JsonValue MakeMediaError(int error_code, const std::string& message)
{
    memochat::json::JsonValue resp;
    resp["error"] = static_cast<double>(error_code);
    if (!message.empty())
        resp["message"] = message;
    return resp;
}

static memochat::json::JsonValue MakeMediaOk(const memochat::json::JsonValue& data)
{
    memochat::json::JsonValue resp;
    resp["error"] = 0.0;
    if (memochat::json::glaze_is_object(data))
    {
        for (const auto& key : memochat::json::getMemberNames(data))
        {
            resp[key] = memochat::json::glaze_get(data, key);
        }
    }
    return resp;
}

static memochat::json::JsonValue MakeMediaResponse(const Http2MediaSupport::MediaResult& result)
{
    memochat::json::JsonValue resp = MakeMediaOk(result.data);
    resp["error"] = static_cast<double>(result.error);
    if (!result.message.empty())
    {
        resp["message"] = result.message;
    }
    return resp;
}

static int ExtractIntHeader(const Http2Request& req, const std::string& name, int default_val = 0)
{
    auto it = req.headers.find(name);
    if (it != req.headers.end())
    {
        try
        {
            return std::stoi(it->second);
        }
        catch (...)
        {
        }
    }
    return default_val;
}

void Http2MediaHandlers::HandleUploadMediaInit(const Http2Request& req, Http2Response& resp)
{
    memochat::json::JsonValue root;
    if (!Http2MediaSupport::ParseJsonBody(req.body, root))
    {
        resp.SetJsonBody(memochat::json::glaze_stringify(MakeMediaError(1, "invalid json")));
        return;
    }

    memochat::media::MediaUploadInitRequestDto upload_request;
    if (!memochat::media::DecodeMediaUploadInitRequest(req.body, &upload_request))
    {
        resp.SetJsonBody(memochat::json::glaze_stringify(MakeMediaError(1, "invalid json")));
        return;
    }

    auto result = Http2MediaSupport::HandleUploadMediaInit(upload_request.uid,
                                                           upload_request.token,
                                                           upload_request.media_type,
                                                           upload_request.file_name,
                                                           upload_request.mime,
                                                           upload_request.file_size);
    memochat::json::JsonValue out = MakeMediaResponse(result);
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
    bool json_chunk = false;

    if (Http2MediaSupport::ParseJsonBody(body_str, root))
    {
        json_chunk = true;
        memochat::media::MediaUploadChunkJsonRequestDto chunk_request;
        if (!memochat::media::DecodeMediaUploadChunkJsonRequest(body_str, &chunk_request))
        {
            resp.SetJsonBody(memochat::json::glaze_stringify(MakeMediaError(1, "invalid json")));
            return;
        }
        uid = chunk_request.uid;
        token = chunk_request.token;
        upload_id = chunk_request.upload_id;
        index = chunk_request.index;
        chunk_data_base64 = chunk_request.data_base64;
    }
    else
    {
        uid = ExtractIntHeader(req, "X-Uid");
        auto it = req.headers.find("X-Token");
        if (it != req.headers.end())
            token = it->second;
        it = req.headers.find("X-Upload-Id");
        if (it != req.headers.end())
            upload_id = it->second;
        index = ExtractIntHeader(req, "X-Chunk-Index");
    }

    auto result = json_chunk
                      ? Http2MediaSupport::HandleUploadMediaChunk(uid, token, upload_id, index, chunk_data_base64)
                      : Http2MediaSupport::HandleUploadMediaChunkBytes(uid, token, upload_id, index, body_str);
    memochat::json::JsonValue out = MakeMediaResponse(result);
    resp.SetJsonBody(memochat::json::glaze_stringify(out));
}

void Http2MediaHandlers::HandleUploadMediaStatus(const Http2Request& req, Http2Response& resp)
{
    int uid = 0;
    std::string token;
    std::string upload_id;

    auto it = req.headers.find("X-Uid");
    if (it != req.headers.end())
    {
        try
        {
            uid = std::stoi(it->second);
        }
        catch (...)
        {
        }
    }
    it = req.headers.find("X-Token");
    if (it != req.headers.end())
        token = it->second;
    it = req.headers.find("X-Upload-Id");
    if (it != req.headers.end())
        upload_id = it->second;

    auto result = Http2MediaSupport::HandleUploadMediaStatus(uid, token, upload_id);
    memochat::json::JsonValue out = MakeMediaResponse(result);
    resp.SetJsonBody(memochat::json::glaze_stringify(out));
}

void Http2MediaHandlers::HandleUploadMediaComplete(const Http2Request& req, Http2Response& resp)
{
    memochat::json::JsonValue root;
    if (!Http2MediaSupport::ParseJsonBody(req.body, root))
    {
        resp.SetJsonBody(memochat::json::glaze_stringify(MakeMediaError(1, "invalid json")));
        return;
    }

    memochat::media::MediaUploadCompleteRequestDto upload_request;
    if (!memochat::media::DecodeMediaUploadCompleteRequest(req.body, &upload_request))
    {
        resp.SetJsonBody(memochat::json::glaze_stringify(MakeMediaError(1, "invalid json")));
        return;
    }

    auto result = Http2MediaSupport::HandleUploadMediaComplete(upload_request.uid,
                                                               upload_request.token,
                                                               upload_request.upload_id);
    memochat::json::JsonValue out = MakeMediaResponse(result);
    resp.SetJsonBody(memochat::json::glaze_stringify(out));
}

void Http2MediaHandlers::HandleUploadMedia(const Http2Request& req, Http2Response& resp)
{
    memochat::json::JsonValue root;
    if (!Http2MediaSupport::ParseJsonBody(req.body, root))
    {
        resp.SetJsonBody(memochat::json::glaze_stringify(MakeMediaError(1, "invalid json")));
        return;
    }

    memochat::media::MediaUploadSimpleRequestDto upload_request;
    if (!memochat::media::DecodeMediaUploadSimpleRequest(req.body, &upload_request))
    {
        resp.SetJsonBody(memochat::json::glaze_stringify(MakeMediaError(1, "invalid json")));
        return;
    }

    auto result = Http2MediaSupport::HandleUploadMediaSimple(upload_request.uid,
                                                             upload_request.token,
                                                             upload_request.media_type,
                                                             upload_request.file_name,
                                                             upload_request.mime,
                                                             upload_request.data_base64);
    memochat::json::JsonValue out = MakeMediaResponse(result);
    resp.SetJsonBody(memochat::json::glaze_stringify(out));
}

void Http2MediaHandlers::HandleMediaDownload(const Http2Request& req, Http2Response& resp)
{
    int uid = 0;
    std::string token;
    std::string media_key;

    auto it = req.headers.find("X-Uid");
    if (it != req.headers.end())
    {
        try
        {
            uid = std::stoi(it->second);
        }
        catch (...)
        {
        }
    }
    it = req.headers.find("X-Token");
    if (it != req.headers.end())
        token = it->second;
    it = req.headers.find("X-Asset");
    if (it != req.headers.end())
        media_key = it->second;

    if (uid <= 0 || token.empty() || media_key.empty())
    {
        resp.SetJsonBody(memochat::json::glaze_stringify(MakeMediaError(1, "missing media key or auth params")));
        return;
    }

    auto result = Http2MediaSupport::HandleMediaDownloadInfo(uid, token, media_key);
    if (result.error != 0)
    {
        memochat::json::JsonValue out = MakeMediaError(result.error, result.message);
        resp.SetJsonBody(memochat::json::glaze_stringify(out));
        return;
    }

    if (memochat::json::glaze_has_key(result.data, "data"))
    {
        resp.status_code = 200;
        resp.content_type = memochat::json::glaze_safe_get<std::string>(result.data, "content_type", "");
        resp.body = memochat::json::glaze_safe_get<std::string>(result.data, "data", "");
        return;
    }

    if (memochat::json::glaze_has_key(result.data, "path"))
    {
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
