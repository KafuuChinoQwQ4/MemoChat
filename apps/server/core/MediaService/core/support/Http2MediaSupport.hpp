#pragma once

#include <string>
#include <string_view>
#include <vector>
#include "json/GlazeCompat.hpp"

namespace Http2MediaSupport
{

struct MediaResult
{
    int error = 0;
    std::string message;
    memochat::json::JsonValue data;
};

bool ParseJsonBody(std::string_view body_sv, memochat::json::JsonValue& root);
std::string DecodeBase64(const std::string& input);

MediaResult HandleUploadMediaInit(const std::string& access_token,
                                  const std::string& media_type,
                                  const std::string& file_name,
                                  const std::string& mime,
                                  int64_t file_size,
                                  std::vector<int> grant_uids = {},
                                  int64_t grant_group_id = 0,
                                  bool grant_public = false,
                                  bool grant_friends = false);

MediaResult HandleUploadMediaChunk(const std::string& access_token,
                                   const std::string& upload_id,
                                   int index,
                                   const std::string& chunk_data_base64);

MediaResult HandleUploadMediaChunkBytes(const std::string& access_token,
                                        const std::string& upload_id,
                                        int index,
                                        std::string_view chunk_data);

MediaResult HandleUploadMediaStatus(const std::string& access_token, const std::string& upload_id);

MediaResult HandleUploadMediaComplete(const std::string& access_token, const std::string& upload_id);

MediaResult HandleUploadMediaSimple(const std::string& access_token,
                                    const std::string& media_type,
                                    const std::string& file_name,
                                    const std::string& mime,
                                    const std::string& data_base64,
                                    std::vector<int> grant_uids = {},
                                    int64_t grant_group_id = 0,
                                    bool grant_public = false,
                                    bool grant_friends = false);

MediaResult HandleMediaDownloadInfo(const std::string& access_token, const std::string& media_key);

} // namespace Http2MediaSupport
