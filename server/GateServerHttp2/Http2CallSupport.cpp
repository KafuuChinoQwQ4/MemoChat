#include "WinCompat.h"
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

bool ParseJsonBody(std::string_view body_sv, Json::Value& root) {
    std::string body_str(body_sv);
    Json::Reader reader;
    return reader.parse(body_str, root) && root.isObject();
}

CallResult HandleCallStart(const Json::Value& req, const std::string& trace_id) {
    CallResult result;
    Json::Value response;
    bool ok = CallService::GetInstance()->StartCall(req, response, trace_id);
    result.error = ok ? response.get("error", 0).asInt()
                       : response.get("error", 1).asInt();
    result.data = response;
    return result;
}

CallResult HandleCallAccept(const Json::Value& req, const std::string& trace_id) {
    CallResult result;
    Json::Value response;
    bool ok = CallService::GetInstance()->AcceptCall(req, response, trace_id);
    result.error = ok ? response.get("error", 0).asInt()
                       : response.get("error", 1).asInt();
    result.data = response;
    return result;
}

CallResult HandleCallReject(const Json::Value& req, const std::string& trace_id) {
    CallResult result;
    Json::Value response;
    bool ok = CallService::GetInstance()->RejectCall(req, response, trace_id);
    result.error = ok ? response.get("error", 0).asInt()
                       : response.get("error", 1).asInt();
    result.data = response;
    return result;
}

CallResult HandleCallCancel(const Json::Value& req, const std::string& trace_id) {
    CallResult result;
    Json::Value response;
    bool ok = CallService::GetInstance()->CancelCall(req, response, trace_id);
    result.error = ok ? response.get("error", 0).asInt()
                       : response.get("error", 1).asInt();
    result.data = response;
    return result;
}

CallResult HandleCallHangup(const Json::Value& req, const std::string& trace_id) {
    CallResult result;
    Json::Value response;
    bool ok = CallService::GetInstance()->HangupCall(req, response, trace_id);
    result.error = ok ? response.get("error", 0).asInt()
                       : response.get("error", 1).asInt();
    result.data = response;
    return result;
}

CallResult HandleCallTokenGet(int uid, const std::string& token,
                               const std::string& call_id, const std::string& role,
                               const std::string& trace_id) {
    CallResult result;
    Json::Value response;
    CallService::GetInstance()->GetToken(uid, token, call_id, role, response, trace_id);
    result.error = response.get("error", 0).asInt();
    result.data = response;
    return result;
}

CallResult HandleCallTokenPost(const Json::Value& req, const std::string& trace_id) {
    CallResult result;
    const int uid = req.get("uid", 0).asInt();
    const std::string token = req.get("token", "").asString();
    const std::string call_id = req.get("call_id", "").asString();
    const std::string role = req.get("role", "").asString();
    Json::Value response;
    CallService::GetInstance()->GetToken(uid, token, call_id, role, response, trace_id);
    result.error = response.get("error", 0).asInt();
    result.data = response;
    return result;
}

}  // namespace Http2CallSupport
