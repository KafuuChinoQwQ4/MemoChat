#pragma once

#include "CallSessionTypes.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace memochat::call
{

struct CallSessionCacheDto
{
    std::string call_id;
    std::string room_name;
    std::string call_type;
    int caller_uid = 0;
    int callee_uid = 0;
    std::string state;
    int64_t started_at_ms = 0;
    int64_t accepted_at_ms = 0;
    int64_t ended_at_ms = 0;
    int64_t expires_at_ms = 0;
    int duration_sec = 0;
    std::string reason;
    std::string trace_id;
    int64_t updated_at_ms = 0;
};

CallSessionCacheDto ToCallSessionCacheDto(const CallSessionInfo& session);
void ApplyCallSessionCacheDto(const CallSessionCacheDto& dto, CallSessionInfo& session);
bool EncodeCallSessionCache(const CallSessionInfo& session, std::string* out, std::string* error_out = nullptr);
bool DecodeCallSessionCache(std::string_view body, CallSessionCacheDto* out, std::string* error_out = nullptr);
bool DecodeCallSessionCache(std::string_view body, CallSessionInfo* out, std::string* error_out = nullptr);

} // namespace memochat::call
