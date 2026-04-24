#include "CallService.h"

#include "ChatGrpcClient.h"
#include "ConfigMgr.h"
#include "PostgresMgr.h"
#include "RedisMgr.h"
#include "logging/Logger.h"
#include "json/GlazeCompat.h"
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#include "../common/WinSdkCompat.h"
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
#endif

namespace {
constexpr short kCallEventMsgId = 1085;

int64_t NowMs()
{
    return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

std::string NewId()
{
    return boost::uuids::to_string(boost::uuids::random_generator()());
}

bool ValidateGateUserToken(int uid, const std::string& token)
{
    if (uid <= 0 || token.empty()) {
        return false;
    }
    std::string token_value;
    if (!RedisMgr::GetInstance()->Get(std::string(USERTOKENPREFIX) + std::to_string(uid), token_value)) {
        return false;
    }
    return !token_value.empty() && token_value == token;
}

std::string Base64UrlEncode(const std::string& input)
{
    static const char* kTable = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    std::string out;
    out.reserve(((input.size() + 2) / 3) * 4);
    unsigned int val = 0;
    int valb = -6;
    for (unsigned char c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(kTable[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) {
        out.push_back(kTable[((val << 8) >> (valb + 8)) & 0x3F]);
    }
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
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, BCRYPT_ALG_HANDLE_HMAC_FLAG) != 0) {
        return {};
    }
    if (BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&object_size),
        sizeof(object_size), &data_size, 0) != 0) {
        BCryptCloseAlgorithmProvider(alg, 0);
        return {};
    }
    hash_object.resize(object_size);
    if (BCryptCreateHash(alg, &hash, hash_object.data(), object_size,
        reinterpret_cast<PUCHAR>(const_cast<char*>(key.data())), static_cast<ULONG>(key.size()), 0) != 0) {
        BCryptCloseAlgorithmProvider(alg, 0);
        return {};
    }
    if (BCryptHashData(hash, reinterpret_cast<PUCHAR>(const_cast<char*>(message.data())),
        static_cast<ULONG>(message.size()), 0) != 0) {
        BCryptDestroyHash(hash);
        BCryptCloseAlgorithmProvider(alg, 0);
        return {};
    }
    if (BCryptFinishHash(hash, hash_value.data(), static_cast<ULONG>(hash_value.size()), 0) != 0) {
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
}

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
    if (!enabled.empty()) {
        cfg.enabled = enabled == "1" || enabled == "true" || enabled == "TRUE" || enabled == "True";
    }
    if (!base_url.empty()) cfg.base_url = base_url;
    if (!livekit_url.empty()) cfg.livekit_url = livekit_url;
    if (!api_key.empty()) cfg.api_key = api_key;
    if (!api_secret.empty()) cfg.api_secret = api_secret;
    if (!room_prefix.empty()) cfg.room_prefix = room_prefix;
    if (!ring_timeout.empty()) cfg.ring_timeout_sec = std::max(15, std::atoi(ring_timeout.c_str()));
    if (!busy_key_ttl.empty()) cfg.busy_key_ttl_sec = std::max(15, std::atoi(busy_key_ttl.c_str()));
    if (!token_ttl.empty()) cfg.token_ttl_sec = std::max(300, std::atoi(token_ttl.c_str()));
    return cfg;
}

bool CallService::ParseAuthRequest(const memochat::json::JsonValue& request, int& uid, std::string& token, std::string& call_id) const
{
    uid = memochat::json::glaze_safe_get<int>(request, "uid", 0);
    token = memochat::json::glaze_safe_get<std::string>(request, "token", "");
    call_id = memochat::json::glaze_safe_get<std::string>(request, "call_id", "");
    return uid > 0 && !token.empty();
}

bool CallService::LoadSession(const std::string& call_id, CallSessionInfo& session) const
{
    std::string json_value;
    if (RedisMgr::GetInstance()->Get(std::string(CALL_SESSION_PREFIX) + call_id, json_value)) {
        memochat::json::JsonValue root;
        if (memochat::json::glaze_parse(root, json_value)) {
            session.call_id = memochat::json::glaze_safe_get<std::string>(root, "call_id", "");
            session.room_name = memochat::json::glaze_safe_get<std::string>(root, "room_name", "");
            session.call_type = memochat::json::glaze_safe_get<std::string>(root, "call_type", "");
            session.caller_uid = memochat::json::glaze_safe_get<int>(root, "caller_uid", 0);
            session.callee_uid = memochat::json::glaze_safe_get<int>(root, "callee_uid", 0);
            session.state = memochat::json::glaze_safe_get<std::string>(root, "state", "");
            session.started_at_ms = memochat::json::glaze_safe_get<int64_t>(root, "started_at_ms", 0);
            session.accepted_at_ms = memochat::json::glaze_safe_get<int64_t>(root, "accepted_at_ms", 0);
            session.ended_at_ms = memochat::json::glaze_safe_get<int64_t>(root, "ended_at_ms", 0);
            session.expires_at_ms = memochat::json::glaze_safe_get<int64_t>(root, "expires_at_ms", 0);
            session.duration_sec = memochat::json::glaze_safe_get<int>(root, "duration_sec", 0);
            session.reason = memochat::json::glaze_safe_get<std::string>(root, "reason", "");
            session.trace_id = memochat::json::glaze_safe_get<std::string>(root, "trace_id", "");
            session.updated_at_ms = memochat::json::glaze_safe_get<int64_t>(root, "updated_at_ms", 0);
            return !session.call_id.empty();
        }
    }
    return PostgresMgr::GetInstance()->GetCallSession(call_id, session);
}

bool CallService::SaveSession(const CallSessionInfo& session, int ttl_seconds) const
{
    memochat::json::JsonValue root = memochat::json::make_document();
    root["call_id"] = session.call_id;
    root["room_name"] = session.room_name;
    root["call_type"] = session.call_type;
    root["caller_uid"] = session.caller_uid;
    root["callee_uid"] = session.callee_uid;
    root["state"] = session.state;
    root["started_at_ms"] = session.started_at_ms;
    root["accepted_at_ms"] = session.accepted_at_ms;
    root["ended_at_ms"] = session.ended_at_ms;
    root["expires_at_ms"] = session.expires_at_ms;
    root["duration_sec"] = session.duration_sec;
    root["reason"] = session.reason;
    root["trace_id"] = session.trace_id;
    root["updated_at_ms"] = session.updated_at_ms;
    const std::string payload = memochat::json::glaze_stringify(root);
    if (ttl_seconds > 0) {
        RedisMgr::GetInstance()->SetEx(std::string(CALL_SESSION_PREFIX) + session.call_id, payload, ttl_seconds);
    } else {
        RedisMgr::GetInstance()->Set(std::string(CALL_SESSION_PREFIX) + session.call_id, payload);
    }
    return PostgresMgr::GetInstance()->UpsertCallSession(session);
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
    RedisMgr::GetInstance()->SetEx(std::string(CALL_RINGING_PREFIX) + std::to_string(session.caller_uid), session.call_id, ttl_seconds);
    RedisMgr::GetInstance()->SetEx(std::string(CALL_RINGING_PREFIX) + std::to_string(session.callee_uid), session.call_id, ttl_seconds);
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
    for (int uid : touids) {
        std::string server_name;
        if (!RedisMgr::GetInstance()->Get(std::string(USERIPPREFIX) + std::to_string(uid), server_name) || server_name.empty()) {
            continue;
        }
        routed[server_name].push_back(uid);
    }

    const std::string payload_json = memochat::json::glaze_stringify(payload);
    for (const auto& entry : routed) {
        GroupEventNotifyReq req;
        req.set_tcp_msgid(kCallEventMsgId);
        req.set_payload_json(payload_json);
        for (int uid : entry.second) {
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
    memochat::json::JsonValue payload = memochat::json::make_document();
    payload["error"] = ErrorCodes::Success;
    payload["event_type"] = event_type;
    payload["call_id"] = session.call_id;
    payload["room_name"] = session.room_name;
    payload["call_type"] = session.call_type;
    payload["caller_uid"] = session.caller_uid;
    payload["callee_uid"] = session.callee_uid;
    payload["caller_name"] = caller.nick.empty() ? caller.name : caller.nick;
    payload["callee_name"] = callee.nick.empty() ? callee.name : callee.nick;
    payload["caller_icon"] = caller.icon;
    payload["callee_icon"] = callee.icon;
    payload["started_at"] = session.started_at_ms;
    payload["accepted_at"] = session.accepted_at_ms;
    payload["ended_at"] = session.ended_at_ms;
    payload["expires_at"] = session.expires_at_ms;
    payload["state"] = session.state;
    payload["reason"] = reason.empty() ? session.reason : reason;
    payload["trace_id"] = session.trace_id;
    payload["livekit_url"] = cfg.livekit_url;
    return payload;
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
    payload["nbf"] = now_sec - 5;
    payload["exp"] = now_sec + cfg.token_ttl_sec;
    payload["name"] = std::string("memochat-") + role + "-" + std::to_string(uid);
    payload["video"] = video;

    const std::string encoded_header = Base64UrlEncode(memochat::json::glaze_stringify(header));
    const std::string encoded_payload = Base64UrlEncode(memochat::json::glaze_stringify(payload));
    const std::string signing_input = encoded_header + "." + encoded_payload;
    const std::string signature = Base64UrlEncode(HmacSha256(cfg.api_secret, signing_input));
    return signing_input + "." + signature;
}

bool CallService::StartCall(const memochat::json::JsonValue& request, memochat::json::JsonValue& response, const std::string& trace_id)
{
    const auto cfg = LoadConfig();
    if (!cfg.enabled) {
        response["error"] = ErrorCodes::Error_Json;
        return true;
    }
    int uid = 0;
    int peer_uid = memochat::json::glaze_safe_get<int>(request, "peer_uid", 0);
    std::string token;
    std::string unused_call_id;
    if (!ParseAuthRequest(request, uid, token, unused_call_id) || !ValidateGateUserToken(uid, token) || peer_uid <= 0 || uid == peer_uid) {
        response["error"] = ErrorCodes::TokenInvalid;
        return true;
    }
    const std::string call_type = memochat::json::glaze_safe_get<std::string>(request, "call_type", "");
    if (call_type != "voice" && call_type != "video") {
        response["error"] = ErrorCodes::Error_Json;
        return true;
    }
    if (!PostgresMgr::GetInstance()->IsFriend(uid, peer_uid) || !PostgresMgr::GetInstance()->IsFriend(peer_uid, uid)) {
        response["error"] = ErrorCodes::CallPermissionDenied;
        return true;
    }
    if (!RedisMgr::GetInstance()->ExistsKey(std::string(USERIPPREFIX) + std::to_string(peer_uid))) {
        response["error"] = ErrorCodes::CallTargetOffline;
        return true;
    }
    if (RedisMgr::GetInstance()->ExistsKey(std::string(CALL_ACTIVE_PREFIX) + std::to_string(uid)) ||
        RedisMgr::GetInstance()->ExistsKey(std::string(CALL_ACTIVE_PREFIX) + std::to_string(peer_uid)) ||
        RedisMgr::GetInstance()->ExistsKey(std::string(CALL_RINGING_PREFIX) + std::to_string(uid)) ||
        RedisMgr::GetInstance()->ExistsKey(std::string(CALL_RINGING_PREFIX) + std::to_string(peer_uid))) {
        response["error"] = ErrorCodes::CallBusy;
        return true;
    }

    CallUserProfile caller;
    CallUserProfile callee;
    if (!PostgresMgr::GetInstance()->GetCallUserProfile(uid, caller) ||
        !PostgresMgr::GetInstance()->GetCallUserProfile(peer_uid, callee)) {
        response["error"] = ErrorCodes::UidInvalid;
        return true;
    }

    CallSessionInfo session;
    session.call_id = NewId();
    session.room_name = cfg.room_prefix + "-" + session.call_id.substr(0, 8);
    session.call_type = call_type;
    session.caller_uid = uid;
    session.callee_uid = peer_uid;
    session.state = "ringing";
    session.started_at_ms = NowMs();
    session.expires_at_ms = session.started_at_ms + static_cast<int64_t>(cfg.ring_timeout_sec) * 1000;
    session.trace_id = trace_id;
    session.updated_at_ms = session.started_at_ms;
    SaveSession(session, cfg.ring_timeout_sec + 600);
    SetRingingState(session, std::max(cfg.ring_timeout_sec + 5, cfg.busy_key_ttl_sec));

    response = BuildEventPayload("call.state_sync", session, caller, callee);
    response["peer_uid"] = peer_uid;
    response["peer_name"] = callee.nick.empty() ? callee.name : callee.nick;
    response["peer_icon"] = callee.icon;
    response["livekit_url"] = cfg.livekit_url;

    NotifyUsers({ peer_uid }, BuildEventPayload("call.invite", session, caller, callee));
    memolog::LogInfo("call.start.dispatched", "call invite dispatched",
        {{"uid", std::to_string(uid)}, {"peer_uid", std::to_string(peer_uid)}, {"call_id", session.call_id}, {"call_type", call_type}, {"module", "call"}});
    return true;
}

bool CallService::AcceptCall(const memochat::json::JsonValue& request, memochat::json::JsonValue& response, const std::string& trace_id)
{
    int uid = 0;
    std::string token;
    std::string call_id;
    if (!ParseAuthRequest(request, uid, token, call_id) || !ValidateGateUserToken(uid, token)) {
        response["error"] = ErrorCodes::TokenInvalid;
        return true;
    }
    CallSessionInfo session;
    if (!LoadSession(call_id, session)) {
        response["error"] = ErrorCodes::CallNotFound;
        return true;
    }
    if (session.callee_uid != uid) {
        response["error"] = ErrorCodes::CallPermissionDenied;
        return true;
    }
    if (session.state != "ringing") {
        response["error"] = ErrorCodes::CallStateInvalid;
        return true;
    }
    CallUserProfile caller;
    CallUserProfile callee;
    PostgresMgr::GetInstance()->GetCallUserProfile(session.caller_uid, caller);
    PostgresMgr::GetInstance()->GetCallUserProfile(session.callee_uid, callee);
    session.state = "accepted";
    session.accepted_at_ms = NowMs();
    session.updated_at_ms = session.accepted_at_ms;
    session.trace_id = trace_id.empty() ? session.trace_id : trace_id;
    SaveSession(session, 86400);
    ClearBusyState(session);
    SetActiveState(session);
    response = BuildEventPayload("call.accepted", session, caller, callee);
    response["livekit_url"] = LoadConfig().livekit_url;
    NotifyUsers({ session.caller_uid, session.callee_uid }, response);
    return true;
}

bool CallService::RejectCall(const memochat::json::JsonValue& request, memochat::json::JsonValue& response, const std::string& trace_id)
{
    int uid = 0;
    std::string token;
    std::string call_id;
    if (!ParseAuthRequest(request, uid, token, call_id) || !ValidateGateUserToken(uid, token)) {
        response["error"] = ErrorCodes::TokenInvalid;
        return true;
    }
    CallSessionInfo session;
    if (!LoadSession(call_id, session)) {
        response["error"] = ErrorCodes::CallNotFound;
        return true;
    }
    if (session.callee_uid != uid) {
        response["error"] = ErrorCodes::CallPermissionDenied;
        return true;
    }
    if (session.state != "ringing") {
        response["error"] = ErrorCodes::CallStateInvalid;
        return true;
    }
    CallUserProfile caller;
    CallUserProfile callee;
    PostgresMgr::GetInstance()->GetCallUserProfile(session.caller_uid, caller);
    PostgresMgr::GetInstance()->GetCallUserProfile(session.callee_uid, callee);
    session.state = "rejected";
    session.reason = "rejected";
    session.ended_at_ms = NowMs();
    session.updated_at_ms = session.ended_at_ms;
    session.trace_id = trace_id.empty() ? session.trace_id : trace_id;
    SaveSession(session, 86400);
    ClearBusyState(session);
    response = BuildEventPayload("call.rejected", session, caller, callee);
    NotifyUsers({ session.caller_uid, session.callee_uid }, response);
    return true;
}

bool CallService::CancelCall(const memochat::json::JsonValue& request, memochat::json::JsonValue& response, const std::string& trace_id)
{
    int uid = 0;
    std::string token;
    std::string call_id;
    if (!ParseAuthRequest(request, uid, token, call_id) || !ValidateGateUserToken(uid, token)) {
        response["error"] = ErrorCodes::TokenInvalid;
        return true;
    }
    CallSessionInfo session;
    if (!LoadSession(call_id, session)) {
        response["error"] = ErrorCodes::CallNotFound;
        return true;
    }
    if (session.caller_uid != uid) {
        response["error"] = ErrorCodes::CallPermissionDenied;
        return true;
    }
    if (session.state != "ringing") {
        response["error"] = ErrorCodes::CallStateInvalid;
        return true;
    }
    const auto now_ms = NowMs();
    CallUserProfile caller;
    CallUserProfile callee;
    PostgresMgr::GetInstance()->GetCallUserProfile(session.caller_uid, caller);
    PostgresMgr::GetInstance()->GetCallUserProfile(session.callee_uid, callee);
    session.state = now_ms > session.expires_at_ms ? "timeout" : "cancelled";
    session.reason = session.state;
    session.ended_at_ms = now_ms;
    session.updated_at_ms = now_ms;
    session.trace_id = trace_id.empty() ? session.trace_id : trace_id;
    SaveSession(session, 86400);
    ClearBusyState(session);
    response = BuildEventPayload(session.state == "timeout" ? "call.timeout" : "call.cancel", session, caller, callee);
    NotifyUsers({ session.caller_uid, session.callee_uid }, response);
    return true;
}

bool CallService::HangupCall(const memochat::json::JsonValue& request, memochat::json::JsonValue& response, const std::string& trace_id)
{
    int uid = 0;
    std::string token;
    std::string call_id;
    if (!ParseAuthRequest(request, uid, token, call_id) || !ValidateGateUserToken(uid, token)) {
        response["error"] = ErrorCodes::TokenInvalid;
        return true;
    }
    CallSessionInfo session;
    if (!LoadSession(call_id, session)) {
        response["error"] = ErrorCodes::CallNotFound;
        return true;
    }
    if (session.caller_uid != uid && session.callee_uid != uid) {
        response["error"] = ErrorCodes::CallPermissionDenied;
        return true;
    }
    if (session.state != "accepted" && session.state != "joining" && session.state != "in_call") {
        response["error"] = ErrorCodes::CallStateInvalid;
        return true;
    }
    CallUserProfile caller;
    CallUserProfile callee;
    PostgresMgr::GetInstance()->GetCallUserProfile(session.caller_uid, caller);
    PostgresMgr::GetInstance()->GetCallUserProfile(session.callee_uid, callee);
    session.state = "ended";
    session.reason = "hangup";
    session.ended_at_ms = NowMs();
    if (session.accepted_at_ms > 0 && session.ended_at_ms > session.accepted_at_ms) {
        session.duration_sec = static_cast<int>((session.ended_at_ms - session.accepted_at_ms) / 1000);
    }
    session.updated_at_ms = session.ended_at_ms;
    session.trace_id = trace_id.empty() ? session.trace_id : trace_id;
    SaveSession(session, 86400);
    ClearBusyState(session);
    response = BuildEventPayload("call.hangup", session, caller, callee);
    NotifyUsers({ session.caller_uid, session.callee_uid }, response);
    return true;
}

bool CallService::GetToken(int uid, const std::string& token, const std::string& call_id, const std::string& role,
                           memochat::json::JsonValue& response, const std::string& trace_id)
{
    if (!ValidateGateUserToken(uid, token)) {
        response["error"] = ErrorCodes::TokenInvalid;
        return true;
    }
    CallSessionInfo session;
    if (!LoadSession(call_id, session)) {
        response["error"] = ErrorCodes::CallNotFound;
        return true;
    }
    const bool is_caller = session.caller_uid == uid;
    const bool is_callee = session.callee_uid == uid;
    if (!is_caller && !is_callee) {
        response["error"] = ErrorCodes::CallPermissionDenied;
        return true;
    }
    if (session.state != "accepted" && session.state != "joining" && session.state != "in_call") {
        response["error"] = ErrorCodes::CallStateInvalid;
        return true;
    }
    session.state = "joining";
    session.updated_at_ms = NowMs();
    session.trace_id = trace_id.empty() ? session.trace_id : trace_id;
    SaveSession(session, 86400);
    response["error"] = ErrorCodes::Success;
    response["call_id"] = session.call_id;
    response["room_name"] = session.room_name;
    response["role"] = role.empty() ? (is_caller ? "caller" : "callee") : role;
    response["livekit_url"] = LoadConfig().livekit_url;
    response["token"] = CreateToken(session, uid, memochat::json::glaze_safe_get<std::string>(response, "role", ""));
    response["trace_id"] = session.trace_id;
    return true;
}

