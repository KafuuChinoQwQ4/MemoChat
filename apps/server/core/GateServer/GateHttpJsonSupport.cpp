#include "GateHttpJsonSupport.h"

#include "HttpConnection.h"
#include "logging/TraceContext.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = memochat::json;

bool GateHttpJsonSupport::ParseJsonBody(std::shared_ptr<HttpConnection> connection, json::JsonValue& root, json::JsonValue& src_root) {
    memolog::TraceContext::SetTraceId(connection ? connection->_trace_id : "");
    connection->_response.set(http::field::content_type, "application/json");
    const auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
    json::JsonReader reader;
    if (!reader.parse(body_str, src_root)) {
        root["error"] = ErrorCodes::Error_Json;
        return false;
    }
    return true;
}

bool GateHttpJsonSupport::HandleJsonPost(
    std::shared_ptr<HttpConnection> connection,
    const std::function<bool(const json::JsonValue&, json::JsonValue&, const std::string&)>& fn) {
    json::JsonValue root = json::JsonValue{};
    json::JsonValue src_root = json::JsonValue{};
    if (!ParseJsonBody(connection, root, src_root)) {
        std::string err_str = json::glaze_stringify(root);
        beast::ostream(connection->_response.body()) << err_str;
        return true;
    }
    fn(src_root, root, connection->_trace_id);
    root["trace_id"] = connection->_trace_id;
    std::string response_str = json::glaze_stringify(root);
    beast::ostream(connection->_response.body()) << response_str;
    return true;
}
