#pragma once

#include <string>
#include <string_view>
#include "json/GlazeCompat.h"

namespace Http2AuthSupport {

struct LoginResult {
    int error = 0;
    std::string message;
    memochat::json::JsonValue data;
};

bool ParseJsonBody(std::string_view body_sv, memochat::json::JsonValue& root);
memochat::json::JsonValue MakeError(int error_code, const std::string& message);
memochat::json::JsonValue MakeOk(const memochat::json::JsonValue& data);
LoginResult HandleGetVarifyCode(const std::string& email);
LoginResult HandleUserRegister(const memochat::json::JsonValue& req);
LoginResult HandleResetPwd(const memochat::json::JsonValue& req);
LoginResult HandleUserLogin(const memochat::json::JsonValue& req);
LoginResult HandleUserLogout(int uid, const std::string& token);

} // namespace Http2AuthSupport
