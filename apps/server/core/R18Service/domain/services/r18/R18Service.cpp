#include "services/r18/R18Service.h"

#include "RedisMgr.h"
#include "const.h"
#include "json/GlazeCompat.h"
#include "r18/R18SourceService.h"
#include "support/UserTokenValidator.h"

#include <cstdlib>
#include <exception>
#include <functional>
#include <string>

namespace memochat::gate::services::r18
{
namespace
{

using memochat::json::JsonValue;

constexpr const char* kDefaultR18SourceId = "";

bool ValidateUserToken(int uid, const std::string& token)
{
    return memochat::auth::ValidateUserToken(uid, token);
}

std::string QueryParam(const memochat::gate::routing::GateRequest& request,
                       const std::string& key,
                       const std::string& fallback = {})
{
    const auto it = request.query.find(key);
    return it == request.query.end() ? fallback : it->second;
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

void WriteJson(memochat::gate::routing::GateResponse& response, const JsonValue& root, const char* content_type)
{
    response.status = 200;
    response.content_type = content_type;
    response.body = memochat::json::glaze_stringify(root);
}

void WriteGetJson(memochat::gate::routing::GateResponse& response, const JsonValue& root)
{
    WriteJson(response, root, "text/json");
}

void WritePostJson(memochat::gate::routing::GateResponse& response, const JsonValue& root)
{
    WriteJson(response, root, "application/json");
}

bool HandleJsonRequest(const memochat::gate::routing::GateRequest& request,
                       memochat::gate::routing::GateResponse& response,
                       const std::function<bool(const JsonValue&, JsonValue&, const std::string&)>& fn)
{
    JsonValue root;
    JsonValue src_root;
    memochat::json::JsonReader reader;
    if (!reader.parse(request.body, src_root))
    {
        root["error"] = ErrorCodes::Error_Json;
        WritePostJson(response, root);
        return true;
    }

    fn(src_root, root, request.trace_id);
    root["trace_id"] = request.trace_id;
    WritePostJson(response, root);
    return true;
}

} // namespace

R18Service& R18Service::Instance()
{
    static R18Service instance;
    return instance;
}

bool R18Service::HandleListSources(const memochat::gate::routing::GateRequest& request,
                                   memochat::gate::routing::GateResponse& response)
{
    JsonValue root;
    const int uid = std::atoi(QueryParam(request, "uid").c_str());
    const std::string token = QueryParam(request, "token");
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
    WriteGetJson(response, root);
    return true;
}

bool R18Service::HandleImportSource(const memochat::gate::routing::GateRequest& request,
                                    memochat::gate::routing::GateResponse& response)
{
    return HandleJsonRequest(
        request,
        response,
        [](const JsonValue& src, JsonValue& root, const std::string&)
        {
            int uid = 0;
            std::string token;
            if (!RequireAuth(src, root, uid, token))
            {
                return true;
            }

            const std::string file_name = memochat::json::glaze_safe_get<std::string>(src, "file_name", "source.zip");
            const std::string encoded = memochat::json::glaze_safe_get<std::string>(src, "data_base64", "");
            const std::string manifest_json = memochat::json::glaze_safe_get<std::string>(src, "manifest_json", "");
            std::string binary;
            if (encoded.empty() || !memochat::r18::DecodeBase64(encoded, binary))
            {
                root["error"] = ErrorCodes::Error_Json;
                root["message"] = "invalid plugin package payload";
                return true;
            }

            std::string error;
            auto rec = memochat::r18::R18SourceService::Instance().ImportZip(file_name, manifest_json, binary, &error);
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
}

bool R18Service::HandleEnableSource(const memochat::gate::routing::GateRequest& request,
                                    memochat::gate::routing::GateResponse& response)
{
    return HandleJsonRequest(request,
                             response,
                             [](const JsonValue& src, JsonValue& root, const std::string&)
                             {
                                 int uid = 0;
                                 std::string token;
                                 if (!RequireAuth(src, root, uid, token))
                                 {
                                     return true;
                                 }

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
}

bool R18Service::HandleDisableSource(const memochat::gate::routing::GateRequest& request,
                                     memochat::gate::routing::GateResponse& response)
{
    return HandleJsonRequest(
        request,
        response,
        [](const JsonValue& src, JsonValue& root, const std::string&)
        {
            int uid = 0;
            std::string token;
            if (!RequireAuth(src, root, uid, token))
            {
                return true;
            }

            const std::string source_id = memochat::json::glaze_safe_get<std::string>(src, "source_id", "");
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
}

bool R18Service::HandleSearch(const memochat::gate::routing::GateRequest& request,
                              memochat::gate::routing::GateResponse& response)
{
    return HandleJsonRequest(
        request,
        response,
        [](const JsonValue& src, JsonValue& root, const std::string&)
        {
            int uid = 0;
            std::string token;
            if (!RequireAuth(src, root, uid, token))
            {
                return true;
            }

            const std::string source_id =
                memochat::json::glaze_safe_get<std::string>(src, "source_id", kDefaultR18SourceId);
            const std::string keyword = memochat::json::glaze_safe_get<std::string>(src, "keyword", "");
            const int page = static_cast<int>(memochat::json::glaze_safe_get<int64_t>(src, "page", 1LL));
            WriteOk(root, memochat::r18::R18SourceService::Instance().Search(source_id, keyword, page, uid, token));
            return true;
        });
}

bool R18Service::HandleComicDetail(const memochat::gate::routing::GateRequest& request,
                                   memochat::gate::routing::GateResponse& response)
{
    return HandleJsonRequest(
        request,
        response,
        [](const JsonValue& src, JsonValue& root, const std::string&)
        {
            int uid = 0;
            std::string token;
            if (!RequireAuth(src, root, uid, token))
            {
                return true;
            }

            const std::string source_id =
                memochat::json::glaze_safe_get<std::string>(src, "source_id", kDefaultR18SourceId);
            const std::string comic_id = memochat::json::glaze_safe_get<std::string>(src, "comic_id", "");
            WriteOk(root, memochat::r18::R18SourceService::Instance().Detail(source_id, comic_id, uid, token));
            return true;
        });
}

bool R18Service::HandleChapterPages(const memochat::gate::routing::GateRequest& request,
                                    memochat::gate::routing::GateResponse& response)
{
    return HandleJsonRequest(
        request,
        response,
        [](const JsonValue& src, JsonValue& root, const std::string&)
        {
            int uid = 0;
            std::string token;
            if (!RequireAuth(src, root, uid, token))
            {
                return true;
            }

            const std::string source_id =
                memochat::json::glaze_safe_get<std::string>(src, "source_id", kDefaultR18SourceId);
            const std::string chapter_id = memochat::json::glaze_safe_get<std::string>(src, "chapter_id", "");
            WriteOk(root, memochat::r18::R18SourceService::Instance().Pages(source_id, chapter_id, uid, token));
            return true;
        });
}

bool R18Service::HandleFavoriteToggle(const memochat::gate::routing::GateRequest& request,
                                      memochat::gate::routing::GateResponse& response)
{
    return HandleJsonRequest(request,
                             response,
                             [](const JsonValue& src, JsonValue& root, const std::string&)
                             {
                                 int uid = 0;
                                 std::string token;
                                 if (!RequireAuth(src, root, uid, token))
                                 {
                                     return true;
                                 }

                                 JsonValue data;
                                 data["source_id"] = memochat::json::glaze_safe_get<std::string>(src, "source_id", "");
                                 data["comic_id"] = memochat::json::glaze_safe_get<std::string>(src, "comic_id", "");
                                 data["favorited"] = memochat::json::glaze_safe_get<bool>(src, "favorited", true);
                                 WriteOk(root, data);
                                 return true;
                             });
}

bool R18Service::HandleHistoryUpdate(const memochat::gate::routing::GateRequest& request,
                                     memochat::gate::routing::GateResponse& response)
{
    return HandleJsonRequest(request,
                             response,
                             [](const JsonValue& src, JsonValue& root, const std::string&)
                             {
                                 int uid = 0;
                                 std::string token;
                                 if (!RequireAuth(src, root, uid, token))
                                 {
                                     return true;
                                 }

                                 JsonValue data;
                                 data["source_id"] = memochat::json::glaze_safe_get<std::string>(src, "source_id", "");
                                 data["comic_id"] = memochat::json::glaze_safe_get<std::string>(src, "comic_id", "");
                                 data["chapter_id"] =
                                     memochat::json::glaze_safe_get<std::string>(src, "chapter_id", "");
                                 data["page_index"] = memochat::json::glaze_safe_get<int64_t>(src, "page_index", 0LL);
                                 WriteOk(root, data);
                                 return true;
                             });
}

bool R18Service::HandleHistory(const memochat::gate::routing::GateRequest& request,
                               memochat::gate::routing::GateResponse& response)
{
    JsonValue root;
    const int uid = std::atoi(QueryParam(request, "uid").c_str());
    const std::string token = QueryParam(request, "token");
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
    WriteGetJson(response, root);
    return true;
}

bool R18Service::HandleImage(const memochat::gate::routing::GateRequest& request,
                             memochat::gate::routing::GateResponse& response)
{
    const int uid = std::atoi(QueryParam(request, "uid").c_str());
    const std::string token = QueryParam(request, "token");
    if (!ValidateUserToken(uid, token))
    {
        response.status = 401;
        response.content_type = "text/plain";
        response.body = "token invalid";
        return true;
    }

    const std::string source_id = QueryParam(request, "source_id", kDefaultR18SourceId);
    const std::string image_url = QueryParam(request, "image_url");
    try
    {
        auto payload = memochat::r18::R18SourceService::Instance().FetchImage(source_id, image_url);
        response.status = 200;
        response.content_type = payload.content_type;
        response.body = payload.body;
    }
    catch (const std::exception& exc)
    {
        response.status = 502;
        response.content_type = "text/plain";
        response.body = std::string("image fetch failed: ") + exc.what();
    }
    return true;
}

} // namespace memochat::gate::services::r18
