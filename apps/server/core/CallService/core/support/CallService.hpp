#pragma once

#include "Singleton.hpp"
#include "json/GlazeCompat.hpp"
#include <string>
#include <vector>

struct CallSessionInfo;
struct CallUserProfile;

class CallService : public Singleton<CallService>
{
    friend class Singleton<CallService>;

public:
    bool StartCall(int uid,
                   const memochat::json::JsonValue& request,
                   memochat::json::JsonValue& response,
                   const std::string& trace_id);
    bool AcceptCall(int uid,
                    const memochat::json::JsonValue& request,
                    memochat::json::JsonValue& response,
                    const std::string& trace_id);
    bool RejectCall(int uid,
                    const memochat::json::JsonValue& request,
                    memochat::json::JsonValue& response,
                    const std::string& trace_id);
    bool CancelCall(int uid,
                    const memochat::json::JsonValue& request,
                    memochat::json::JsonValue& response,
                    const std::string& trace_id);
    bool HangupCall(int uid,
                    const memochat::json::JsonValue& request,
                    memochat::json::JsonValue& response,
                    const std::string& trace_id);
    bool GetToken(int uid,
                  const std::string& call_id,
                  const std::string& role,
                  memochat::json::JsonValue& response,
                  const std::string& trace_id);

private:
    struct CallConfig
    {
        bool enabled = true;
        std::string base_url = "http://127.0.0.1:8080";
        std::string livekit_url = "ws://127.0.0.1:7880";
        std::string api_key;
        std::string api_secret;
        std::string room_prefix = "memochat";
        int ring_timeout_sec = 45;
        int busy_key_ttl_sec = 50;
        int token_ttl_sec = 3600;
    };

    CallService();
    CallConfig LoadConfig() const;
    bool LoadSession(const std::string& call_id, CallSessionInfo& session) const;
    bool SaveSession(const CallSessionInfo& session, int ttl_seconds) const;
    void ClearBusyState(const CallSessionInfo& session) const;
    void SetRingingState(const CallSessionInfo& session, int ttl_seconds) const;
    void SetActiveState(const CallSessionInfo& session) const;
    bool NotifyUsers(const std::vector<int>& touids, const memochat::json::JsonValue& payload) const;
    memochat::json::JsonValue BuildEventPayload(const std::string& event_type,
                                                const CallSessionInfo& session,
                                                const CallUserProfile& caller,
                                                const CallUserProfile& callee,
                                                const std::string& reason = std::string()) const;
    std::string CreateToken(const CallSessionInfo& session, int uid, const std::string& role) const;
};
