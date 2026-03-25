#include "WinCompat.h"
#include "Http2Handlers.h"
#include "../GateServerCore/PostgresMgr.h"
#include <iostream>
#include <json/json.h>

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

    Json::Value root;
    Json::Reader reader;
    Json::Value src_root;

    bool parse_success = reader.parse(req.body, src_root);
    if (!parse_success) {
        std::cout << "Failed to parse JSON data!" << std::endl;
        root["error"] = 1;
        resp.SetJsonBody(root.toStyledString());
        return;
    }

    if (!src_root.isMember("email")) {
        root["error"] = 1;
        resp.SetJsonBody(root.toStyledString());
        return;
    }

    auto email = src_root["email"].asString();
    int uid = 0;
    std::string name = "";

    root["error"] = 0;
    root["email"] = src_root["email"];
    root["name"] = name;
    root["uid"] = uid;

    resp.SetJsonBody(root.toStyledString());
}
