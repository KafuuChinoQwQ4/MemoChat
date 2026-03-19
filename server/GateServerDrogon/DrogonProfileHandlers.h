#pragma once

#include <drogon/HttpController.h>
#include <drogon/HttpResponse.h>
#include <json/json.h>

using namespace drogon;

class DrogonProfileHandlers : public drogon::HttpController<DrogonProfileHandlers>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(DrogonProfileHandlers::HandleUserUpdateProfile, "/user_update_profile", Post);
    METHOD_ADD(DrogonProfileHandlers::HandleGetUserInfo, "/get_user_info", Post);
    METHOD_LIST_END

    void HandleUserUpdateProfile(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void HandleGetUserInfo(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);

  private:
    Json::Value CreateErrorResponse(int error_code);
    Json::Value ParseJsonBody(const std::string& body_str);
};
