#include "DrogonMediaHandlers.h"
#include <iostream>
#include <json/json.h>

using namespace drogon;

Json::Value DrogonMediaHandlers::CreateErrorResponse(int error_code)
{
    Json::Value resp;
    resp["error"] = error_code;
    return resp;
}

void DrogonMediaHandlers::HandleUploadMediaInit(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    std::cout << "HandleUploadMediaInit: " << body_str << std::endl;
    
    Json::Value resp;
    resp["error"] = 0;
    resp["message"] = "upload initialized";
    resp["upload_id"] = "stub_upload_id";
    
    auto response = HttpResponse::newHttpJsonResponse(resp);
    callback(response);
}

void DrogonMediaHandlers::HandleUploadMediaChunk(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    std::cout << "HandleUploadMediaChunk: " << body_str << std::endl;
    
    Json::Value resp;
    resp["error"] = 0;
    resp["message"] = "chunk received";
    
    auto response = HttpResponse::newHttpJsonResponse(resp);
    callback(response);
}

void DrogonMediaHandlers::HandleUploadMediaStatus(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    std::cout << "HandleUploadMediaStatus: " << body_str << std::endl;
    
    Json::Value resp;
    resp["error"] = 0;
    resp["status"] = "in_progress";
    
    auto response = HttpResponse::newHttpJsonResponse(resp);
    callback(response);
}

void DrogonMediaHandlers::HandleUploadMediaComplete(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    std::cout << "HandleUploadMediaComplete: " << body_str << std::endl;
    
    Json::Value resp;
    resp["error"] = 0;
    resp["message"] = "upload complete";
    resp["media_id"] = "stub_media_id";
    
    auto response = HttpResponse::newHttpJsonResponse(resp);
    callback(response);
}

void DrogonMediaHandlers::HandleUploadMedia(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    std::cout << "HandleUploadMedia: " << body_str << std::endl;
    
    Json::Value resp;
    resp["error"] = 0;
    resp["message"] = "media uploaded";
    resp["media_id"] = "stub_media_id";
    
    auto response = HttpResponse::newHttpJsonResponse(resp);
    callback(response);
}

void DrogonMediaHandlers::HandleMediaDownload(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto body_str = req->getBody();
    std::cout << "HandleMediaDownload: " << body_str << std::endl;
    
    Json::Value resp;
    resp["error"] = 0;
    resp["url"] = "stub_download_url";
    
    auto response = HttpResponse::newHttpJsonResponse(resp);
    callback(response);
}
