#pragma once

#include <cstdint>
#include <string>

struct CallUserProfile
{
    int uid = 0;
    std::string user_id;
    std::string name;
    std::string nick;
    std::string icon;
};

struct CallSessionInfo
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
