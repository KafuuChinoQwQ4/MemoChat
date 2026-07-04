#include "services/media/MediaUploadSessionDto.hpp"

#include "json/TypedJsonCodec.hpp"

#include <algorithm>
#include <utility>

import memochat.media.upload_session_algorithms;

namespace memochat::media
{
namespace
{

std::vector<int> NormalizeSessionGrantUids(std::vector<int> values)
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

} // namespace

MediaUploadSessionDto NormalizeMediaUploadSession(MediaUploadSessionDto session)
{
    if (upload_session::modules::ShouldUseDefaultMediaType(session.media_type.empty()))
    {
        session.media_type = "file";
    }
    if (upload_session::modules::ShouldUseDefaultStorageProvider(session.storage_provider.empty()))
    {
        session.storage_provider = "local";
    }
    session.grant_uids = NormalizeSessionGrantUids(std::move(session.grant_uids));
    if (session.grant_group_id < 0)
    {
        session.grant_group_id = 0;
    }
    return session;
}

bool IsValidMediaUploadSession(const MediaUploadSessionDto& session)
{
    return upload_session::modules::HasValidUploadSessionShape(session.uid > 0,
                                                               !session.upload_id.empty(),
                                                               !session.file_name.empty(),
                                                               session.file_size > 0,
                                                               session.chunk_size > 0,
                                                               session.total_chunks > 0);
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
