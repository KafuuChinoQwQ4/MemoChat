#include "services/r18/R18Service.hpp"

#include "ConfigMgr.hpp"
#include "RedisMgr.hpp"
#include "const.hpp"
#include "json/GlazeCompat.hpp"
#include "r18/R18PublicDtos.hpp"
#include "r18/R18SourceRecordCodec.hpp"
#include "r18/R18SourceService.hpp"
#include "r18/R18LibraryStore.hpp"
#include "r18/R18BrowserImportService.hpp"
#include "r18/R18EhentaiSessionService.hpp"
#include "r18/R18SourceCredentialStore.hpp"
#include "services/account/AccountPersistence.hpp"
#include "support/BearerAccessAuth.hpp"

#include <algorithm>
#include <atomic>
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

std::atomic<int>& ActiveImageFetches()
{
    static std::atomic<int> active{0};
    return active;
}

class ImageFetchSlot
{
public:
    bool TryAcquire()
    {
        int active = ActiveImageFetches().load(std::memory_order_relaxed);
        while (memochat::r18::service::modules::ShouldAdmitImageFetch(active))
        {
            if (ActiveImageFetches().compare_exchange_weak(active,
                                                           active + 1,
                                                           std::memory_order_acquire,
                                                           std::memory_order_relaxed))
            {
                acquired_ = true;
                return true;
            }
        }
        return false;
    }

    ~ImageFetchSlot()
    {
        if (acquired_)
            ActiveImageFetches().fetch_sub(1, std::memory_order_release);
    }

private:
    bool acquired_ = false;
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
    return HandleJsonRequest(
        request,
        response,
        [](const JsonValue& src, JsonValue& root, const std::string&, int uid)
        {
            const auto body = memochat::r18::R18SearchRequestFromJsonValue(src);
            WriteOk(root,
                    memochat::r18::R18SourceService::Instance()
                        .SearchForUser(uid, body.source_id, body.keyword, body.page, body.sort, body.tag));
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

bool R18Service::HandleVideoResolve(const memochat::gate::routing::GateRequest& request,
                                    memochat::gate::routing::GateResponse& response)
{
    const bool handled = HandleJsonRequest(
        request,
        response,
        [](const JsonValue& src, JsonValue& root, const std::string&, int uid)
        {
            memochat::r18::R18VideoResolveRequestDto body;
            std::string error;
            if (!memochat::r18::DecodeR18VideoResolveRequest(memochat::json::glaze_stringify(src), &body, &error))
            {
                root["error"] = ErrorCodes::Error_Json;
                root["message"] = error;
                return true;
            }

            JsonValue data;
            if (!memochat::r18::R18SourceService::Instance()
                     .ResolveVideoForUser(uid, body.source_id, body.chapter_id, &data, &error))
            {
                root["error"] = ErrorCodes::Error_Json;
                root["message"] = error;
                return true;
            }
            WriteOk(root, data);
            return true;
        });
    response.headers["Cache-Control"] = "no-store";
    return handled;
}

bool R18Service::HandleFavoriteToggle(const memochat::gate::routing::GateRequest& request,
                                      memochat::gate::routing::GateResponse& response)
{
    return HandleJsonRequest(request,
                             response,
                             [](const JsonValue& src, JsonValue& root, const std::string&, int uid)
                             {
                                 const auto body = memochat::r18::R18FavoriteToggleRequestFromJsonValue(src);
                                 JsonValue data;
                                 std::string error;
                                 if (!memochat::r18::R18LibraryStore::Instance().ToggleFavorite(uid,
                                                                                                body.source_id,
                                                                                                body.comic_id,
                                                                                                body.favorited,
                                                                                                body.title,
                                                                                                body.cover,
                                                                                                body.author,
                                                                                                body.subtitle,
                                                                                                body.folder_ids,
                                                                                                &data,
                                                                                                &error))
                                 {
                                     root["error"] = ErrorCodes::Error_Json;
                                     root["message"] = error.empty() ? "favorite toggle failed" : error;
                                     return false;
                                 }
                                 WriteOk(root, data);
                                 return true;
                             });
}

bool R18Service::HandleLibrary(const memochat::gate::routing::GateRequest& request,
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
    WriteOk(root, memochat::r18::R18LibraryStore::Instance().ListLibrary(uid));
    root["trace_id"] = request.trace_id;
    WriteGetJson(response, root);
    response.headers["Cache-Control"] = "no-store";
    return true;
}

bool R18Service::HandleFavorites(const memochat::gate::routing::GateRequest& request,
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
    const std::string folder_id = QueryParam(request, "folder_id");
    WriteOk(root, memochat::r18::R18LibraryStore::Instance().ListFavorites(uid, folder_id));
    root["trace_id"] = request.trace_id;
    WriteGetJson(response, root);
    response.headers["Cache-Control"] = "no-store";
    return true;
}

bool R18Service::HandleFolderCreate(const memochat::gate::routing::GateRequest& request,
                                    memochat::gate::routing::GateResponse& response)
{
    return HandleJsonRequest(request,
                             response,
                             [](const JsonValue& src, JsonValue& root, const std::string&, int uid)
                             {
                                 const std::string name = memochat::json::glaze_safe_get<std::string>(src, "name", "");
                                 JsonValue data;
                                 std::string error;
                                 if (!memochat::r18::R18LibraryStore::Instance().CreateFolder(uid, name, &data, &error))
                                 {
                                     root["error"] = ErrorCodes::Error_Json;
                                     root["message"] = error.empty() ? "create folder failed" : error;
                                     return false;
                                 }
                                 WriteOk(root, data);
                                 return true;
                             });
}

bool R18Service::HandleFolderRename(const memochat::gate::routing::GateRequest& request,
                                    memochat::gate::routing::GateResponse& response)
{
    return HandleJsonRequest(
        request,
        response,
        [](const JsonValue& src, JsonValue& root, const std::string&, int uid)
        {
            const std::string folder_id = memochat::json::glaze_safe_get<std::string>(src, "folder_id", "");
            const std::string name = memochat::json::glaze_safe_get<std::string>(src, "name", "");
            JsonValue data;
            std::string error;
            if (!memochat::r18::R18LibraryStore::Instance().RenameFolder(uid, folder_id, name, &data, &error))
            {
                root["error"] = ErrorCodes::Error_Json;
                root["message"] = error.empty() ? "rename folder failed" : error;
                return false;
            }
            WriteOk(root, data);
            return true;
        });
}

bool R18Service::HandleFolderDelete(const memochat::gate::routing::GateRequest& request,
                                    memochat::gate::routing::GateResponse& response)
{
    return HandleJsonRequest(
        request,
        response,
        [](const JsonValue& src, JsonValue& root, const std::string&, int uid)
        {
            const std::string folder_id = memochat::json::glaze_safe_get<std::string>(src, "folder_id", "");
            JsonValue data;
            std::string error;
            if (!memochat::r18::R18LibraryStore::Instance().DeleteFolder(uid, folder_id, &data, &error))
            {
                root["error"] = ErrorCodes::Error_Json;
                root["message"] = error.empty() ? "delete folder failed" : error;
                return false;
            }
            WriteOk(root, data);
            return true;
        });
}

bool R18Service::HandleFavoriteAssign(const memochat::gate::routing::GateRequest& request,
                                      memochat::gate::routing::GateResponse& response)
{
    return HandleJsonRequest(
        request,
        response,
        [](const JsonValue& src, JsonValue& root, const std::string&, int uid)
        {
            const std::string source_id = memochat::json::glaze_safe_get<std::string>(src, "source_id", "");
            const std::string comic_id = memochat::json::glaze_safe_get<std::string>(src, "comic_id", "");
            std::vector<std::string> folder_ids;
            const auto folders = memochat::json::glaze_get(src, "folder_ids");
            if (const auto* arr = memochat::json::glaze_get_array(folders))
            {
                for (const auto& entry : *arr)
                {
                    JsonValue v(entry);
                    if (v.isString())
                    {
                        const std::string id = v.asString();
                        if (!id.empty())
                            folder_ids.push_back(id);
                    }
                }
            }
            const std::string single = memochat::json::glaze_safe_get<std::string>(src, "folder_id", "");
            if (!single.empty() && std::find(folder_ids.begin(), folder_ids.end(), single) == folder_ids.end())
                folder_ids.push_back(single);

            JsonValue data;
            std::string error;
            if (!memochat::r18::R18LibraryStore::Instance()
                     .AssignFolders(uid, source_id, comic_id, folder_ids, &data, &error))
            {
                root["error"] = ErrorCodes::Error_Json;
                root["message"] = error.empty() ? "assign folders failed" : error;
                return false;
            }
            WriteOk(root, data);
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
    const std::string scramble_id_text = QueryParam(request, "scramble_id");
    long long scramble_id = 0;
    if (!scramble_id_text.empty())
    {
        char* end = nullptr;
        const long long parsed = std::strtoll(scramble_id_text.c_str(), &end, 10);
        if (end != scramble_id_text.c_str() && end != nullptr && *end == '\0' && parsed > 0 && parsed <= 1000000000LL)
            scramble_id = parsed;
    }

    ImageFetchSlot slot;
    if (!slot.TryAcquire())
    {
        response.status = memochat::r18::service::modules::ServiceUnavailableHttpStatus();
        response.content_type = memochat::r18::service::modules::PlainTextContentType();
        response.body = memochat::r18::service::modules::ImageBusyMessage();
        response.headers["Cache-Control"] = "no-store";
        response.headers["Retry-After"] = "1";
        return true;
    }
    auto payload =
        memochat::r18::R18SourceService::Instance().FetchImageForUser(uid, source_id, image_url, scramble_id);
    if (!payload.ok)
    {
        response.status = memochat::r18::service::modules::BadGatewayHttpStatus();
        response.content_type = memochat::r18::service::modules::PlainTextContentType();
        response.body = std::string(memochat::r18::service::modules::ImageFetchFailedPrefix()) + payload.error;
        response.headers["Cache-Control"] = "no-store";
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

bool R18Service::HandleCheckin(const memochat::gate::routing::GateRequest& request,
                               memochat::gate::routing::GateResponse& response)
{
    return HandleJsonRequest(
        request,
        response,
        [](const JsonValue& src, JsonValue& root, const std::string&, int uid)
        {
            const std::string source_id = memochat::json::glaze_safe_get<std::string>(src, "source_id", "jm.official");
            const auto result = memochat::r18::R18SourceService::Instance().CheckinForUser(uid, source_id);
            const std::string status = memochat::json::glaze_safe_get<std::string>(result, "status", "error");
            const bool ok = status == "ok" || status == "already";
            if (!ok)
            {
                root["error"] = ErrorCodes::Error_Json;
                root["message"] = memochat::json::glaze_safe_get<std::string>(result, "message", "check-in failed");
                root["data"] = result;
                return true;
            }
            WriteOk(root, result);
            return true;
        });
}

bool R18Service::HandleBrowserImportStart(const memochat::gate::routing::GateRequest& request,
                                          memochat::gate::routing::GateResponse& response)
{
    return HandleJsonRequest(
        request,
        response,
        [](const JsonValue& src, JsonValue& root, const std::string&, int uid)
        {
            memochat::r18::R18BrowserImportStartRequestDto req;
            std::string decode_error;
            const std::string body_str = memochat::json::glaze_stringify(src);
            if (!memochat::r18::DecodeR18BrowserImportStartRequest(body_str, &req, &decode_error))
            {
                root["error"] = ErrorCodes::Error_Json;
                root["message"] = decode_error.empty() ? "invalid request" : decode_error;
                return true;
            }

            auto result =
                memochat::r18::R18BrowserImportService::Instance().StartImport(uid, req.source_id, req.client_kind);

            if (!result.ok)
            {
                root["error"] = ErrorCodes::Error_Json;
                root["message"] = result.error.empty() ? "failed to start import" : result.error;
                return true;
            }

            memochat::r18::R18BrowserImportStartResponseDto response_dto;
            response_dto.import_id = result.import_id;
            response_dto.ticket = result.ticket;
            response_dto.expires_at_ms = result.expires_at_ms;

            WriteOk(root, memochat::r18::R18BrowserImportStartResponseToJsonValue(response_dto));
            return true;
        });
}

bool R18Service::HandleBrowserImportComplete(const memochat::gate::routing::GateRequest& request,
                                             memochat::gate::routing::GateResponse& response)
{
    // This endpoint is capability-authenticated (no JWT required)
    JsonValue root;
    memochat::r18::R18BrowserImportCompleteRequestDto req;
    std::string decode_error;
    if (!memochat::r18::DecodeR18BrowserImportCompleteRequest(request.body, &req, &decode_error))
    {
        root["error"] = ErrorCodes::Error_Json;
        root["message"] = decode_error.empty() ? "invalid request" : decode_error;
        WritePostJson(response, root);
        response.status = 400;
        return true;
    }

    memochat::r18::EhentaiSessionCookies cookies;
    cookies.ipb_member_id = req.ipb_member_id;
    cookies.ipb_pass_hash = req.ipb_pass_hash;
    cookies.igneous = req.igneous;
    cookies.sk = req.sk;

    auto complete_result = memochat::r18::R18BrowserImportService::Instance().CompleteImport(req.ticket, cookies);

    if (!complete_result.ok)
    {
        memochat::r18::R18BrowserImportCompleteResponseDto response_dto;
        response_dto.success = false;
        response_dto.message = complete_result.error.empty() ? "import failed" : complete_result.error;
        WriteOk(root, memochat::r18::R18BrowserImportCompleteResponseToJsonValue(response_dto));
        WritePostJson(response, root);
        response.status = 400;
        return true;
    }

    // Validate session with E-Hentai upstream
    auto validation = memochat::r18::R18EhentaiSessionService::Instance().ValidateSession(cookies);

    if (!validation.ok)
    {
        memochat::r18::R18BrowserImportService::Instance().SetStatus(
            complete_result.uid,
            complete_result.import_id,
            memochat::r18::BrowserImportStatus::Failed,
            validation.error.empty() ? "session validation failed" : validation.error);
        memochat::r18::R18BrowserImportCompleteResponseDto response_dto;
        response_dto.success = false;
        response_dto.message = validation.error.empty() ? "session validation failed" : validation.error;
        WriteOk(root, memochat::r18::R18BrowserImportCompleteResponseToJsonValue(response_dto));
        WritePostJson(response, root);
        response.status = 400;
        return true;
    }
    if (complete_result.source_id == "exhentai.official" && !validation.exhentai_access)
    {
        memochat::r18::R18BrowserImportService::Instance().SetStatus(
            complete_result.uid,
            complete_result.import_id,
            memochat::r18::BrowserImportStatus::Failed,
            "ExHentai access was not granted by the imported session");
        memochat::r18::R18BrowserImportCompleteResponseDto response_dto;
        response_dto.success = false;
        response_dto.message = "ExHentai access was not granted by the imported session";
        WriteOk(root, memochat::r18::R18BrowserImportCompleteResponseToJsonValue(response_dto));
        WritePostJson(response, root);
        response.status = 400;
        return true;
    }

    // Import session for both ehentai.official and exhentai.official
    std::string error;
    bool imported_ehentai = false;
    bool imported_exhentai = false;

    if (validation.ehentai_access)
    {
        imported_ehentai = memochat::r18::R18SourceCredentialStore::Instance().ImportEhentaiSession(
            complete_result.uid,
            "ehentai.official",
            validation.normalized_cookie_header,
            "authenticated",
            "Browser import successful",
            &error);
    }

    if (validation.exhentai_access)
    {
        imported_exhentai = memochat::r18::R18SourceCredentialStore::Instance().ImportEhentaiSession(
            complete_result.uid,
            "exhentai.official",
            validation.normalized_cookie_header,
            "authenticated",
            "Browser import successful (ExHentai access)",
            &error);
    }

    memochat::r18::R18BrowserImportCompleteResponseDto response_dto;
    response_dto.success = imported_ehentai || imported_exhentai;
    response_dto.message = response_dto.success ? "Session imported successfully" : error;
    memochat::r18::R18BrowserImportService::Instance().SetStatus(complete_result.uid,
                                                                 complete_result.import_id,
                                                                 response_dto.success
                                                                     ? memochat::r18::BrowserImportStatus::Authenticated
                                                                     : memochat::r18::BrowserImportStatus::Failed,
                                                                 response_dto.success ? "" : response_dto.message);

    WriteOk(root, memochat::r18::R18BrowserImportCompleteResponseToJsonValue(response_dto));
    WritePostJson(response, root);
    return true;
}

bool R18Service::HandleBrowserImportStatus(const memochat::gate::routing::GateRequest& request,
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

    const std::string import_id = QueryParam(request, "import_id", "");
    if (import_id.empty())
    {
        root["error"] = ErrorCodes::Error_Json;
        root["message"] = "import_id is required";
        WriteGetJson(response, root);
        response.status = 400;
        return true;
    }

    auto status_result = memochat::r18::R18BrowserImportService::Instance().GetStatus(uid, import_id);

    if (!status_result.ok)
    {
        root["error"] = ErrorCodes::Error_Json;
        root["message"] = status_result.error.empty() ? "status check failed" : status_result.error;
        WriteGetJson(response, root);
        response.status = 404;
        return true;
    }

    JsonValue data;
    switch (status_result.status)
    {
        case memochat::r18::BrowserImportStatus::Pending:
            data["status"] = "pending";
            break;
        case memochat::r18::BrowserImportStatus::Authenticated:
            data["status"] = "authenticated";
            break;
        case memochat::r18::BrowserImportStatus::Failed:
            data["status"] = "failed";
            break;
        case memochat::r18::BrowserImportStatus::Expired:
            data["status"] = "expired";
            break;
    }
    data["message"] = status_result.message;

    WriteOk(root, data);
    WriteGetJson(response, root);
    response.headers["Cache-Control"] = "no-store";
    return true;
}

bool R18Service::HandleSessionImport(const memochat::gate::routing::GateRequest& request,
                                     memochat::gate::routing::GateResponse& response)
{
    return HandleJsonRequest(
        request,
        response,
        [](const JsonValue& src, JsonValue& root, const std::string&, int uid)
        {
            memochat::r18::R18SessionImportRequestDto req;
            std::string decode_error;
            const std::string body_str = memochat::json::glaze_stringify(src);
            if (!memochat::r18::DecodeR18SessionImportRequest(body_str, &req, &decode_error))
            {
                root["error"] = ErrorCodes::Error_Json;
                root["message"] = decode_error.empty() ? "invalid request" : decode_error;
                return true;
            }

            const bool is_ehentai_family = req.source_id == "ehentai.official" || req.source_id == "exhentai.official";

            // ── E-Hentai / ExHentai: validate with upstream before storing ──────────
            if (is_ehentai_family)
            {
                memochat::r18::EhentaiSessionCookies cookies;
                cookies.ipb_member_id = req.ipb_member_id;
                cookies.ipb_pass_hash = req.ipb_pass_hash;
                cookies.igneous = req.igneous;
                cookies.sk = req.sk;

                auto validation = memochat::r18::R18EhentaiSessionService::Instance().ValidateSession(cookies);

                if (!validation.ok)
                {
                    memochat::r18::R18SessionImportResponseDto response_dto;
                    response_dto.success = false;
                    response_dto.message = validation.error.empty() ? "session validation failed" : validation.error;
                    WriteOk(root, memochat::r18::R18SessionImportResponseToJsonValue(response_dto));
                    return true;
                }
                if (req.source_id == "exhentai.official" && !validation.exhentai_access)
                {
                    memochat::r18::R18SessionImportResponseDto response_dto;
                    response_dto.success = false;
                    response_dto.message = "ExHentai access was not granted by the imported session";
                    response_dto.ehentai_access = validation.ehentai_access;
                    response_dto.exhentai_access = false;
                    WriteOk(root, memochat::r18::R18SessionImportResponseToJsonValue(response_dto));
                    return true;
                }

                std::string error;
                bool imported_ehentai = false;
                bool imported_exhentai = false;

                if (validation.ehentai_access)
                {
                    imported_ehentai = memochat::r18::R18SourceCredentialStore::Instance().ImportEhentaiSession(
                        uid,
                        "ehentai.official",
                        validation.normalized_cookie_header,
                        "authenticated",
                        "Session import successful",
                        &error);
                }

                if (validation.exhentai_access)
                {
                    imported_exhentai = memochat::r18::R18SourceCredentialStore::Instance().ImportEhentaiSession(
                        uid,
                        "exhentai.official",
                        validation.normalized_cookie_header,
                        "authenticated",
                        "Session import successful (ExHentai access)",
                        &error);
                }

                memochat::r18::R18SessionImportResponseDto response_dto;
                response_dto.success = req.source_id == "exhentai.official" ? imported_exhentai : imported_ehentai;
                response_dto.message = response_dto.success ? "Session imported successfully" : error;
                response_dto.ehentai_access = validation.ehentai_access;
                response_dto.exhentai_access = validation.exhentai_access;

                WriteOk(root, memochat::r18::R18SessionImportResponseToJsonValue(response_dto));
                return true;
            }

            // ── Generic cookie import (nhentai, hanime1, …) ──────────────────────────
            // cookie_header is a pre-formatted "name=value; name2=value2" string.
            const std::string& cookie = req.cookie_header;
            if (cookie.empty())
            {
                root["error"] = ErrorCodes::Error_Json;
                root["message"] = "cookie_header is required for this source";
                return true;
            }

            std::string error;
            const bool ok =
                memochat::r18::R18SourceCredentialStore::Instance().ImportCookieSession(uid,
                                                                                        req.source_id,
                                                                                        cookie,
                                                                                        "authenticated",
                                                                                        "Cookie import successful",
                                                                                        &error);

            memochat::r18::R18SessionImportResponseDto response_dto;
            response_dto.success = ok;
            response_dto.message = ok ? "Cookie imported successfully" : error;
            response_dto.ehentai_access = false;
            response_dto.exhentai_access = false;

            WriteOk(root, memochat::r18::R18SessionImportResponseToJsonValue(response_dto));
            return true;
        });
}

} // namespace memochat::gate::services::r18
