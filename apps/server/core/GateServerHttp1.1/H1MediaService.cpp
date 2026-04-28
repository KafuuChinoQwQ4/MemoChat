#include "H1MediaService.h"

#include "H1Connection.h"
#include "H1LogicSystem.h"
#include "const.h"
#include "json/GlazeCompat.h"
#include "logging/Logger.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

namespace beast = boost::beast;
namespace http = beast::http;

namespace {

memochat::json::JsonValue MakeMediaNotImplementedResponse(const char* route)
{
    memolog::LogWarn("gate.h1.media.not_implemented", "HTTP/1.1 media route is not migrated yet", {{"route", route}});
    memochat::json::JsonValue root;
    root["error"] = ErrorCodes::MediaUploadFailed;
    root["message"] = "HTTP/1.1 media route is not migrated yet";
    root["route"] = route;
    return root;
}

} // namespace

void H1MediaService::RegisterRoutes(H1LogicSystem& logic)
{
    logic.RegPost("/upload_media_init", [](std::shared_ptr<H1Connection> connection) {
        auto root = MakeMediaNotImplementedResponse("/upload_media_init");
        connection->_response.set(http::field::content_type, "text/json");
        beast::ostream(connection->_response.body()) << memochat::json::glaze_stringify(root);
        return true;
    });
    logic.RegPost("/upload_media_chunk", [](std::shared_ptr<H1Connection> connection) {
        auto root = MakeMediaNotImplementedResponse("/upload_media_chunk");
        connection->_response.set(http::field::content_type, "text/json");
        beast::ostream(connection->_response.body()) << memochat::json::glaze_stringify(root);
        return true;
    });
    logic.RegGet("/upload_media_status", [](std::shared_ptr<H1Connection> connection) {
        auto root = MakeMediaNotImplementedResponse("/upload_media_status");
        connection->_response.set(http::field::content_type, "text/json");
        beast::ostream(connection->_response.body()) << memochat::json::glaze_stringify(root);
        return true;
    });
    logic.RegPost("/upload_media_complete", [](std::shared_ptr<H1Connection> connection) {
        auto root = MakeMediaNotImplementedResponse("/upload_media_complete");
        connection->_response.set(http::field::content_type, "text/json");
        beast::ostream(connection->_response.body()) << memochat::json::glaze_stringify(root);
        return true;
    });
    logic.RegPost("/upload_media", [](std::shared_ptr<H1Connection> connection) {
        auto root = MakeMediaNotImplementedResponse("/upload_media");
        connection->_response.set(http::field::content_type, "text/json");
        beast::ostream(connection->_response.body()) << memochat::json::glaze_stringify(root);
        return true;
    });
    logic.RegGet("/media/download", [](std::shared_ptr<H1Connection> connection) {
        auto root = MakeMediaNotImplementedResponse("/media/download");
        connection->_response.set(http::field::content_type, "text/json");
        beast::ostream(connection->_response.body()) << memochat::json::glaze_stringify(root);
        return true;
    });
}
