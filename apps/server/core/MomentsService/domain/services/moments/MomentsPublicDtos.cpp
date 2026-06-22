#include "services/moments/MomentsPublicDtos.h"

#include <cstdlib>

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
    try
    {
        return value.asInt64();
    }
    catch (...)
    {
    }
    try
    {
        return static_cast<int64_t>(value.asInt());
    }
    catch (...)
    {
    }
    try
    {
        return static_cast<int64_t>(value.asDouble());
    }
    catch (...)
    {
    }
    const std::string text = value.asString();
    if (!text.empty())
    {
        char* end = nullptr;
        const long long parsed = std::strtoll(text.c_str(), &end, 10);
        if (end && *end == '\0')
        {
            return static_cast<int64_t>(parsed);
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
    try
    {
        return value.asBool();
    }
    catch (...)
    {
    }
    const std::string text = value.asString();
    if (text == "true" || text == "1")
    {
        return true;
    }
    if (text == "false" || text == "0")
    {
        return false;
    }
    try
    {
        return value.asInt() != 0;
    }
    catch (...)
    {
    }
    try
    {
        return value.asDouble() != 0.0;
    }
    catch (...)
    {
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
    if (item.media_type != "image" && item.media_type != "video")
    {
        item.media_type = "text";
    }
    if ((item.media_type == "image" || item.media_type == "video") && item.media_key.empty())
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

MomentsAuthFieldsDto MomentsAuthFieldsFromJsonValue(const memochat::json::JsonValue& root)
{
    MomentsAuthFieldsDto request;
    request.uid = memochat::json::glaze_safe_get<int>(root, "uid", 0);
    request.login_ticket = memochat::json::glaze_safe_get<std::string>(root, "login_ticket", "");
    return request;
}

MomentPublishRequestDto MomentPublishRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    MomentPublishRequestDto request;
    const MomentsAuthFieldsDto auth = MomentsAuthFieldsFromJsonValue(root);
    request.uid = auth.uid;
    request.login_ticket = auth.login_ticket;
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
    const MomentsAuthFieldsDto auth = MomentsAuthFieldsFromJsonValue(root);
    request.uid = auth.uid;
    request.login_ticket = auth.login_ticket;
    request.last_moment_id = MomentsReadInt64(root, "last_moment_id", 0);
    request.limit = MomentsReadInt(root, "limit", 20);
    request.author_uid = MomentsReadInt(root, "author_uid", 0);
    if (request.limit <= 0)
    {
        request.limit = 20;
    }
    if (request.limit > 50)
    {
        request.limit = 50;
    }
    return request;
}

MomentIdRequestDto MomentIdRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    MomentIdRequestDto request;
    const MomentsAuthFieldsDto auth = MomentsAuthFieldsFromJsonValue(root);
    request.uid = auth.uid;
    request.login_ticket = auth.login_ticket;
    request.moment_id = MomentsReadInt64(root, "moment_id", 0);
    return request;
}

MomentLikeRequestDto MomentLikeRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    MomentLikeRequestDto request;
    const MomentsAuthFieldsDto auth = MomentsAuthFieldsFromJsonValue(root);
    request.uid = auth.uid;
    request.login_ticket = auth.login_ticket;
    request.moment_id = MomentsReadInt64(root, "moment_id", 0);
    request.like = MomentsReadBool(root, "like", true);
    return request;
}

MomentCommentRequestDto MomentCommentRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    MomentCommentRequestDto request;
    const MomentsAuthFieldsDto auth = MomentsAuthFieldsFromJsonValue(root);
    request.uid = auth.uid;
    request.login_ticket = auth.login_ticket;
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
    const MomentsAuthFieldsDto auth = MomentsAuthFieldsFromJsonValue(root);
    request.uid = auth.uid;
    request.login_ticket = auth.login_ticket;
    request.moment_id = MomentsReadInt64(root, "moment_id", 0);
    request.last_comment_id = MomentsReadInt64(root, "last_comment_id", 0);
    request.limit = MomentsReadInt(root, "limit", 20);
    if (request.limit <= 0)
    {
        request.limit = 20;
    }
    if (request.limit > 50)
    {
        request.limit = 50;
    }
    return request;
}

MomentCommentLikeRequestDto MomentCommentLikeRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    MomentCommentLikeRequestDto request;
    const MomentsAuthFieldsDto auth = MomentsAuthFieldsFromJsonValue(root);
    request.uid = auth.uid;
    request.login_ticket = auth.login_ticket;
    request.comment_id = MomentsReadInt64(root, "comment_id", 0);
    request.like = MomentsReadBool(root, "like", true);
    return request;
}

} // namespace memochat::gate::services::moments
