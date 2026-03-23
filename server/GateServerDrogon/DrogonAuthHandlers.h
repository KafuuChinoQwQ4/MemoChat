#pragma once

#include <drogon/HttpController.h>
#include <drogon/HttpResponse.h>

using namespace drogon;

class DrogonAuthHandlers : public drogon::HttpController<DrogonAuthHandlers>
{
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(DrogonAuthHandlers::HandleGetVarifyCode, "/get_varifycode", Post);
    METHOD_ADD(DrogonAuthHandlers::HandleUserRegister, "/user_register", Post);
    METHOD_ADD(DrogonAuthHandlers::HandleResetPwd, "/reset_pwd", Post);
    METHOD_ADD(DrogonAuthHandlers::HandleUserLogin, "/user_login", Post);
    METHOD_ADD(DrogonAuthHandlers::HandleUserLogout, "/user_logout", Post);
    METHOD_LIST_END

    void HandleGetVarifyCode(const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback);
    void HandleUserRegister(const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback);
    void HandleResetPwd(const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback);
    void HandleUserLogin(const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback);
    void HandleUserLogout(const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback);
};
