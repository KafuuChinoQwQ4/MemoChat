#include "GateHttp3JsonSupport.h"
#include "GateHttp3Connection.h"
#include "json/GlazeCompat.h"
#include <sstream>

bool GateHttp3JsonSupport::ParseJsonBody(std::shared_ptr<GateHttp3Connection> connection,
                                         memochat::json::JsonValue& root, memochat::json::JsonValue& src_root) {
    if (!connection) {
        root["error"] = 1;
        root["message"] = "null connection";
        return false;
    }

    const std::string body = connection->GetRequestBody();
    if (body.empty()) {
        root["error"] = 1;
        root["message"] = "empty request body";
        return false;
    }

    std::string parse_error;
    if (!memochat::json::glaze_parse(src_root, body, &parse_error)) {
        root["error"] = 1;
        root["message"] = "json parse error: " + parse_error;
        return false;
    }
    return true;
}

bool GateHttp3JsonSupport::HandleJsonPost(
    std::shared_ptr<GateHttp3Connection> connection,
    std::function<bool(const memochat::json::JsonValue&, memochat::json::JsonValue&, const std::string&)> fn) {

    memochat::json::JsonValue root;
    memochat::json::JsonValue src_root;

    root["trace_id"] = connection->GetTraceId();
    root["request_id"] = connection->GetRequestId();

    if (!ParseJsonBody(connection, root, src_root)) {
        std::string resp = memochat::json::writeString(root);
        connection->SendResponse(400, resp, "application/json");
        return true;
    }

    std::string trace_id = connection->GetTraceId();
    try {
        if (!fn(src_root, root, trace_id)) {
            if (!root.isMember("error") || memochat::json::glaze_safe_get<int>(root, "error", 0) == 0) {
                root["error"] = 1;
            }
            std::string resp = memochat::json::writeString(root);
            connection->SendResponse(400, resp, "application/json");
            return true;
        }
        root["error"] = 0;
        std::string resp = memochat::json::writeString(root);
        connection->SendResponse(200, resp, "application/json");
        return true;
    } catch (const std::exception& e) {
        root = memochat::json::glaze_empty_object();
        root["trace_id"] = connection->GetTraceId();
        root["request_id"] = connection->GetRequestId();
        root["error"] = 1;
        root["message"] = e.what();
        std::string resp = memochat::json::writeString(root);
        connection->SendResponse(500, resp, "application/json");
        return true;
    }
}

