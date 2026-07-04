#pragma once

#include "json/GlazeCompat.hpp"

#include <cstdint>
#include <string>

namespace memochat::chat::history
{

struct ChatPrivateHistoryRequestDto
{
    int uid = 0;
    int peer_uid = 0;
    int64_t before_ts = 0;
    std::string before_msg_id;
    int limit = 20;
};

struct ChatGroupHistoryRequestDto
{
    int uid = 0;
    int64_t group_id = 0;
    int64_t before_ts = 0;
    int64_t before_seq = 0;
    int limit = 20;
};

// Private-history response root shell (symmetric to group::ChatGroupHistoryResponseDto, slice BP).
// Owns only the leading scalars; the dynamic messages array stays caller-applied immediately after
// the shell to preserve wire order (glz object_t is insertion-ordered).
struct ChatPrivateHistoryResponseDto
{
    int error = 0;
    int peer_uid = 0;
    bool has_more = false;
};

inline ChatPrivateHistoryRequestDto ChatPrivateHistoryRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    ChatPrivateHistoryRequestDto request;
    request.uid = root["fromuid"].asInt();
    request.peer_uid = root["peer_uid"].asInt();
    request.before_ts = root.get("before_ts", 0).asInt64();
    request.before_msg_id = root.get("before_msg_id", "").asString();
    request.limit = root.get("limit", 20).asInt();
    return request;
}

inline ChatGroupHistoryRequestDto ChatGroupHistoryRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    ChatGroupHistoryRequestDto request;
    request.uid = root["fromuid"].asInt();
    request.group_id = root["groupid"].asInt64();
    request.before_ts = root.get("before_ts", 0).asInt64();
    request.before_seq = root.get("before_seq", 0).asInt64();
    request.limit = root.get("limit", 20).asInt();
    return request;
}

inline memochat::json::JsonValue ToJsonValue(const ChatPrivateHistoryResponseDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["peer_uid"] = dto.peer_uid;
    value["has_more"] = dto.has_more;
    return value;
}

} // namespace memochat::chat::history
