#include "WinCompat.h"
#include "json/GlazeCompat.h"
#include "Http2CallHandlers.h"
#include "Http2CallSupport.h"

static memochat::json::JsonValue MakeCallError(int error_code, const std::string& message) {
    memochat::json::JsonValue resp;
    resp["error"] = error_code;
    if (!message.empty()) {
        resp["message"] = message;
    }
    return resp;
}

static memochat::json::JsonValue MakeCallOk(const memochat::json::JsonValue& data) {
    memochat::json::JsonValue resp;
    resp["error"] = 0;
    if (memochat::json::glaze_is_object(data)) {
        for (const auto& key : memochat::json::getMemberNames(data)) {
            resp[key] = data[key];
        }
    }
    return resp;
}

static std::string ExtractTraceId(const Http2Request& req) {
    auto it = req.headers.find("X-Trace-Id");
    if (it != req.headers.end()) return it->second;
    it = req.headers.find("x-trace-id");
    if (it != req.headers.end()) return it->second;
    return "";
}

void Http2CallHandlers::HandleCallStart(const Http2Request& req, Http2Response& resp)
{
    memochat::json::JsonValue root;
    if (!Http2CallSupport::ParseJsonBody(req.body, root)) {
        resp.SetJsonBody(memochat::json::glaze_stringify(MakeCallError(1, "invalid json")));
        return;
    }
    auto trace_id = ExtractTraceId(req);
    auto result = Http2CallSupport::HandleCallStart(root, trace_id);
    memochat::json::JsonValue out = MakeCallOk(result.data);
    resp.SetJsonBody(memochat::json::glaze_stringify(out));
}

void Http2CallHandlers::HandleCallAccept(const Http2Request& req, Http2Response& resp)
{
    memochat::json::JsonValue root;
    if (!Http2CallSupport::ParseJsonBody(req.body, root)) {
        resp.SetJsonBody(memochat::json::glaze_stringify(MakeCallError(1, "invalid json")));
        return;
    }
    auto trace_id = ExtractTraceId(req);
    auto result = Http2CallSupport::HandleCallAccept(root, trace_id);
    memochat::json::JsonValue out = MakeCallOk(result.data);
    resp.SetJsonBody(memochat::json::glaze_stringify(out));
}

void Http2CallHandlers::HandleCallReject(const Http2Request& req, Http2Response& resp)
{
    memochat::json::JsonValue root;
    if (!Http2CallSupport::ParseJsonBody(req.body, root)) {
        resp.SetJsonBody(memochat::json::glaze_stringify(MakeCallError(1, "invalid json")));
        return;
    }
    auto trace_id = ExtractTraceId(req);
    auto result = Http2CallSupport::HandleCallReject(root, trace_id);
    memochat::json::JsonValue out = MakeCallOk(result.data);
    resp.SetJsonBody(memochat::json::glaze_stringify(out));
}

void Http2CallHandlers::HandleCallCancel(const Http2Request& req, Http2Response& resp)
{
    memochat::json::JsonValue root;
    if (!Http2CallSupport::ParseJsonBody(req.body, root)) {
        resp.SetJsonBody(memochat::json::glaze_stringify(MakeCallError(1, "invalid json")));
        return;
    }
    auto trace_id = ExtractTraceId(req);
    auto result = Http2CallSupport::HandleCallCancel(root, trace_id);
    memochat::json::JsonValue out = MakeCallOk(result.data);
    resp.SetJsonBody(memochat::json::glaze_stringify(out));
}

void Http2CallHandlers::HandleCallHangup(const Http2Request& req, Http2Response& resp)
{
    memochat::json::JsonValue root;
    if (!Http2CallSupport::ParseJsonBody(req.body, root)) {
        resp.SetJsonBody(memochat::json::glaze_stringify(MakeCallError(1, "invalid json")));
        return;
    }
    auto trace_id = ExtractTraceId(req);
    auto result = Http2CallSupport::HandleCallHangup(root, trace_id);
    memochat::json::JsonValue out = MakeCallOk(result.data);
    resp.SetJsonBody(memochat::json::glaze_stringify(out));
}

void Http2CallHandlers::HandleCallToken(const Http2Request& req, Http2Response& resp)
{
    memochat::json::JsonValue root;
    int uid = 0;
    std::string token;
    std::string call_id;
    std::string role;

    if (Http2CallSupport::ParseJsonBody(req.body, root)) {
        uid = memochat::json::glaze_safe_get<int>(root, "uid", 0);
        token = memochat::json::glaze_safe_get<std::string>(root, "token", "");
        call_id = memochat::json::glaze_safe_get<std::string>(root, "call_id", "");
        role = memochat::json::glaze_safe_get<std::string>(root, "role", "");
    }

    auto trace_id = ExtractTraceId(req);
    auto result = Http2CallSupport::HandleCallTokenGet(uid, token, call_id, role, trace_id);
    memochat::json::JsonValue out = MakeCallOk(result.data);
    resp.SetJsonBody(memochat::json::glaze_stringify(out));
}

void Http2CallHandlers::HandleCallTokenPost(const Http2Request& req, Http2Response& resp)
{
    memochat::json::JsonValue root;
    if (!Http2CallSupport::ParseJsonBody(req.body, root)) {
        resp.SetJsonBody(memochat::json::glaze_stringify(MakeCallError(1, "invalid json")));
        return;
    }
    auto trace_id = ExtractTraceId(req);
    auto result = Http2CallSupport::HandleCallTokenPost(root, trace_id);
    memochat::json::JsonValue out = MakeCallOk(result.data);
    resp.SetJsonBody(memochat::json::glaze_stringify(out));
}

