#include "r18/R18SourceService.hpp"

#include "r18/R18AdapterUtils.hpp"
#include "r18/R18EhentaiAdapter.hpp"
#include "r18/R18JmAdapter.hpp"
#include "r18/R18NhentaiAdapter.hpp"
#include "r18/R18PicacgAdapter.hpp"
#include "r18/R18SourceCredentialStore.hpp"
#include "r18/R18SourceRecordCodec.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>

import memochat.r18.source_algorithms;
import memochat.r18.source_service_algorithms;
import memochat.r18.picacg_adapter_algorithms;

namespace memochat::r18
{
namespace
{

using json::JsonValue;

const char* const kMockSourceId = source_service::modules::MockSourceId();

bool IsBuiltinSourceId(const std::string& id)
{
    return id == kMockSourceId || id == kJmSourceId || id == kPicacgSourceId || id == kNhentaiSourceId ||
           id == kEhentaiSourceId;
}

bool IsOfficialBuiltinSourceId(const std::string& source_id)
{
    return source_id == kJmSourceId || source_id == kPicacgSourceId || source_id == kNhentaiSourceId ||
           source_id == kEhentaiSourceId;
}

bool SourceNeedsAccount(const std::string& source_id)
{
    // e-hentai supports public listing without login; cookie is optional.
    return source_id == kPicacgSourceId;
}

bool SourceSupportsDirectAccess(const std::string& source_id)
{
    return source_id == kJmSourceId || source_id == kNhentaiSourceId || source_id == kMockSourceId;
}

std::string FailureTitleForSource(const std::string& source_id)
{
    return IsOfficialBuiltinSourceId(source_id) ? "官方源请求失败" : "内容源请求失败";
}

JsonValue ErrorData(const std::string& source_id, const std::string& message)
{
    JsonValue data;
    data["source_id"] = source_id;
    data["items"] = JsonValue{json::array_t{}};
    data["error_message"] = message;

    JsonValue item;
    item["source_id"] = source_id;
    item["comic_id"] = "";
    item["title"] = FailureTitleForSource(source_id);
    item["subtitle"] = message;
    item["description"] = message;
    item["cover"] = "";
    item["tags"] = JsonValue{json::array_t{}};
    json::glaze_append(data["items"], item);
    return data;
}

bool PicacgSigningConfigured()
{
    const std::string api_key = picacg_adapter::modules::ApiKey();
    const std::string hmac_key = picacg_adapter::modules::HmacKey();
    return picacg_adapter::modules::HasValidSigningMaterial(api_key.size(), hmac_key.size());
}

void ApplyRuntimeSourceAvailability(R18SourceRecord& rec, int uid = 0)
{
    if (rec.id == kNhentaiSourceId)
    {
        rec.enabled = true;
        rec.status = source_service::modules::OkStatus();
        rec.message = "direct access";
        return;
    }
    if (rec.id == kJmSourceId)
    {
        if (rec.status.empty() || rec.status == "credentials-missing" || rec.status == "auth-required")
        {
            rec.enabled = true;
            rec.status = source_service::modules::OkStatus();
            rec.message.clear();
        }
        return;
    }
    if (rec.id == kPicacgSourceId)
    {
        if (!PicacgSigningConfigured())
        {
            rec.enabled = false;
            rec.status = "credentials-missing";
            rec.message = picacg_adapter::modules::MissingCredentialsMessage();
            return;
        }
        auto cred = R18SourceCredentialStore::Instance().Get(uid, rec.id);
        const bool authenticated =
            cred && (!cred->session_token.empty()) && (cred->status == "authenticated" || cred->status == "configured");
        if (authenticated)
        {
            rec.enabled = true;
            rec.status = source_service::modules::OkStatus();
            rec.message.clear();
        }
        else
        {
            rec.enabled = false;
            rec.status = source_service::modules::AuthRequiredStatus();
            rec.message = picacg_adapter::modules::AccountLoginRequiredMessage();
        }
        return;
    }
    if (rec.id == kEhentaiSourceId)
    {
        // Public e-hentai listing works without cookies; cookies improve access/quality.
        rec.enabled = true;
        auto cred = R18SourceCredentialStore::Instance().Get(uid, rec.id);
        if (cred && (!cred->session_cookie.empty() || !cred->password.empty()))
        {
            rec.status = source_service::modules::OkStatus();
            rec.message = cred->session_cookie.empty() ? "cookie optional / not logged in" : "cookie configured";
        }
        else
        {
            rec.status = source_service::modules::DirectAccessStatus();
            rec.message = "direct access; optional cookie for restricted content";
        }
        return;
    }
}

json::JsonValue PublicSourceRecord(const R18SourceRecord& source, int uid = 0)
{
    R18SourceRecord public_record = source;
    ApplyRuntimeSourceAvailability(public_record, uid);
    json::JsonValue rec = R18SourceRecordToPublicJsonValue(public_record);
    if (!json::glaze_has_key(rec, "title") || json::glaze_safe_get<std::string>(rec, "title", "").empty())
        rec["title"] = public_record.name;
    rec["auth_required"] = SourceNeedsAccount(public_record.id);
    rec["direct_access"] = SourceSupportsDirectAccess(public_record.id) || public_record.id == kEhentaiSourceId;
    if (auto cred = R18SourceCredentialStore::Instance().Get(uid, public_record.id))
    {
        rec["account_status"] = cred->status;
        rec["account_username"] = cred->username;
        rec["has_account"] = !cred->username.empty() || !cred->password.empty() || !cred->session_token.empty() ||
                             !cred->session_cookie.empty();
    }
    else
    {
        rec["account_status"] = "not_configured";
        rec["account_username"] = "";
        rec["has_account"] = false;
    }
    return rec;
}

std::filesystem::path ResolveDataRoot()
{
    std::error_code ec;
    const auto cwd = std::filesystem::current_path(ec);
    if (ec)
    {
        return std::filesystem::path("data") / "r18" / "sources";
    }
    std::string leaf = cwd.filename().string();
    modules::LowerAsciiInPlace(leaf.data(), leaf.size());
    const auto base = leaf.rfind(source_service::modules::GateShellPrefix(), 0) == 0 ? cwd.parent_path() : cwd;
    return base / "data" / "r18" / "sources";
}

bool EndsWithCaseInsensitive(const std::string& value, const std::string& suffix)
{
    if (value.size() < suffix.size())
        return false;
    return std::equal(suffix.rbegin(),
                      suffix.rend(),
                      value.rbegin(),
                      [](char a, char b)
                      {
                          return modules::EqualsAsciiCaseInsensitive(a, b);
                      });
}

std::string Stem(const std::string& file_name)
{
    std::filesystem::path p(file_name);
    auto stem = p.stem().string();
    return stem.empty() ? "imported-source" : stem;
}

std::string NormalizeId(std::string value)
{
    for (char& c : value)
    {
        c = modules::NormalizeSourceIdChar(static_cast<unsigned char>(c));
    }
    value.erase(std::unique(value.begin(),
                            value.end(),
                            [](char a, char b)
                            {
                                return a == '-' && b == '-';
                            }),
                value.end());
    while (!value.empty() && value.front() == '-')
        value.erase(value.begin());
    while (!value.empty() && value.back() == '-')
        value.pop_back();
    return value.empty() ? "imported-source" : value;
}

std::string TrimAscii(std::string value)
{
    const auto is_space = [](unsigned char c)
    {
        return modules::IsAsciiSpace(c);
    };
    while (!value.empty() && is_space(static_cast<unsigned char>(value.front())))
        value.erase(value.begin());
    while (!value.empty() && is_space(static_cast<unsigned char>(value.back())))
        value.pop_back();
    return value;
}

std::string NormalizePathSegment(std::string value, const std::string& fallback)
{
    for (char& c : value)
    {
        c = modules::NormalizePathSegmentChar(static_cast<unsigned char>(c));
    }
    value.erase(std::unique(value.begin(),
                            value.end(),
                            [](char a, char b)
                            {
                                return a == '-' && b == '-';
                            }),
                value.end());
    while (!value.empty() && modules::IsPathSegmentTrimChar(value.front()))
        value.erase(value.begin());
    while (!value.empty() && modules::IsPathSegmentTrimChar(value.back()))
        value.pop_back();
    return value.empty() ? fallback : value;
}

bool LooksLikeJavaScript(const std::string& file_name, const std::string& manifest_json, const std::string& binary)
{
    if (EndsWithCaseInsensitive(file_name, ".js"))
        return true;
    if (!manifest_json.empty())
    {
        JsonValue manifest;
        if (json::glaze_parse(manifest, manifest_json))
        {
            const std::string format = json::glaze_safe_get<std::string>(manifest, "format", "");
            if (format == source_service::modules::SourceJsFormat() || format == "javascript")
                return true;
        }
    }
    const auto probe =
        binary.substr(0, std::min<std::size_t>(binary.size(), source_service::modules::ManifestProbeWindow()));
    return source_service::modules::MatchesJsSourceProbe(probe.find("class ") != std::string::npos,
                                                         probe.find("ComicSource") != std::string::npos,
                                                         probe.find("search") != std::string::npos);
}

} // namespace

R18SourceService& R18SourceService::Instance()
{
    static R18SourceService svc;
    return svc;
}

std::size_t SourceImportLimitBytes()
{
    return source_service::modules::MaxSourceImportBytes();
}

R18SourceService::R18SourceService()
{
    data_root_ = ResolveDataRoot();
    image_cache_root_ = data_root_.parent_path() / "image-cache";
    std::error_code ec;
    std::filesystem::create_directories(data_root_, ec);
    std::filesystem::create_directories(image_cache_root_, ec);
    InstallBuiltinSourcesLocked();
    LoadLocked();
}

void R18SourceService::InstallBuiltinSourcesLocked()
{
    const auto install = [this](const char* id, const char* name, const char* version)
    {
        if (sources_.find(id) != sources_.end())
            return;
        R18SourceRecord rec;
        rec.id = id;
        rec.name = name;
        rec.version = version;
        rec.format = source_service::modules::NativeFormat();
        rec.enabled = true;
        rec.builtin = true;
        rec.status = source_service::modules::OkStatus();
        sources_[rec.id] = rec;
    };
    install(kJmSourceId, "禁漫天堂", source_service::modules::JmSourceVersion());
    install(kPicacgSourceId, "哔咔漫画", source_service::modules::PicacgSourceVersion());
    install(kNhentaiSourceId, "nHentai", source_service::modules::NhentaiSourceVersion());
    install(kEhentaiSourceId, "e-hentai", source_service::modules::EhentaiSourceVersion());
}

json::JsonValue R18SourceService::ListSources()
{
    return ListSourcesForUser(0);
}

json::JsonValue R18SourceService::ListSourcesForUser(int uid)
{
    std::lock_guard<std::mutex> lock(mu_);
    InstallBuiltinSourcesLocked();
    LoadLocked();
    json::JsonValue arr{json::array_t{}};
    const auto append_source = [this, &arr, uid](const std::string& id)
    {
        if (id == kPicacgSourceId && !PicacgSigningConfigured())
            return;
        const auto it = sources_.find(id);
        if (it == sources_.end())
            return;
        json::glaze_append(arr, PublicSourceRecord(it->second, uid));
    };
    append_source(kJmSourceId);
    append_source(kPicacgSourceId);
    append_source(kNhentaiSourceId);
    append_source(kEhentaiSourceId);
    for (const auto& [id, source] : sources_)
    {
        if (id != kJmSourceId && id != kPicacgSourceId && id != kNhentaiSourceId && id != kEhentaiSourceId &&
            id != kMockSourceId && source.enabled && source.status != source_service::modules::StagedJsStatus())
        {
            json::glaze_append(arr, PublicSourceRecord(source, uid));
        }
    }
    return arr;
}

json::JsonValue R18SourceService::ListAccounts(int uid)
{
    JsonValue accounts = R18SourceCredentialStore::Instance().ListPublicAccounts(uid);
    JsonValue catalog = ListSourcesForUser(uid);
    JsonValue data;
    data["accounts"] = accounts;
    data["sources"] = catalog;
    JsonValue managed{json::array_t{}};
    const auto append_managed = [&](const char* id, const char* name, bool required, bool direct)
    {
        JsonValue item;
        item["source_id"] = id;
        item["name"] = name;
        item["auth_required"] = required;
        item["direct_access"] = direct;
        if (auto cred = R18SourceCredentialStore::Instance().Get(uid, id))
        {
            item["username"] = cred->username;
            item["has_password"] = !cred->password.empty();
            item["has_session"] = !cred->session_token.empty() || !cred->session_cookie.empty();
            item["status"] = cred->status;
            item["message"] = cred->message;
            item["updated_at_ms"] = cred->updated_at_ms;
        }
        else
        {
            item["username"] = "";
            item["has_password"] = false;
            item["has_session"] = false;
            item["status"] = direct ? source_service::modules::DirectAccessStatus() : "not_configured";
            item["message"] = direct ? "no account required" : "account not configured";
            item["updated_at_ms"] = 0;
        }
        json::glaze_append(managed, item);
    };
    append_managed(kJmSourceId, "禁漫天堂", false, true);
    if (PicacgSigningConfigured())
        append_managed(kPicacgSourceId, "哔咔漫画", true, false);
    append_managed(kEhentaiSourceId, "e-hentai", false, true); // cookie optional
    data["managed"] = managed;
    return data;
}

bool R18SourceService::SaveAccount(int uid,
                                   const std::string& source_id,
                                   const std::string& username,
                                   const std::string& password,
                                   std::string* error)
{
    if (!IsOfficialBuiltinSourceId(source_id) && source_id != kMockSourceId)
    {
        // still allow saving cookies for imported sources later; for now only builtins
    }
    if (!R18SourceCredentialStore::Instance().UpsertLogin(uid, source_id, username, password, error))
        return false;
    // Auto-login when credentials are present and source supports/needs remote auth.
    if (SourceNeedsAccount(source_id) || source_id == kEhentaiSourceId || source_id == kJmSourceId)
    {
        std::string login_error;
        if (!LoginAccount(uid, source_id, &login_error) && error != nullptr && error->empty())
            *error = login_error;
        // Save still succeeds even if remote login fails; status is marked error.
    }
    return true;
}

bool R18SourceService::LoginAccount(int uid, const std::string& source_id, std::string* error)
{
    auto cred = R18SourceCredentialStore::Instance().Get(uid, source_id);
    if (!cred)
    {
        if (error)
            *error = "account not configured";
        return false;
    }
    if (source_id == kPicacgSourceId)
    {
        if (cred->username.empty() || cred->password.empty())
        {
            if (error)
                *error = "Picacg username/password required";
            R18SourceCredentialStore::Instance().MarkError(uid, source_id, "username/password required", nullptr);
            return false;
        }
        std::string token;
        std::string login_error;
        if (!PicacgLogin(cred->username, cred->password, &token, &login_error))
        {
            R18SourceCredentialStore::Instance().MarkError(uid, source_id, login_error, nullptr);
            if (error)
                *error = login_error;
            return false;
        }
        return R18SourceCredentialStore::Instance()
            .UpdateSession(uid, source_id, token, "", "authenticated", "login ok", error);
    }
    if (source_id == kEhentaiSourceId)
    {
        // Prefer explicit cookie in password/session fields; username unused for cookie auth.
        std::string cookie = cred->session_cookie;
        if (cookie.empty() && !cred->password.empty() &&
            (cred->password.find("ipb_member_id=") != std::string::npos ||
             cred->password.find("igneous=") != std::string::npos || cred->password.find("sk=") != std::string::npos))
        {
            cookie = cred->password;
        }
        if (cookie.empty() && !cred->username.empty() && !cred->password.empty() &&
            cred->password.find('=') == std::string::npos)
        {
            // No remote username/password login implemented; store as configured.
            return R18SourceCredentialStore::Instance().UpdateSession(
                uid,
                source_id,
                "",
                "",
                "configured",
                "saved; paste e-hentai cookie into password field for restricted content",
                error);
        }
        if (!cookie.empty())
        {
            return R18SourceCredentialStore::Instance()
                .UpdateSession(uid, source_id, "", cookie, "authenticated", "cookie saved", error);
        }
        return R18SourceCredentialStore::Instance()
            .UpdateSession(uid, source_id, "", "", "configured", "direct access without cookie", error);
    }
    if (source_id == kJmSourceId)
    {
        if (cred->username.empty() || cred->password.empty())
        {
            // JM login is optional; mark as direct access when no credentials given.
            return R18SourceCredentialStore::Instance()
                .UpdateSession(uid, source_id, "", "", "authenticated", "direct access (no account)", error);
        }
        std::string jm_uid;
        std::string login_error;
        if (!JmLogin(cred->username, cred->password, &jm_uid, &login_error))
        {
            R18SourceCredentialStore::Instance().MarkError(uid, source_id, login_error, nullptr);
            if (error)
                *error = login_error;
            return false;
        }
        return R18SourceCredentialStore::Instance()
            .UpdateSession(uid, source_id, jm_uid, "", "authenticated", "login ok", error);
    }
    // Direct-access sources: mark configured, no remote login.
    return R18SourceCredentialStore::Instance()
        .UpdateSession(uid, source_id, "", "", "authenticated", "direct access", error);
}

bool R18SourceService::ClearAccount(int uid, const std::string& source_id, std::string* error)
{
    return R18SourceCredentialStore::Instance().Clear(uid, source_id, error);
}

json::JsonValue R18SourceService::CheckinForUser(int uid, const std::string& source_id)
{
    json::JsonValue data;
    data["source_id"] = source_id;
    if (source_id == kJmSourceId)
    {
        const std::string jm_uid = SessionTokenFor(uid, source_id);
        if (jm_uid.empty())
        {
            data["status"] = "not_logged_in";
            data["message"] = "JM check-in requires an account login first";
            return data;
        }
        std::string checkin_error;
        if (JmCheckin(jm_uid, &data, &checkin_error))
        {
            return data;
        }
        data["status"] = "error";
        data["message"] = checkin_error;
        return data;
    }
    data["status"] = "unsupported";
    data["message"] = "check-in is not supported for this source";
    return data;
}

std::string R18SourceService::SessionTokenFor(int uid, const std::string& source_id)
{
    auto cred = R18SourceCredentialStore::Instance().Get(uid, source_id);
    if (!cred)
        return {};
    return cred->session_token;
}

std::string R18SourceService::SessionCookieFor(int uid, const std::string& source_id)
{
    auto cred = R18SourceCredentialStore::Instance().Get(uid, source_id);
    if (!cred)
        return {};
    return cred->session_cookie;
}

bool R18SourceService::EnableSource(const std::string& id, bool enabled, std::string* error)
{
    const std::string normalized_id = TrimAscii(id);
    std::lock_guard<std::mutex> lock(mu_);
    InstallBuiltinSourcesLocked();
    LoadLocked();
    auto it = sources_.find(normalized_id);
    if (it == sources_.end())
    {
        if (error)
            *error = source_service::modules::SourceNotFoundMessage();
        return false;
    }
    it->second.enabled = enabled;
    SaveLocked();
    return true;
}

bool R18SourceService::DeleteSource(const std::string& id, std::string* error)
{
    const std::string normalized_id = TrimAscii(id);
    std::lock_guard<std::mutex> lock(mu_);
    InstallBuiltinSourcesLocked();
    LoadLocked();
    if (IsBuiltinSourceId(normalized_id))
    {
        if (error)
            *error = source_service::modules::CannotDeleteBuiltinMessage();
        return false;
    }
    auto it = sources_.find(normalized_id);
    if (it == sources_.end())
    {
        if (error)
            *error = source_service::modules::SourceNotFoundMessage();
        return false;
    }
    // Remove files from disk
    const std::string path = it->second.path;
    if (!path.empty())
    {
        std::error_code ec;
        std::filesystem::remove_all(std::filesystem::path(path).parent_path(), ec);
    }
    sources_.erase(it);
    SaveLocked();
    return true;
}

R18SourceRecord R18SourceService::ImportZip(const std::string& file_name,
                                            const std::string& manifest_json,
                                            const std::string& binary,
                                            std::string* error)
{
    std::lock_guard<std::mutex> lock(mu_);
    InstallBuiltinSourcesLocked();
    LoadLocked();
    const bool zip_payload = source_service::modules::IsZipMagic(binary.empty() ? '\0' : binary[0],
                                                                 binary.size() >= 2 ? binary[1] : '\0',
                                                                 binary.size());
    const bool js_payload = LooksLikeJavaScript(file_name, manifest_json, binary);
    if (source_service::modules::ShouldRejectSourceImport(zip_payload, js_payload, binary.size()))
    {
        if (error)
        {
            if (binary.size() > source_service::modules::MaxSourceImportBytes())
                *error = source_service::modules::SourcePackageTooLargeMessage();
            else if (zip_payload)
                *error = source_service::modules::NativePackageRejectedMessage();
            else
                *error = source_service::modules::InvalidPackagePayloadMessage();
        }
        return {};
    }

    json::JsonValue manifest;
    if (!manifest_json.empty() && !json::glaze_parse(manifest, manifest_json))
    {
        if (error)
            *error = source_service::modules::InvalidManifestMessage();
        return {};
    }

    const std::string fallback_id = NormalizeId(Stem(file_name));
    R18SourceRecord rec;
    rec.id = manifest_json.empty() ? fallback_id : json::glaze_safe_get<std::string>(manifest, "id", fallback_id);
    rec.id = NormalizeId(rec.id);
    rec.name = manifest_json.empty() ? rec.id : json::glaze_safe_get<std::string>(manifest, "name", rec.id);
    rec.version =
        manifest_json.empty()
            ? source_service::modules::DefaultVersion()
            : json::glaze_safe_get<std::string>(manifest, "version", source_service::modules::DefaultVersion());
    rec.version = NormalizePathSegment(rec.version, source_service::modules::DefaultVersion());
    rec.format = source_service::modules::SourceJsFormat();
    rec.source_url = manifest_json.empty() ? "" : json::glaze_safe_get<std::string>(manifest, "source_url", "");
    rec.catalog_url = manifest_json.empty() ? "" : json::glaze_safe_get<std::string>(manifest, "catalog_url", "");
    rec.enabled = false;
    rec.builtin = false;
    rec.status = source_service::modules::StagedJsStatus();
    rec.message = "JavaScript source saved. Execution requires a MemoChat source runtime adapter.";

    if (rec.id.empty())
    {
        if (error)
            *error = source_service::modules::SourceIdEmptyMessage();
        return {};
    }
    if (IsBuiltinSourceId(rec.id))
    {
        if (error)
            *error = source_service::modules::ReservedSourceIdMessage();
        return {};
    }

    const auto dir = data_root_ / rec.id / rec.version;
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec)
    {
        if (error)
            *error = source_service::modules::CreateDirFailedMessage();
        return {};
    }
    const auto source_path = dir / "source.js";
    std::ofstream out(source_path, std::ios::binary | std::ios::trunc);
    if (!out.is_open())
    {
        if (error)
            *error = source_service::modules::PersistFailedMessage();
        return {};
    }
    out.write(binary.data(), static_cast<std::streamsize>(binary.size()));
    out.close();
    if (!manifest_json.empty())
    {
        std::ofstream manifest_out(dir / "manifest.json", std::ios::binary | std::ios::trunc);
        manifest_out << manifest_json;
    }

    rec.path = source_path.string();
    sources_[rec.id] = rec;
    SaveLocked();
    return rec;
}

json::JsonValue R18SourceService::Search(const std::string& source_id, const std::string& keyword, int page)
{
    return SearchForUser(0, source_id, keyword, page);
}

json::JsonValue
R18SourceService::SearchForUser(int uid, const std::string& source_id, const std::string& keyword, int page)
{
    std::string dispatch_error;
    if (!CanDispatchSourceForUser(uid, source_id, &dispatch_error))
        return ErrorData(source_id, dispatch_error);

    if (source_id == kJmSourceId)
    {
        json::JsonValue result;
        std::string error;
        const std::string jm_uid = SessionTokenFor(uid, source_id);
        const bool ok = jm_uid.empty()
                            ? JmSearch(keyword, source_service::modules::NormalizeSearchPage(page), &result, &error)
                            : JmSearchWithToken(keyword,
                                                source_service::modules::NormalizeSearchPage(page),
                                                jm_uid,
                                                &result,
                                                &error);
        if (ok)
            return result;
        return ErrorData(kJmSourceId, error);
    }
    if (source_id == kPicacgSourceId)
    {
        json::JsonValue result;
        std::string error;
        const std::string token = SessionTokenFor(uid, source_id);
        const bool ok = token.empty()
                            ? PicacgSearch(keyword, source_service::modules::NormalizeSearchPage(page), &result, &error)
                            : PicacgSearchWithToken(keyword,
                                                    source_service::modules::NormalizeSearchPage(page),
                                                    token,
                                                    &result,
                                                    &error);
        if (ok)
            return result;
        return ErrorData(kPicacgSourceId, error);
    }
    if (source_id == kNhentaiSourceId)
    {
        json::JsonValue result;
        std::string error;
        if (NhentaiSearch(keyword, source_service::modules::NormalizeSearchPage(page), &result, &error))
            return result;
        return ErrorData(kNhentaiSourceId, error);
    }
    if (source_id == kEhentaiSourceId)
    {
        json::JsonValue result;
        std::string error;
        if (EhentaiSearch(keyword,
                          source_service::modules::NormalizeSearchPage(page),
                          SessionCookieFor(uid, source_id),
                          &result,
                          &error))
            return result;
        return ErrorData(kEhentaiSourceId, error);
    }

    json::JsonValue data;
    data["source_id"] = source_id;
    data["keyword"] = keyword;
    data["page"] = page;
    data["items"] = json::JsonValue{json::array_t{}};
    data["max_page"] = 1;

    json::JsonValue first;
    first["source_id"] = source_id;
    first["comic_id"] = "mock-" + std::to_string(page) + "-1";
    first["title"] = keyword.empty() ? "R18 Preview Comic" : ("R18 Preview: " + keyword);
    first["subtitle"] = SourceSnapshot(source_id).value_or(R18SourceRecord{}).message;
    first["cover"] = "/api/r18/image?source_id=" + detail::UrlEncode(source_id) + "&image_id=cover";
    first["author"] = source_id;
    first["tags"] = detail::MakeTags({"sample"});
    json::glaze_append(data["items"], first);
    return data;
}

json::JsonValue R18SourceService::Detail(const std::string& source_id, const std::string& comic_id)
{
    return DetailForUser(0, source_id, comic_id);
}

json::JsonValue R18SourceService::DetailForUser(int uid, const std::string& source_id, const std::string& comic_id)
{
    std::string dispatch_error;
    if (!CanDispatchSourceForUser(uid, source_id, &dispatch_error))
        return ErrorData(source_id, dispatch_error);

    if (source_id == kJmSourceId)
    {
        json::JsonValue result;
        std::string error;
        const std::string jm_uid = SessionTokenFor(uid, source_id);
        const bool ok =
            jm_uid.empty() ? JmDetail(comic_id, &result, &error) : JmDetailWithToken(comic_id, jm_uid, &result, &error);
        if (ok)
            return result;
        json::JsonValue data;
        data["source_id"] = kJmSourceId;
        data["comic_id"] = comic_id;
        data["title"] = FailureTitleForSource(kJmSourceId);
        data["description"] = error;
        data["cover"] = "";
        data["chapters"] = json::JsonValue{json::array_t{}};
        return data;
    }
    if (source_id == kPicacgSourceId)
    {
        json::JsonValue result;
        std::string error;
        const std::string token = SessionTokenFor(uid, source_id);
        const bool ok = token.empty() ? PicacgDetail(comic_id, &result, &error)
                                      : PicacgDetailWithToken(comic_id, token, &result, &error);
        if (ok)
            return result;
        json::JsonValue data;
        data["source_id"] = kPicacgSourceId;
        data["comic_id"] = comic_id;
        data["title"] = FailureTitleForSource(kPicacgSourceId);
        data["description"] = error;
        data["cover"] = "";
        data["chapters"] = json::JsonValue{json::array_t{}};
        return data;
    }
    if (source_id == kNhentaiSourceId)
    {
        json::JsonValue result;
        std::string error;
        if (NhentaiDetail(comic_id, &result, &error))
            return result;
        json::JsonValue data;
        data["source_id"] = kNhentaiSourceId;
        data["comic_id"] = comic_id;
        data["title"] = FailureTitleForSource(kNhentaiSourceId);
        data["description"] = error;
        data["cover"] = "";
        data["chapters"] = json::JsonValue{json::array_t{}};
        return data;
    }
    if (source_id == kEhentaiSourceId)
    {
        json::JsonValue result;
        std::string error;
        if (EhentaiDetail(comic_id, SessionCookieFor(uid, source_id), &result, &error))
            return result;
        json::JsonValue data;
        data["source_id"] = kEhentaiSourceId;
        data["comic_id"] = comic_id;
        data["title"] = FailureTitleForSource(kEhentaiSourceId);
        data["description"] = error;
        data["cover"] = "";
        data["chapters"] = json::JsonValue{json::array_t{}};
        return data;
    }

    const auto source = SourceSnapshot(source_id);
    json::JsonValue data;
    data["source_id"] = source_id;
    data["comic_id"] = comic_id;
    data["title"] = (source && !source->name.empty() ? source->name : "Unknown Source") + " Preview Comic";
    data["description"] = source ? source->message : "The selected source was not found.";
    data["cover"] = "/api/r18/image?source_id=" + detail::UrlEncode(source_id) + "&image_id=cover";
    data["chapters"] = json::JsonValue{json::array_t{}};
    for (int i = 1; i <= source_service::modules::PreviewChapterCount(); ++i)
    {
        json::JsonValue ch;
        ch["source_id"] = source_id;
        ch["comic_id"] = comic_id;
        ch["chapter_id"] = comic_id + "-ch" + std::to_string(i);
        ch["title"] = "Chapter " + std::to_string(i);
        ch["order"] = i;
        json::glaze_append(data["chapters"], ch);
    }
    return data;
}

json::JsonValue R18SourceService::Pages(const std::string& source_id, const std::string& chapter_id)
{
    return PagesForUser(0, source_id, chapter_id);
}

json::JsonValue R18SourceService::PagesForUser(int uid, const std::string& source_id, const std::string& chapter_id)
{
    std::string dispatch_error;
    if (!CanDispatchSourceForUser(uid, source_id, &dispatch_error))
        return ErrorData(source_id, dispatch_error);

    if (source_id == kJmSourceId)
    {
        json::JsonValue result;
        std::string error;
        const std::string jm_uid = SessionTokenFor(uid, source_id);
        const bool ok = jm_uid.empty() ? JmPages(chapter_id, &result, &error)
                                       : JmPagesWithToken(chapter_id, jm_uid, &result, &error);
        if (ok)
            return result;
        json::JsonValue data;
        data["source_id"] = kJmSourceId;
        data["chapter_id"] = chapter_id;
        data["error_message"] = error;
        data["pages"] = json::JsonValue{json::array_t{}};
        return data;
    }
    if (source_id == kPicacgSourceId)
    {
        const auto sep = chapter_id.find(':');
        const std::string comic_id = sep != std::string::npos ? chapter_id.substr(0, sep) : chapter_id;
        json::JsonValue result;
        std::string error;
        const std::string token = SessionTokenFor(uid, source_id);
        const bool ok = token.empty() ? PicacgPages(comic_id, chapter_id, &result, &error)
                                      : PicacgPagesWithToken(comic_id, chapter_id, token, &result, &error);
        if (ok)
            return result;
        json::JsonValue data;
        data["source_id"] = kPicacgSourceId;
        data["chapter_id"] = chapter_id;
        data["error_message"] = error;
        data["pages"] = json::JsonValue{json::array_t{}};
        return data;
    }
    if (source_id == kNhentaiSourceId)
    {
        json::JsonValue result;
        std::string error;
        if (NhentaiPages(chapter_id, &result, &error))
            return result;
        json::JsonValue data;
        data["source_id"] = kNhentaiSourceId;
        data["chapter_id"] = chapter_id;
        data["error_message"] = error;
        data["pages"] = json::JsonValue{json::array_t{}};
        return data;
    }
    if (source_id == kEhentaiSourceId)
    {
        json::JsonValue result;
        std::string error;
        if (EhentaiPages(chapter_id, SessionCookieFor(uid, source_id), &result, &error))
            return result;
        json::JsonValue data;
        data["source_id"] = kEhentaiSourceId;
        data["chapter_id"] = chapter_id;
        data["error_message"] = error;
        data["pages"] = json::JsonValue{json::array_t{}};
        return data;
    }

    json::JsonValue data;
    data["source_id"] = source_id;
    data["chapter_id"] = chapter_id;
    data["pages"] = json::JsonValue{json::array_t{}};
    for (int i = 1; i <= source_service::modules::PreviewPageCount(); ++i)
    {
        json::JsonValue page;
        page["index"] = i;
        page["image_id"] = chapter_id + "-p" + std::to_string(i);
        page["url"] = "/api/r18/image?source_id=" + detail::UrlEncode(source_id) +
                      "&image_id=" + detail::UrlEncode(chapter_id + "-p" + std::to_string(i));
        json::glaze_append(data["pages"], page);
    }
    return data;
}

R18ImagePayload R18SourceService::FetchImage(const std::string& source_id, const std::string& image_url)
{
    return FetchImageForUser(0, source_id, image_url);
}

R18ImagePayload R18SourceService::FetchImageForUser(int uid, const std::string& source_id, const std::string& image_url)
{
    std::string dispatch_error;
    if (!CanDispatchSourceForUser(uid, source_id, &dispatch_error))
        return detail::FailedImage(dispatch_error);

    if (source_id == kJmSourceId && !image_url.empty())
        return JmFetchImage(image_cache_root_, image_url);
    if (source_id == kPicacgSourceId && !image_url.empty())
        return PicacgFetchImage(image_cache_root_, image_url);
    if (source_id == kNhentaiSourceId && !image_url.empty())
        return NhentaiFetchImage(image_cache_root_, image_url);
    if (source_id == kEhentaiSourceId && !image_url.empty())
        return EhentaiFetchImage(image_cache_root_, image_url, SessionCookieFor(uid, source_id));
    return detail::PlaceholderImage("R18 Source Image", "preview");
}

bool R18SourceService::CanDispatchSource(const std::string& source_id, std::string* error)
{
    return CanDispatchSourceForUser(0, source_id, error);
}

bool R18SourceService::CanDispatchSourceForUser(int uid, const std::string& source_id, std::string* error)
{
    const auto source = SourceSnapshot(source_id);
    if (!source.has_value())
    {
        if (error != nullptr)
            *error = source_service::modules::SourceNotFoundMessage();
        return false;
    }

    R18SourceRecord effective = *source;
    ApplyRuntimeSourceAvailability(effective, uid);
    if (source_service::modules::ShouldDispatchSource(true, effective.enabled))
        return true;

    if (error != nullptr)
    {
        if (!effective.message.empty())
            *error = effective.message;
        else
            *error = source_service::modules::SourceDisabledMessage();
    }
    return false;
}

std::optional<R18SourceRecord> R18SourceService::SourceSnapshot(const std::string& source_id)
{
    std::lock_guard<std::mutex> lock(mu_);
    InstallBuiltinSourcesLocked();
    LoadLocked();
    auto it = sources_.find(source_id);
    if (it == sources_.end())
        return std::nullopt;
    return it->second;
}

void R18SourceService::LoadLocked()
{
    const auto manifest_path = data_root_ / "sources.json";
    std::error_code ec;
    const bool has_shared_manifest = std::filesystem::exists(manifest_path, ec) && !ec;
    LoadManifestLocked(manifest_path);
    const auto cwd = std::filesystem::current_path(ec);
    if (ec)
    {
        return;
    }
    const auto legacy_root = cwd / "data" / "r18" / "sources";
    if (!has_shared_manifest && legacy_root != data_root_)
    {
        const auto before = sources_.size();
        LoadManifestLocked(legacy_root / "sources.json");
        if (sources_.size() != before)
            SaveLocked();
    }
}

void R18SourceService::LoadManifestLocked(const std::filesystem::path& manifest_path)
{
    std::ifstream in(manifest_path, std::ios::binary);
    if (!in.is_open())
        return;
    const std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    json::JsonValue root;
    if (!json::glaze_parse(root, content))
        return;
    json::JsonValue sources = json::glaze_get(root, "sources");
    auto arr = json::glaze_get_array(sources);
    if (!arr)
        return;
    for (const auto& item_json : *arr)
    {
        R18SourceRecord rec;
        if (!R18SourceRecordFromJsonValue(json::JsonValue(item_json), &rec))
            continue;
        if (!rec.id.empty() && !rec.builtin && !IsBuiltinSourceId(rec.id))
            sources_[rec.id] = rec;
    }
}

void R18SourceService::SaveLocked()
{
    json::JsonValue root;
    root["sources"] = json::JsonValue{json::array_t{}};
    for (const auto& [_, source] : sources_)
        if (!source.builtin)
            json::glaze_append(root["sources"], R18SourceRecordToJsonValue(source));
    std::ofstream out(data_root_ / "sources.json", std::ios::binary | std::ios::trunc);
    out << json::glaze_stringify(root);
}

} // namespace memochat::r18
