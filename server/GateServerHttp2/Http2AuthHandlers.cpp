#include "WinCompat.h"
#include "Http2AuthHandlers.h"
#include "Http2AuthSupport.h"
#include "../GateServerCore/const.h"

void Http2AuthHandlers::HandleGetVarifyCode(const Http2Request& req, Http2Response& resp)
{
    Json::Value root;
    if (!Http2AuthSupport::ParseJsonBody(req.body, root)) {
        resp.SetJsonBody(Http2AuthSupport::MakeError(ErrorCodes::Error_Json, "invalid json"));
        return;
    }
    if (!root.isMember("email")) {
        resp.SetJsonBody(Http2AuthSupport::MakeError(ErrorCodes::Error_Json, "email is required"));
        return;
    }
    auto result = Http2AuthSupport::HandleGetVarifyCode(root["email"].asString());
    Json::Value out = Http2AuthSupport::MakeError(result.error, result.message);
    if (result.data.isObject()) {
        for (const auto& key : result.data.getMemberNames()) {
            out[key] = result.data[key];
        }
    }
    resp.SetJsonBody(out.toStyledString());
}

void Http2AuthHandlers::HandleUserRegister(const Http2Request& req, Http2Response& resp)
{
    Json::Value root;
    if (!Http2AuthSupport::ParseJsonBody(req.body, root)) {
        resp.SetJsonBody(Http2AuthSupport::MakeError(ErrorCodes::Error_Json, "invalid json"));
        return;
    }
    auto result = Http2AuthSupport::HandleUserRegister(root);
    Json::Value out = Http2AuthSupport::MakeError(result.error, result.message);
    if (result.data.isObject()) {
        for (const auto& key : result.data.getMemberNames()) {
            out[key] = result.data[key];
        }
    }
    resp.SetJsonBody(out.toStyledString());
}

void Http2AuthHandlers::HandleResetPwd(const Http2Request& req, Http2Response& resp)
{
    Json::Value root;
    if (!Http2AuthSupport::ParseJsonBody(req.body, root)) {
        resp.SetJsonBody(Http2AuthSupport::MakeError(ErrorCodes::Error_Json, "invalid json"));
        return;
    }
    auto result = Http2AuthSupport::HandleResetPwd(root);
    Json::Value out = Http2AuthSupport::MakeError(result.error, result.message);
    if (result.data.isObject()) {
        for (const auto& key : result.data.getMemberNames()) {
            out[key] = result.data[key];
        }
    }
    resp.SetJsonBody(out.toStyledString());
}

void Http2AuthHandlers::HandleUserLogin(const Http2Request& req, Http2Response& resp)
{
    Json::Value root;
    if (!Http2AuthSupport::ParseJsonBody(req.body, root)) {
        resp.SetJsonBody(Http2AuthSupport::MakeError(ErrorCodes::Error_Json, "invalid json"));
        return;
    }
    auto result = Http2AuthSupport::HandleUserLogin(root);
    Json::Value out = Http2AuthSupport::MakeError(result.error, result.message);
    if (result.data.isObject()) {
        for (const auto& key : result.data.getMemberNames()) {
            out[key] = result.data[key];
        }
    }
    resp.SetJsonBody(out.toStyledString());
}

void Http2AuthHandlers::HandleUserLogout(const Http2Request& req, Http2Response& resp)
{
    Json::Value root;
    int uid = 0;
    std::string token;
    if (Http2AuthSupport::ParseJsonBody(req.body, root)) {
        uid = root.get("uid", 0).asInt();
        token = root.get("token", "").asString();
    }
    auto result = Http2AuthSupport::HandleUserLogout(uid, token);
    Json::Value out = Http2AuthSupport::MakeError(result.error, result.message);
    resp.SetJsonBody(out.toStyledString());
}
