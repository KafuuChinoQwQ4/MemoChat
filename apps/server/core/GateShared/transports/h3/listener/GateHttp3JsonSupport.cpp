#include "GateHttp3JsonSupport.hpp"
#include "GateHttp3Connection.hpp"
#include "json/GlazeCompat.hpp"
#include <sstream>

import memochat.gate.h3_json_support_algorithms;

namespace
{
namespace json_support_modules = memochat::gate::h3::json_support::modules;
} // namespace

bool GateHttp3JsonSupport::ParseJsonBody(std::shared_ptr<GateHttp3Connection> connection,
                                         memochat::json::JsonValue& root,
                                         memochat::json::JsonValue& src_root)
{
    if (!connection)
    {
        root["error"] = json_support_modules::ErrorCode();
        root["message"] = json_support_modules::NullConnectionMessage();
        return false;
    }

    const std::string body = connection->GetRequestBody();
    if (body.empty())
    {
        root["error"] = json_support_modules::ErrorCode();
        root["message"] = json_support_modules::EmptyBodyMessage();
        return false;
    }

    std::string parse_error;
    if (!memochat::json::glaze_parse(src_root, body, &parse_error))
    {
        root["error"] = json_support_modules::ErrorCode();
        root["message"] = std::string(json_support_modules::JsonParseErrorPrefix()) + parse_error;
        return false;
    }
    return true;
}

bool GateHttp3JsonSupport::HandleJsonPost(
    std::shared_ptr<GateHttp3Connection> connection,
    std::function<bool(const memochat::json::JsonValue&, memochat::json::JsonValue&, const std::string&)> fn)
{
    memochat::json::JsonValue root;
    memochat::json::JsonValue src_root;

    root["trace_id"] = connection->GetTraceId();
    root["request_id"] = connection->GetRequestId();

    if (!ParseJsonBody(connection, root, src_root))
    {
        std::string resp = memochat::json::writeString(root);
        connection->SendResponse(json_support_modules::BadRequestStatus(),
                                 resp,
                                 json_support_modules::ResponseContentType());
        return true;
    }

    std::string trace_id = connection->GetTraceId();
    try
    {
        if (!fn(src_root, root, trace_id))
        {
            if (json_support_modules::ShouldForceErrorCode(root.isMember("error"),
                                                           memochat::json::glaze_safe_get<int>(root, "error", 0)))
            {
                root["error"] = json_support_modules::ErrorCode();
            }
            std::string resp = memochat::json::writeString(root);
            connection->SendResponse(json_support_modules::BadRequestStatus(),
                                     resp,
                                     json_support_modules::ResponseContentType());
            return true;
        }
        root["error"] = json_support_modules::SuccessCode();
        std::string resp = memochat::json::writeString(root);
        connection->SendResponse(json_support_modules::OkStatus(), resp, json_support_modules::ResponseContentType());
        return true;
    }
    catch (const std::exception& e)
    {
        root = memochat::json::glaze_empty_object();
        root["trace_id"] = connection->GetTraceId();
        root["request_id"] = connection->GetRequestId();
        root["error"] = json_support_modules::ErrorCode();
        root["message"] = e.what();
        std::string resp = memochat::json::writeString(root);
        connection->SendResponse(json_support_modules::InternalErrorStatus(),
                                 resp,
                                 json_support_modules::ResponseContentType());
        return true;
    }
}
