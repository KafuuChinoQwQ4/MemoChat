#include "WinCompat.h"
#include "Http2CallHandlers.h"
#include "Http2CallSupport.h"

static Json::Value MakeCallError(int error_code, const std::string& message) {
    Json::Value resp;
    resp["error"] = error_code;
    if (!message.empty()) {
        resp["message"] = message;
    }
    return resp;
}

static Json::Value MakeCallOk(const Json::Value& data) {
    Json::Value resp;
    resp["error"] = 0;
    if (data.isObject()) {
        for (const auto& key : data.getMemberNames()) {
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
    Json::Value root;
    if (!Http2CallSupport::ParseJsonBody(req.body, root)) {
        resp.SetJsonBody(MakeCallError(1, "invalid json").toStyledString());
        return;
    }
    auto trace_id = ExtractTraceId(req);
    auto result = Http2CallSupport::HandleCallStart(root, trace_id);
    Json::Value out = MakeCallOk(result.data);
    resp.SetJsonBody(out.toStyledString());
}

void Http2CallHandlers::HandleCallAccept(const Http2Request& req, Http2Response& resp)
{
    Json::Value root;
    if (!Http2CallSupport::ParseJsonBody(req.body, root)) {
        resp.SetJsonBody(MakeCallError(1, "invalid json").toStyledString());
        return;
    }
    auto trace_id = ExtractTraceId(req);
    auto result = Http2CallSupport::HandleCallAccept(root, trace_id);
    Json::Value out = MakeCallOk(result.data);
    resp.SetJsonBody(out.toStyledString());
}

void Http2CallHandlers::HandleCallReject(const Http2Request& req, Http2Response& resp)
{
    Json::Value root;
    if (!Http2CallSupport::ParseJsonBody(req.body, root)) {
        resp.SetJsonBody(MakeCallError(1, "invalid json").toStyledString());
        return;
    }
    auto trace_id = ExtractTraceId(req);
    auto result = Http2CallSupport::HandleCallReject(root, trace_id);
    Json::Value out = MakeCallOk(result.data);
    resp.SetJsonBody(out.toStyledString());
}

void Http2CallHandlers::HandleCallCancel(const Http2Request& req, Http2Response& resp)
{
    Json::Value root;
    if (!Http2CallSupport::ParseJsonBody(req.body, root)) {
        resp.SetJsonBody(MakeCallError(1, "invalid json").toStyledString());
        return;
    }
    auto trace_id = ExtractTraceId(req);
    auto result = Http2CallSupport::HandleCallCancel(root, trace_id);
    Json::Value out = MakeCallOk(result.data);
    resp.SetJsonBody(out.toStyledString());
}

void Http2CallHandlers::HandleCallHangup(const Http2Request& req, Http2Response& resp)
{
    Json::Value root;
    if (!Http2CallSupport::ParseJsonBody(req.body, root)) {
        resp.SetJsonBody(MakeCallError(1, "invalid json").toStyledString());
        return;
    }
    auto trace_id = ExtractTraceId(req);
    auto result = Http2CallSupport::HandleCallHangup(root, trace_id);
    Json::Value out = MakeCallOk(result.data);
    resp.SetJsonBody(out.toStyledString());
}

void Http2CallHandlers::HandleCallToken(const Http2Request& req, Http2Response& resp)
{
    Json::Value root;
    int uid = 0;
    std::string token;
    std::string call_id;
    std::string role;

    if (Http2CallSupport::ParseJsonBody(req.body, root)) {
        uid = root.get("uid", 0).asInt();
        token = root.get("token", "").asString();
        call_id = root.get("call_id", "").asString();
        role = root.get("role", "").asString();
    }

    auto trace_id = ExtractTraceId(req);
    auto result = Http2CallSupport::HandleCallTokenGet(uid, token, call_id, role, trace_id);
    Json::Value out = MakeCallOk(result.data);
    resp.SetJsonBody(out.toStyledString());
}

void Http2CallHandlers::HandleCallTokenPost(const Http2Request& req, Http2Response& resp)
{
    Json::Value root;
    if (!Http2CallSupport::ParseJsonBody(req.body, root)) {
        resp.SetJsonBody(MakeCallError(1, "invalid json").toStyledString());
        return;
    }
    auto trace_id = ExtractTraceId(req);
    auto result = Http2CallSupport::HandleCallTokenPost(root, trace_id);
    Json::Value out = MakeCallOk(result.data);
    resp.SetJsonBody(out.toStyledString());
}
