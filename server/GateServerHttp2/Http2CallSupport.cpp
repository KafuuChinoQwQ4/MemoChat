#include "WinCompat.h"
#include "json/GlazeCompat.h"
#include "Http2CallSupport.h"
#include "../GateServerCore/CallService.h"
#include "../GateServerCore/const.h"
#include <sstream>
#include <chrono>

namespace {

int64_t NowMs() {
    return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

}  // namespace

namespace Http2CallSupport {

bool ParseJsonBody(std::string_view body_sv, memochat::json::JsonValue& root) {
    std::string body_str(body_sv);
    return memochat::json::glaze_parse(root, body_str) && memochat::json::glaze_is_object(root);
}

CallResult HandleCallStart(const memochat::json::JsonValue& req, const std::string& trace_id) {
    CallResult result;
    memochat::json::JsonValue response;
    bool ok = CallService::GetInstance()->StartCall(req, response, trace_id);
    result.error = ok ? memochat::json::glaze_safe_get<int>(response, "error", 0)
                       : memochat::json::glaze_safe_get<int>(response, "error", 1);
    result.data = response;
    return result;
}

CallResult HandleCallAccept(const memochat::json::JsonValue& req, const std::string& trace_id) {
    CallResult result;
    memochat::json::JsonValue response;
    bool ok = CallService::GetInstance()->AcceptCall(req, response, trace_id);
    result.error = ok ? memochat::json::glaze_safe_get<int>(response, "error", 0)
                       : memochat::json::glaze_safe_get<int>(response, "error", 1);
    result.data = response;
    return result;
}

CallResult HandleCallReject(const memochat::json::JsonValue& req, const std::string& trace_id) {
    CallResult result;
    memochat::json::JsonValue response;
    bool ok = CallService::GetInstance()->RejectCall(req, response, trace_id);
    result.error = ok ? memochat::json::glaze_safe_get<int>(response, "error", 0)
                       : memochat::json::glaze_safe_get<int>(response, "error", 1);
    result.data = response;
    return result;
}

CallResult HandleCallCancel(const memochat::json::JsonValue& req, const std::string& trace_id) {
    CallResult result;
    memochat::json::JsonValue response;
    bool ok = CallService::GetInstance()->CancelCall(req, response, trace_id);
    result.error = ok ? memochat::json::glaze_safe_get<int>(response, "error", 0)
                       : memochat::json::glaze_safe_get<int>(response, "error", 1);
    result.data = response;
    return result;
}

CallResult HandleCallHangup(const memochat::json::JsonValue& req, const std::string& trace_id) {
    CallResult result;
    memochat::json::JsonValue response;
    bool ok = CallService::GetInstance()->HangupCall(req, response, trace_id);
    result.error = ok ? memochat::json::glaze_safe_get<int>(response, "error", 0)
                       : memochat::json::glaze_safe_get<int>(response, "error", 1);
    result.data = response;
    return result;
}

CallResult HandleCallTokenGet(int uid, const std::string& token,
                               const std::string& call_id, const std::string& role,
                               const std::string& trace_id) {
    CallResult result;
    memochat::json::JsonValue response;
    CallService::GetInstance()->GetToken(uid, token, call_id, role, response, trace_id);
    result.error = memochat::json::glaze_safe_get<int>(response, "error", 0);
    result.data = response;
    return result;
}

CallResult HandleCallTokenPost(const memochat::json::JsonValue& req, const std::string& trace_id) {
    CallResult result;
    const int uid = memochat::json::glaze_safe_get<int>(req, "uid", 0);
    const std::string token = memochat::json::glaze_safe_get<std::string>(req, "token", "");
    const std::string call_id = memochat::json::glaze_safe_get<std::string>(req, "call_id", "");
    const std::string role = memochat::json::glaze_safe_get<std::string>(req, "role", "");
    memochat::json::JsonValue response;
    CallService::GetInstance()->GetToken(uid, token, call_id, role, response, trace_id);
    result.error = memochat::json::glaze_safe_get<int>(response, "error", 0);
    result.data = response;
    return result;
}

}  // namespace Http2CallSupport

