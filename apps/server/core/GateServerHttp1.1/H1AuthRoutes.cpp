#include "H1AuthRoutes.h"

#include "H1Connection.h"
#include "H1JsonSupport.h"
#include "H1LogicSystem.h"
#include "CallService.h"
#include "const.h"
#include "json/GlazeCompat.h"
#include "logging/Logger.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

namespace beast = boost::beast;
namespace http = beast::http;

namespace {

memochat::json::JsonValue MakeNotImplementedResponse(const char* route)
{
    memolog::LogWarn("gate.h1.route.not_implemented", "HTTP/1.1 route is not migrated yet", {{"route", route}});
    memochat::json::JsonValue root;
    root["error"] = ErrorCodes::RPCFailed;
    root["message"] = "HTTP/1.1 route is not migrated yet";
    root["route"] = route;
    return root;
}

} // namespace

void H1AuthService::RegisterRoutes(H1LogicSystem& logic)
{
    logic.RegPost("/get_varifycode", [](std::shared_ptr<H1Connection> connection) {
        auto root = MakeNotImplementedResponse("/get_varifycode");
        connection->_response.set(http::field::content_type, "text/json");
        beast::ostream(connection->_response.body()) << memochat::json::glaze_stringify(root);
        return true;
    });
    logic.RegPost("/user_register", [](std::shared_ptr<H1Connection> connection) {
        auto root = MakeNotImplementedResponse("/user_register");
        connection->_response.set(http::field::content_type, "text/json");
        beast::ostream(connection->_response.body()) << memochat::json::glaze_stringify(root);
        return true;
    });
    logic.RegPost("/reset_pwd", [](std::shared_ptr<H1Connection> connection) {
        auto root = MakeNotImplementedResponse("/reset_pwd");
        connection->_response.set(http::field::content_type, "text/json");
        beast::ostream(connection->_response.body()) << memochat::json::glaze_stringify(root);
        return true;
    });
    logic.RegPost("/user_login", [](std::shared_ptr<H1Connection> connection) {
        auto root = MakeNotImplementedResponse("/user_login");
        connection->_response.set(http::field::content_type, "text/json");
        beast::ostream(connection->_response.body()) << memochat::json::glaze_stringify(root);
        return true;
    });
}

void H1ProfileService::RegisterRoutes(H1LogicSystem& logic)
{
    logic.RegPost("/user_update_profile", [](std::shared_ptr<H1Connection> connection) {
        auto root = MakeNotImplementedResponse("/user_update_profile");
        connection->_response.set(http::field::content_type, "text/json");
        beast::ostream(connection->_response.body()) << memochat::json::glaze_stringify(root);
        return true;
    });
}

void H1CallServiceRoutes::RegisterRoutes(H1LogicSystem& logic)
{
    logic.RegPost("/api/call/start", [](std::shared_ptr<H1Connection> connection) {
        return H1JsonSupport::HandleJsonPost(connection,
            [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string& trace_id) {
                return CallService::GetInstance()->StartCall(src_root, root, trace_id);
            });
    });
    logic.RegPost("/api/call/accept", [](std::shared_ptr<H1Connection> connection) {
        return H1JsonSupport::HandleJsonPost(connection,
            [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string& trace_id) {
                return CallService::GetInstance()->AcceptCall(src_root, root, trace_id);
            });
    });
    logic.RegPost("/api/call/reject", [](std::shared_ptr<H1Connection> connection) {
        return H1JsonSupport::HandleJsonPost(connection,
            [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string& trace_id) {
                return CallService::GetInstance()->RejectCall(src_root, root, trace_id);
            });
    });
    logic.RegPost("/api/call/cancel", [](std::shared_ptr<H1Connection> connection) {
        return H1JsonSupport::HandleJsonPost(connection,
            [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string& trace_id) {
                return CallService::GetInstance()->CancelCall(src_root, root, trace_id);
            });
    });
    logic.RegPost("/api/call/hangup", [](std::shared_ptr<H1Connection> connection) {
        return H1JsonSupport::HandleJsonPost(connection,
            [](const memochat::json::JsonValue& src_root, memochat::json::JsonValue& root, const std::string& trace_id) {
                return CallService::GetInstance()->HangupCall(src_root, root, trace_id);
            });
    });
}
