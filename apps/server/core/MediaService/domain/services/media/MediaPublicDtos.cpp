#include "services/media/MediaPublicDtos.h"

#include "json/GlazeCompat.h"
#include "json/TypedJsonCodec.h"

#include <algorithm>
#include <cctype>
#include <exception>
#include <utility>

namespace
{

std::string LowercaseAscii(std::string_view value)
{
    std::string out(value);
    std::transform(out.begin(),
                   out.end(),
                   out.begin(),
                   [](unsigned char c)
                   {
                       return static_cast<char>(std::tolower(c));
                   });
    return out;
}

bool ParseJsonForMediaPublic(std::string_view body, memochat::json::JsonValue* out, std::string* error_out)
{
    if (out == nullptr)
    {
        if (error_out != nullptr)
        {
            *error_out = "output pointer is null";
        }
        return false;
    }
    if (!memochat::json::glaze_parse(*out, body))
    {
        if (error_out != nullptr)
        {
            *error_out = "invalid json";
        }
        return false;
    }
    return true;
}

template <typename T> bool WriteTypedJsonNoThrow(const T& value, std::string* out, std::string* error_out)
{
    try
    {
        return memochat::json::WriteTypedJson(value, out, error_out);
    }
    catch (const std::exception& e)
    {
        if (error_out != nullptr)
        {
            *error_out = e.what();
        }
        return false;
    }
}

template <typename T> memochat::json::JsonValue TypedJsonToJsonValue(const T& value)
{
    std::string body;
    if (!WriteTypedJsonNoThrow(value, &body, nullptr))
    {
        return memochat::json::JsonValue(memochat::json::object_t{});
    }

    memochat::json::JsonValue root;
    if (!memochat::json::glaze_parse(root, body))
    {
        return memochat::json::JsonValue(memochat::json::object_t{});
    }
    return root;
}

} // namespace

namespace memochat::media
{

MediaUploadInitRequestDto NormalizeMediaUploadInitRequest(MediaUploadInitRequestDto request)
{
    if (request.media_type.empty())
    {
        request.media_type = "file";
    }
    return request;
}

MediaUploadSimpleRequestDto NormalizeMediaUploadSimpleRequest(MediaUploadSimpleRequestDto request)
{
    if (request.media_type.empty())
    {
        request.media_type = "file";
    }
    return request;
}

bool DecodeMediaUploadInitRequest(std::string_view body, MediaUploadInitRequestDto* out, std::string* error_out)
{
    if (out == nullptr)
    {
        if (error_out != nullptr)
        {
            *error_out = "output pointer is null";
        }
        return false;
    }
    memochat::json::JsonValue root;
    if (!ParseJsonForMediaPublic(body, &root, error_out))
    {
        return false;
    }
    MediaUploadInitRequestDto parsed;
    parsed.uid = memochat::json::glaze_safe_get<int>(root, "uid", 0);
    parsed.token = memochat::json::glaze_safe_get<std::string>(root, "token", "");
    parsed.media_type = memochat::json::glaze_safe_get<std::string>(root, "media_type", "file");
    parsed.file_name = memochat::json::glaze_safe_get<std::string>(root, "file_name", "");
    parsed.mime = memochat::json::glaze_safe_get<std::string>(root, "mime", "");
    parsed.file_size = memochat::json::glaze_safe_get<int64_t>(root, "file_size", 0LL);
    *out = NormalizeMediaUploadInitRequest(std::move(parsed));
    return true;
}

bool DecodeMediaUploadChunkJsonRequest(std::string_view body,
                                       MediaUploadChunkJsonRequestDto* out,
                                       std::string* error_out)
{
    if (out == nullptr)
    {
        if (error_out != nullptr)
        {
            *error_out = "output pointer is null";
        }
        return false;
    }
    memochat::json::JsonValue root;
    if (!ParseJsonForMediaPublic(body, &root, error_out))
    {
        return false;
    }
    out->uid = memochat::json::glaze_safe_get<int>(root, "uid", 0);
    out->token = memochat::json::glaze_safe_get<std::string>(root, "token", "");
    out->upload_id = memochat::json::glaze_safe_get<std::string>(root, "upload_id", "");
    out->index = memochat::json::glaze_safe_get<int>(root, "index", -1);
    out->data_base64 = memochat::json::glaze_safe_get<std::string>(root, "data_base64", "");
    return true;
}

bool DecodeMediaUploadCompleteRequest(std::string_view body,
                                      MediaUploadCompleteRequestDto* out,
                                      std::string* error_out)
{
    if (out == nullptr)
    {
        if (error_out != nullptr)
        {
            *error_out = "output pointer is null";
        }
        return false;
    }
    memochat::json::JsonValue root;
    if (!ParseJsonForMediaPublic(body, &root, error_out))
    {
        return false;
    }
    out->uid = memochat::json::glaze_safe_get<int>(root, "uid", 0);
    out->token = memochat::json::glaze_safe_get<std::string>(root, "token", "");
    out->upload_id = memochat::json::glaze_safe_get<std::string>(root, "upload_id", "");
    return true;
}

bool DecodeMediaUploadSimpleRequest(std::string_view body, MediaUploadSimpleRequestDto* out, std::string* error_out)
{
    if (out == nullptr)
    {
        if (error_out != nullptr)
        {
            *error_out = "output pointer is null";
        }
        return false;
    }
    memochat::json::JsonValue root;
    if (!ParseJsonForMediaPublic(body, &root, error_out))
    {
        return false;
    }
    MediaUploadSimpleRequestDto parsed;
    parsed.uid = memochat::json::glaze_safe_get<int>(root, "uid", 0);
    parsed.token = memochat::json::glaze_safe_get<std::string>(root, "token", "");
    parsed.media_type = memochat::json::glaze_safe_get<std::string>(root, "media_type", "file");
    parsed.file_name = memochat::json::glaze_safe_get<std::string>(root, "file_name", "");
    parsed.mime = memochat::json::glaze_safe_get<std::string>(root, "mime", "");
    parsed.data_base64 = memochat::json::glaze_safe_get<std::string>(root, "data_base64", "");
    *out = NormalizeMediaUploadSimpleRequest(std::move(parsed));
    return true;
}

memochat::json::JsonValue MediaUploadInitResponseToJsonValue(const MediaUploadInitResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

memochat::json::JsonValue MediaUploadChunkResponseToJsonValue(const MediaUploadChunkResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

memochat::json::JsonValue MediaUploadStatusResponseToJsonValue(const MediaUploadStatusResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

memochat::json::JsonValue MediaUploadAssetResponseToJsonValue(const MediaUploadAssetResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

bool IsJsonContentType(std::string_view content_type)
{
    return LowercaseAscii(content_type).find("application/json") != std::string::npos;
}

} // namespace memochat::media
