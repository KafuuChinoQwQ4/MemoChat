#include "services/r18/R18Service.hpp"

#include "RedisMgr.hpp"
#include "const.hpp"
#include "json/GlazeCompat.hpp"
#include "r18/R18PublicDtos.hpp"
#include "r18/R18SourceRecordCodec.hpp"
#include "r18/R18SourceService.hpp"
#include "support/BearerAccessAuth.hpp"

#include <cstdlib>
#include <exception>
#include <functional>
#include <string>

import memochat.r18.service_algorithms;

namespace memochat::gate::services::r18
{
namespace
{

using memochat::json::JsonValue;

std::string QueryParam(const memochat::gate::routing::GateRequest& request,
                       const std::string& key,
                       const std::string& fallback = {})
{
    const auto it = request.query.find(key);
    return it == request.query.end() ? fallback : it->second;
}

bool RequireBearerAuth(const memochat::gate::routing::GateRequest& request, JsonValue& root, int& uid)
{
    if (!memochat::auth::ResolveBearerAccessUserId(request, uid))
    {
        root["error"] = ErrorCodes::TokenInvalid;
        root["message"] = memochat::r18::service::modules::TokenInvalidMessage();
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

void WriteJson(memochat::gate::routing::GateResponse& response, const JsonValue& root, const char* content_type)
{
    response.status = memochat::r18::service::modules::SuccessHttpStatus();
    response.content_type = content_type;
    response.body = memochat::json::glaze_stringify(root);
}

void WriteGetJson(memochat::gate::routing::GateResponse& response, const JsonValue& root)
{
    WriteJson(response, root, memochat::r18::service::modules::GetJsonContentType());
}

void WritePostJson(memochat::gate::routing::GateResponse& response, const JsonValue& root)
{
    WriteJson(response, root, memochat::r18::service::modules::PostJsonContentType());
}

bool HandleJsonRequest(const memochat::gate::routing::GateRequest& request,
                       memochat::gate::routing::GateResponse& response,
                       const std::function<bool(const JsonValue&, JsonValue&, const std::string&, int)>& fn)
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

    int uid = 0;
    if (!RequireBearerAuth(request, root, uid))
    {
        WritePostJson(response, root);
        return true;
    }

    fn(src_root, root, request.trace_id, uid);
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
    int uid = 0;
    if (!RequireBearerAuth(request, root, uid))
    {
        // root already populated by RequireBearerAuth.
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
        [](const JsonValue& src, JsonValue& root, const std::string&, int)
        {
            const std::string file_name =
                memochat::json::glaze_safe_get<std::string>(src,
                                                            memochat::r18::service::modules::ImportFileNameField(),
                                                            memochat::r18::service::modules::DefaultImportFileName());
            const std::string encoded =
                memochat::json::glaze_safe_get<std::string>(src,
                                                            memochat::r18::service::modules::ImportDataBase64Field(),
                                                            memochat::r18::service::modules::EmptyFieldDefault());
            const std::string manifest_json =
                memochat::json::glaze_safe_get<std::string>(src,
                                                            memochat::r18::service::modules::ImportManifestJsonField(),
                                                            memochat::r18::service::modules::EmptyFieldDefault());
            std::string binary;
            const bool decode_ok = !encoded.empty() && memochat::r18::DecodeBase64(encoded, binary);
            if (memochat::r18::service::modules::ShouldRejectImportPayload(encoded.empty(), decode_ok))
            {
                root["error"] = ErrorCodes::Error_Json;
                root["message"] = memochat::r18::service::modules::InvalidPluginPackagePayloadMessage();
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
            data["source"] = memochat::r18::R18SourceRecordToJsonValue(rec);
            WriteOk(root, data);
            return true;
        });
}

bool R18Service::HandleEnableSource(const memochat::gate::routing::GateRequest& request,
                                    memochat::gate::routing::GateResponse& response)
{
    return HandleJsonRequest(
        request,
        response,
        [](const JsonValue& src, JsonValue& root, const std::string&, int)
        {
            const auto body = memochat::r18::R18SourceToggleRequestFromJsonValue(src);

            std::string error;
            if (!memochat::r18::R18SourceService::Instance().EnableSource(body.source_id, true, &error))
            {
                root["error"] = ErrorCodes::Error_Json;
                root["message"] = error;
                return true;
            }

            memochat::r18::R18SourceToggleResponseDto toggle_response;
            toggle_response.source_id = body.source_id;
            toggle_response.enabled = true;
            WriteOk(root, memochat::r18::R18SourceToggleResponseToJsonValue(toggle_response));
            return true;
        });
}

bool R18Service::HandleDisableSource(const memochat::gate::routing::GateRequest& request,
                                     memochat::gate::routing::GateResponse& response)
{
    return HandleJsonRequest(
        request,
        response,
        [](const JsonValue& src, JsonValue& root, const std::string&, int)
        {
            const auto body = memochat::r18::R18SourceToggleRequestFromJsonValue(src);

            std::string error;
            if (!memochat::r18::R18SourceService::Instance().EnableSource(body.source_id, false, &error))
            {
                root["error"] = ErrorCodes::Error_Json;
                root["message"] = error;
                return true;
            }

            memochat::r18::R18SourceToggleResponseDto toggle_response;
            toggle_response.source_id = body.source_id;
            toggle_response.enabled = false;
            WriteOk(root, memochat::r18::R18SourceToggleResponseToJsonValue(toggle_response));
            return true;
        });
}

bool R18Service::HandleDeleteSource(const memochat::gate::routing::GateRequest& request,
                                    memochat::gate::routing::GateResponse& response)
{
    return HandleJsonRequest(request,
                             response,
                             [](const JsonValue& src, JsonValue& root, const std::string&, int)
                             {
                                 const auto body = memochat::r18::R18SourceToggleRequestFromJsonValue(src);
                                 std::string error;
                                 if (!memochat::r18::R18SourceService::Instance().DeleteSource(body.source_id, &error))
                                 {
                                     root["error"] = ErrorCodes::Error_Json;
                                     root["message"] = error;
                                     return true;
                                 }
                                 memochat::r18::R18SourceToggleResponseDto resp;
                                 resp.source_id = body.source_id;
                                 WriteOk(root, memochat::r18::R18SourceToggleResponseToJsonValue(resp));
                                 return true;
                             });
}

bool R18Service::HandleSearch(const memochat::gate::routing::GateRequest& request,
                              memochat::gate::routing::GateResponse& response)
{
    return HandleJsonRequest(
        request,
        response,
        [](const JsonValue& src, JsonValue& root, const std::string&, int)
        {
            const auto body = memochat::r18::R18SearchRequestFromJsonValue(src);
            WriteOk(root, memochat::r18::R18SourceService::Instance().Search(body.source_id, body.keyword, body.page));
            return true;
        });
}

bool R18Service::HandleComicDetail(const memochat::gate::routing::GateRequest& request,
                                   memochat::gate::routing::GateResponse& response)
{
    return HandleJsonRequest(
        request,
        response,
        [](const JsonValue& src, JsonValue& root, const std::string&, int)
        {
            const auto body = memochat::r18::R18ComicDetailRequestFromJsonValue(src);
            WriteOk(root, memochat::r18::R18SourceService::Instance().Detail(body.source_id, body.comic_id));
            return true;
        });
}

bool R18Service::HandleChapterPages(const memochat::gate::routing::GateRequest& request,
                                    memochat::gate::routing::GateResponse& response)
{
    return HandleJsonRequest(
        request,
        response,
        [](const JsonValue& src, JsonValue& root, const std::string&, int)
        {
            const auto body = memochat::r18::R18ChapterPagesRequestFromJsonValue(src);
            WriteOk(root, memochat::r18::R18SourceService::Instance().Pages(body.source_id, body.chapter_id));
            return true;
        });
}

bool R18Service::HandleFavoriteToggle(const memochat::gate::routing::GateRequest& request,
                                      memochat::gate::routing::GateResponse& response)
{
    return HandleJsonRequest(request,
                             response,
                             [](const JsonValue& src, JsonValue& root, const std::string&, int)
                             {
                                 const auto body = memochat::r18::R18FavoriteToggleRequestFromJsonValue(src);
                                 memochat::r18::R18FavoriteToggleResponseDto resp;
                                 resp.source_id = body.source_id;
                                 resp.comic_id = body.comic_id;
                                 resp.favorited = body.favorited;
                                 WriteOk(root, memochat::r18::R18FavoriteToggleResponseToJsonValue(resp));
                                 return true;
                             });
}

bool R18Service::HandleHistoryUpdate(const memochat::gate::routing::GateRequest& request,
                                     memochat::gate::routing::GateResponse& response)
{
    return HandleJsonRequest(request,
                             response,
                             [](const JsonValue& src, JsonValue& root, const std::string&, int)
                             {
                                 const auto body = memochat::r18::R18HistoryUpdateRequestFromJsonValue(src);
                                 memochat::r18::R18HistoryUpdateResponseDto resp;
                                 resp.source_id = body.source_id;
                                 resp.comic_id = body.comic_id;
                                 resp.chapter_id = body.chapter_id;
                                 resp.page_index = body.page_index;
                                 WriteOk(root, memochat::r18::R18HistoryUpdateResponseToJsonValue(resp));
                                 return true;
                             });
}

bool R18Service::HandleHistory(const memochat::gate::routing::GateRequest& request,
                               memochat::gate::routing::GateResponse& response)
{
    JsonValue root;
    int uid = 0;
    if (!RequireBearerAuth(request, root, uid))
    {
        // root already populated by RequireBearerAuth.
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
    JsonValue root;
    int uid = 0;
    if (!RequireBearerAuth(request, root, uid))
    {
        response.status = memochat::r18::service::modules::UnauthorizedHttpStatus();
        response.content_type = memochat::r18::service::modules::PlainTextContentType();
        response.body = memochat::r18::service::modules::TokenInvalidMessage();
        return true;
    }
    const std::string source_id = QueryParam(request, "source_id", memochat::r18::service::modules::DefaultSourceId());
    const std::string image_url = QueryParam(request, "image_url");
    try
    {
        auto payload = memochat::r18::R18SourceService::Instance().FetchImage(source_id, image_url);
        response.status = memochat::r18::service::modules::SuccessHttpStatus();
        response.content_type = payload.content_type;
        response.body = std::move(payload.body);
    }
    catch (const std::exception& exc)
    {
        response.status = memochat::r18::service::modules::BadGatewayHttpStatus();
        response.content_type = memochat::r18::service::modules::PlainTextContentType();
        response.body = std::string(memochat::r18::service::modules::ImageFetchFailedPrefix()) + exc.what();
    }
    return true;
}

} // namespace memochat::gate::services::r18
