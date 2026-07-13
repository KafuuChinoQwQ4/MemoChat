#include "r18/R18SourceCredentialStore.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>

namespace memochat::r18
{
namespace
{

using json::JsonValue;

int64_t NowMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::string Trim(std::string value)
{
    while (!value.empty() &&
           (value.front() == ' ' || value.front() == '\t' || value.front() == '\n' || value.front() == '\r'))
        value.erase(value.begin());
    while (!value.empty() &&
           (value.back() == ' ' || value.back() == '\t' || value.back() == '\n' || value.back() == '\r'))
        value.pop_back();
    return value;
}

} // namespace

R18SourceCredentialStore& R18SourceCredentialStore::Instance()
{
    static R18SourceCredentialStore store;
    return store;
}

R18SourceCredentialStore::R18SourceCredentialStore()
{
    root_ = ResolveRoot();
    EnsureRootLocked();
}

std::filesystem::path R18SourceCredentialStore::ResolveRoot() const
{
    std::error_code ec;
    const auto cwd = std::filesystem::current_path(ec);
    if (!ec)
    {
        const auto local = cwd / "data" / "r18" / "credentials";
        if (std::filesystem::exists(local, ec) || std::filesystem::create_directories(local, ec) || !ec)
            return local;
    }
    return std::filesystem::temp_directory_path() / "memochat_r18_credentials";
}

std::filesystem::path R18SourceCredentialStore::PathForUid(int uid) const
{
    return root_ / (std::to_string(uid) + ".json");
}

void R18SourceCredentialStore::EnsureRootLocked()
{
    std::error_code ec;
    std::filesystem::create_directories(root_, ec);
}

void R18SourceCredentialStore::LoadUidLocked(int uid)
{
    if (by_uid_.find(uid) != by_uid_.end())
        return;

    std::unordered_map<std::string, R18SourceCredential> table;
    const auto path = PathForUid(uid);
    std::error_code ec;
    if (!std::filesystem::exists(path, ec))
    {
        by_uid_[uid] = std::move(table);
        return;
    }
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open())
    {
        by_uid_[uid] = std::move(table);
        return;
    }
    const std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    JsonValue root;
    if (!json::glaze_parse(root, content) || !root.is_array())
    {
        by_uid_[uid] = std::move(table);
        return;
    }
    for (std::size_t i = 0; i < root.size(); ++i)
    {
        const auto item = root[static_cast<int>(i)];
        R18SourceCredential cred;
        cred.source_id = json::glaze_safe_get<std::string>(item, "source_id", "");
        if (cred.source_id.empty())
            continue;
        cred.username = json::glaze_safe_get<std::string>(item, "username", "");
        cred.password = json::glaze_safe_get<std::string>(item, "password", "");
        cred.session_token = json::glaze_safe_get<std::string>(item, "session_token", "");
        cred.session_cookie = json::glaze_safe_get<std::string>(item, "session_cookie", "");
        cred.status = json::glaze_safe_get<std::string>(item, "status", "configured");
        cred.message = json::glaze_safe_get<std::string>(item, "message", "");
        cred.updated_at_ms = json::glaze_safe_get<int64_t>(item, "updated_at_ms", 0);
        table[cred.source_id] = std::move(cred);
    }
    by_uid_[uid] = std::move(table);
}

void R18SourceCredentialStore::SaveUidLocked(int uid)
{
    EnsureRootLocked();
    LoadUidLocked(uid);
    JsonValue arr{json::array_t{}};
    for (const auto& [id, cred] : by_uid_[uid])
    {
        JsonValue item;
        item["source_id"] = cred.source_id;
        item["username"] = cred.username;
        item["password"] = cred.password;
        item["session_token"] = cred.session_token;
        item["session_cookie"] = cred.session_cookie;
        item["status"] = cred.status;
        item["message"] = cred.message;
        item["updated_at_ms"] = cred.updated_at_ms;
        json::glaze_append(arr, item);
    }
    std::ofstream out(PathForUid(uid), std::ios::binary | std::ios::trunc);
    out << json::glaze_stringify(arr);
}

JsonValue R18SourceCredentialStore::ToPublicJson(const R18SourceCredential& cred) const
{
    JsonValue item;
    item["source_id"] = cred.source_id;
    item["username"] = cred.username;
    item["has_password"] = !cred.password.empty();
    item["has_session"] = !cred.session_token.empty() || !cred.session_cookie.empty();
    item["status"] = cred.status;
    item["message"] = cred.message;
    item["updated_at_ms"] = cred.updated_at_ms;
    // Never expose password/session secrets in API responses.
    return item;
}

JsonValue R18SourceCredentialStore::ListPublicAccounts(int uid)
{
    std::lock_guard<std::mutex> lock(mu_);
    LoadUidLocked(uid);
    JsonValue arr{json::array_t{}};
    for (const auto& [id, cred] : by_uid_[uid])
        json::glaze_append(arr, ToPublicJson(cred));
    return arr;
}

std::optional<R18SourceCredential> R18SourceCredentialStore::Get(int uid, const std::string& source_id)
{
    std::lock_guard<std::mutex> lock(mu_);
    LoadUidLocked(uid);
    const auto it = by_uid_[uid].find(source_id);
    if (it == by_uid_[uid].end())
        return std::nullopt;
    return it->second;
}

bool R18SourceCredentialStore::UpsertLogin(int uid,
                                           const std::string& source_id,
                                           const std::string& username,
                                           const std::string& password,
                                           std::string* error)
{
    const auto sid = Trim(source_id);
    if (sid.empty())
    {
        if (error)
            *error = "source_id is required";
        return false;
    }
    std::lock_guard<std::mutex> lock(mu_);
    LoadUidLocked(uid);
    auto& cred = by_uid_[uid][sid];
    cred.source_id = sid;
    cred.username = Trim(username);
    if (!password.empty())
        cred.password = password;
    if (cred.password.empty() && cred.username.empty() && cred.session_token.empty() && cred.session_cookie.empty())
        cred.status = "not_configured";
    else if (cred.session_token.empty() && cred.session_cookie.empty())
        cred.status = "configured";
    cred.message.clear();
    cred.updated_at_ms = NowMs();
    SaveUidLocked(uid);
    return true;
}

bool R18SourceCredentialStore::Clear(int uid, const std::string& source_id, std::string* error)
{
    const auto sid = Trim(source_id);
    if (sid.empty())
    {
        if (error)
            *error = "source_id is required";
        return false;
    }
    std::lock_guard<std::mutex> lock(mu_);
    LoadUidLocked(uid);
    by_uid_[uid].erase(sid);
    SaveUidLocked(uid);
    return true;
}

bool R18SourceCredentialStore::UpdateSession(int uid,
                                             const std::string& source_id,
                                             const std::string& session_token,
                                             const std::string& session_cookie,
                                             const std::string& status,
                                             const std::string& message,
                                             std::string* error)
{
    const auto sid = Trim(source_id);
    if (sid.empty())
    {
        if (error)
            *error = "source_id is required";
        return false;
    }
    std::lock_guard<std::mutex> lock(mu_);
    LoadUidLocked(uid);
    auto& cred = by_uid_[uid][sid];
    cred.source_id = sid;
    if (!session_token.empty())
        cred.session_token = session_token;
    if (!session_cookie.empty())
        cred.session_cookie = session_cookie;
    cred.status = status.empty() ? "authenticated" : status;
    cred.message = message;
    cred.updated_at_ms = NowMs();
    SaveUidLocked(uid);
    return true;
}

bool R18SourceCredentialStore::MarkError(int uid,
                                         const std::string& source_id,
                                         const std::string& message,
                                         std::string* error)
{
    // Preserve username/password on login failure so the user can retry without re-entering.
    // UpdateSession only mutates session/status/message fields and does not clear existing secrets.
    const auto sid = Trim(source_id);
    if (sid.empty())
    {
        if (error)
            *error = "source_id is required";
        return false;
    }
    std::lock_guard<std::mutex> lock(mu_);
    LoadUidLocked(uid);
    auto& cred = by_uid_[uid][sid];
    cred.source_id = sid;
    // Keep username/password/session as-is; only flip status for UI.
    cred.status = "error";
    cred.message = message;
    cred.updated_at_ms = NowMs();
    SaveUidLocked(uid);
    return true;
}

} // namespace memochat::r18
