#include "WinCompat.h"
#include "Http2AuthHandlers.h"
#include "Http2AuthSupport.h"
#include "../GateServerCore/const.h"

void Http2AuthHandlers::HandleGetVarifyCode(const Http2Request& req, Http2Response& resp)
{
    memochat::json::JsonValue root;
    if (!Http2AuthSupport::ParseJsonBody(req.body, root)) {
        resp.SetJsonBody(memochat::json::glaze_stringify(Http2AuthSupport::MakeError(ErrorCodes::Error_Json, "invalid json")));
        return;
    }
    if (!memochat::json::glaze_has_key(root, "email")) {
        resp.SetJsonBody(memochat::json::glaze_stringify(Http2AuthSupport::MakeError(ErrorCodes::Error_Json, "email is required")));
        return;
    }
    auto result = Http2AuthSupport::HandleGetVarifyCode(memochat::json::glaze_safe_get<std::string>(root, "email", ""));
    memochat::json::JsonValue out = Http2AuthSupport::MakeError(result.error, result.message);
    if (memochat::json::glaze_is_object(result.data)) {
        for (const auto& key : memochat::json::getMemberNames(result.data)) {
            out[key] = memochat::json::glaze_get(result.data, key);
        }
    }
    resp.SetJsonBody(memochat::json::glaze_stringify(out));
}

void Http2AuthHandlers::HandleUserRegister(const Http2Request& req, Http2Response& resp)
{
    memochat::json::JsonValue root;
    if (!Http2AuthSupport::ParseJsonBody(req.body, root)) {
        resp.SetJsonBody(memochat::json::glaze_stringify(Http2AuthSupport::MakeError(ErrorCodes::Error_Json, "invalid json")));
        return;
    }
    auto result = Http2AuthSupport::HandleUserRegister(root);
    memochat::json::JsonValue out = Http2AuthSupport::MakeError(result.error, result.message);
    if (memochat::json::glaze_is_object(result.data)) {
        for (const auto& key : memochat::json::getMemberNames(result.data)) {
            out[key] = memochat::json::glaze_get(result.data, key);
        }
    }
    resp.SetJsonBody(memochat::json::glaze_stringify(out));
}

void Http2AuthHandlers::HandleResetPwd(const Http2Request& req, Http2Response& resp)
{
    memochat::json::JsonValue root;
    if (!Http2AuthSupport::ParseJsonBody(req.body, root)) {
        resp.SetJsonBody(memochat::json::glaze_stringify(Http2AuthSupport::MakeError(ErrorCodes::Error_Json, "invalid json")));
        return;
    }
    auto result = Http2AuthSupport::HandleResetPwd(root);
    memochat::json::JsonValue out = Http2AuthSupport::MakeError(result.error, result.message);
    if (memochat::json::glaze_is_object(result.data)) {
        for (const auto& key : memochat::json::getMemberNames(result.data)) {
            out[key] = memochat::json::glaze_get(result.data, key);
        }
    }
    resp.SetJsonBody(memochat::json::glaze_stringify(out));
}

void Http2AuthHandlers::HandleUserLogin(const Http2Request& req, Http2Response& resp)
{
    memochat::json::JsonValue root;
    if (!Http2AuthSupport::ParseJsonBody(req.body, root)) {
        resp.SetJsonBody(memochat::json::glaze_stringify(Http2AuthSupport::MakeError(ErrorCodes::Error_Json, "invalid json")));
        return;
    }
    auto result = Http2AuthSupport::HandleUserLogin(root);
    memochat::json::JsonValue out = Http2AuthSupport::MakeError(result.error, result.message);
    if (memochat::json::glaze_is_object(result.data)) {
        for (const auto& key : memochat::json::getMemberNames(result.data)) {
            out[key] = memochat::json::glaze_get(result.data, key);
        }
    }
    resp.SetJsonBody(memochat::json::glaze_stringify(out));
}

void Http2AuthHandlers::HandleUserLogout(const Http2Request& req, Http2Response& resp)
{
    memochat::json::JsonValue root;
    int uid = 0;
    std::string token;
    if (Http2AuthSupport::ParseJsonBody(req.body, root)) {
        uid = memochat::json::glaze_safe_get<int>(root, "uid", 0);
        token = memochat::json::glaze_safe_get<std::string>(root, "token", "");
    }
    auto result = Http2AuthSupport::HandleUserLogout(uid, token);
    memochat::json::JsonValue out = Http2AuthSupport::MakeError(result.error, result.message);
    resp.SetJsonBody(memochat::json::glaze_stringify(out));
}
