#pragma once

#include <drogon/HttpController.h>
#include <drogon/HttpResponse.h>
#include <json/json.h>

using namespace drogon;

class DrogonMediaHandlers : public drogon::HttpController<DrogonMediaHandlers>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(DrogonMediaHandlers::HandleUploadMediaInit, "/upload_media_init", Post);
    METHOD_ADD(DrogonMediaHandlers::HandleUploadMediaChunk, "/upload_media_chunk", Post);
    METHOD_ADD(DrogonMediaHandlers::HandleUploadMediaStatus, "/upload_media_status", Get);
    METHOD_ADD(DrogonMediaHandlers::HandleUploadMediaComplete, "/upload_media_complete", Post);
    METHOD_ADD(DrogonMediaHandlers::HandleUploadMedia, "/upload_media", Post);
    METHOD_ADD(DrogonMediaHandlers::HandleMediaDownload, "/media/download", Get);
    METHOD_LIST_END

    void HandleUploadMediaInit(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void HandleUploadMediaChunk(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void HandleUploadMediaStatus(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void HandleUploadMediaComplete(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void HandleUploadMedia(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void HandleMediaDownload(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
};
