#pragma once

#include "json/GlazeCompat.hpp"

#include <glaze/glaze.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

struct MomentContentInfo;
struct MomentCommentInfo;
struct MomentInfo;
struct MomentItemInfo;
struct MomentLikeInfo;

namespace memochat::gate::services::moments
{

struct MomentUserProfileDto
{
    std::string user_id;
    std::string user_name;
    std::string user_nick;
    std::string user_icon;
};

struct MomentItemOutputDto
{
    int seq = 0;
    std::string media_type;
    std::string media_key;
    std::string thumb_key;
    std::string content;
    int width = 0;
    int height = 0;
    int duration_ms = 0;
};

struct MomentLikeOutputDto
{
    int uid = 0;
    std::string user_nick;
    std::string user_icon;
    int64_t created_at = 0;
};

struct MomentCommentOutputDto
{
    int64_t id = 0;
    int64_t moment_id = 0;
    int uid = 0;
    std::string user_nick;
    std::string user_icon;
    std::string content;
    int reply_uid = 0;
    std::string reply_nick;
    int64_t created_at = 0;
    int like_count = 0;
    bool has_liked = false;
    std::vector<std::string> like_names;
    std::vector<MomentLikeOutputDto> likes;
};

struct MomentOutputDto
{
    int64_t moment_id = 0;
    int uid = 0;
    int visibility = 0;
    std::string location;
    int64_t created_at = 0;
    int like_count = 0;
    int comment_count = 0;
    bool has_liked = false;
    std::string user_id;
    std::string user_name;
    std::string user_nick;
    std::string user_icon;
    std::vector<MomentItemOutputDto> items;
    std::vector<std::string> like_names;
    std::vector<MomentLikeOutputDto> likes;
    std::vector<MomentCommentOutputDto> comments;
};

struct MomentIdResponseDto
{
    int error = 0;
    int64_t moment_id = 0;
};

struct MomentListResponseDto
{
    int error = 0;
    bool has_more = false;
    std::vector<MomentOutputDto> moments;
};

struct MomentDetailResponseDto
{
    int error = 0;
    MomentOutputDto moment;
};

struct MomentLikeResponseDto
{
    int error = 0;
    int64_t moment_id = 0;
    bool has_liked = false;
    int like_count = 0;
};

struct MomentCommentMutationResponseDto
{
    int error = 0;
    int64_t moment_id = 0;
    bool delete_ = false;
    std::optional<int> comment_count;
};

struct MomentCommentListResponseDto
{
    int error = 0;
    int64_t moment_id = 0;
    bool has_more = false;
    std::optional<int> comment_count;
    std::vector<MomentCommentOutputDto> comments;
};

struct MomentCommentLikeResponseDto
{
    int error = 0;
    int64_t comment_id = 0;
    bool has_liked = false;
    int like_count = 0;
    std::vector<std::string> like_names;
    std::vector<MomentLikeOutputDto> likes;
};

MomentItemOutputDto ToMomentItemOutputDto(const MomentItemInfo& item);
MomentLikeOutputDto ToMomentLikeOutputDto(const MomentLikeInfo& like);
std::vector<MomentLikeOutputDto> ToMomentLikeOutputDtos(const std::vector<MomentLikeInfo>& likes);
std::vector<std::string> ToMomentLikeNames(const std::vector<MomentLikeInfo>& likes);
MomentCommentOutputDto ToMomentCommentOutputDto(const MomentCommentInfo& comment);
std::vector<MomentCommentOutputDto> ToMomentCommentOutputDtos(const std::vector<MomentCommentInfo>& comments);
MomentOutputDto ToMomentOutputDto(const MomentInfo& moment,
                                  bool has_liked,
                                  const MomentUserProfileDto& profile,
                                  const MomentContentInfo* content,
                                  const std::vector<MomentLikeInfo>* likes,
                                  const std::vector<MomentCommentInfo>* comments);

bool EncodeMomentOutput(const MomentOutputDto& moment, std::string* out, std::string* error_out = nullptr);

memochat::json::JsonValue ToJsonValue(const MomentItemOutputDto& item);
memochat::json::JsonValue ToJsonValue(const MomentLikeOutputDto& like);
memochat::json::JsonValue ToJsonValue(const std::vector<std::string>& values);
memochat::json::JsonValue ToJsonValue(const std::vector<MomentLikeOutputDto>& likes);
memochat::json::JsonValue ToJsonValue(const MomentCommentOutputDto& comment);
memochat::json::JsonValue ToJsonValue(const std::vector<MomentCommentOutputDto>& comments);
memochat::json::JsonValue ToJsonValue(const MomentOutputDto& moment);
memochat::json::JsonValue ToJsonValue(const MomentIdResponseDto& response);
memochat::json::JsonValue ToJsonValue(const MomentListResponseDto& response);
memochat::json::JsonValue ToJsonValue(const MomentDetailResponseDto& response);
memochat::json::JsonValue ToJsonValue(const MomentLikeResponseDto& response);
memochat::json::JsonValue ToJsonValue(const MomentCommentMutationResponseDto& response);
memochat::json::JsonValue ToJsonValue(const MomentCommentListResponseDto& response);
memochat::json::JsonValue ToJsonValue(const MomentCommentLikeResponseDto& response);

} // namespace memochat::gate::services::moments

template <> struct glz::meta<memochat::gate::services::moments::MomentCommentMutationResponseDto>
{
    using T = memochat::gate::services::moments::MomentCommentMutationResponseDto;
    static constexpr auto value = glz::object("error",
                                              &T::error,
                                              "moment_id",
                                              &T::moment_id,
                                              "delete",
                                              &T::delete_,
                                              "comment_count",
                                              &T::comment_count);
};
