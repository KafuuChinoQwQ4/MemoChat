#include "WinCompat.h"
#include "DrogonCallHandlers.h"
#include "DrogonCallSupport.h"
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>

using namespace drogon;

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
    resp["error"] = 0;  // Success
    if (data.isObject()) {
        for (const auto& key : data.getMemberNames()) {
            resp[key] = data[key];
        }
    }
    return resp;
}

std::string ExtractTraceId(const HttpRequestPtr& req) {
    auto trace_id = req->getHeader("X-Trace-Id");
    if (trace_id.empty()) {
        trace_id = req->getHeader("x-trace-id");
    }
    return trace_id;
}

void DrogonCallHandlers::HandleCallStart(const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    Json::Value root;
    if (!DrogonCallSupport::ParseJsonBody(body_str, root)) {
        callback(HttpResponse::newHttpJsonResponse(MakeCallError(1, "invalid json")));
        return;
    }
    auto trace_id = ExtractTraceId(req);
    auto result = DrogonCallSupport::HandleCallStart(root, trace_id);
    Json::Value out = MakeCallOk(result.data);
    callback(HttpResponse::newHttpJsonResponse(out));
}

void DrogonCallHandlers::HandleCallAccept(const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    Json::Value root;
    if (!DrogonCallSupport::ParseJsonBody(body_str, root)) {
        callback(HttpResponse::newHttpJsonResponse(MakeCallError(1, "invalid json")));
        return;
    }
    auto trace_id = ExtractTraceId(req);
    auto result = DrogonCallSupport::HandleCallAccept(root, trace_id);
    Json::Value out = MakeCallOk(result.data);
    callback(HttpResponse::newHttpJsonResponse(out));
}

void DrogonCallHandlers::HandleCallReject(const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    Json::Value root;
    if (!DrogonCallSupport::ParseJsonBody(body_str, root)) {
        callback(HttpResponse::newHttpJsonResponse(MakeCallError(1, "invalid json")));
        return;
    }
    auto trace_id = ExtractTraceId(req);
    auto result = DrogonCallSupport::HandleCallReject(root, trace_id);
    Json::Value out = MakeCallOk(result.data);
    callback(HttpResponse::newHttpJsonResponse(out));
}

void DrogonCallHandlers::HandleCallCancel(const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    Json::Value root;
    if (!DrogonCallSupport::ParseJsonBody(body_str, root)) {
        callback(HttpResponse::newHttpJsonResponse(MakeCallError(1, "invalid json")));
        return;
    }
    auto trace_id = ExtractTraceId(req);
    auto result = DrogonCallSupport::HandleCallCancel(root, trace_id);
    Json::Value out = MakeCallOk(result.data);
    callback(HttpResponse::newHttpJsonResponse(out));
}

void DrogonCallHandlers::HandleCallHangup(const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    Json::Value root;
    if (!DrogonCallSupport::ParseJsonBody(body_str, root)) {
        callback(HttpResponse::newHttpJsonResponse(MakeCallError(1, "invalid json")));
        return;
    }
    auto trace_id = ExtractTraceId(req);
    auto result = DrogonCallSupport::HandleCallHangup(root, trace_id);
    Json::Value out = MakeCallOk(result.data);
    callback(HttpResponse::newHttpJsonResponse(out));
}

void DrogonCallHandlers::HandleCallToken(const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    Json::Value root;
    int uid = 0;
    std::string token;
    std::string call_id;
    std::string role;

    // Try to parse JSON body first
    if (DrogonCallSupport::ParseJsonBody(body_str, root)) {
        uid = root.get("uid", 0).asInt();
        token = root.get("token", "").asString();
        call_id = root.get("call_id", "").asString();
        role = root.get("role", "").asString();
    }

    auto trace_id = ExtractTraceId(req);
    auto result = DrogonCallSupport::HandleCallTokenGet(uid, token, call_id, role, trace_id);
    Json::Value out = MakeCallOk(result.data);
    callback(HttpResponse::newHttpJsonResponse(out));
}

void DrogonCallHandlers::HandleCallTokenPost(const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    Json::Value root;
    if (!DrogonCallSupport::ParseJsonBody(body_str, root)) {
        callback(HttpResponse::newHttpJsonResponse(MakeCallError(1, "invalid json")));
        return;
    }
    auto trace_id = ExtractTraceId(req);
    auto result = DrogonCallSupport::HandleCallTokenPost(root, trace_id);
    Json::Value out = MakeCallOk(result.data);
    callback(HttpResponse::newHttpJsonResponse(out));
}
