#include "services/r18/R18Service.hpp"

#include "ConfigMgr.hpp"
#include "RedisMgr.hpp"
#include "const.hpp"
#include "json/GlazeCompat.hpp"
#include "r18/R18PublicDtos.hpp"
#include "r18/R18SourceRecordCodec.hpp"
#include "r18/R18SourceService.hpp"
#include "services/account/AccountPersistence.hpp"
#include "support/BearerAccessAuth.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <string>
#include <string_view>

import memochat.r18.service_algorithms;

namespace memochat::gate::services::r18
{
namespace
{

using memochat::gate::services::account::R18AccessPolicy;
using memochat::gate::services::account::R18AccessState;
using memochat::json::JsonValue;

enum class R18AccessDecision
{
    Allowed,
    Denied,
    Unavailable,
};

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

const char* AccessStateName(R18AccessState state)
{
    switch (state)
    {
        case R18AccessState::Allowed:
            return "allowed";
        case R18AccessState::Revoked:
            return "revoked";
        case R18AccessState::Denied:
        default:
            return "denied";
    }
}

JsonValue R18AccessPolicyData(const R18AccessPolicy& policy)
{
    JsonValue data;
    data["allowed"] = policy.Allowed();
    data["adult_attested_at_ms"] = policy.adult_attested_at_ms;
    data["state"] = AccessStateName(policy.r18_access_state);
    data["can_attest"] = policy.r18_access_state != R18AccessState::Revoked;
    return data;
}

R18AccessDecision RequireR18Access(int uid, JsonValue& root)
{
    R18AccessPolicy policy;
    if (!memochat::gate::services::account::AccountPersistence::Instance().GetR18AccessPolicy(uid, policy))
    {
        root["error"] = ErrorCodes::RPCFailed;
        root["message"] = "R18 access policy is temporarily unavailable";
        return R18AccessDecision::Unavailable;
    }
    if (!policy.Allowed())
    {
        root["error"] = ErrorCodes::R18AccessDenied;
        root["message"] = policy.r18_access_state == R18AccessState::Revoked ? "R18 access has been revoked"
                                                                             : "adult attestation is required";
        root["data"] = R18AccessPolicyData(policy);
        return R18AccessDecision::Denied;
    }
    return R18AccessDecision::Allowed;
}

int AccessFailureStatus(R18AccessDecision decision)
{
    return decision == R18AccessDecision::Unavailable ? 503 : 403;
}

int64_t EpochMilliseconds()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::string TrimCopy(std::string value)
{
    const auto is_space = [](unsigned char ch)
    {
        return std::isspace(ch) != 0;
    };
    value.erase(value.begin(),
                std::find_if(value.begin(),
                             value.end(),
                             [&](char ch)
                             {
                                 return !is_space(static_cast<unsigned char>(ch));
                             }));
    value.erase(std::find_if(value.rbegin(),
                             value.rend(),
                             [&](char ch)
                             {
                                 return !is_space(static_cast<unsigned char>(ch));
                             })
                    .base(),
                value.end());
    return value;
}

bool ConstantTimeEquals(std::string_view left, std::string_view right)
{
    unsigned char difference = static_cast<unsigned char>(left.size() ^ right.size());
    const std::size_t max_size = std::max(left.size(), right.size());
    for (std::size_t index = 0; index < max_size; ++index)
    {
        const unsigned char left_value = index < left.size() ? static_cast<unsigned char>(left[index]) : 0;
        const unsigned char right_value = index < right.size() ? static_cast<unsigned char>(right[index]) : 0;
        difference |= static_cast<unsigned char>(left_value ^ right_value);
    }
    return difference == 0;
}

std::string SourceAdminHeader()
{
    std::string header = TrimCopy(ConfigMgr::Inst().GetValue("R18SourceAdmin", "AuthHeader"));
    return header.empty() ? memochat::r18::service::modules::DefaultSourceAdminHeader() : header;
}

bool RequireSourceAdmin(const memochat::gate::routing::GateRequest& request, JsonValue& root)
{
    const std::string configured_key = TrimCopy(ConfigMgr::Inst().GetValue("R18SourceAdmin", "AdminKey"));
    const std::string supplied_key =
        TrimCopy(memochat::auth::FindHeaderValueCaseInsensitive(request.headers, SourceAdminHeader()));
    const bool matches = ConstantTimeEquals(configured_key, supplied_key);
    if (memochat::r18::service::modules::ShouldRejectSourceAdminAuth(!configured_key.empty(),
                                                                     supplied_key.empty(),
                                                                     matches))
    {
        root["error"] = ErrorCodes::TokenInvalid;
        root["message"] = memochat::r18::service::modules::SourceAdminRequiredMessage();
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
        response.status = memochat::r18::service::modules::UnauthorizedHttpStatus();
        response.headers["Cache-Control"] = "no-store";
        return true;
    }

    const auto access = RequireR18Access(uid, root);
    if (access != R18AccessDecision::Allowed)
    {
        WritePostJson(response, root);
        response.status = AccessFailureStatus(access);
        response.headers["Cache-Control"] = "no-store";
        return true;
    }

    fn(src_root, root, request.trace_id, uid);
    root["trace_id"] = request.trace_id;
    WritePostJson(response, root);
    return true;
}

bool HandleAdminJsonRequest(const memochat::gate::routing::GateRequest& request,
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
        response.status = memochat::r18::service::modules::UnauthorizedHttpStatus();
        response.headers["Cache-Control"] = "no-store";
        return true;
    }
    if (!RequireSourceAdmin(request, root))
    {
        WritePostJson(response, root);
        response.status = memochat::r18::service::modules::ForbiddenHttpStatus();
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

bool R18Service::HandleAccessStatus(const memochat::gate::routing::GateRequest& request,
                                    memochat::gate::routing::GateResponse& response)
{
    JsonValue root;
    int uid = 0;
    if (!RequireBearerAuth(request, root, uid))
    {
        WriteGetJson(response, root);
        response.status = 401;
        response.headers["Cache-Control"] = "no-store";
        return true;
    }

    R18AccessPolicy policy;
    if (!memochat::gate::services::account::AccountPersistence::Instance().GetR18AccessPolicy(uid, policy))
    {
        root["error"] = ErrorCodes::RPCFailed;
        root["message"] = "R18 access policy is temporarily unavailable";
        WriteGetJson(response, root);
        response.status = 503;
        response.headers["Cache-Control"] = "no-store";
        return true;
    }

    WriteOk(root, R18AccessPolicyData(policy));
    WriteGetJson(response, root);
    response.headers["Cache-Control"] = "no-store";
    return true;
}

bool R18Service::HandleAccessAttest(const memochat::gate::routing::GateRequest& request,
                                    memochat::gate::routing::GateResponse& response)
{
    JsonValue root;
    int uid = 0;
    if (!RequireBearerAuth(request, root, uid))
    {
        WritePostJson(response, root);
        response.status = 401;
        response.headers["Cache-Control"] = "no-store";
        return true;
    }

    R18AccessPolicy policy;
    if (!memochat::gate::services::account::AccountPersistence::Instance().AttestAdultForR18(uid,
                                                                                             EpochMilliseconds(),
                                                                                             policy))
    {
        root["error"] = ErrorCodes::RPCFailed;
        root["message"] = "adult attestation could not be persisted";
        WritePostJson(response, root);
        response.status = 503;
        response.headers["Cache-Control"] = "no-store";
        return true;
    }
    if (!policy.Allowed())
    {
        root["error"] = ErrorCodes::R18AccessDenied;
        root["message"] = "R18 access has been revoked";
        root["data"] = R18AccessPolicyData(policy);
        WritePostJson(response, root);
        response.status = 403;
        response.headers["Cache-Control"] = "no-store";
        return true;
    }

    WriteOk(root, R18AccessPolicyData(policy));
    WritePostJson(response, root);
    response.headers["Cache-Control"] = "no-store";
    return true;
}

bool R18Service::HandleListSources(const memochat::gate::routing::GateRequest& request,
                                   memochat::gate::routing::GateResponse& response)
{
    JsonValue root;
    int uid = 0;
    if (!RequireBearerAuth(request, root, uid))
    {
        WriteGetJson(response, root);
        response.status = memochat::r18::service::modules::UnauthorizedHttpStatus();
        response.headers["Cache-Control"] = "no-store";
        return true;
    }
    else if (const auto access = RequireR18Access(uid, root); access != R18AccessDecision::Allowed)
    {
        WriteGetJson(response, root);
        response.status = AccessFailureStatus(access);
        response.headers["Cache-Control"] = "no-store";
        return true;
    }
    else
    {
        JsonValue data;
        data["sources"] = memochat::r18::R18SourceService::Instance().ListSourcesForUser(uid);
        WriteOk(root, data);
    }
    WriteGetJson(response, root);
    return true;
}

bool R18Service::HandleImportSource(const memochat::gate::routing::GateRequest& request,
                                    memochat::gate::routing::GateResponse& response)
{
    return HandleAdminJsonRequest(
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
            const bool decode_ok =
                !encoded.empty() &&
                memochat::r18::DecodeBase64Bounded(encoded, binary, memochat::r18::SourceImportLimitBytes());
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
            data["source"] = memochat::r18::R18SourceRecordToPublicJsonValue(rec);
            WriteOk(root, data);
            return true;
        });
}

bool R18Service::HandleEnableSource(const memochat::gate::routing::GateRequest& request,
                                    memochat::gate::routing::GateResponse& response)
{
    return HandleAdminJsonRequest(
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
    return HandleAdminJsonRequest(
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
    return HandleAdminJsonRequest(
        request,
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
    return HandleJsonRequest(request,
                             response,
                             [](const JsonValue& src, JsonValue& root, const std::string&, int uid)
                             {
                                 const auto body = memochat::r18::R18SearchRequestFromJsonValue(src);
                                 WriteOk(root,
                                         memochat::r18::R18SourceService::Instance().SearchForUser(uid,
                                                                                                   body.source_id,
                                                                                                   body.keyword,
                                                                                                   body.page));
                                 return true;
                             });
}

bool R18Service::HandleComicDetail(const memochat::gate::routing::GateRequest& request,
                                   memochat::gate::routing::GateResponse& response)
{
    return HandleJsonRequest(
        request,
        response,
        [](const JsonValue& src, JsonValue& root, const std::string&, int uid)
        {
            const auto body = memochat::r18::R18ComicDetailRequestFromJsonValue(src);
            WriteOk(root,
                    memochat::r18::R18SourceService::Instance().DetailForUser(uid, body.source_id, body.comic_id));
            return true;
        });
}

bool R18Service::HandleChapterPages(const memochat::gate::routing::GateRequest& request,
                                    memochat::gate::routing::GateResponse& response)
{
    return HandleJsonRequest(
        request,
        response,
        [](const JsonValue& src, JsonValue& root, const std::string&, int uid)
        {
            const auto body = memochat::r18::R18ChapterPagesRequestFromJsonValue(src);
            WriteOk(root,
                    memochat::r18::R18SourceService::Instance().PagesForUser(uid, body.source_id, body.chapter_id));
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
        WriteGetJson(response, root);
        response.status = memochat::r18::service::modules::UnauthorizedHttpStatus();
        response.headers["Cache-Control"] = "no-store";
        return true;
    }
    else if (const auto access = RequireR18Access(uid, root); access != R18AccessDecision::Allowed)
    {
        WriteGetJson(response, root);
        response.status = AccessFailureStatus(access);
        response.headers["Cache-Control"] = "no-store";
        return true;
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
    const auto access = RequireR18Access(uid, root);
    if (access != R18AccessDecision::Allowed)
    {
        response.status = AccessFailureStatus(access);
        response.content_type = memochat::r18::service::modules::PlainTextContentType();
        response.body = memochat::json::glaze_safe_get<std::string>(root, "message", "R18 access denied");
        response.headers["Cache-Control"] = "no-store";
        return true;
    }
    const std::string source_id = QueryParam(request, "source_id", memochat::r18::service::modules::DefaultSourceId());
    const std::string image_url = QueryParam(request, "image_url");
    auto payload = memochat::r18::R18SourceService::Instance().FetchImageForUser(uid, source_id, image_url);
    if (!payload.ok)
    {
        response.status = memochat::r18::service::modules::BadGatewayHttpStatus();
        response.content_type = memochat::r18::service::modules::PlainTextContentType();
        response.body = std::string(memochat::r18::service::modules::ImageFetchFailedPrefix()) + payload.error;
        return true;
    }
    response.status = memochat::r18::service::modules::SuccessHttpStatus();
    response.content_type = payload.content_type;
    response.body = std::move(payload.body);
    return true;
}

bool R18Service::HandleListAccounts(const memochat::gate::routing::GateRequest& request,
                                    memochat::gate::routing::GateResponse& response)
{
    JsonValue root;
    int uid = 0;
    if (!RequireBearerAuth(request, root, uid))
    {
        WriteGetJson(response, root);
        response.status = memochat::r18::service::modules::UnauthorizedHttpStatus();
        response.headers["Cache-Control"] = "no-store";
        return true;
    }
    if (const auto access = RequireR18Access(uid, root); access != R18AccessDecision::Allowed)
    {
        WriteGetJson(response, root);
        response.status = AccessFailureStatus(access);
        response.headers["Cache-Control"] = "no-store";
        return true;
    }
    WriteOk(root, memochat::r18::R18SourceService::Instance().ListAccounts(uid));
    WriteGetJson(response, root);
    response.headers["Cache-Control"] = "no-store";
    return true;
}

bool R18Service::HandleSaveAccount(const memochat::gate::routing::GateRequest& request,
                                   memochat::gate::routing::GateResponse& response)
{
    return HandleJsonRequest(
        request,
        response,
        [](const JsonValue& src, JsonValue& root, const std::string&, int uid)
        {
            const std::string source_id = memochat::json::glaze_safe_get<std::string>(src, "source_id", "");
            const std::string username = memochat::json::glaze_safe_get<std::string>(src, "username", "");
            const std::string password = memochat::json::glaze_safe_get<std::string>(src, "password", "");
            std::string error;
            if (!memochat::r18::R18SourceService::Instance().SaveAccount(uid, source_id, username, password, &error))
            {
                root["error"] = ErrorCodes::Error_Json;
                root["message"] = error.empty() ? "failed to save account" : error;
                return true;
            }
            // Credentials are persisted even when remote login fails; for auth-required
            // sources surface the login error so the unified account manager can prompt retry.
            auto accounts = memochat::r18::R18SourceService::Instance().ListAccounts(uid);
            if (!error.empty())
            {
                root["error"] = ErrorCodes::Error_Json;
                root["message"] = error;
                root["data"] = std::move(accounts);
                return true;
            }
            WriteOk(root, accounts);
            return true;
        });
}

bool R18Service::HandleLoginAccount(const memochat::gate::routing::GateRequest& request,
                                    memochat::gate::routing::GateResponse& response)
{
    return HandleJsonRequest(
        request,
        response,
        [](const JsonValue& src, JsonValue& root, const std::string&, int uid)
        {
            const std::string source_id = memochat::json::glaze_safe_get<std::string>(src, "source_id", "");
            const std::string username = memochat::json::glaze_safe_get<std::string>(src, "username", "");
            const std::string password = memochat::json::glaze_safe_get<std::string>(src, "password", "");
            std::string error;
            bool ok = true;
            if (!username.empty() || !password.empty())
            {
                ok =
                    memochat::r18::R18SourceService::Instance().SaveAccount(uid, source_id, username, password, &error);
            }
            else
            {
                ok = memochat::r18::R18SourceService::Instance().LoginAccount(uid, source_id, &error);
            }
            auto accounts = memochat::r18::R18SourceService::Instance().ListAccounts(uid);
            if (!ok || !error.empty())
            {
                root["error"] = ErrorCodes::Error_Json;
                root["message"] = error.empty() ? "login failed" : error;
                root["data"] = std::move(accounts);
                return true;
            }
            WriteOk(root, accounts);
            return true;
        });
}

bool R18Service::HandleClearAccount(const memochat::gate::routing::GateRequest& request,
                                    memochat::gate::routing::GateResponse& response)
{
    return HandleJsonRequest(request,
                             response,
                             [](const JsonValue& src, JsonValue& root, const std::string&, int uid)
                             {
                                 const std::string source_id =
                                     memochat::json::glaze_safe_get<std::string>(src, "source_id", "");
                                 std::string error;
                                 if (!memochat::r18::R18SourceService::Instance().ClearAccount(uid, source_id, &error))
                                 {
                                     root["error"] = ErrorCodes::Error_Json;
                                     root["message"] = error.empty() ? "failed to clear account" : error;
                                     return true;
                                 }
                                 WriteOk(root, memochat::r18::R18SourceService::Instance().ListAccounts(uid));
                                 return true;
                             });
}

} // namespace memochat::gate::services::r18
