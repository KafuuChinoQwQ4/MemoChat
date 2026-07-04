#include "GateHttpJsonSupport.hpp"

#include "HttpConnection.hpp"
#include "logging/TraceContext.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

import memochat.gate.http_json_support_algorithms;

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = memochat::json;
namespace http_json_modules = memochat::gate::http_json::modules;

bool GateHttpJsonSupport::ParseJsonBody(std::shared_ptr<HttpConnection> connection,
                                        json::JsonValue& root,
                                        json::JsonValue& src_root)
{
    memolog::TraceContext::SetTraceId(connection ? connection->_trace_id : http_json_modules::EmptyTraceId());
    connection->_response.set(http::field::content_type, http_json_modules::JsonContentType());
    const auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
    json::JsonReader reader;
    const bool parsed_json = reader.parse(body_str, src_root);
    if (http_json_modules::ShouldWriteJsonParseError(parsed_json))
    {
        root["error"] = ErrorCodes::Error_Json;
        return false;
    }
    return true;
}

bool GateHttpJsonSupport::HandleJsonPost(
    std::shared_ptr<HttpConnection> connection,
    const std::function<bool(const json::JsonValue&, json::JsonValue&, const std::string&)>& fn)
{
    json::JsonValue root = json::JsonValue{};
    json::JsonValue src_root = json::JsonValue{};
    const bool parsed_json = ParseJsonBody(connection, root, src_root);
    if (!parsed_json)
    {
        std::string err_str = json::glaze_stringify(root);
        beast::ostream(connection->_response.body()) << err_str;
        return true;
    }
    fn(src_root, root, connection->_trace_id);
    if (http_json_modules::ShouldAttachTraceId(parsed_json))
    {
        root["trace_id"] = connection->_trace_id;
    }
    std::string response_str = json::glaze_stringify(root);
    beast::ostream(connection->_response.body()) << response_str;
    return true;
}
