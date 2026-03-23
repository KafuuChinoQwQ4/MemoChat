#pragma once

#include <drogon/HttpResponse.h>
#include <json/json.h>
#include <string>
#include <string_view>

namespace DrogonAuthSupport {

struct LoginResult {
    int error = 0;
    std::string message;
    Json::Value data;
};

bool ParseJsonBody(std::string_view body_sv, Json::Value& root);
Json::Value MakeError(int error_code, const std::string& message);
Json::Value MakeOk(const Json::Value& data);

LoginResult HandleGetVarifyCode(const std::string& email);
LoginResult HandleUserRegister(const Json::Value& req);
LoginResult HandleResetPwd(const Json::Value& req);
LoginResult HandleUserLogin(const Json::Value& req);
LoginResult HandleUserLogout(int uid, const std::string& token);

}  // namespace DrogonAuthSupport
