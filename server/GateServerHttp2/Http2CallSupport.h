#pragma once

#include <string>
#include <string_view>
#include "json/GlazeCompat.h"

namespace Http2CallSupport {

struct CallResult {
    int error = 0;
    std::string message;
    memochat::json::JsonValue data;
};

bool ParseJsonBody(std::string_view body_sv, memochat::json::JsonValue& root);
CallResult HandleCallStart(const memochat::json::JsonValue& req, const std::string& trace_id);
CallResult HandleCallAccept(const memochat::json::JsonValue& req, const std::string& trace_id);
CallResult HandleCallReject(const memochat::json::JsonValue& req, const std::string& trace_id);
CallResult HandleCallCancel(const memochat::json::JsonValue& req, const std::string& trace_id);
CallResult HandleCallHangup(const memochat::json::JsonValue& req, const std::string& trace_id);
CallResult HandleCallTokenGet(int uid, const std::string& token, const std::string& call_id, const std::string& role, const std::string& trace_id);
CallResult HandleCallTokenPost(const memochat::json::JsonValue& req, const std::string& trace_id);

} // namespace Http2CallSupport
