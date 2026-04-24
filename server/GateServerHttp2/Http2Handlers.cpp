#include "WinCompat.h"
#include "Http2Handlers.h"
#include "json/GlazeCompat.h"
#include "../GateServerCore/PostgresMgr.h"
#include <iostream>

void Http2Handlers::HandleGetTest(const Http2Request& req, Http2Response& resp)
{
    (void)req;
    resp.SetStatus(200, "OK");
    resp.SetHeader("Content-Type", "text/plain");
    resp.body = "receive get_test req";
}

void Http2Handlers::HandleTestProcedure(const Http2Request& req, Http2Response& resp)
{
    std::cout << "receive body is " << req.body << std::endl;
    memochat::json::JsonValue root;
    memochat::json::JsonValue src_root;
    // reader_parse(json_str, JsonValue_out) - correct argument order
    bool parse_success = memochat::json::reader_parse(req.body, src_root);
    if (!parse_success) {
        std::cout << "Failed to parse JSON data!" << std::endl;
        root["error"] = 1;
        resp.SetJsonBody(memochat::json::glaze_stringify(root));
        return;
    }
    if (!memochat::json::glaze_has_key(src_root, "email")) {
        root["error"] = 1;
        resp.SetJsonBody(memochat::json::glaze_stringify(root));
        return;
    }
    auto email = memochat::json::glaze_safe_get<std::string>(src_root, "email", "");
    int uid = 0;
    std::string name = "";
    root["error"] = 0;
    root["email"] = memochat::json::glaze_safe_get<std::string>(src_root, "email", "");
    root["name"] = name;
    root["uid"] = uid;
    resp.SetJsonBody(memochat::json::glaze_stringify(root));
}
