#include "WinCompat.h"
#include "Http2ProfileHandlers.h"
#include "../GateServerCore/Http2ProfileSupport.h"
#include "../GateServerCore/const.h"

void Http2ProfileHandlers::HandleUserUpdateProfile(const Http2Request& req, Http2Response& resp)
{
    Json::Value root;
    if (!Http2ProfileSupport::ParseJsonBody(req.body, root)) {
        resp.SetJsonBody(Http2ProfileSupport::MakeError(1, "invalid json"));
        return;
    }
    auto result = Http2ProfileSupport::HandleUserUpdateProfile(root);
    Json::Value out = Http2ProfileSupport::MakeError(result.error, result.message);
    if (result.data.isObject()) {
        for (const auto& key : result.data.getMemberNames()) {
            out[key] = result.data[key];
        }
    }
    resp.SetJsonBody(out.toStyledString());
}

void Http2ProfileHandlers::HandleGetUserInfo(const Http2Request& req, Http2Response& resp)
{
    Json::Value root;
    int uid = 0;
    if (Http2ProfileSupport::ParseJsonBody(req.body, root)) {
        uid = root.get("uid", 0).asInt();
    }
    if (uid <= 0) {
        resp.SetJsonBody(Http2ProfileSupport::MakeError(1, "invalid uid"));
        return;
    }
    auto result = Http2ProfileSupport::HandleGetUserInfo(uid);
    Json::Value out = Http2ProfileSupport::MakeError(result.error, result.message);
    if (result.data.isObject()) {
        for (const auto& key : result.data.getMemberNames()) {
            out[key] = result.data[key];
        }
    }
    resp.SetJsonBody(out.toStyledString());
}
