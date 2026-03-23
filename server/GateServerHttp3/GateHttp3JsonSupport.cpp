#include "GateHttp3JsonSupport.h"
#include "GateHttp3Connection.h"
#include <json/json.h>
#include <sstream>

bool GateHttp3JsonSupport::ParseJsonBody(std::shared_ptr<GateHttp3Connection> connection,
                                         Json::Value& root, Json::Value& src_root) {
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

    Json::Reader reader;
    if (!reader.parse(body, src_root)) {
        root["error"] = 1;
        root["message"] = "json parse error: " + reader.getFormattedErrorMessages();
        return false;
    }
    return true;
}

bool GateHttp3JsonSupport::HandleJsonPost(
    std::shared_ptr<GateHttp3Connection> connection,
    std::function<bool(const Json::Value&, Json::Value&, const std::string&)> fn) {

    Json::Value root;
    Json::Value src_root;

    root["trace_id"] = connection->GetTraceId();
    root["request_id"] = connection->GetRequestId();

    if (!ParseJsonBody(connection, root, src_root)) {
        std::string resp = root.toStyledString();
        connection->SendResponse(400, resp, "application/json");
        return true;
    }

    std::string trace_id = connection->GetTraceId();
    try {
        if (!fn(src_root, root, trace_id)) {
            if (!root.isMember("error") || root["error"].asInt() == 0) {
                root["error"] = 1;
            }
            std::string resp = root.toStyledString();
            connection->SendResponse(400, resp, "application/json");
            return true;
        }
        root["error"] = 0;
        std::string resp = root.toStyledString();
        connection->SendResponse(200, resp, "application/json");
        return true;
    } catch (const std::exception& e) {
        root.clear();
        root["trace_id"] = connection->GetTraceId();
        root["request_id"] = connection->GetRequestId();
        root["error"] = 1;
        root["message"] = e.what();
        std::string resp = root.toStyledString();
        connection->SendResponse(500, resp, "application/json");
        return true;
    }
}

