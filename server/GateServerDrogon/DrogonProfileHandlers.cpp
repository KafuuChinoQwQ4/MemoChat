#include "WinCompat.h"
#include "DrogonProfileHandlers.h"
#include "DrogonProfileSupport.h"
#include "../GateServerCore/const.h"

using namespace drogon;

void DrogonProfileHandlers::HandleUserUpdateProfile(const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    Json::Value root;
    if (!DrogonProfileSupport::ParseJsonBody(body_str, root)) {
        auto resp = HttpResponse::newHttpJsonResponse(
            DrogonProfileSupport::MakeError(1, "invalid json"));
        callback(resp);
        return;
    }
    auto result = DrogonProfileSupport::HandleUserUpdateProfile(root);
    Json::Value out = DrogonProfileSupport::MakeError(result.error, result.message);
    if (result.data.isObject()) {
        for (const auto& key : result.data.getMemberNames()) {
            out[key] = result.data[key];
        }
    }
    auto resp = HttpResponse::newHttpJsonResponse(out);
    callback(resp);
}

void DrogonProfileHandlers::HandleGetUserInfo(const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    Json::Value root;
    int uid = 0;
    if (DrogonProfileSupport::ParseJsonBody(body_str, root)) {
        uid = root.get("uid", 0).asInt();
    }
    if (uid <= 0) {
        auto resp = HttpResponse::newHttpJsonResponse(
            DrogonProfileSupport::MakeError(1, "invalid uid"));
        callback(resp);
        return;
    }
    auto result = DrogonProfileSupport::HandleGetUserInfo(uid);
    Json::Value out = DrogonProfileSupport::MakeError(result.error, result.message);
    if (result.data.isObject()) {
        for (const auto& key : result.data.getMemberNames()) {
            out[key] = result.data[key];
        }
    }
    auto resp = HttpResponse::newHttpJsonResponse(out);
    callback(resp);
}
