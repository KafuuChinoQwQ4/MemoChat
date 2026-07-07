#include "modules/call/CallRouteModule.hpp"

#include "CallPublicDtos.hpp"
#include "CallService.hpp"
#include "const.hpp"
#include "json/GlazeCompat.hpp"
#include "logging/Logger.hpp"
#include "routing/RouteRegistry.hpp"
#include "support/BearerAccessAuth.hpp"

#include <string>

import memochat.call.route_module_algorithms;
import memochat.call.route_response_algorithms;

namespace memochat::gate::modules::call
{
namespace
{
namespace response_modules = memochat::call::route_response::modules;

using JsonPostHandler = bool (*)(int, const memochat::json::JsonValue&, memochat::json::JsonValue&, const std::string&);

void WriteJson(memochat::gate::routing::GateResponse& response,
               const std::string& content_type,
               const memochat::json::JsonValue& root)
{
    response.status = response_modules::OkStatus();
    response.content_type = content_type;
    response.body = memochat::json::glaze_stringify(root);
}

bool HandleJsonPost(const memochat::gate::routing::GateRequest& request,
                    memochat::gate::routing::GateResponse& response,
                    JsonPostHandler handler)
{
    int uid = 0;
    if (!memochat::auth::ResolveBearerAccessUserId(request, uid))
    {
        memochat::json::JsonValue root;
        root["error"] = ErrorCodes::TokenInvalid;
        root["trace_id"] = request.trace_id;
        WriteJson(response, response_modules::JsonContentType(), root);
        return true;
    }

    memochat::json::JsonValue root;
    memochat::json::JsonValue src_root;
    memochat::json::JsonReader reader;
    if (!reader.parse(request.body, src_root))
    {
        root["error"] = ErrorCodes::Error_Json;
        WriteJson(response, response_modules::JsonContentType(), root);
        return true;
    }

    handler(uid, src_root, root, request.trace_id);
    root["trace_id"] = request.trace_id;
    WriteJson(response, response_modules::JsonContentType(), root);
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

bool StartCall(int uid,
               const memochat::json::JsonValue& src_root,
               memochat::json::JsonValue& root,
               const std::string& trace_id)
{
    memolog::LogInfo("call.start.requested", "call start requested", {{"trace_id", trace_id}, {"module", "call"}});
    return CallService::GetInstance()->StartCall(uid, src_root, root, trace_id);
}

bool AcceptCall(int uid,
                const memochat::json::JsonValue& src_root,
                memochat::json::JsonValue& root,
                const std::string& trace_id)
{
    return CallService::GetInstance()->AcceptCall(uid, src_root, root, trace_id);
}

bool RejectCall(int uid,
                const memochat::json::JsonValue& src_root,
                memochat::json::JsonValue& root,
                const std::string& trace_id)
{
    return CallService::GetInstance()->RejectCall(uid, src_root, root, trace_id);
}

bool CancelCall(int uid,
                const memochat::json::JsonValue& src_root,
                memochat::json::JsonValue& root,
                const std::string& trace_id)
{
    return CallService::GetInstance()->CancelCall(uid, src_root, root, trace_id);
}

bool HangupCall(int uid,
                const memochat::json::JsonValue& src_root,
                memochat::json::JsonValue& root,
                const std::string& trace_id)
{
    return CallService::GetInstance()->HangupCall(uid, src_root, root, trace_id);
}

bool PostToken(int uid,
               const memochat::json::JsonValue& src_root,
               memochat::json::JsonValue& root,
               const std::string& trace_id)
{
    const memochat::call::CallTokenRequestDto token_request = memochat::call::CallTokenRequestFromJsonValue(src_root);
    return CallService::GetInstance()->GetToken(uid, token_request.call_id, token_request.role, root, trace_id);
}

bool HandleGetToken(const memochat::gate::routing::GateRequest& request,
                    memochat::gate::routing::GateResponse& response)
{
    int uid = 0;
    const std::string call_id = QueryValue(request, "call_id");
    const std::string role = QueryValue(request, "role");

    memochat::json::JsonValue root;
    if (!memochat::auth::ResolveBearerAccessUserId(request, uid))
    {
        root["error"] = ErrorCodes::TokenInvalid;
    }
    else
    {
        CallService::GetInstance()->GetToken(uid, call_id, role, root, request.trace_id);
    }

    response.status = response_modules::OkStatus();
    response.content_type = response_modules::TokenJsonContentType();
    response.body = memochat::json::glaze_stringify(root);
    if (memochat::call::route_modules::IsJsonObjectTailForTraceAppend(
            response.body.empty(),
            response.body.empty() ? '\0' : response.body.back(),
            response.body.find(response_modules::TraceIdSearchToken()) != std::string::npos))
    {
        response.body.pop_back();
        response.body += ",\"trace_id\":\"" + request.trace_id + "\"}";
    }
    return true;
}

} // namespace

void CallRouteModule::RegisterRoutes(memochat::gate::routing::RouteRegistry& registry)
{
    namespace modules = memochat::call::route_modules;

    registry.Register(
        modules::PostMethod(),
        modules::StartPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return HandleJsonPost(request, response, StartCall);
        });
    registry.Register(
        modules::PostMethod(),
        modules::AcceptPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return HandleJsonPost(request, response, AcceptCall);
        });
    registry.Register(
        modules::PostMethod(),
        modules::RejectPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return HandleJsonPost(request, response, RejectCall);
        });
    registry.Register(
        modules::PostMethod(),
        modules::CancelPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return HandleJsonPost(request, response, CancelCall);
        });
    registry.Register(
        modules::PostMethod(),
        modules::HangupPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return HandleJsonPost(request, response, HangupCall);
        });
    registry.Register(modules::GetMethod(), modules::TokenPath(), HandleGetToken);
    registry.Register(
        modules::PostMethod(),
        modules::TokenPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return HandleJsonPost(request, response, PostToken);
        });
}

} // namespace memochat::gate::modules::call
