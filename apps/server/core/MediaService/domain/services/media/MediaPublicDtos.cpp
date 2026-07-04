#include "services/media/MediaPublicDtos.hpp"

#include "json/GlazeCompat.hpp"
#include "json/TypedJsonCodec.hpp"

#include <algorithm>
#include <exception>
#include <utility>

import memochat.media.public_dto_algorithms;

namespace
{
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

std::vector<int> NormalizeGrantUids(std::vector<int> values)
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

std::vector<int> ParseGrantUids(const memochat::json::JsonValue& root)
{
    std::vector<int> values;
    if (!memochat::json::isMember(root, "grant_uids") || !root["grant_uids"].isArray())
    {
        return values;
    }
    for (const auto& item : root["grant_uids"])
    {
        const int uid = item.asInt();
        if (uid > 0)
        {
            values.push_back(uid);
        }
    }
    return NormalizeGrantUids(std::move(values));
}

} // namespace

namespace memochat::media
{

MediaUploadInitRequestDto NormalizeMediaUploadInitRequest(MediaUploadInitRequestDto request)
{
    if (public_dto::modules::ShouldUseDefaultMediaType(request.media_type.empty()))
    {
        request.media_type = "file";
    }
    request.grant_uids = NormalizeGrantUids(std::move(request.grant_uids));
    if (request.grant_group_id < 0)
    {
        request.grant_group_id = 0;
    }
    return request;
}

MediaUploadSimpleRequestDto NormalizeMediaUploadSimpleRequest(MediaUploadSimpleRequestDto request)
{
    if (public_dto::modules::ShouldUseDefaultMediaType(request.media_type.empty()))
    {
        request.media_type = "file";
    }
    request.grant_uids = NormalizeGrantUids(std::move(request.grant_uids));
    if (request.grant_group_id < 0)
    {
        request.grant_group_id = 0;
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
    parsed.grant_uids = ParseGrantUids(root);
    parsed.grant_group_id = memochat::json::glaze_safe_get<int64_t>(root, "grant_group_id", 0LL);
    parsed.grant_public = memochat::json::glaze_safe_get<bool>(root, "grant_public", false);
    parsed.grant_friends = memochat::json::glaze_safe_get<bool>(root, "grant_friends", false);
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

bool DecodeMediaUploadCompleteRequest(std::string_view body, MediaUploadCompleteRequestDto* out, std::string* error_out)
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
    parsed.grant_uids = ParseGrantUids(root);
    parsed.grant_group_id = memochat::json::glaze_safe_get<int64_t>(root, "grant_group_id", 0LL);
    parsed.grant_public = memochat::json::glaze_safe_get<bool>(root, "grant_public", false);
    parsed.grant_friends = memochat::json::glaze_safe_get<bool>(root, "grant_friends", false);
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
    return public_dto::modules::ContainsApplicationJson(content_type.data(), content_type.size());
}

} // namespace memochat::media
