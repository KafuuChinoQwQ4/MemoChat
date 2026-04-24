#pragma once

#include "Singleton.h"
#include "PostgresDao.h"
#include "json/GlazeCompat.h"
#include <string>
#include <vector>

class CallService : public Singleton<CallService>
{
    friend class Singleton<CallService>;
public:
    bool StartCall(const memochat::json::JsonValue& request, memochat::json::JsonValue& response, const std::string& trace_id);
    bool AcceptCall(const memochat::json::JsonValue& request, memochat::json::JsonValue& response, const std::string& trace_id);
    bool RejectCall(const memochat::json::JsonValue& request, memochat::json::JsonValue& response, const std::string& trace_id);
    bool CancelCall(const memochat::json::JsonValue& request, memochat::json::JsonValue& response, const std::string& trace_id);
    bool HangupCall(const memochat::json::JsonValue& request, memochat::json::JsonValue& response, const std::string& trace_id);
    bool GetToken(int uid, const std::string& token, const std::string& call_id, const std::string& role,
                  memochat::json::JsonValue& response, const std::string& trace_id);

private:
    struct CallConfig {
        bool enabled = true;
        std::string base_url = "http://127.0.0.1:8080";
        std::string livekit_url = "ws://127.0.0.1:7880";
        std::string api_key = "devkey";
        std::string api_secret = "secret";
        std::string room_prefix = "memochat";
        int ring_timeout_sec = 45;
        int busy_key_ttl_sec = 50;
        int token_ttl_sec = 3600;
    };

    CallService();
    CallConfig LoadConfig() const;
    bool ParseAuthRequest(const memochat::json::JsonValue& request, int& uid, std::string& token, std::string& call_id) const;
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

