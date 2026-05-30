#include "GateRouteModules.h"

#include "GateHttpJsonSupport.h"
#include "HttpConnection.h"
#include "LogicSystem.h"
#include "RedisMgr.h"
#include "const.h"
#include "json/GlazeCompat.h"
#include "r18/R18SourceService.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <cstdlib>
#include <exception>
#include <string>

namespace beast = boost::beast;
namespace http = beast::http;

namespace
{

using memochat::json::JsonValue;

constexpr const char* kDefaultR18SourceId = "";

bool ValidateUserToken(int uid, const std::string& token)
{
    if (uid <= 0 || token.empty())
        return false;
    std::string token_value;
    return RedisMgr::GetInstance()->Get(std::string(USERTOKENPREFIX) + std::to_string(uid), token_value) &&
           token_value == token;
}

std::string
QueryParam(const std::shared_ptr<HttpConnection>& connection, const std::string& key, const std::string& fallback = {})
{
    const auto& params = connection->GetParams();
    const auto it = params.find(key);
    return it == params.end() ? fallback : it->second;
}

bool RequireAuth(const JsonValue& src, JsonValue& root, int& uid, std::string& token)
{
    uid = static_cast<int>(memochat::json::glaze_safe_get<int64_t>(src, "uid", 0LL));
    token = memochat::json::glaze_safe_get<std::string>(src, "token", "");
    if (!ValidateUserToken(uid, token))
    {
        root["error"] = ErrorCodes::TokenInvalid;
        root["message"] = "token invalid";
        return false;
    }
    return true;
}

void WriteOk(JsonValue& root, const JsonValue& data)
{
    root["error"] = ErrorCodes::Success;
    root["message"] = "";
    root["data"] = data;
}

JsonValue ToSourceJson(const memochat::r18::R18SourceRecord& rec)
{
    JsonValue source;
    source["id"] = rec.id;
    source["name"] = rec.name;
    source["version"] = rec.version;
    source["path"] = rec.path;
    source["format"] = rec.format;
    source["source_url"] = rec.source_url;
    source["catalog_url"] = rec.catalog_url;
    source["enabled"] = rec.enabled;
    source["builtin"] = rec.builtin;
    source["status"] = rec.status;
    source["message"] = rec.message;
    return source;
}

void WriteJson(std::shared_ptr<HttpConnection> connection, const JsonValue& root)
{
    auto& response = connection->GetResponse();
    response.set(http::field::content_type, "text/json");
    beast::ostream(response.body()) << memochat::json::glaze_stringify(root);
}

} // namespace

void R18HttpServiceRoutes::RegisterRoutes(LogicSystem& logic)
{
    logic.RegGet("/api/r18/sources",
                 [](std::shared_ptr<HttpConnection> connection)
                 {
                     JsonValue root;
                     const int uid = std::atoi(QueryParam(connection, "uid").c_str());
                     const std::string token = QueryParam(connection, "token");
                     if (!ValidateUserToken(uid, token))
                     {
                         root["error"] = ErrorCodes::TokenInvalid;
                         root["message"] = "token invalid";
                     }
                     else
                     {
                         JsonValue data;
                         data["sources"] = memochat::r18::R18SourceService::Instance().ListSources();
                         WriteOk(root, data);
                     }
                     WriteJson(connection, root);
                     return true;
                 });

    logic.RegPost(
        "/api/r18/source/import",
        [](std::shared_ptr<HttpConnection> connection)
        {
            return GateHttpJsonSupport::HandleJsonPost(
                connection,
                [](const JsonValue& src, JsonValue& root, const std::string&)
                {
                    int uid = 0;
                    std::string token;
                    if (!RequireAuth(src, root, uid, token))
                        return true;

                    const std::string file_name =
                        memochat::json::glaze_safe_get<std::string>(src, "file_name", "source.zip");
                    const std::string encoded = memochat::json::glaze_safe_get<std::string>(src, "data_base64", "");
                    const std::string manifest_json =
                        memochat::json::glaze_safe_get<std::string>(src, "manifest_json", "");
                    std::string binary;
                    if (encoded.empty() || !memochat::r18::DecodeBase64(encoded, binary))
                    {
                        root["error"] = ErrorCodes::Error_Json;
                        root["message"] = "invalid plugin package payload";
                        return true;
                    }

                    std::string error;
                    auto rec =
                        memochat::r18::R18SourceService::Instance().ImportZip(file_name, manifest_json, binary, &error);
                    if (!error.empty())
                    {
                        root["error"] = ErrorCodes::Error_Json;
                        root["message"] = error;
                        return true;
                    }

                    JsonValue data;
                    data["source"] = ToSourceJson(rec);
                    WriteOk(root, data);
                    return true;
                });
        });

    logic.RegPost("/api/r18/source/enable",
                  [](std::shared_ptr<HttpConnection> connection)
                  {
                      return GateHttpJsonSupport::HandleJsonPost(
                          connection,
                          [](const JsonValue& src, JsonValue& root, const std::string&)
                          {
                              int uid = 0;
                              std::string token;
                              if (!RequireAuth(src, root, uid, token))
                                  return true;

                              const std::string source_id =
                                  memochat::json::glaze_safe_get<std::string>(src, "source_id", "");
                              std::string error;
                              if (!memochat::r18::R18SourceService::Instance().EnableSource(source_id, true, &error))
                              {
                                  root["error"] = ErrorCodes::Error_Json;
                                  root["message"] = error;
                                  return true;
                              }

                              JsonValue data;
                              data["source_id"] = source_id;
                              data["enabled"] = true;
                              WriteOk(root, data);
                              return true;
                          });
                  });

    logic.RegPost("/api/r18/source/disable",
                  [](std::shared_ptr<HttpConnection> connection)
                  {
                      return GateHttpJsonSupport::HandleJsonPost(
                          connection,
                          [](const JsonValue& src, JsonValue& root, const std::string&)
                          {
                              int uid = 0;
                              std::string token;
                              if (!RequireAuth(src, root, uid, token))
                                  return true;

                              const std::string source_id =
                                  memochat::json::glaze_safe_get<std::string>(src, "source_id", "");
                              std::string error;
                              if (!memochat::r18::R18SourceService::Instance().EnableSource(source_id, false, &error))
                              {
                                  root["error"] = ErrorCodes::Error_Json;
                                  root["message"] = error;
                                  return true;
                              }

                              JsonValue data;
                              data["source_id"] = source_id;
                              data["enabled"] = false;
                              WriteOk(root, data);
                              return true;
                          });
                  });

    logic.RegPost(
        "/api/r18/search",
        [](std::shared_ptr<HttpConnection> connection)
        {
            return GateHttpJsonSupport::HandleJsonPost(
                connection,
                [](const JsonValue& src, JsonValue& root, const std::string&)
                {
                    int uid = 0;
                    std::string token;
                    if (!RequireAuth(src, root, uid, token))
                        return true;

                    const std::string source_id =
                        memochat::json::glaze_safe_get<std::string>(src, "source_id", kDefaultR18SourceId);
                    const std::string keyword = memochat::json::glaze_safe_get<std::string>(src, "keyword", "");
                    const int page = static_cast<int>(memochat::json::glaze_safe_get<int64_t>(src, "page", 1LL));
                    WriteOk(root,
                            memochat::r18::R18SourceService::Instance().Search(source_id, keyword, page, uid, token));
                    return true;
                });
        });

    logic.RegPost(
        "/api/r18/comic/detail",
        [](std::shared_ptr<HttpConnection> connection)
        {
            return GateHttpJsonSupport::HandleJsonPost(
                connection,
                [](const JsonValue& src, JsonValue& root, const std::string&)
                {
                    int uid = 0;
                    std::string token;
                    if (!RequireAuth(src, root, uid, token))
                        return true;

                    const std::string source_id =
                        memochat::json::glaze_safe_get<std::string>(src, "source_id", kDefaultR18SourceId);
                    const std::string comic_id = memochat::json::glaze_safe_get<std::string>(src, "comic_id", "");
                    WriteOk(root, memochat::r18::R18SourceService::Instance().Detail(source_id, comic_id, uid, token));
                    return true;
                });
        });

    logic.RegPost(
        "/api/r18/chapter/pages",
        [](std::shared_ptr<HttpConnection> connection)
        {
            return GateHttpJsonSupport::HandleJsonPost(
                connection,
                [](const JsonValue& src, JsonValue& root, const std::string&)
                {
                    int uid = 0;
                    std::string token;
                    if (!RequireAuth(src, root, uid, token))
                        return true;

                    const std::string source_id =
                        memochat::json::glaze_safe_get<std::string>(src, "source_id", kDefaultR18SourceId);
                    const std::string chapter_id = memochat::json::glaze_safe_get<std::string>(src, "chapter_id", "");
                    WriteOk(root, memochat::r18::R18SourceService::Instance().Pages(source_id, chapter_id, uid, token));
                    return true;
                });
        });

    logic.RegPost("/api/r18/favorite/toggle",
                  [](std::shared_ptr<HttpConnection> connection)
                  {
                      return GateHttpJsonSupport::HandleJsonPost(
                          connection,
                          [](const JsonValue& src, JsonValue& root, const std::string&)
                          {
                              int uid = 0;
                              std::string token;
                              if (!RequireAuth(src, root, uid, token))
                                  return true;

                              JsonValue data;
                              data["source_id"] = memochat::json::glaze_safe_get<std::string>(src, "source_id", "");
                              data["comic_id"] = memochat::json::glaze_safe_get<std::string>(src, "comic_id", "");
                              data["favorited"] = memochat::json::glaze_safe_get<bool>(src, "favorited", true);
                              WriteOk(root, data);
                              return true;
                          });
                  });

    logic.RegPost("/api/r18/history/update",
                  [](std::shared_ptr<HttpConnection> connection)
                  {
                      return GateHttpJsonSupport::HandleJsonPost(
                          connection,
                          [](const JsonValue& src, JsonValue& root, const std::string&)
                          {
                              int uid = 0;
                              std::string token;
                              if (!RequireAuth(src, root, uid, token))
                                  return true;

                              JsonValue data;
                              data["source_id"] = memochat::json::glaze_safe_get<std::string>(src, "source_id", "");
                              data["comic_id"] = memochat::json::glaze_safe_get<std::string>(src, "comic_id", "");
                              data["chapter_id"] = memochat::json::glaze_safe_get<std::string>(src, "chapter_id", "");
                              data["page_index"] = memochat::json::glaze_safe_get<int64_t>(src, "page_index", 0LL);
                              WriteOk(root, data);
                              return true;
                          });
                  });

    logic.RegGet("/api/r18/history",
                 [](std::shared_ptr<HttpConnection> connection)
                 {
                     JsonValue root;
                     const int uid = std::atoi(QueryParam(connection, "uid").c_str());
                     const std::string token = QueryParam(connection, "token");
                     if (!ValidateUserToken(uid, token))
                     {
                         root["error"] = ErrorCodes::TokenInvalid;
                         root["message"] = "token invalid";
                     }
                     else
                     {
                         JsonValue data;
                         data["items"] = JsonValue{memochat::json::array_t{}};
                         WriteOk(root, data);
                     }
                     WriteJson(connection, root);
                     return true;
                 });

    logic.RegGet("/api/r18/image",
                 [](std::shared_ptr<HttpConnection> connection)
                 {
                     const int uid = std::atoi(QueryParam(connection, "uid").c_str());
                     const std::string token = QueryParam(connection, "token");
                     auto& response = connection->GetResponse();
                     if (!ValidateUserToken(uid, token))
                     {
                         response.result(http::status::unauthorized);
                         response.set(http::field::content_type, "text/plain");
                         beast::ostream(response.body()) << "token invalid";
                         return true;
                     }

                     const std::string source_id = QueryParam(connection, "source_id", kDefaultR18SourceId);
                     const std::string image_url = QueryParam(connection, "image_url");
                     try
                     {
                         auto payload = memochat::r18::R18SourceService::Instance().FetchImage(source_id, image_url);
                         response.set(http::field::content_type, payload.content_type);
                         beast::ostream(response.body()) << payload.body;
                     }
                     catch (const std::exception& exc)
                     {
                         response.result(http::status::bad_gateway);
                         response.set(http::field::content_type, "text/plain");
                         beast::ostream(response.body()) << "image fetch failed: " << exc.what();
                     }
                     return true;
                 });
}
