#include "WinCompat.h"
#include "json/GlazeCompat.h"
#include "Http2ProfileHandlers.h"
#include "../GateServerCore/Http2ProfileSupport.h"
#include "../GateServerCore/const.h"

void Http2ProfileHandlers::HandleUserUpdateProfile(const Http2Request& req, Http2Response& resp)
{
    memochat::json::JsonValue root;
    if (!Http2ProfileSupport::ParseJsonBody(req.body, root)) {
        resp.SetJsonBody(memochat::json::glaze_stringify(Http2ProfileSupport::MakeError(1, "invalid json")));
        return;
    }
    auto result = Http2ProfileSupport::HandleUserUpdateProfile(root);
    memochat::json::JsonValue out = Http2ProfileSupport::MakeError(result.error, result.message);
    if (memochat::json::glaze_is_object(result.data)) {
        for (const auto& key : memochat::json::getMemberNames(result.data)) {
            out[key] = memochat::json::glaze_get(result.data, key);
        }
    }
    resp.SetJsonBody(memochat::json::glaze_stringify(out));
}

void Http2ProfileHandlers::HandleGetUserInfo(const Http2Request& req, Http2Response& resp)
{
    memochat::json::JsonValue root;
    int uid = 0;
    if (Http2ProfileSupport::ParseJsonBody(req.body, root)) {
        uid = memochat::json::glaze_safe_get<int>(root, "uid", 0);
    }
    if (uid <= 0) {
        resp.SetJsonBody(memochat::json::glaze_stringify(Http2ProfileSupport::MakeError(1, "invalid uid")));
        return;
    }
    auto result = Http2ProfileSupport::HandleGetUserInfo(uid);
    memochat::json::JsonValue out = Http2ProfileSupport::MakeError(result.error, result.message);
    if (memochat::json::glaze_is_object(result.data)) {
        for (const auto& key : memochat::json::getMemberNames(result.data)) {
            out[key] = memochat::json::glaze_get(result.data, key);
        }
    }
    resp.SetJsonBody(memochat::json::glaze_stringify(out));
}

