#include "WinCompat.h"
#include "DrogonAuthHandlers.h"
#include "DrogonAuthSupport.h"
#include "../GateServerCore/const.h"

using namespace drogon;

void DrogonAuthHandlers::HandleGetVarifyCode(const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    Json::Value root;
    if (!DrogonAuthSupport::ParseJsonBody(body_str, root)) {
        auto resp = HttpResponse::newHttpJsonResponse(
            DrogonAuthSupport::MakeError(ErrorCodes::Error_Json, "invalid json"));
        callback(resp);
        return;
    }
    if (!root.isMember("email")) {
        auto resp = HttpResponse::newHttpJsonResponse(
            DrogonAuthSupport::MakeError(ErrorCodes::Error_Json, "email is required"));
        callback(resp);
        return;
    }
    auto result = DrogonAuthSupport::HandleGetVarifyCode(root["email"].asString());
    Json::Value out = DrogonAuthSupport::MakeError(result.error, result.message);
    if (result.data.isObject()) {
        for (const auto& key : result.data.getMemberNames()) {
            out[key] = result.data[key];
        }
    }
    auto resp = HttpResponse::newHttpJsonResponse(out);
    callback(resp);
}

void DrogonAuthHandlers::HandleUserRegister(const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    Json::Value root;
    if (!DrogonAuthSupport::ParseJsonBody(body_str, root)) {
        auto resp = HttpResponse::newHttpJsonResponse(
            DrogonAuthSupport::MakeError(ErrorCodes::Error_Json, "invalid json"));
        callback(resp);
        return;
    }
    auto result = DrogonAuthSupport::HandleUserRegister(root);
    Json::Value out = DrogonAuthSupport::MakeError(result.error, result.message);
    if (result.data.isObject()) {
        for (const auto& key : result.data.getMemberNames()) {
            out[key] = result.data[key];
        }
    }
    auto resp = HttpResponse::newHttpJsonResponse(out);
    callback(resp);
}

void DrogonAuthHandlers::HandleResetPwd(const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    Json::Value root;
    if (!DrogonAuthSupport::ParseJsonBody(body_str, root)) {
        auto resp = HttpResponse::newHttpJsonResponse(
            DrogonAuthSupport::MakeError(ErrorCodes::Error_Json, "invalid json"));
        callback(resp);
        return;
    }
    auto result = DrogonAuthSupport::HandleResetPwd(root);
    Json::Value out = DrogonAuthSupport::MakeError(result.error, result.message);
    if (result.data.isObject()) {
        for (const auto& key : result.data.getMemberNames()) {
            out[key] = result.data[key];
        }
    }
    auto resp = HttpResponse::newHttpJsonResponse(out);
    callback(resp);
}

void DrogonAuthHandlers::HandleUserLogin(const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    Json::Value root;
    if (!DrogonAuthSupport::ParseJsonBody(body_str, root)) {
        auto resp = HttpResponse::newHttpJsonResponse(
            DrogonAuthSupport::MakeError(ErrorCodes::Error_Json, "invalid json"));
        callback(resp);
        return;
    }
    auto result = DrogonAuthSupport::HandleUserLogin(root);
    Json::Value out = DrogonAuthSupport::MakeError(result.error, result.message);
    if (result.data.isObject()) {
        for (const auto& key : result.data.getMemberNames()) {
            out[key] = result.data[key];
        }
    }
    auto resp = HttpResponse::newHttpJsonResponse(out);
    callback(resp);
}

void DrogonAuthHandlers::HandleUserLogout(const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    Json::Value root;
    int uid = 0;
    std::string token;
    if (DrogonAuthSupport::ParseJsonBody(body_str, root)) {
        uid = root.get("uid", 0).asInt();
        token = root.get("token", "").asString();
    }
    auto result = DrogonAuthSupport::HandleUserLogout(uid, token);
    Json::Value out = DrogonAuthSupport::MakeError(result.error, result.message);
    auto resp = HttpResponse::newHttpJsonResponse(out);
    callback(resp);
}
