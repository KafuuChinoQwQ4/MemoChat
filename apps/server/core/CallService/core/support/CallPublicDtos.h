#pragma once

#include "json/GlazeCompat.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace memochat::call
{

struct CallAuthRequestDto
{
    int uid = 0;
    std::string token;
    std::string call_id;
};

struct CallStartRequestDto
{
    int uid = 0;
    std::string token;
    int peer_uid = 0;
    std::string call_type;
};

struct CallTokenRequestDto
{
    int uid = 0;
    std::string token;
    std::string call_id;
    std::string role;
};

struct CallEventResponseDto
{
    int error = 0;
    std::string event_type;
    std::string call_id;
    std::string room_name;
    std::string call_type;
    int caller_uid = 0;
    int callee_uid = 0;
    std::string caller_name;
    std::string callee_name;
    std::string caller_icon;
    std::string callee_icon;
    int64_t started_at = 0;
    int64_t accepted_at = 0;
    int64_t ended_at = 0;
    int64_t expires_at = 0;
    std::string state;
    std::string reason;
    std::string trace_id;
    std::string livekit_url;
};

struct CallStartResponseDto
{
    int error = 0;
    std::string event_type;
    std::string call_id;
    std::string room_name;
    std::string call_type;
    int caller_uid = 0;
    int callee_uid = 0;
    std::string caller_name;
    std::string callee_name;
    std::string caller_icon;
    std::string callee_icon;
    int64_t started_at = 0;
    int64_t accepted_at = 0;
    int64_t ended_at = 0;
    int64_t expires_at = 0;
    std::string state;
    std::string reason;
    std::string trace_id;
    std::string livekit_url;
    int peer_uid = 0;
    std::string peer_name;
    std::string peer_icon;
};

struct CallTokenResponseDto
{
    int error = 0;
    std::string call_id;
    std::string room_name;
    std::string role;
    std::string livekit_url;
    std::string token;
    std::string trace_id;
};

CallAuthRequestDto CallAuthRequestFromJsonValue(const memochat::json::JsonValue& root);
CallStartRequestDto CallStartRequestFromJsonValue(const memochat::json::JsonValue& root);
CallTokenRequestDto CallTokenRequestFromJsonValue(const memochat::json::JsonValue& root);

memochat::json::JsonValue CallEventResponseToJsonValue(const CallEventResponseDto& response);
memochat::json::JsonValue CallStartResponseToJsonValue(const CallStartResponseDto& response);
memochat::json::JsonValue CallTokenResponseToJsonValue(const CallTokenResponseDto& response);

bool DecodeCallAuthRequest(std::string_view body, CallAuthRequestDto* out, std::string* error_out = nullptr);
bool DecodeCallStartRequest(std::string_view body, CallStartRequestDto* out, std::string* error_out = nullptr);
bool DecodeCallTokenRequest(std::string_view body, CallTokenRequestDto* out, std::string* error_out = nullptr);

} // namespace memochat::call
