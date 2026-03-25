#pragma once

#include <json/json.h>
#include <string>
#include <string_view>

namespace Http2ProfileSupport {

struct ProfileResult {
    int error = 0;
    std::string message;
    Json::Value data;
};

bool ParseJsonBody(std::string_view body_sv, Json::Value& root);
Json::Value MakeError(int error_code, const std::string& message);

ProfileResult HandleUserUpdateProfile(const Json::Value& req);
ProfileResult HandleGetUserInfo(int uid);

}  // namespace Http2ProfileSupport
