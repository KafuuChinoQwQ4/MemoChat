#include "services/moments/MomentsPublicDtos.hpp"

#include <charconv>

import memochat.moments.public_algorithms;

namespace
{

memochat::json::JsonValue ReadMember(const memochat::json::JsonValue& obj, const char* key)
{
    if (!memochat::json::isMember(obj, key))
    {
        return memochat::json::JsonValue();
    }
    return obj[key];
}

} // namespace

namespace memochat::gate::services::moments
{

int64_t MomentsReadInt64(const memochat::json::JsonValue& obj, const char* key, int64_t fallback)
{
    const memochat::json::JsonValue value = ReadMember(obj, key);
    if (value.isNumber())
    {
        return value.asInt64();
    }
    const std::string text = value.asString();
    if (!text.empty())
    {
        int64_t parsed = 0;
        const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), parsed);
        if (ec == std::errc{} && ptr == text.data() + text.size())
        {
            return parsed;
        }
    }
    return fallback;
}

int MomentsReadInt(const memochat::json::JsonValue& obj, const char* key, int fallback)
{
    return static_cast<int>(MomentsReadInt64(obj, key, fallback));
}

bool MomentsReadBool(const memochat::json::JsonValue& obj, const char* key, bool fallback)
{
    const memochat::json::JsonValue value = ReadMember(obj, key);
    if (value.isBool())
    {
        return value.asBool();
    }
    const std::string text = value.asString();
    bool parsed_text = false;
    if (memochat::moments::modules::TryParseBoolText(text.data(), text.size(), &parsed_text))
    {
        return parsed_text;
    }
    if (value.isNumber())
    {
        return value.asDouble() != 0.0;
    }
    return fallback;
}

std::string MomentsReadString(const memochat::json::JsonValue& obj, const char* key, const std::string& fallback)
{
    const memochat::json::JsonValue value = ReadMember(obj, key);
    const std::string text = value.asString();
    return text.empty() ? fallback : text;
}

void NormalizeMomentPublicItem(MomentItemInfo& item)
{
    const auto item_kind = memochat::moments::modules::NormalizeMomentItemKind(item.media_type.data(),
                                                                               item.media_type.size(),
                                                                               item.media_key.data(),
                                                                               item.media_key.size());
    if (item_kind == memochat::moments::modules::kMomentItemKindImage)
    {
        item.media_type = "image";
    }
    else if (item_kind == memochat::moments::modules::kMomentItemKindVideo)
    {
        item.media_type = "video";
    }
    else
    {
        item.media_type = "text";
    }
    if (item.media_type == "text")
    {
        item.media_key.clear();
        item.thumb_key.clear();
        item.width = 0;
        item.height = 0;
        item.duration_ms = 0;
    }
}

MomentPublishRequestDto MomentPublishRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    MomentPublishRequestDto request;
    request.visibility = MomentsReadInt(root, "visibility", 0);
    request.location = MomentsReadString(root, "location");

    if (memochat::json::isMember(root, "items") && root["items"].isArray())
    {
        for (const auto& item_json : root["items"])
        {
            MomentItemInfo item;
            item.media_type = item_json.get("media_type", "text").asString();
            item.media_key = item_json.get("media_key", "").asString();
            item.thumb_key = item_json.get("thumb_key", "").asString();
            item.content = item_json.get("content", "").asString();
            item.width = item_json.get("width", 0).asInt();
            item.height = item_json.get("height", 0).asInt();
            item.duration_ms = item_json.get("duration_ms", 0).asInt();
            NormalizeMomentPublicItem(item);
            request.items.push_back(item);
        }
    }

    const std::string raw_content = MomentsReadString(root, "content");
    if (request.items.empty() && !raw_content.empty())
    {
        MomentItemInfo item;
        item.media_type = "text";
        item.content = raw_content;
        NormalizeMomentPublicItem(item);
        request.items.push_back(item);
    }

    return request;
}

MomentListRequestDto MomentListRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    MomentListRequestDto request;
    request.last_moment_id = MomentsReadInt64(root, "last_moment_id", 0);
    request.limit = MomentsReadInt(root, "limit", 20);
    request.author_uid = MomentsReadInt(root, "author_uid", 0);
    request.limit = memochat::moments::modules::NormalizePageLimit(request.limit, 20, 50);
    return request;
}

MomentIdRequestDto MomentIdRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    MomentIdRequestDto request;
    request.moment_id = MomentsReadInt64(root, "moment_id", 0);
    return request;
}

MomentLikeRequestDto MomentLikeRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    MomentLikeRequestDto request;
    request.moment_id = MomentsReadInt64(root, "moment_id", 0);
    request.like = MomentsReadBool(root, "like", true);
    return request;
}

MomentCommentRequestDto MomentCommentRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    MomentCommentRequestDto request;
    request.moment_id = MomentsReadInt64(root, "moment_id", 0);
    request.content = MomentsReadString(root, "content");
    request.reply_uid = MomentsReadInt(root, "reply_uid", 0);
    request.comment_id = MomentsReadInt64(root, "comment_id", 0);
    request.delete_ = MomentsReadBool(root, "delete", false);
    request.delete_mode = request.delete_ || (request.comment_id > 0 && request.content.empty());
    return request;
}

MomentCommentListRequestDto MomentCommentListRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    MomentCommentListRequestDto request;
    request.moment_id = MomentsReadInt64(root, "moment_id", 0);
    request.last_comment_id = MomentsReadInt64(root, "last_comment_id", 0);
    request.limit = MomentsReadInt(root, "limit", 20);
    request.limit = memochat::moments::modules::NormalizePageLimit(request.limit, 20, 50);
    return request;
}

MomentCommentLikeRequestDto MomentCommentLikeRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    MomentCommentLikeRequestDto request;
    request.comment_id = MomentsReadInt64(root, "comment_id", 0);
    request.like = MomentsReadBool(root, "like", true);
    return request;
}

} // namespace memochat::gate::services::moments
