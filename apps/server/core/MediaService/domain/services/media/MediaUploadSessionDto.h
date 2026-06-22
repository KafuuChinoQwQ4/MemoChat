#pragma once

#include "json/GlazeCompat.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace memochat::media
{

struct MediaUploadSessionDto
{
    int uid = 0;
    std::string upload_id;
    std::string media_type;
    std::string file_name;
    std::string mime;
    int64_t file_size = 0;
    int chunk_size = 0;
    int total_chunks = 0;
    int64_t created_at = 0;
    int64_t expires_at = 0;
    std::string storage_provider;
};

bool IsValidMediaUploadSession(const MediaUploadSessionDto& session);
MediaUploadSessionDto NormalizeMediaUploadSession(MediaUploadSessionDto session);
bool EncodeMediaUploadSession(const MediaUploadSessionDto& session, std::string* out, std::string* error_out = nullptr);
bool DecodeMediaUploadSession(std::string_view body, MediaUploadSessionDto* out, std::string* error_out = nullptr);
memochat::json::JsonValue MediaUploadSessionToJsonValue(const MediaUploadSessionDto& session);
bool MediaUploadSessionFromJsonValue(const memochat::json::JsonValue& value,
                                     MediaUploadSessionDto* out,
                                     std::string* error_out = nullptr);

} // namespace memochat::media
