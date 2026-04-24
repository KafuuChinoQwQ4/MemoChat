#include "H1JsonSupport.h"
#include "H1Connection.h"
#include "logging/TraceContext.h"
#include "json/GlazeCompat.h"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

namespace beast = boost::beast;
namespace http = beast::http;

bool H1JsonSupport::ParseJsonBody(std::shared_ptr<H1Connection> connection,
                                   memochat::json::JsonValue& root,
                                   memochat::json::JsonValue& src_root)
{
    memolog::TraceContext::SetTraceId(connection ? connection->_trace_id : "");
    connection->_response.set(http::field::content_type, "text/json");
    const auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
    if (!memochat::json::glaze_parse(src_root, body_str)) {
        root["error"] = ErrorCodes::Error_Json;
        return false;
    }
    return true;
}

bool H1JsonSupport::HandleJsonPost(
    std::shared_ptr<H1Connection> connection,
    const std::function<bool(const memochat::json::JsonValue&, memochat::json::JsonValue&, const std::string&)>& fn)
{
    memochat::json::JsonValue root;
    memochat::json::JsonValue src_root;
    if (!ParseJsonBody(connection, root, src_root)) {
        beast::ostream(connection->_response.body()) << memochat::json::glaze_stringify(root);
        return true;
    }

    fn(src_root, root, connection->_trace_id);

    if (!memochat::json::glaze_has_key(root, "trace_id")) {
        root["trace_id"] = connection->_trace_id;
    }
    beast::ostream(connection->_response.body()) << memochat::json::glaze_stringify(root);
    return true;
}

