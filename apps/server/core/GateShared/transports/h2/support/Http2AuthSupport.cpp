#include "WinCompat.h"
#include "json/GlazeCompat.h"
#include "Http2AuthSupport.h"
#include "AuthCache.h"
#include "logging/Logger.h"
#include "routing/GateRequest.h"
#include "routing/GateResponse.h"
#include "services/auth/AuthService.h"
#include "const.h"

namespace Http2AuthSupport
{

using memochat::gate::core::AuthCache;

namespace
{

using AuthService = memochat::gate::services::auth::AuthService;
using GateRequest = memochat::gate::routing::GateRequest;
using GateResponse = memochat::gate::routing::GateResponse;
using AuthHandler = bool (AuthService::*)(const GateRequest&, GateResponse&);

GateRequest BuildAuthRequest(std::string_view route_path, const memochat::json::JsonValue& req)
{
    GateRequest request;
    request.method = "POST";
    request.path = std::string(route_path);
    request.target = request.path;
    request.body = memochat::json::glaze_stringify(req);
    return request;
}

LoginResult FromGateResponse(const GateResponse& response)
{
    LoginResult result;
    memochat::json::JsonValue root;
    if (!memochat::json::glaze_parse(root, response.body) || !memochat::json::glaze_is_object(root))
    {
        result.error = response.status >= 400 ? response.status : ErrorCodes::RPCFailed;
        result.message = "auth response parse failed";
        return result;
    }

    result.error = memochat::json::glaze_safe_get<int>(
        root,
        "error",
        response.status >= 400 ? response.status : ErrorCodes::Success);
    result.message = memochat::json::glaze_safe_get<std::string>(root, "message", "");
    result.data = root;
    return result;
}

LoginResult DispatchAuthRoute(std::string_view route_path, const memochat::json::JsonValue& req, AuthHandler handler)
{
    const GateRequest request = BuildAuthRequest(route_path, req);
    GateResponse response;
    if (!(AuthService::Instance().*handler)(request, response))
    {
        LoginResult result;
        result.error = ErrorCodes::RPCFailed;
        result.message = "auth route not handled";
        return result;
    }

    return FromGateResponse(response);
}

} // namespace

bool ParseJsonBody(std::string_view body_sv, memochat::json::JsonValue& root)
{
    std::string body_str(body_sv);
    return memochat::json::glaze_parse(root, body_str) && memochat::json::glaze_is_object(root);
}

memochat::json::JsonValue MakeError(int error_code, const std::string& message)
{
    memochat::json::JsonValue resp;
    resp["error"] = error_code;
    if (!message.empty())
    {
        resp["message"] = message;
    }
    return resp;
}

memochat::json::JsonValue MakeOk(const memochat::json::JsonValue& data)
{
    memochat::json::JsonValue resp;
    resp["error"] = 0;
    if (memochat::json::glaze_is_object(data) || memochat::json::glaze_is_array(data))
    {
        for (const auto& key : memochat::json::getMemberNames(data))
        {
            resp[key] = data[key];
        }
    }
    return resp;
}

LoginResult HandleGetVarifyCode(const std::string& email)
{
    memochat::json::JsonValue req(memochat::json::object_t{});
    req["email"] = email;
    return DispatchAuthRoute("/get_varifycode", req, &AuthService::HandleGetVarifyCode);
}

LoginResult HandleUserRegister(const memochat::json::JsonValue& req)
{
    return DispatchAuthRoute("/user_register", req, &AuthService::HandleUserRegister);
}

LoginResult HandleResetPwd(const memochat::json::JsonValue& req)
{
    return DispatchAuthRoute("/reset_pwd", req, &AuthService::HandleResetPwd);
}

LoginResult HandleUserLogin(const memochat::json::JsonValue& req)
{
    return DispatchAuthRoute("/user_login", req, &AuthService::HandleUserLogin);
}

LoginResult HandleUserLogout(int uid, const std::string& token)
{
    LoginResult result;
    if (uid <= 0 || token.empty())
    {
        result.error = ErrorCodes::TokenInvalid;
        result.message = "invalid token";
        return result;
    }
    std::string stored_token;
    if (AuthCache::Instance().GetHttpToken(uid, stored_token) && stored_token == token)
    {
        AuthCache::Instance().DeleteHttpToken(uid);
    }
    result.error = ErrorCodes::Success;
    result.message = "logout success";
    memolog::LogInfo("http2.user_logout", "user logout via HTTP2 handler", {{"uid", std::to_string(uid)}});
    return result;
}

} // namespace Http2AuthSupport
