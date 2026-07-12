#include "CallService.hpp"

#include "CallPersistence.hpp"
#include "CallPublicDtos.hpp"
#include "CallSessionCacheDto.hpp"
#include "ChatGrpcClient.hpp"
#include "ConfigMgr.hpp"
#include "RedisMgr.hpp"
#include "random/Uuid.hpp"
#include "logging/Logger.hpp"
#include "json/GlazeCompat.hpp"
#include <charconv>
#include <chrono>
#include <sstream>
#include <unordered_map>
#include <vector>

import memochat.call.token_algorithms;
import memochat.call.service_algorithms;
import memochat.call.session_math_algorithms;

#ifdef _WIN32
#include "WinSdkCompat.hpp"
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
#endif

namespace
{
namespace service_modules = memochat::call::service::modules;
namespace session_math_modules = memochat::call::session_math::modules;

const short kCallEventMsgId = session_math_modules::CallEventNotifyMsgId();

int ParseIntOrDefault(const std::string& raw, int fallback)
{
    int value = 0;
    const auto [ptr, ec] = std::from_chars(raw.data(), raw.data() + raw.size(), value);
    return !raw.empty() && ec == std::errc{} && ptr == raw.data() + raw.size() ? value : fallback;
}

int64_t NowMs()
{
    return static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count());
}

std::string DisplayName(const CallUserProfile& profile)
{
    return profile.nick.empty() ? profile.name : profile.nick;
}

memochat::call::CallEventResponseDto MakeCallEventResponseDto(const std::string& event_type,
                                                              const CallSessionInfo& session,
                                                              const CallUserProfile& caller,
                                                              const CallUserProfile& callee,
                                                              const std::string& reason,
                                                              const std::string& livekit_url)
{
    memochat::call::CallEventResponseDto response;
    response.error = ErrorCodes::Success;
    response.event_type = event_type;
    response.call_id = session.call_id;
    response.room_name = session.room_name;
    response.call_type = session.call_type;
    response.caller_uid = session.caller_uid;
    response.callee_uid = session.callee_uid;
    response.caller_name = DisplayName(caller);
    response.callee_name = DisplayName(callee);
    response.caller_icon = caller.icon;
    response.callee_icon = callee.icon;
    response.started_at = session.started_at_ms;
    response.accepted_at = session.accepted_at_ms;
    response.ended_at = session.ended_at_ms;
    response.expires_at = session.expires_at_ms;
    response.state = session.state;
    response.reason = reason.empty() ? session.reason : reason;
    response.trace_id = session.trace_id;
    response.livekit_url = livekit_url;
    return response;
}

memochat::call::CallStartResponseDto
MakeCallStartResponseDto(const memochat::call::CallEventResponseDto& event, int peer_uid, const CallUserProfile& peer)
{
    memochat::call::CallStartResponseDto response;
    response.error = event.error;
    response.event_type = event.event_type;
    response.call_id = event.call_id;
    response.room_name = event.room_name;
    response.call_type = event.call_type;
    response.caller_uid = event.caller_uid;
    response.callee_uid = event.callee_uid;
    response.caller_name = event.caller_name;
    response.callee_name = event.callee_name;
    response.caller_icon = event.caller_icon;
    response.callee_icon = event.callee_icon;
    response.started_at = event.started_at;
    response.accepted_at = event.accepted_at;
    response.ended_at = event.ended_at;
    response.expires_at = event.expires_at;
    response.state = event.state;
    response.reason = event.reason;
    response.trace_id = event.trace_id;
    response.livekit_url = event.livekit_url;
    response.peer_uid = peer_uid;
    response.peer_name = DisplayName(peer);
    response.peer_icon = peer.icon;
    return response;
}

std::string Base64UrlEncode(const std::string& input)
{
    std::string out(memochat::call::modules::Base64UrlEncodedSize(input.size()), '\0');
    const auto encoded_size =
        memochat::call::modules::EncodeBase64Url(reinterpret_cast<const unsigned char*>(input.data()),
                                                 input.size(),
                                                 out.data(),
                                                 out.size());
    if (encoded_size == 0 && !input.empty())
    {
        return {};
    }
    out.resize(encoded_size);
    return out;
}

#ifdef _WIN32
std::string HmacSha256(const std::string& key, const std::string& message)
{
    BCRYPT_ALG_HANDLE alg = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    DWORD object_size = 0;
    DWORD data_size = 0;
    std::vector<UCHAR> hash_object;
    std::vector<UCHAR> hash_value(32);
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, BCRYPT_ALG_HANDLE_HMAC_FLAG) != 0)
    {
        return {};
    }
    if (BCryptGetProperty(alg,
                          BCRYPT_OBJECT_LENGTH,
                          reinterpret_cast<PUCHAR>(&object_size),
                          sizeof(object_size),
                          &data_size,
                          0) != 0)
    {
        BCryptCloseAlgorithmProvider(alg, 0);
        return {};
    }
    hash_object.resize(object_size);
    if (BCryptCreateHash(alg,
                         &hash,
                         hash_object.data(),
                         object_size,
                         reinterpret_cast<PUCHAR>(const_cast<char*>(key.data())),
                         static_cast<ULONG>(key.size()),
                         0) != 0)
    {
        BCryptCloseAlgorithmProvider(alg, 0);
        return {};
    }
    if (BCryptHashData(hash,
                       reinterpret_cast<PUCHAR>(const_cast<char*>(message.data())),
                       static_cast<ULONG>(message.size()),
                       0) != 0)
    {
        BCryptDestroyHash(hash);
        BCryptCloseAlgorithmProvider(alg, 0);
        return {};
    }
    if (BCryptFinishHash(hash, hash_value.data(), static_cast<ULONG>(hash_value.size()), 0) != 0)
    {
        BCryptDestroyHash(hash);
        BCryptCloseAlgorithmProvider(alg, 0);
        return {};
    }
    BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(alg, 0);
    return std::string(reinterpret_cast<const char*>(hash_value.data()), hash_value.size());
}
#else
std::string HmacSha256(const std::string&, const std::string&)
{
    return {};
}
#endif
} // namespace

CallService::CallService() = default;

CallService::CallConfig CallService::LoadConfig() const
{
    CallConfig cfg;
    auto& ini = ConfigMgr::Inst();
    const auto enabled = ini["Call"]["Enabled"];
    const auto base_url = ini["Call"]["BaseUrl"];
    const auto livekit_url = ini["Call"]["LiveKitUrl"];
    const auto api_key = ini["Call"]["ApiKey"];
    const auto api_secret = ini["Call"]["ApiSecret"];
    const auto room_prefix = ini["Call"]["RoomPrefix"];
    const auto ring_timeout = ini["Call"]["RingTimeoutSec"];
    const auto busy_key_ttl = ini["Call"]["BusyKeyTtlSec"];
    const auto token_ttl = ini["Call"]["TokenTtlSec"];
    if (!enabled.empty())
    {
        cfg.enabled = service_modules::IsEnabledText(enabled.c_str());
    }
    if (!base_url.empty())
        cfg.base_url = base_url;
    if (!livekit_url.empty())
        cfg.livekit_url = livekit_url;
    if (!api_key.empty())
        cfg.api_key = api_key;
    if (!api_secret.empty())
        cfg.api_secret = api_secret;
    if (!room_prefix.empty())
        cfg.room_prefix = room_prefix;
    if (!ring_timeout.empty())
        cfg.ring_timeout_sec = service_modules::NormalizeRingTimeoutSec(ParseIntOrDefault(ring_timeout, 0));
    if (!busy_key_ttl.empty())
        cfg.busy_key_ttl_sec = service_modules::NormalizeBusyKeyTtlSec(ParseIntOrDefault(busy_key_ttl, 0));
    if (!token_ttl.empty())
        cfg.token_ttl_sec = service_modules::NormalizeTokenTtlSec(ParseIntOrDefault(token_ttl, 0));
    return cfg;
}

bool CallService::LoadSession(const std::string& call_id, CallSessionInfo& session) const
{
    std::string json_value;
    if (RedisMgr::GetInstance()->Get(std::string(CALL_SESSION_PREFIX) + call_id, json_value))
    {
        if (memochat::call::DecodeCallSessionCache(json_value, &session))
        {
            return true;
        }
    }
    return CallPersistence::Instance().LoadSession(call_id, session);
}

bool CallService::SaveSession(const CallSessionInfo& session, int ttl_seconds) const
{
    std::string payload;
    if (!memochat::call::EncodeCallSessionCache(session, &payload))
    {
        return false;
    }
    if (ttl_seconds > 0)
    {
        RedisMgr::GetInstance()->SetEx(std::string(CALL_SESSION_PREFIX) + session.call_id, payload, ttl_seconds);
    }
    else
    {
        RedisMgr::GetInstance()->Set(std::string(CALL_SESSION_PREFIX) + session.call_id, payload);
    }
    return CallPersistence::Instance().SaveSession(session);
}

void CallService::ClearBusyState(const CallSessionInfo& session) const
{
    RedisMgr::GetInstance()->Del(std::string(CALL_RINGING_PREFIX) + std::to_string(session.caller_uid));
    RedisMgr::GetInstance()->Del(std::string(CALL_RINGING_PREFIX) + std::to_string(session.callee_uid));
    RedisMgr::GetInstance()->Del(std::string(CALL_ACTIVE_PREFIX) + std::to_string(session.caller_uid));
    RedisMgr::GetInstance()->Del(std::string(CALL_ACTIVE_PREFIX) + std::to_string(session.callee_uid));
}

void CallService::SetRingingState(const CallSessionInfo& session, int ttl_seconds) const
{
    RedisMgr::GetInstance()->SetEx(std::string(CALL_RINGING_PREFIX) + std::to_string(session.caller_uid),
                                   session.call_id,
                                   ttl_seconds);
    RedisMgr::GetInstance()->SetEx(std::string(CALL_RINGING_PREFIX) + std::to_string(session.callee_uid),
                                   session.call_id,
                                   ttl_seconds);
}

void CallService::SetActiveState(const CallSessionInfo& session) const
{
    const auto cfg = LoadConfig();
    RedisMgr::GetInstance()->SetEx(std::string(CALL_ACTIVE_PREFIX) + std::to_string(session.caller_uid),
                                   session.call_id,
                                   cfg.busy_key_ttl_sec);
    RedisMgr::GetInstance()->SetEx(std::string(CALL_ACTIVE_PREFIX) + std::to_string(session.callee_uid),
                                   session.call_id,
                                   cfg.busy_key_ttl_sec);
}

bool CallService::NotifyUsers(const std::vector<int>& touids, const memochat::json::JsonValue& payload) const
{
    std::unordered_map<std::string, std::vector<int>> routed;
    for (int uid : touids)
    {
        std::string server_name;
        if (!RedisMgr::GetInstance()->Get(std::string(USERIPPREFIX) + std::to_string(uid), server_name) ||
            server_name.empty())
        {
            continue;
        }
        routed[server_name].push_back(uid);
    }

    const std::string payload_json = memochat::json::glaze_stringify(payload);
    for (const auto& entry : routed)
    {
        GroupEventNotifyReq req;
        req.set_tcp_msgid(kCallEventMsgId);
        req.set_payload_json(payload_json);
        for (int uid : entry.second)
        {
            req.add_touids(uid);
        }
        ChatGrpcClient::GetInstance()->NotifyCallEvent(entry.first, req);
    }
    return true;
}

memochat::json::JsonValue CallService::BuildEventPayload(const std::string& event_type,
                                                         const CallSessionInfo& session,
                                                         const CallUserProfile& caller,
                                                         const CallUserProfile& callee,
                                                         const std::string& reason) const
{
    const auto cfg = LoadConfig();
    const memochat::call::CallEventResponseDto response =
        MakeCallEventResponseDto(event_type, session, caller, callee, reason, cfg.livekit_url);
    return memochat::call::CallEventResponseToJsonValue(response);
}

std::string CallService::CreateToken(const CallSessionInfo& session, int uid, const std::string& role) const
{
    const auto cfg = LoadConfig();
    const int64_t now_sec = NowMs() / 1000;
    memochat::json::JsonValue header = memochat::json::make_document();
    header["alg"] = "HS256";
    header["typ"] = "JWT";
    memochat::json::JsonValue video = memochat::json::make_document();
    video["room"] = session.room_name;
    video["roomJoin"] = true;
    video["canPublish"] = true;
    video["canSubscribe"] = true;
    memochat::json::JsonValue payload = memochat::json::make_document();
    payload["iss"] = cfg.api_key;
    payload["sub"] = std::to_string(uid);
    payload["nbf"] = static_cast<int64_t>(service_modules::TokenNotBeforeSec(now_sec));
    payload["exp"] = static_cast<int64_t>(service_modules::TokenExpiresAtSec(now_sec, cfg.token_ttl_sec));
    payload["name"] = std::string("memochat-") + role + "-" + std::to_string(uid);
    payload["video"] = video;

    const std::string encoded_header = Base64UrlEncode(memochat::json::glaze_stringify(header));
    const std::string encoded_payload = Base64UrlEncode(memochat::json::glaze_stringify(payload));
    const std::string signing_input = encoded_header + "." + encoded_payload;
    const std::string signature = Base64UrlEncode(HmacSha256(cfg.api_secret, signing_input));
    return signing_input + "." + signature;
}

bool CallService::StartCall(int uid,
                            const memochat::json::JsonValue& request,
                            memochat::json::JsonValue& response,
                            const std::string& trace_id)
{
    const auto cfg = LoadConfig();
    if (!cfg.enabled)
    {
        response["error"] = ErrorCodes::Error_Json;
        return true;
    }
    const memochat::call::CallStartRequestDto start_request = memochat::call::CallStartRequestFromJsonValue(request);
    int peer_uid = start_request.peer_uid;
    if (!service_modules::HasValidStartPeer(uid, peer_uid))
    {
        response["error"] = ErrorCodes::TokenInvalid;
        return true;
    }
    const std::string call_type = start_request.call_type;
    if (!service_modules::IsSupportedCallType(call_type.c_str()))
    {
        response["error"] = ErrorCodes::Error_Json;
        return true;
    }
    if (!CallPersistence::Instance().AreUsersMutualFriends(uid, peer_uid))
    {
        response["error"] = ErrorCodes::CallPermissionDenied;
        return true;
    }
    if (!RedisMgr::GetInstance()->ExistsKey(std::string(USERIPPREFIX) + std::to_string(peer_uid)))
    {
        response["error"] = ErrorCodes::CallTargetOffline;
        return true;
    }
    if (RedisMgr::GetInstance()->ExistsKey(std::string(CALL_ACTIVE_PREFIX) + std::to_string(uid)) ||
        RedisMgr::GetInstance()->ExistsKey(std::string(CALL_ACTIVE_PREFIX) + std::to_string(peer_uid)) ||
        RedisMgr::GetInstance()->ExistsKey(std::string(CALL_RINGING_PREFIX) + std::to_string(uid)) ||
        RedisMgr::GetInstance()->ExistsKey(std::string(CALL_RINGING_PREFIX) + std::to_string(peer_uid)))
    {
        response["error"] = ErrorCodes::CallBusy;
        return true;
    }

    CallUserProfile caller;
    CallUserProfile callee;
    if (!CallPersistence::Instance().LoadUserProfiles(uid, peer_uid, caller, callee))
    {
        response["error"] = ErrorCodes::UidInvalid;
        return true;
    }

    CallSessionInfo session;
    std::string uuid_error;
    if (!memochat::random::GenerateUuid(session.call_id, &uuid_error))
    {
        memolog::LogError(
            "call.start.id_generation_failed",
            "call identifier generation failed",
            {{"uid", std::to_string(uid)}, {"peer_uid", std::to_string(peer_uid)}, {"error", uuid_error}});
        response["error"] = ErrorCodes::RPCFailed;
        return true;
    }
    session.room_name = cfg.room_prefix + "-" + session.call_id.substr(0, session_math_modules::RoomShortIdLength());
    session.call_type = call_type;
    session.caller_uid = uid;
    session.callee_uid = peer_uid;
    session.state = service_modules::RingingState();
    session.started_at_ms = NowMs();
    session.expires_at_ms = session_math_modules::SessionExpiryMs(session.started_at_ms, cfg.ring_timeout_sec);
    session.trace_id = trace_id;
    session.updated_at_ms = session.started_at_ms;
    SaveSession(session, service_modules::RingingSessionTtlSec(cfg.ring_timeout_sec));
    SetRingingState(session, service_modules::RingingBusyTtlSec(cfg.ring_timeout_sec, cfg.busy_key_ttl_sec));

    const memochat::call::CallEventResponseDto event_response =
        MakeCallEventResponseDto(service_modules::StateSyncEvent(), session, caller, callee, "", cfg.livekit_url);
    response = memochat::call::CallStartResponseToJsonValue(MakeCallStartResponseDto(event_response, peer_uid, callee));

    NotifyUsers({peer_uid}, BuildEventPayload(service_modules::InviteEvent(), session, caller, callee));
    memolog::LogInfo("call.start.dispatched",
                     "call invite dispatched",
                     {{"uid", std::to_string(uid)},
                      {"peer_uid", std::to_string(peer_uid)},
                      {"call_id", session.call_id},
                      {"call_type", call_type},
                      {"module", "call"}});
    return true;
}

bool CallService::AcceptCall(int uid,
                             const memochat::json::JsonValue& request,
                             memochat::json::JsonValue& response,
                             const std::string& trace_id)
{
    const memochat::call::CallAuthRequestDto auth_request = memochat::call::CallAuthRequestFromJsonValue(request);
    const std::string& call_id = auth_request.call_id;
    if (call_id.empty())
    {
        response["error"] = ErrorCodes::TokenInvalid;
        return true;
    }
    CallSessionInfo session;
    if (!LoadSession(call_id, session))
    {
        response["error"] = ErrorCodes::CallNotFound;
        return true;
    }
    if (session.callee_uid != uid)
    {
        response["error"] = ErrorCodes::CallPermissionDenied;
        return true;
    }
    if (!service_modules::IsRingingState(session.state.c_str()))
    {
        response["error"] = ErrorCodes::CallStateInvalid;
        return true;
    }
    CallUserProfile caller;
    CallUserProfile callee;
    CallPersistence::Instance().LoadUserProfiles(session.caller_uid, session.callee_uid, caller, callee);
    session.state = service_modules::AcceptedState();
    session.accepted_at_ms = NowMs();
    session.updated_at_ms = session.accepted_at_ms;
    session.trace_id = trace_id.empty() ? session.trace_id : trace_id;
    SaveSession(session, service_modules::PersistentSessionTtlSec());
    ClearBusyState(session);
    SetActiveState(session);
    response = BuildEventPayload(service_modules::AcceptedEvent(), session, caller, callee);
    NotifyUsers({session.caller_uid, session.callee_uid}, response);
    return true;
}

bool CallService::RejectCall(int uid,
                             const memochat::json::JsonValue& request,
                             memochat::json::JsonValue& response,
                             const std::string& trace_id)
{
    const memochat::call::CallAuthRequestDto auth_request = memochat::call::CallAuthRequestFromJsonValue(request);
    const std::string& call_id = auth_request.call_id;
    if (call_id.empty())
    {
        response["error"] = ErrorCodes::TokenInvalid;
        return true;
    }
    CallSessionInfo session;
    if (!LoadSession(call_id, session))
    {
        response["error"] = ErrorCodes::CallNotFound;
        return true;
    }
    if (session.callee_uid != uid)
    {
        response["error"] = ErrorCodes::CallPermissionDenied;
        return true;
    }
    if (!service_modules::IsRingingState(session.state.c_str()))
    {
        response["error"] = ErrorCodes::CallStateInvalid;
        return true;
    }
    CallUserProfile caller;
    CallUserProfile callee;
    CallPersistence::Instance().LoadUserProfiles(session.caller_uid, session.callee_uid, caller, callee);
    session.state = service_modules::RejectedState();
    session.reason = service_modules::RejectedState();
    session.ended_at_ms = NowMs();
    session.updated_at_ms = session.ended_at_ms;
    session.trace_id = trace_id.empty() ? session.trace_id : trace_id;
    SaveSession(session, service_modules::PersistentSessionTtlSec());
    ClearBusyState(session);
    response = BuildEventPayload(service_modules::RejectedEvent(), session, caller, callee);
    NotifyUsers({session.caller_uid, session.callee_uid}, response);
    return true;
}

bool CallService::CancelCall(int uid,
                             const memochat::json::JsonValue& request,
                             memochat::json::JsonValue& response,
                             const std::string& trace_id)
{
    const memochat::call::CallAuthRequestDto auth_request = memochat::call::CallAuthRequestFromJsonValue(request);
    const std::string& call_id = auth_request.call_id;
    if (call_id.empty())
    {
        response["error"] = ErrorCodes::TokenInvalid;
        return true;
    }
    CallSessionInfo session;
    if (!LoadSession(call_id, session))
    {
        response["error"] = ErrorCodes::CallNotFound;
        return true;
    }
    if (session.caller_uid != uid)
    {
        response["error"] = ErrorCodes::CallPermissionDenied;
        return true;
    }
    if (!service_modules::IsRingingState(session.state.c_str()))
    {
        response["error"] = ErrorCodes::CallStateInvalid;
        return true;
    }
    const auto now_ms = NowMs();
    CallUserProfile caller;
    CallUserProfile callee;
    CallPersistence::Instance().LoadUserProfiles(session.caller_uid, session.callee_uid, caller, callee);
    const bool timed_out = service_modules::HasCancelTimedOut(now_ms, session.expires_at_ms);
    session.state = service_modules::CancelTerminalState(timed_out);
    session.reason = session.state;
    session.ended_at_ms = now_ms;
    session.updated_at_ms = now_ms;
    session.trace_id = trace_id.empty() ? session.trace_id : trace_id;
    SaveSession(session, service_modules::PersistentSessionTtlSec());
    ClearBusyState(session);
    response = BuildEventPayload(service_modules::CancelTerminalEvent(timed_out), session, caller, callee);
    NotifyUsers({session.caller_uid, session.callee_uid}, response);
    return true;
}

bool CallService::HangupCall(int uid,
                             const memochat::json::JsonValue& request,
                             memochat::json::JsonValue& response,
                             const std::string& trace_id)
{
    const memochat::call::CallAuthRequestDto auth_request = memochat::call::CallAuthRequestFromJsonValue(request);
    const std::string& call_id = auth_request.call_id;
    if (call_id.empty())
    {
        response["error"] = ErrorCodes::TokenInvalid;
        return true;
    }
    CallSessionInfo session;
    if (!LoadSession(call_id, session))
    {
        response["error"] = ErrorCodes::CallNotFound;
        return true;
    }
    if (session.caller_uid != uid && session.callee_uid != uid)
    {
        response["error"] = ErrorCodes::CallPermissionDenied;
        return true;
    }
    if (!service_modules::IsActiveCallState(session.state.c_str()))
    {
        response["error"] = ErrorCodes::CallStateInvalid;
        return true;
    }
    CallUserProfile caller;
    CallUserProfile callee;
    CallPersistence::Instance().LoadUserProfiles(session.caller_uid, session.callee_uid, caller, callee);
    session.state = service_modules::EndedState();
    session.reason = service_modules::HangupReason();
    session.ended_at_ms = NowMs();
    if (session_math_modules::ShouldComputeDurationSec(session.accepted_at_ms, session.ended_at_ms))
    {
        session.duration_sec = session_math_modules::SessionDurationSec(session.accepted_at_ms, session.ended_at_ms);
    }
    session.updated_at_ms = session.ended_at_ms;
    session.trace_id = trace_id.empty() ? session.trace_id : trace_id;
    SaveSession(session, service_modules::PersistentSessionTtlSec());
    ClearBusyState(session);
    response = BuildEventPayload(service_modules::HangupEvent(), session, caller, callee);
    NotifyUsers({session.caller_uid, session.callee_uid}, response);
    return true;
}

bool CallService::GetToken(int uid,
                           const std::string& call_id,
                           const std::string& role,
                           memochat::json::JsonValue& response,
                           const std::string& trace_id)
{
    if (call_id.empty())
    {
        response["error"] = ErrorCodes::TokenInvalid;
        return true;
    }
    CallSessionInfo session;
    if (!LoadSession(call_id, session))
    {
        response["error"] = ErrorCodes::CallNotFound;
        return true;
    }
    const bool is_caller = session.caller_uid == uid;
    const bool is_callee = session.callee_uid == uid;
    if (!is_caller && !is_callee)
    {
        response["error"] = ErrorCodes::CallPermissionDenied;
        return true;
    }
    if (!service_modules::IsActiveCallState(session.state.c_str()))
    {
        response["error"] = ErrorCodes::CallStateInvalid;
        return true;
    }
    const auto cfg = LoadConfig();
    if (cfg.api_key.empty() || cfg.api_secret.empty())
    {
        memolog::LogError("call.livekit.config_missing",
                          "LiveKit API credentials missing",
                          {{"missing_api_key", cfg.api_key.empty() ? "true" : "false"},
                           {"missing_api_secret", cfg.api_secret.empty() ? "true" : "false"}});
        response["error"] = ErrorCodes::Error_Json;
        return true;
    }
    session.state = service_modules::JoiningState();
    session.updated_at_ms = NowMs();
    session.trace_id = trace_id.empty() ? session.trace_id : trace_id;
    SaveSession(session, service_modules::PersistentSessionTtlSec());
    memochat::call::CallTokenResponseDto token_response;
    token_response.error = ErrorCodes::Success;
    token_response.call_id = session.call_id;
    token_response.room_name = session.room_name;
    token_response.role = role.empty() ? service_modules::DefaultParticipantRole(is_caller) : role;
    token_response.livekit_url = cfg.livekit_url;
    token_response.token = CreateToken(session, uid, token_response.role);
    token_response.trace_id = session.trace_id;
    response = memochat::call::CallTokenResponseToJsonValue(token_response);
    return true;
}
