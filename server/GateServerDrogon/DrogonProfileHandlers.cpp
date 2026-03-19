#include "DrogonProfileHandlers.h"
#include <iostream>
#include <json/json.h>

using namespace drogon;

Json::Value DrogonProfileHandlers::CreateErrorResponse(int error_code)
{
    Json::Value resp;
    resp["error"] = error_code;
    return resp;
}

Json::Value DrogonProfileHandlers::ParseJsonBody(const std::string& body_str)
{
    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(body_str, root)) {
        return CreateErrorResponse(1);
    }
    return root;
}

void DrogonProfileHandlers::HandleUserUpdateProfile(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    std::cout << "HandleUserUpdateProfile: " << body_str << std::endl;
    
    Json::Value resp;
    resp["error"] = 0;
    resp["message"] = "profile updated";
    
    auto response = HttpResponse::newHttpJsonResponse(resp);
    callback(response);
}

void DrogonProfileHandlers::HandleGetUserInfo(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    std::cout << "HandleGetUserInfo: " << body_str << std::endl;
    
    Json::Value resp;
    resp["error"] = 0;
    resp["username"] = "stub_user";
    resp["email"] = "stub@example.com";
    
    auto response = HttpResponse::newHttpJsonResponse(resp);
    callback(response);
}
