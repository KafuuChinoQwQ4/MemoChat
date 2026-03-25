#pragma once

#include <json/json.h>
#include <string>
#include <string_view>

namespace Http2CallSupport {

struct CallResult {
    int error = 0;
    std::string message;
    Json::Value data;
};

bool ParseJsonBody(std::string_view body_sv, Json::Value& root);

CallResult HandleCallStart(const Json::Value& req, const std::string& trace_id);
CallResult HandleCallAccept(const Json::Value& req, const std::string& trace_id);
CallResult HandleCallReject(const Json::Value& req, const std::string& trace_id);
CallResult HandleCallCancel(const Json::Value& req, const std::string& trace_id);
CallResult HandleCallHangup(const Json::Value& req, const std::string& trace_id);
CallResult HandleCallTokenGet(int uid, const std::string& token,
                               const std::string& call_id, const std::string& role,
                               const std::string& trace_id);
CallResult HandleCallTokenPost(const Json::Value& req, const std::string& trace_id);

}  // namespace Http2CallSupport
