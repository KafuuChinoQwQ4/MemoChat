#include "DrogonCallHandlers.h"
#include <iostream>
#include <json/json.h>

using namespace drogon;

Json::Value DrogonCallHandlers::ParseJsonBody(const std::string& body_str)
{
    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(body_str, root)) {
        Json::Value err;
        err["error"] = 1;
        return err;
    }
    return root;
}

void DrogonCallHandlers::HandleCallStart(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    std::cout << "HandleCallStart: " << body_str << std::endl;
    
    Json::Value resp;
    resp["error"] = 0;
    resp["message"] = "call started";
    
    auto response = HttpResponse::newHttpJsonResponse(resp);
    callback(response);
}

void DrogonCallHandlers::HandleCallAccept(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    std::cout << "HandleCallAccept: " << body_str << std::endl;
    
    Json::Value resp;
    resp["error"] = 0;
    resp["message"] = "call accepted";
    
    auto response = HttpResponse::newHttpJsonResponse(resp);
    callback(response);
}

void DrogonCallHandlers::HandleCallReject(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    std::cout << "HandleCallReject: " << body_str << std::endl;
    
    Json::Value resp;
    resp["error"] = 0;
    resp["message"] = "call rejected";
    
    auto response = HttpResponse::newHttpJsonResponse(resp);
    callback(response);
}

void DrogonCallHandlers::HandleCallCancel(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    std::cout << "HandleCallCancel: " << body_str << std::endl;
    
    Json::Value resp;
    resp["error"] = 0;
    resp["message"] = "call cancelled";
    
    auto response = HttpResponse::newHttpJsonResponse(resp);
    callback(response);
}

void DrogonCallHandlers::HandleCallHangup(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    std::cout << "HandleCallHangup: " << body_str << std::endl;
    
    Json::Value resp;
    resp["error"] = 0;
    resp["message"] = "call hung up";
    
    auto response = HttpResponse::newHttpJsonResponse(resp);
    callback(response);
}

void DrogonCallHandlers::HandleCallToken(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    std::cout << "HandleCallToken: " << body_str << std::endl;
    
    Json::Value resp;
    resp["error"] = 0;
    resp["token"] = "stub_call_token";
    
    auto response = HttpResponse::newHttpJsonResponse(resp);
    callback(response);
}

void DrogonCallHandlers::HandleCallTokenPost(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    std::cout << "HandleCallTokenPost: " << body_str << std::endl;
    
    Json::Value resp;
    resp["error"] = 0;
    resp["token"] = "stub_call_token";
    
    auto response = HttpResponse::newHttpJsonResponse(resp);
    callback(response);
}
