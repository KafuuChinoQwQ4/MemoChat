#include "services/media/MediaUploadSessionDto.h"

#include "json/TypedJsonCodec.h"

#include <utility>

namespace memochat::media
{

MediaUploadSessionDto NormalizeMediaUploadSession(MediaUploadSessionDto session)
{
    if (session.media_type.empty())
    {
        session.media_type = "file";
    }
    if (session.storage_provider.empty())
    {
        session.storage_provider = "local";
    }
    return session;
}

bool IsValidMediaUploadSession(const MediaUploadSessionDto& session)
{
    return session.uid > 0 && !session.upload_id.empty() && !session.file_name.empty() && session.file_size > 0 &&
           session.chunk_size > 0 && session.total_chunks > 0;
}

bool EncodeMediaUploadSession(const MediaUploadSessionDto& session, std::string* out, std::string* error_out)
{
    return memochat::json::WriteTypedJson(NormalizeMediaUploadSession(session), out, error_out);
}

bool DecodeMediaUploadSession(std::string_view body, MediaUploadSessionDto* out, std::string* error_out)
{
    if (out == nullptr)
    {
        if (error_out != nullptr)
        {
            *error_out = "output pointer is null";
        }
        return false;
    }
    MediaUploadSessionDto parsed;
    if (!memochat::json::ReadTypedJson(body, &parsed, error_out))
    {
        return false;
    }
    parsed = NormalizeMediaUploadSession(std::move(parsed));
    if (!IsValidMediaUploadSession(parsed))
    {
        return false;
    }
    *out = std::move(parsed);
    return true;
}

memochat::json::JsonValue MediaUploadSessionToJsonValue(const MediaUploadSessionDto& session)
{
    std::string body;
    if (!EncodeMediaUploadSession(session, &body))
    {
        return memochat::json::JsonValue(memochat::json::object_t{});
    }
    memochat::json::JsonValue value;
    if (!memochat::json::glaze_parse(value, body))
    {
        return memochat::json::JsonValue(memochat::json::object_t{});
    }
    return value;
}

bool MediaUploadSessionFromJsonValue(const memochat::json::JsonValue& value,
                                     MediaUploadSessionDto* out,
                                     std::string* error_out)
{
    return DecodeMediaUploadSession(memochat::json::glaze_stringify(value), out, error_out);
}

} // namespace memochat::media
