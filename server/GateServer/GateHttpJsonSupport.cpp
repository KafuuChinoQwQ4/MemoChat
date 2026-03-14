#include "GateHttpJsonSupport.h"

#include "HttpConnection.h"
#include "logging/TraceContext.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <json/json.h>

namespace beast = boost::beast;
namespace http = beast::http;

bool GateHttpJsonSupport::ParseJsonBody(std::shared_ptr<HttpConnection> connection, Json::Value& root, Json::Value& src_root) {
    memolog::TraceContext::SetTraceId(connection ? connection->_trace_id : "");
    connection->_response.set(http::field::content_type, "text/json");
    const auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
    Json::Reader reader;
    if (!reader.parse(body_str, src_root)) {
        root["error"] = ErrorCodes::Error_Json;
        return false;
    }
    return true;
}

bool GateHttpJsonSupport::HandleJsonPost(
    std::shared_ptr<HttpConnection> connection,
    const std::function<bool(const Json::Value&, Json::Value&, const std::string&)>& fn) {
    Json::Value root;
    Json::Value src_root;
    if (!ParseJsonBody(connection, root, src_root)) {
        beast::ostream(connection->_response.body()) << root.toStyledString();
        return true;
    }
    fn(src_root, root, connection->_trace_id);
    if (!root.isMember("trace_id")) {
        root["trace_id"] = connection->_trace_id;
    }
    beast::ostream(connection->_response.body()) << root.toStyledString();
    return true;
}
