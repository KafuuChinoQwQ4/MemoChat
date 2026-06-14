#include "H1MediaService.h"

#include "H1Connection.h"
#include "H1LogicSystem.h"
#include "Http2MediaSupport.h"
#include "const.h"
#include "json/GlazeCompat.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <cstdlib>
#include <string>
#include <string_view>

namespace beast = boost::beast;
namespace http = beast::http;

std::string H1MediaService::RequestBody(const std::shared_ptr<H1Connection>& connection)
{
    return boost::beast::buffers_to_string(connection->_request.body().data());
}

std::string H1MediaService::HeaderValue(const std::shared_ptr<H1Connection>& connection, std::string_view name)
{
    const auto it = connection->_request.find(std::string(name));
    if (it == connection->_request.end())
    {
        return {};
    }
    return std::string(it->value().data(), it->value().size());
}

int H1MediaService::HeaderIntValue(const std::shared_ptr<H1Connection>& connection, std::string_view name, int fallback)
{
    const std::string value = HeaderValue(connection, name);
    if (value.empty())
    {
        return fallback;
    }
    try
    {
        return std::stoi(value);
    }
    catch (...)
    {
        return fallback;
    }
}

bool H1MediaService::IsJsonRequest(const std::shared_ptr<H1Connection>& connection)
{
    std::string content_type;
    const auto it = connection->_request.find(http::field::content_type);
    if (it != connection->_request.end())
    {
        content_type = std::string(it->value().data(), it->value().size());
    }
    return content_type.find("application/json") != std::string::npos ||
           content_type.find("text/json") != std::string::npos;
}

void H1MediaService::WriteJsonResponse(const std::shared_ptr<H1Connection>& connection,
                                       const memochat::json::JsonValue& root)
{
    connection->_response.set(http::field::content_type, "application/json");
    beast::ostream(connection->_response.body()) << memochat::json::glaze_stringify(root);
}

std::string H1MediaService::QueryValue(const std::shared_ptr<H1Connection>& connection, const std::string& key)
{
    const auto it = connection->_get_params.find(key);
    return it == connection->_get_params.end() ? std::string() : it->second;
}

bool H1MediaService::ParseJsonRequest(const std::shared_ptr<H1Connection>& connection, memochat::json::JsonValue& root)
{
    return Http2MediaSupport::ParseJsonBody(RequestBody(connection), root);
}

namespace
{

memochat::json::JsonValue MakeMediaResponse(const Http2MediaSupport::MediaResult& result)
{
    memochat::json::JsonValue root = memochat::json::glaze_empty_object();
    if (memochat::json::glaze_is_object(result.data))
    {
        for (const auto& key : memochat::json::getMemberNames(result.data))
        {
            root[key] = memochat::json::glaze_get(result.data, key);
        }
    }
    root["error"] = result.error;
    if (!result.message.empty())
    {
        root["message"] = result.message;
    }
    return root;
}

memochat::json::JsonValue MakeMediaErrorJson(int error, const std::string& message)
{
    memochat::json::JsonValue root = memochat::json::glaze_empty_object();
    root["error"] = error;
    root["message"] = message;
    return root;
}

} // namespace

void H1MediaService::RegisterRoutes(H1LogicSystem& logic)
{
    logic.RegPost("/upload_media_init",
                  [](std::shared_ptr<H1Connection> connection)
                  {
                      memochat::json::JsonValue root;
                      if (!H1MediaService::ParseJsonRequest(connection, root))
                      {
                          H1MediaService::WriteJsonResponse(connection,
                                                            MakeMediaErrorJson(ErrorCodes::Error_Json, "invalid json"));
                          return true;
                      }

                      auto result = Http2MediaSupport::HandleUploadMediaInit(
                          memochat::json::glaze_safe_get<int>(root, "uid", 0),
                          memochat::json::glaze_safe_get<std::string>(root, "token", ""),
                          memochat::json::glaze_safe_get<std::string>(root, "media_type", "file"),
                          memochat::json::glaze_safe_get<std::string>(root, "file_name", ""),
                          memochat::json::glaze_safe_get<std::string>(root, "mime", ""),
                          memochat::json::glaze_safe_get<int64_t>(root, "file_size", 0LL));
                      H1MediaService::WriteJsonResponse(connection, MakeMediaResponse(result));
                      return true;
                  });

    logic.RegPost("/upload_media_chunk",
                  [](std::shared_ptr<H1Connection> connection)
                  {
                      Http2MediaSupport::MediaResult result;
                      if (H1MediaService::IsJsonRequest(connection))
                      {
                          memochat::json::JsonValue root;
                          if (!H1MediaService::ParseJsonRequest(connection, root))
                          {
                              H1MediaService::WriteJsonResponse(
                                  connection,
                                  MakeMediaErrorJson(ErrorCodes::Error_Json, "invalid json"));
                              return true;
                          }
                          result = Http2MediaSupport::HandleUploadMediaChunk(
                              memochat::json::glaze_safe_get<int>(root, "uid", 0),
                              memochat::json::glaze_safe_get<std::string>(root, "token", ""),
                              memochat::json::glaze_safe_get<std::string>(root, "upload_id", ""),
                              memochat::json::glaze_safe_get<int>(root, "index", -1),
                              memochat::json::glaze_safe_get<std::string>(root, "data_base64", ""));
                      }
                      else
                      {
                          result = Http2MediaSupport::HandleUploadMediaChunkBytes(
                              H1MediaService::HeaderIntValue(connection, "X-Uid"),
                              H1MediaService::HeaderValue(connection, "X-Token"),
                              H1MediaService::HeaderValue(connection, "X-Upload-Id"),
                              H1MediaService::HeaderIntValue(connection, "X-Chunk-Index", -1),
                              H1MediaService::RequestBody(connection));
                      }
                      H1MediaService::WriteJsonResponse(connection, MakeMediaResponse(result));
                      return true;
                  });

    logic.RegGet("/upload_media_status",
                 [](std::shared_ptr<H1Connection> connection)
                 {
                     auto result = Http2MediaSupport::HandleUploadMediaStatus(
                         std::atoi(H1MediaService::QueryValue(connection, "uid").c_str()),
                         H1MediaService::QueryValue(connection, "token"),
                         H1MediaService::QueryValue(connection, "upload_id"));
                     H1MediaService::WriteJsonResponse(connection, MakeMediaResponse(result));
                     return true;
                 });

    logic.RegPost("/upload_media_complete",
                  [](std::shared_ptr<H1Connection> connection)
                  {
                      memochat::json::JsonValue root;
                      if (!H1MediaService::ParseJsonRequest(connection, root))
                      {
                          H1MediaService::WriteJsonResponse(connection,
                                                            MakeMediaErrorJson(ErrorCodes::Error_Json, "invalid json"));
                          return true;
                      }

                      auto result = Http2MediaSupport::HandleUploadMediaComplete(
                          memochat::json::glaze_safe_get<int>(root, "uid", 0),
                          memochat::json::glaze_safe_get<std::string>(root, "token", ""),
                          memochat::json::glaze_safe_get<std::string>(root, "upload_id", ""));
                      H1MediaService::WriteJsonResponse(connection, MakeMediaResponse(result));
                      return true;
                  });

    logic.RegPost("/upload_media",
                  [](std::shared_ptr<H1Connection> connection)
                  {
                      memochat::json::JsonValue root;
                      if (!H1MediaService::ParseJsonRequest(connection, root))
                      {
                          H1MediaService::WriteJsonResponse(connection,
                                                            MakeMediaErrorJson(ErrorCodes::Error_Json, "invalid json"));
                          return true;
                      }

                      auto result = Http2MediaSupport::HandleUploadMediaSimple(
                          memochat::json::glaze_safe_get<int>(root, "uid", 0),
                          memochat::json::glaze_safe_get<std::string>(root, "token", ""),
                          memochat::json::glaze_safe_get<std::string>(root, "media_type", "file"),
                          memochat::json::glaze_safe_get<std::string>(root, "file_name", ""),
                          memochat::json::glaze_safe_get<std::string>(root, "mime", ""),
                          memochat::json::glaze_safe_get<std::string>(root, "data_base64", ""));
                      H1MediaService::WriteJsonResponse(connection, MakeMediaResponse(result));
                      return true;
                  });

    logic.RegGet(
        "/media/download",
        [](std::shared_ptr<H1Connection> connection)
        {
            const int uid = std::atoi(H1MediaService::QueryValue(connection, "uid").c_str());
            const std::string token = H1MediaService::QueryValue(connection, "token");
            const std::string media_key = H1MediaService::QueryValue(connection, "asset");
            auto result = Http2MediaSupport::HandleMediaDownloadInfo(uid, token, media_key);
            if (result.error != 0)
            {
                H1MediaService::WriteJsonResponse(connection, MakeMediaResponse(result));
                return true;
            }

            if (memochat::json::glaze_has_key(result.data, "data"))
            {
                connection->_response.set(http::field::content_type,
                                          memochat::json::glaze_safe_get<std::string>(result.data, "content_type", ""));
                const std::string body = memochat::json::glaze_safe_get<std::string>(result.data, "data", "");
                beast::ostream(connection->_response.body())
                    .write(body.data(), static_cast<std::streamsize>(body.size()));
                return true;
            }

            if (memochat::json::glaze_has_key(result.data, "path"))
            {
                connection->SetFileResponse(
                    memochat::json::glaze_safe_get<std::string>(result.data, "path", ""),
                    memochat::json::glaze_safe_get<std::string>(result.data, "content_type", ""));
                return true;
            }

            H1MediaService::WriteJsonResponse(connection, MakeMediaErrorJson(ErrorCodes::UidInvalid, "file not found"));
            return true;
        });
}
