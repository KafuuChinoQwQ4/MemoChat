#include "modules/call/CallRouteModule.h"

#include "CallPublicDtos.h"
#include "CallService.h"
#include "const.h"
#include "json/GlazeCompat.h"
#include "logging/Logger.h"
#include "routing/RouteRegistry.h"

#include <cstdlib>
#include <string>

namespace memochat::gate::modules::call
{
namespace
{

using JsonPostHandler = bool (*)(const memochat::json::JsonValue&, memochat::json::JsonValue&, const std::string&);

void WriteJson(memochat::gate::routing::GateResponse& response,
               const std::string& content_type,
               const memochat::json::JsonValue& root)
{
    response.status = 200;
    response.content_type = content_type;
    response.body = memochat::json::glaze_stringify(root);
}

bool HandleJsonPost(const memochat::gate::routing::GateRequest& request,
                    memochat::gate::routing::GateResponse& response,
                    JsonPostHandler handler)
{
    memochat::json::JsonValue root;
    memochat::json::JsonValue src_root;
    memochat::json::JsonReader reader;
    if (!reader.parse(request.body, src_root))
    {
        root["error"] = ErrorCodes::Error_Json;
        WriteJson(response, "application/json", root);
        return true;
    }

    handler(src_root, root, request.trace_id);
    root["trace_id"] = request.trace_id;
    WriteJson(response, "application/json", root);
    return true;
}

std::string QueryValue(const memochat::gate::routing::GateRequest& request, const std::string& key)
{
    const auto iter = request.query.find(key);
    if (iter == request.query.end())
    {
        return "";
    }
    return iter->second;
}

int QueryUid(const memochat::gate::routing::GateRequest& request)
{
    const auto uid = QueryValue(request, "uid");
    return std::atoi(uid.c_str());
}

bool StartCall(const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string& trace_id)
{
    memolog::LogInfo("call.start.requested", "call start requested", {{"trace_id", trace_id}, {"module", "call"}});
    return CallService::GetInstance()->StartCall(src_root, root, trace_id);
}

bool AcceptCall(const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string& trace_id)
{
    return CallService::GetInstance()->AcceptCall(src_root, root, trace_id);
}

bool RejectCall(const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string& trace_id)
{
    return CallService::GetInstance()->RejectCall(src_root, root, trace_id);
}

bool CancelCall(const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string& trace_id)
{
    return CallService::GetInstance()->CancelCall(src_root, root, trace_id);
}

bool HangupCall(const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string& trace_id)
{
    return CallService::GetInstance()->HangupCall(src_root, root, trace_id);
}

bool PostToken(const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string& trace_id)
{
    const memochat::call::CallTokenRequestDto token_request = memochat::call::CallTokenRequestFromJsonValue(src_root);
    return CallService::GetInstance()
        ->GetToken(token_request.uid, token_request.token, token_request.call_id, token_request.role, root, trace_id);
}

bool HandleGetToken(const memochat::gate::routing::GateRequest& request,
                    memochat::gate::routing::GateResponse& response)
{
    const int uid = QueryUid(request);
    const std::string token = QueryValue(request, "token");
    const std::string call_id = QueryValue(request, "call_id");
    const std::string role = QueryValue(request, "role");

    memochat::json::JsonValue root;
    CallService::GetInstance()->GetToken(uid, token, call_id, role, root, request.trace_id);

    response.status = 200;
    response.content_type = "text/json";
    response.body = memochat::json::glaze_stringify(root);
    if (response.body.find("\"trace_id\"") == std::string::npos && !response.body.empty() &&
        response.body.back() == '}')
    {
        response.body.pop_back();
        response.body += ",\"trace_id\":\"" + request.trace_id + "\"}";
    }
    return true;
}

} // namespace

void CallRouteModule::RegisterRoutes(memochat::gate::routing::RouteRegistry& registry)
{
    registry.Register(
        "POST",
        "/api/call/start",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return HandleJsonPost(request, response, StartCall);
        });
    registry.Register(
        "POST",
        "/api/call/accept",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return HandleJsonPost(request, response, AcceptCall);
        });
    registry.Register(
        "POST",
        "/api/call/reject",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return HandleJsonPost(request, response, RejectCall);
        });
    registry.Register(
        "POST",
        "/api/call/cancel",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return HandleJsonPost(request, response, CancelCall);
        });
    registry.Register(
        "POST",
        "/api/call/hangup",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return HandleJsonPost(request, response, HangupCall);
        });
    registry.Register("GET", "/api/call/token", HandleGetToken);
    registry.Register(
        "POST",
        "/api/call/token",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return HandleJsonPost(request, response, PostToken);
        });
}

} // namespace memochat::gate::modules::call
