#pragma once

#include <drogon/HttpController.h>
#include <drogon/HttpResponse.h>
#include <json/json.h>

using namespace drogon;

class DrogonCallHandlers : public drogon::HttpController<DrogonCallHandlers>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(DrogonCallHandlers::HandleCallStart, "/api/call/start", Post);
    METHOD_ADD(DrogonCallHandlers::HandleCallAccept, "/api/call/accept", Post);
    METHOD_ADD(DrogonCallHandlers::HandleCallReject, "/api/call/reject", Post);
    METHOD_ADD(DrogonCallHandlers::HandleCallCancel, "/api/call/cancel", Post);
    METHOD_ADD(DrogonCallHandlers::HandleCallHangup, "/api/call/hangup", Post);
    METHOD_ADD(DrogonCallHandlers::HandleCallToken, "/api/call/token", Get);
    METHOD_ADD(DrogonCallHandlers::HandleCallTokenPost, "/api/call/token", Post);
    METHOD_LIST_END

    void HandleCallStart(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void HandleCallAccept(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void HandleCallReject(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void HandleCallCancel(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void HandleCallHangup(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void HandleCallToken(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void HandleCallTokenPost(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
};
