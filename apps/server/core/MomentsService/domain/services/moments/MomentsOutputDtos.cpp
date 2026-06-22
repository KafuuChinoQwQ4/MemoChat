#include "services/moments/MomentsOutputDtos.h"

#include "json/TypedJsonCodec.h"
#include "persistence/MomentTypes.h"

#include <exception>

namespace
{

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

namespace memochat::gate::services::moments
{

MomentItemOutputDto ToMomentItemOutputDto(const MomentItemInfo& item)
{
    MomentItemOutputDto dto;
    dto.seq = item.seq;
    dto.media_type = item.media_type;
    dto.media_key = item.media_key;
    dto.thumb_key = item.thumb_key;
    dto.content = item.content;
    dto.width = item.width;
    dto.height = item.height;
    dto.duration_ms = item.duration_ms;
    return dto;
}

MomentLikeOutputDto ToMomentLikeOutputDto(const MomentLikeInfo& like)
{
    MomentLikeOutputDto dto;
    dto.uid = like.uid;
    dto.user_nick = like.user_nick;
    dto.user_icon = like.user_icon;
    dto.created_at = like.created_at;
    return dto;
}

std::vector<MomentLikeOutputDto> ToMomentLikeOutputDtos(const std::vector<MomentLikeInfo>& likes)
{
    std::vector<MomentLikeOutputDto> out;
    out.reserve(likes.size());
    for (const auto& like : likes)
    {
        out.push_back(ToMomentLikeOutputDto(like));
    }
    return out;
}

std::vector<std::string> ToMomentLikeNames(const std::vector<MomentLikeInfo>& likes)
{
    std::vector<std::string> names;
    for (const auto& like : likes)
    {
        if (!like.user_nick.empty())
        {
            names.push_back(like.user_nick);
        }
    }
    return names;
}

MomentCommentOutputDto ToMomentCommentOutputDto(const MomentCommentInfo& comment)
{
    MomentCommentOutputDto dto;
    dto.id = comment.id;
    dto.moment_id = comment.moment_id;
    dto.uid = comment.uid;
    dto.user_nick = comment.user_nick;
    dto.user_icon = comment.user_icon;
    dto.content = comment.content;
    dto.reply_uid = comment.reply_uid;
    dto.reply_nick = comment.reply_nick;
    dto.created_at = comment.created_at;
    dto.like_count = comment.like_count;
    dto.has_liked = comment.has_liked;
    dto.like_names = ToMomentLikeNames(comment.likes);
    dto.likes = ToMomentLikeOutputDtos(comment.likes);
    return dto;
}

std::vector<MomentCommentOutputDto> ToMomentCommentOutputDtos(const std::vector<MomentCommentInfo>& comments)
{
    std::vector<MomentCommentOutputDto> out;
    out.reserve(comments.size());
    for (const auto& comment : comments)
    {
        out.push_back(ToMomentCommentOutputDto(comment));
    }
    return out;
}

MomentOutputDto ToMomentOutputDto(const MomentInfo& moment,
                                  bool has_liked,
                                  const MomentUserProfileDto& profile,
                                  const MomentContentInfo* content,
                                  const std::vector<MomentLikeInfo>* likes,
                                  const std::vector<MomentCommentInfo>* comments)
{
    MomentOutputDto dto;
    dto.moment_id = moment.moment_id;
    dto.uid = moment.uid;
    dto.visibility = moment.visibility;
    dto.location = moment.location;
    dto.created_at = moment.created_at;
    dto.like_count = moment.like_count;
    dto.comment_count = moment.comment_count;
    dto.has_liked = has_liked;
    dto.user_id = profile.user_id;
    dto.user_name = profile.user_name;
    dto.user_nick = profile.user_nick;
    dto.user_icon = profile.user_icon;

    if (content != nullptr)
    {
        dto.items.reserve(content->items.size());
        for (const auto& item : content->items)
        {
            dto.items.push_back(ToMomentItemOutputDto(item));
        }
    }

    if (likes != nullptr)
    {
        dto.like_names = ToMomentLikeNames(*likes);
        dto.likes = ToMomentLikeOutputDtos(*likes);
    }

    if (comments != nullptr)
    {
        dto.comments = ToMomentCommentOutputDtos(*comments);
    }

    return dto;
}

bool EncodeMomentOutput(const MomentOutputDto& moment, std::string* out, std::string* error_out)
{
    return WriteTypedJsonNoThrow(moment, out, error_out);
}

memochat::json::JsonValue ToJsonValue(const MomentItemOutputDto& item)
{
    return TypedJsonToJsonValue(item);
}

memochat::json::JsonValue ToJsonValue(const MomentLikeOutputDto& like)
{
    return TypedJsonToJsonValue(like);
}

memochat::json::JsonValue ToJsonValue(const std::vector<std::string>& values)
{
    return TypedJsonToJsonValue(values);
}

memochat::json::JsonValue ToJsonValue(const std::vector<MomentLikeOutputDto>& likes)
{
    return TypedJsonToJsonValue(likes);
}

memochat::json::JsonValue ToJsonValue(const MomentCommentOutputDto& comment)
{
    return TypedJsonToJsonValue(comment);
}

memochat::json::JsonValue ToJsonValue(const std::vector<MomentCommentOutputDto>& comments)
{
    return TypedJsonToJsonValue(comments);
}

memochat::json::JsonValue ToJsonValue(const MomentOutputDto& moment)
{
    return TypedJsonToJsonValue(moment);
}

memochat::json::JsonValue ToJsonValue(const MomentIdResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

memochat::json::JsonValue ToJsonValue(const MomentListResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

memochat::json::JsonValue ToJsonValue(const MomentDetailResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

memochat::json::JsonValue ToJsonValue(const MomentLikeResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

memochat::json::JsonValue ToJsonValue(const MomentCommentMutationResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

memochat::json::JsonValue ToJsonValue(const MomentCommentListResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

memochat::json::JsonValue ToJsonValue(const MomentCommentLikeResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

} // namespace memochat::gate::services::moments
