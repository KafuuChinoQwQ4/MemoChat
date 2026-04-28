#pragma once

#include <string>
#include <string_view>
#include "json/GlazeCompat.h"

namespace Http2MediaSupport {

struct MediaResult {
    int error = 0;
    std::string message;
    memochat::json::JsonValue data;
};

bool ParseJsonBody(std::string_view body_sv, memochat::json::JsonValue& root);
std::string DecodeBase64(const std::string& input);
bool ValidateUserToken(int uid, const std::string& token);

MediaResult HandleUploadMediaInit(int uid, const std::string& token,
    const std::string& media_type, const std::string& file_name,
    const std::string& mime, int64_t file_size);

MediaResult HandleUploadMediaChunk(int uid, const std::string& token,
    const std::string& upload_id, int index,
    const std::string& chunk_data_base64);

MediaResult HandleUploadMediaStatus(int uid, const std::string& token,
    const std::string& upload_id);

MediaResult HandleUploadMediaComplete(int uid, const std::string& token,
    const std::string& upload_id);

MediaResult HandleUploadMediaSimple(int uid, const std::string& token,
    const std::string& media_type, const std::string& file_name,
    const std::string& mime, const std::string& data_base64);

MediaResult HandleMediaDownloadInfo(int uid, const std::string& token,
    const std::string& media_key);

}  // namespace Http2MediaSupport

