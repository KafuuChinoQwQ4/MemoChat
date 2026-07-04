#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "json/GlazeCompat.hpp"

namespace memochat::media
{

struct MediaUploadInitRequestDto
{
    int uid = 0;
    std::string token;
    std::string media_type;
    std::string file_name;
    std::string mime;
    int64_t file_size = 0;
    std::vector<int> grant_uids;
    int64_t grant_group_id = 0;
    bool grant_public = false;
    bool grant_friends = false;
};

struct MediaUploadChunkJsonRequestDto
{
    int uid = 0;
    std::string token;
    std::string upload_id;
    int index = -1;
    std::string data_base64;
};

struct MediaUploadCompleteRequestDto
{
    int uid = 0;
    std::string token;
    std::string upload_id;
};

struct MediaUploadSimpleRequestDto
{
    int uid = 0;
    std::string token;
    std::string media_type;
    std::string file_name;
    std::string mime;
    std::string data_base64;
    std::vector<int> grant_uids;
    int64_t grant_group_id = 0;
    bool grant_public = false;
    bool grant_friends = false;
};

struct MediaUploadInitResponseDto
{
    std::string upload_id;
    int chunk_size = 0;
    int total_chunks = 0;
    std::vector<int> uploaded_chunks;
};

struct MediaUploadChunkResponseDto
{
    std::string upload_id;
    int index = 0;
    int64_t size = 0;
};

struct MediaUploadStatusResponseDto
{
    std::string upload_id;
    int total_chunks = 0;
    int chunk_size = 0;
    std::vector<int> uploaded_chunks;
};

struct MediaUploadAssetResponseDto
{
    std::string media_key;
    std::string media_type;
    std::string file_name;
    std::string mime;
    int64_t size = 0;
    std::string url;
};

MediaUploadInitRequestDto NormalizeMediaUploadInitRequest(MediaUploadInitRequestDto request);
MediaUploadSimpleRequestDto NormalizeMediaUploadSimpleRequest(MediaUploadSimpleRequestDto request);

bool DecodeMediaUploadInitRequest(std::string_view body,
                                  MediaUploadInitRequestDto* out,
                                  std::string* error_out = nullptr);
bool DecodeMediaUploadChunkJsonRequest(std::string_view body,
                                       MediaUploadChunkJsonRequestDto* out,
                                       std::string* error_out = nullptr);
bool DecodeMediaUploadCompleteRequest(std::string_view body,
                                      MediaUploadCompleteRequestDto* out,
                                      std::string* error_out = nullptr);
bool DecodeMediaUploadSimpleRequest(std::string_view body,
                                    MediaUploadSimpleRequestDto* out,
                                    std::string* error_out = nullptr);

memochat::json::JsonValue MediaUploadInitResponseToJsonValue(const MediaUploadInitResponseDto& response);
memochat::json::JsonValue MediaUploadChunkResponseToJsonValue(const MediaUploadChunkResponseDto& response);
memochat::json::JsonValue MediaUploadStatusResponseToJsonValue(const MediaUploadStatusResponseDto& response);
memochat::json::JsonValue MediaUploadAssetResponseToJsonValue(const MediaUploadAssetResponseDto& response);

bool IsJsonContentType(std::string_view content_type);

} // namespace memochat::media
