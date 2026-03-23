#include "WinCompat.h"
#include "DrogonHttpHandlers.h"
#include "../GateServerCore/PostgresMgr.h"
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <iostream>
#include <json/json.h>

using namespace drogon;

void DrogonHttpHandlers::HandleGetTest(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto resp = HttpResponse::newHttpResponse();
    resp->setContentTypeCode(CT_TEXT_PLAIN);
    resp->setBody("receive get_test req");
    callback(resp);
}

void DrogonHttpHandlers::HandleTestProcedure(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    std::cout << "receive body is " << body_str << std::endl;

    Json::Value root;
    Json::Reader reader;
    Json::Value src_root;
    
    // Use std::string constructor to convert string_view to string for Json::Reader
    bool parse_success = reader.parse(std::string(body_str), src_root);
    if (!parse_success) {
        std::cout << "Failed to parse JSON data!" << std::endl;
        root["error"] = 1; // Error_Json
        auto resp = HttpResponse::newHttpJsonResponse(root.toStyledString());
        callback(resp);
        return;
    }

    if (!src_root.isMember("email")) {
        root["error"] = 1;
        auto resp = HttpResponse::newHttpJsonResponse(root.toStyledString());
        callback(resp);
        return;
    }

    auto email = src_root["email"].asString();
    int uid = 0;
    std::string name = "";
    // PostgresMgr::GetInstance()->TestProcedure(email, uid, name);
    
    root["error"] = 0;
    root["email"] = src_root["email"];
    root["name"] = name;
    root["uid"] = uid;
    
    auto resp = HttpResponse::newHttpJsonResponse(root.toStyledString());
    callback(resp);
}
