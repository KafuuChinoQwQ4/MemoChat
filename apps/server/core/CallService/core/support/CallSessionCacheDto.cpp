#include "CallSessionCacheDto.h"

#include "json/TypedJsonCodec.h"

namespace memochat::call
{

CallSessionCacheDto ToCallSessionCacheDto(const CallSessionInfo& session)
{
    CallSessionCacheDto dto;
    dto.call_id = session.call_id;
    dto.room_name = session.room_name;
    dto.call_type = session.call_type;
    dto.caller_uid = session.caller_uid;
    dto.callee_uid = session.callee_uid;
    dto.state = session.state;
    dto.started_at_ms = session.started_at_ms;
    dto.accepted_at_ms = session.accepted_at_ms;
    dto.ended_at_ms = session.ended_at_ms;
    dto.expires_at_ms = session.expires_at_ms;
    dto.duration_sec = session.duration_sec;
    dto.reason = session.reason;
    dto.trace_id = session.trace_id;
    dto.updated_at_ms = session.updated_at_ms;
    return dto;
}

void ApplyCallSessionCacheDto(const CallSessionCacheDto& dto, CallSessionInfo& session)
{
    session.call_id = dto.call_id;
    session.room_name = dto.room_name;
    session.call_type = dto.call_type;
    session.caller_uid = dto.caller_uid;
    session.callee_uid = dto.callee_uid;
    session.state = dto.state;
    session.started_at_ms = dto.started_at_ms;
    session.accepted_at_ms = dto.accepted_at_ms;
    session.ended_at_ms = dto.ended_at_ms;
    session.expires_at_ms = dto.expires_at_ms;
    session.duration_sec = dto.duration_sec;
    session.reason = dto.reason;
    session.trace_id = dto.trace_id;
    session.updated_at_ms = dto.updated_at_ms;
}

bool EncodeCallSessionCache(const CallSessionInfo& session, std::string* out, std::string* error_out)
{
    return memochat::json::WriteTypedJson(ToCallSessionCacheDto(session), out, error_out);
}

bool DecodeCallSessionCache(std::string_view body, CallSessionCacheDto* out, std::string* error_out)
{
    if (!memochat::json::ReadTypedJson(body, out, error_out))
    {
        return false;
    }
    return out != nullptr && !out->call_id.empty();
}

bool DecodeCallSessionCache(std::string_view body, CallSessionInfo* out, std::string* error_out)
{
    if (out == nullptr)
    {
        if (error_out != nullptr)
        {
            *error_out = "output pointer is null";
        }
        return false;
    }

    CallSessionCacheDto dto;
    if (!DecodeCallSessionCache(body, &dto, error_out))
    {
        return false;
    }
    ApplyCallSessionCacheDto(dto, *out);
    return true;
}

} // namespace memochat::call
