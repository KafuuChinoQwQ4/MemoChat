#include "DrogonAuthHandlers.h"
#include <iostream>
#include <json/json.h>

using namespace drogon;

void DrogonAuthHandlers::HandleGetVarifyCode(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    std::cout << "HandleGetVarifyCode: " << body_str << std::endl;
    
    Json::Value resp;
    resp["error"] = 0;
    resp["message"] = "verification code sent";
    
    auto response = HttpResponse::newHttpJsonResponse(resp);
    callback(response);
}

void DrogonAuthHandlers::HandleUserRegister(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    std::cout << "HandleUserRegister: " << body_str << std::endl;
    
    Json::Value resp;
    resp["error"] = 0;
    resp["message"] = "user registered";
    
    auto response = HttpResponse::newHttpJsonResponse(resp);
    callback(response);
}

void DrogonAuthHandlers::HandleResetPwd(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    std::cout << "HandleResetPwd: " << body_str << std::endl;
    
    Json::Value resp;
    resp["error"] = 0;
    resp["message"] = "password reset";
    
    auto response = HttpResponse::newHttpJsonResponse(resp);
    callback(response);
}

void DrogonAuthHandlers::HandleUserLogin(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    std::cout << "HandleUserLogin: " << body_str << std::endl;
    
    Json::Value resp;
    resp["error"] = 0;
    resp["message"] = "login success";
    resp["token"] = "stub_token";
    
    auto response = HttpResponse::newHttpJsonResponse(resp);
    callback(response);
}

void DrogonAuthHandlers::HandleUserLogout(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    std::cout << "HandleUserLogout: " << body_str << std::endl;
    
    Json::Value resp;
    resp["error"] = 0;
    resp["message"] = "logout success";
    
    auto response = HttpResponse::newHttpJsonResponse(resp);
    callback(response);
}
